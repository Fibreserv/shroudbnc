/*******************************************************************************
 * shroudBNC - an object-oriented framework for IRC                            *
 * Copyright (C) 2005 Gunnar Beutner                                           *
 *                                                                             *
 * This program is free software; you can redistribute it and/or               *
 * modify it under the terms of the GNU General Public License                 *
 * as published by the Free Software Foundation; either version 2              *
 * of the License, or (at your option) any later version.                      *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the               *
 * GNU General Public License for more details.                                *
 *                                                                             *
 * You should have received a copy of the GNU General Public License           *
 * along with this program; if not, write to the Free Software                 *
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA. *
 *******************************************************************************/

#include "StdAfx.h"
#include "BouncerConfig.h"
#include "SocketEvents.h"
#include "Connection.h"
#include "ClientConnection.h"
#include "IRCConnection.h"
#include "BouncerUser.h"
#include "BouncerCore.h"
#include "BouncerLog.h"
#include "IdentSupport.h"
#include "ModuleFar.h"
#include "Module.h"
#include "Hashtable.h"
#include "utility.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

SOCKET g_last_sock = 0;
time_t g_LastReconnect = 0;

CBouncerCore::CBouncerCore(CBouncerConfig* Config) {
	m_Config = Config;

	m_Users = NULL;
	m_UserCount = 0;
	
	m_Modules = NULL;
	m_ModuleCount = 0;

	m_Ident = new CIdentSupport();

	const char* Users = Config->ReadString("system.users");

	const char* Args = ArgTokenize(Users);
	int Count = ArgCount(Args);

	m_OtherSockets = NULL;
	m_OtherSocketCount = 0;

	m_Log = new CBouncerLog("sbnc.log");
	m_Log->Clear();
	m_Log->WriteLine("Log system initialized.");

	m_Users = (CBouncerUser**)malloc(sizeof(CBouncerUser*) * Count);
	m_UserCount = Count;

	int i;
	for (i = 0; i < Count; i++) {
		m_Users[i] = new CBouncerUser(ArgGet(Args, i + 1));
	}

	ArgFree(Args);

	char Out[1024];

	m_LoadingModules = true;

	i = 0;
	while (true) {
		snprintf(Out, sizeof(Out), "system.modules.mod%d", i++);

		const char* File = m_Config->ReadString(Out);

		if (File)
			LoadModule(File);
		else
			break;
	}

	m_LoadingModules = false;
}

CBouncerCore::~CBouncerCore() {
	closesocket(m_Listener);

	delete m_Ident;
}

void CBouncerCore::StartMainLoop() {
	int Port = m_Config->ReadInteger("system.port");

	if (Port == 0)
		Port = 9000;

	m_Listener = CreateListener(Port);

	if (m_Listener == INVALID_SOCKET) {
		Log("Could not create listener port");
		exit(0);
	}

	Log("Created main listener.");

	fd_set FDRead, FDWrite;//, FDError;

	int last = time(NULL);

	Log("Starting main loop.");

	while (true) {
		FD_ZERO(&FDRead);
		FD_SET(m_Listener, &FDRead);

		FD_ZERO(&FDWrite);
		//FD_ZERO(&FDError);

		int i;

		for (i = 0; i < m_OtherSocketCount; i++) {
			if (m_OtherSockets[i].Socket != INVALID_SOCKET) {
				FD_SET(m_OtherSockets[i].Socket, &FDRead);

				if (m_OtherSockets[i].Events->HasQueuedData())
					FD_SET(m_OtherSockets[i].Socket, &FDWrite);
			}
		}

		timeval interval = { 1, 0 };

		int ready = select(MAX_SOCKETS, &FDRead, &FDWrite, /*&FDError*/ NULL, &interval);

		int now = time(NULL);

		if (now > last) {
			last = now;

			Pulse(now);
		}

		if (ready > 0) {
			//printf("%d socket(s) ready\n", ready);

			if (FD_ISSET(m_Listener, &FDRead)) {
				sockaddr_in sin_remote;
				socklen_t sin_size = sizeof(sin_remote);

				SOCKET Client = accept(m_Listener, (sockaddr*)&sin_remote, &sin_size);
				HandleConnectingClient(Client, sin_remote);
			}

			for (i = 0; i < m_OtherSocketCount; i++) {
				SOCKET Socket = m_OtherSockets[i].Socket;
				CSocketEvents* Events = m_OtherSockets[i].Events;

				if (Socket != INVALID_SOCKET) {
					if (FD_ISSET(Socket, &FDRead)) {
						if (!Events->Read()) {
							Events->Destroy();
							shutdown(Socket, SD_BOTH); 
							closesocket(Socket);
						}
					}

					if (m_OtherSockets[i].Events && FD_ISSET(Socket, &FDWrite)) {
						Events->Write();
					}
				}
			}
		} else if (ready == -1) {
			//printf("select() failed :/\n");

			fd_set set;

			for (i = 0; i < m_OtherSocketCount; i++) {
				SOCKET Socket = m_OtherSockets[i].Socket;

				if (Socket != INVALID_SOCKET) {
					FD_ZERO(&set);
					FD_SET(Socket, &set);

					timeval zero = { 0, 0 };
					int code = select(MAX_SOCKETS, &set, NULL, NULL, &zero);

					if (code == -1) {
						m_OtherSockets[i].Events->Error();
						m_OtherSockets[i].Events->Destroy();

						shutdown(Socket, SD_BOTH);
						closesocket(Socket);
					}
				}
			}
		}
	}
}

