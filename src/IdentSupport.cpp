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

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CIdentSupport::CIdentSupport(void) {
	m_Ident = NULL;
}

CIdentSupport::~CIdentSupport(void) {
	free(m_Ident);
}

void CIdentSupport::SetIdent(const char *Ident) {
#ifndef _WIN32
	char *homedir = getenv("HOME");

	char *Out = (char *)malloc(strlen(homedir) + 50);

	if (Out == NULL) {
		LOGERROR("malloc failed. Could not set new ident (%s).", Ident);

		return;
	}

	if (homedir) {
		snprintf(Out, strlen(homedir) + 50, "%s/.oidentd.conf", homedir);

		FILE *identConfig = fopen(Out, "w");

		if (identConfig) {
			char *Buf = (char *)malloc(strlen(Ident) + 50);

			snprintf(Buf, strlen(Ident) + 50, "global { reply \"%s\" }", Ident);
			fputs(Buf, identConfig);

			free(Buf);

			fclose(identConfig);
		}
	}

	free(Out);
#endif

	free(m_Ident);
	m_Ident = strdup(Ident);

	if (m_Ident == NULL) {
		LOGERROR("strdup failed. Could not set new ident.");
	}
}

const char *CIdentSupport::GetIdent(void) {
	return m_Ident;
}