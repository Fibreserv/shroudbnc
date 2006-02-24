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

#define BLOCKSIZE 4096
#define OPTIMIZEBLOCKS 16

class CFIFOBuffer : public CZoneObject<CFIFOBuffer, 32> {
	char* m_Buffer; /**< the fifo buffer's data */
	unsigned int m_BufferSize; /**< the size of the buffer */
	unsigned int m_Offset; /**< the number of unused bytes at the
								beginning of the buffer */

	void *ResizeBuffer(void *Buffer, unsigned int OldSize, unsigned int NewSize);
	inline void Optimize(void);
public:
#ifndef SWIG
	CFIFOBuffer();
	virtual ~CFIFOBuffer();
#endif

	virtual unsigned int GetSize(void);

	virtual char *Peek(void);
	virtual char *Read(unsigned int Bytes);
	virtual void Flush(void);

	virtual void Write(const char *Data, unsigned int Size);
	virtual void WriteUnformattedLine(const char *Line);
};