void CBouncerCore::HandleConnectingClient(SOCKET Client, sockaddr_in Remote) {
	if (Client > g_last_sock)
		g_last_sock = Client;

	// destruction is controlled by the main loop
	new CClientConnection(Client, Remote);

	puts("bouncer client connecting...");
}

CBouncerUser* CBouncerCore::GetUser(const char* Name) {
	if (!Name)
		return NULL;

	for (int i = 0; i < m_UserCount; i++) {
		if (m_Users[i] && strcmpi(m_Users[i]->GetUsername(), Name) == 0) {
			return m_Users[i];
		}
	}

	return NULL;
}

void CBouncerCore::GlobalNotice(const char* Text, bool AdminOnly) {
	for (int i = 0; i < m_UserCount; i++) {
		if (m_Users[i] && (!AdminOnly || m_Users[i]->IsAdmin()))
			m_Users[i]->Notice(Text);
	}
}

void CBouncerCore::Pulse(time_t Now) {
	int i;

	for (i = 0; i < m_UserCount; i++) {
		if (!m_Users[i])
			continue;

		if (m_Users[i]->ShouldReconnect()) {
			m_Users[i]->Reconnect();
			break;
		}
	}

	for (i = 0; i < m_ModuleCount; i++) {
		if (!m_Modules[i])
			continue;

		m_Modules[i]->GetModule()->Pulse(Now);
	}
}

CBouncerUser** CBouncerCore::GetUsers(void) {
	return m_Users;
}

int CBouncerCore::GetUserCount(void) {
	return m_UserCount;
}


void CBouncerCore::SetIdent(const char* Ident) {
	m_Ident->SetIdent(Ident);
}

const char* CBouncerCore::GetIdent(void) {
	return m_Ident->GetIdent();
}

CModule** CBouncerCore::GetModules(void) {
	return m_Modules;
}

int CBouncerCore::GetModuleCount(void) {
	return m_ModuleCount;
}

CModule* CBouncerCore::LoadModule(const char* Filename) {
	CModule* Mod = new CModule(Filename);

	for (int i = 0; i < m_ModuleCount; i++) {
		if (m_Modules[i] && m_Modules[i]->GetHandle() == Mod->GetHandle()) {
			delete Mod;

			return NULL;
		}
	}

	if (Mod->GetModule()) {
		m_Modules = (CModule**)realloc(m_Modules, sizeof(CModule*) * ++m_ModuleCount);
		m_Modules[m_ModuleCount - 1] = Mod;

		Log("Loaded module: %s", Mod->GetFilename());

		Mod->GetModule()->Init(this);

		if (!m_LoadingModules)
			UpdateModuleConfig();

		return Mod;
	} else {
		delete Mod;

		return NULL;
	}
}

