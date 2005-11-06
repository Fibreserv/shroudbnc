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

class CIRCConnection;

typedef struct chanmode_s {
	char Mode;
	char* Parameter;
} chanmode_t;

class CNick;
class CBanlist;

class CChannel {
	char* m_Name;

	chanmode_t* m_Modes;
	int m_ModeCount;
	char* m_TempModes;

	char* m_Topic;
	char* m_TopicNick;
	time_t m_TopicStamp;
	int m_HasTopic;
	bool m_ModesValid;
	bool m_HasBans;

	time_t m_Creation;

	CHashtable<CNick*, false, 64, true>* m_Nicks;

	bool m_HasNames;

	CIRCConnection* m_Owner;

	CBanlist* m_Banlist;

	chanmode_t* AllocSlot(void);
	chanmode_t* FindSlot(char Mode);
public:
#ifndef SWIG
	CChannel(const char* Name, CIRCConnection* Owner);
#endif
	virtual ~CChannel(void);

	virtual const char* GetName(void);

	virtual const char* GetChanModes(void);
	virtual void ParseModeChange(const char* source, const char* modes, int pargc, const char** pargv);

	virtual time_t GetCreationTime(void);
	virtual void SetCreationTime(time_t T);

	virtual const char* GetTopic(void);
	virtual void SetTopic(const char* Topic);

	virtual const char* GetTopicNick(void);
	virtual void SetTopicNick(const char* Nick);

	virtual time_t GetTopicStamp(void);
	virtual void SetTopicStamp(time_t TS);

	virtual int HasTopic(void);
	virtual void SetNoTopic(void);

	virtual void AddUser(const char* Nick, const char* ModeChar);
	virtual void RemoveUser(const char* Nick);
	virtual char GetHighestUserFlag(const char* ModeChars);

	virtual bool HasNames(void);
	virtual void SetHasNames(void);
	virtual CHashtable<CNick*, false, 64, true>* GetNames(void);

	virtual void ClearModes(void);
	virtual bool AreModesValid(void);
	virtual void SetModesValid(bool Valid);

	virtual CBanlist* GetBanlist(void);
	virtual void SetHasBans(void);
	virtual bool HasBans(void);
};

#ifndef SWIG
void DestroyCChannel(CChannel* P);
#endif