bool CBouncerCore::UnloadModule(CModule* Module) {
	for (int i = 0; i < m_ModuleCount; i++) {
		if (m_Modules[i] == Module) {
			Log("Unloaded module: %s", Module->GetFilename());

			delete Module;
			m_Modules[i] = NULL;

			UpdateModuleConfig();

			return true;
		}
	}

	return false;
}

void CBouncerCore::UpdateModuleConfig(void) {
	char Out[1024];
	int a = 0;
	char ModBuf[1024] = "";

	for (int i = 0; i < m_ModuleCount; i++) {
		if (m_Modules[i]) {
			snprintf(Out, sizeof(Out), "system.modules.mod%d", a++);
			m_Config->WriteString(Out, m_Modules[i]->GetFilename());
		}
	}

	snprintf(Out, sizeof(Out), "system.modules.mod%d", a);

	m_Config->WriteString(Out, NULL);
}

void CBouncerCore::RegisterSocket(SOCKET Socket, CSocketEvents* EventInterface) {
	m_OtherSockets = (socket_s*)realloc(m_OtherSockets, sizeof(socket_s) * ++m_OtherSocketCount);

	m_OtherSockets[m_OtherSocketCount - 1].Socket = Socket;
	m_OtherSockets[m_OtherSocketCount - 1].Events = EventInterface;
}


void CBouncerCore::UnregisterSocket(SOCKET Socket) {
	for (int i = 0; i < m_OtherSocketCount; i++) {
		if (m_OtherSockets[i].Socket == Socket) {
			m_OtherSockets[i].Events = NULL;
			m_OtherSockets[i].Socket = INVALID_SOCKET;
		}
	}
}

SOCKET CBouncerCore::CreateListener(unsigned short Port) {
	return ::CreateListener(Port);
}

void CBouncerCore::Log(const char* Format, ...) {
	char Out[1024];
	va_list marker;

	va_start(marker, Format);
	vsnprintf(Out, sizeof(Out), Format, marker);
	va_end(marker);

	m_Log->InternalWriteLine(Out);
}

CBouncerConfig* CBouncerCore::GetConfig(void) {
	return m_Config;
}

CBouncerLog* CBouncerCore::GetLog(void) {
	return m_Log;
}

void CBouncerCore::Shutdown(void) {
	for (int i = 0; i < m_OtherSocketCount; i++) {
		if (m_OtherSockets[i].Socket != INVALID_SOCKET)
			shutdown(m_OtherSockets[i].Socket, SD_BOTH);

		if (m_OtherSockets[i].Events)
			m_OtherSockets[i].Events->Destroy();
	}

	exit(0);
}

CBouncerUser* CBouncerCore::CreateUser(const char* Username, const char* Password) {
	CBouncerUser* U = GetUser(Username);
	if (U) {
		if (Password)
			U->SetPassword(Password);

		return U;
	}

	m_Users = (CBouncerUser**)realloc(m_Users, sizeof(CBouncerUser*) * ++m_UserCount);

	m_Users[m_UserCount - 1] = new CBouncerUser(Username);

	if (Password)
		m_Users[m_UserCount - 1]->SetPassword(Password);

	char Out[1024];

	snprintf(Out, sizeof(Out), "New user created: %s", Username);
	m_Log->WriteLine(Out);

	UpdateUserConfig();

	return m_Users[m_UserCount - 1];
}

bool CBouncerCore::RemoveUser(const char* Username) {
	for (int i = 0; i < m_UserCount; i++) {
		if (m_Users[i] && strcmpi(m_Users[i]->GetUsername(), Username) == 0) {
			delete m_Users[i];

			char Out[1024];

			snprintf(Out, sizeof(Out), "User removed: %s", Username);
			m_Log->WriteLine(Out);

			m_Users[i] = NULL;

			UpdateUserConfig();

			return true;
		}
	}

	return false;
}

void CBouncerCore::UpdateUserConfig(void) {
	char Out[1024] = "";

	for (int i = 0; i < m_UserCount; i++) {
		if (m_Users[i]) {
			if (*Out) {
				strncat(Out, " ", sizeof(Out));
				strncat(Out, m_Users[i]->GetUsername(), sizeof(Out));
			} else {
				strcpy(Out, m_Users[i]->GetUsername());
			}
		}
	}

	m_Config->WriteString("system.users", Out);
}

