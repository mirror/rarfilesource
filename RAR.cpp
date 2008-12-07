/*
 * Copyright (C) 2008, OctaneSnail <os@v12pwr.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <windows.h>
#include <streams.h>

#include "rar.h"
#include "utils.h"

#define READ_ITEM(item) { \
	if (!ReadFile (file, &item, sizeof (item), &read, NULL) || read != sizeof (item)) \
	{ \
		DWORD err = GetLastError (); \
		if (!read && !err) return ERROR_HANDLE_EOF; \
		ErrorMsg (err, L"Could not read RAR header"); \
		return S_FALSE; \
	} \
	acc += read; }

#define READ_ITEM2(item, size) { \
	if (!ReadFile (file, item, size, &read, NULL) || read != size) \
	{ \
		DWORD err = GetLastError (); \
		if (!read && !err) return ERROR_HANDLE_EOF; \
		ErrorMsg (err, L"Could not read RAR header"); \
		return S_FALSE; \
	} \
	acc += read; }

DWORD ReadHeader (HANDLE file, rar_header_t *dest)
{
	fixed_header_t fh;
	fixed_file_header_t ffh;
	DWORD read, acc = 0, dword;

	// Read fixed archive header.
	READ_ITEM (fh);

	dest->ch.crc = fh.crc;
	dest->ch.flags = fh.flags;
	dest->ch.type = fh.type;

	if (fh.flags & LONG_BLOCK)
	{
		READ_ITEM (dword);

		if ((MAXDWORD - dword) < fh.size)
		{
			ErrorMsg (0, L"Variable overflow while reading RAR header.");
			return S_FALSE;
		}
		dest->ch.size = dword + fh.size;
	}
	else
		dest->ch.size = fh.size;

	switch (fh.type)
	{
	case HEADER_TYPE_ARCHIVE:
		// Don't bother reading the reserved values.
		break;

	case HEADER_TYPE_FILE:
		READ_ITEM (ffh);

		dest->fh.low_size = ffh.size;
		dest->fh.os = ffh.os;
		dest->fh.crc = ffh.crc;
		dest->fh.timestamp = ffh.timestamp;
		dest->fh.version = ffh.version;
		dest->fh.method = ffh.method;
		dest->fh.name_len = ffh.name_len;
		dest->fh.attributes = ffh.attributes;

		if (fh.flags & LHD_LARGE)
		{
			READ_ITEM (dword);
			dest->fh.high_size = dword;
			dest->fh.size = ((DWORD64) dword << 32) | ffh.size;
			READ_ITEM (dword);
		}
		else
		{
			dest->fh.high_size = 0;
			dest->fh.size = ffh.size;
		}

		dest->fh.filename = new char [dest->fh.name_len + 1];
		if (!dest->fh.filename)
		{
			ErrorMsg (0, L"Out of memory while reading RAR header.");
			return ERROR_OUTOFMEMORY;
		}
		READ_ITEM2 (dest->fh.filename, dest->fh.name_len);
		dest->fh.filename [dest->fh.name_len] = 0;

		if (fh.flags & LHD_SALT)
			READ_ITEM2 (dest->fh.salt, 8);

		if (fh.flags & LHD_EXTTIME)
		{
			WORD flags;
			READ_ITEM (flags);
			for (int i= 0 ; i < 4; i++)
			{
				DWORD rmode = flags >> (3 - i) * 4;
				if (!(rmode & 8))
					continue;

				if (i != 0)
				{
					DWORD dosTime;
					READ_ITEM (dosTime);
				}

				int count = rmode & 3;
				for (int j = 0; j < count; j++)
				{
					BYTE byte;
					READ_ITEM (byte);
				}
			}
		}
	}

	if (acc > dest->ch.size)
	{
		ErrorMsg (0, L"Overrun while reading RAR header.");
		return S_FALSE;
	}

	if (fh.type != HEADER_TYPE_FILE && acc < dest->ch.size)
	{
		SetFilePointer (file, dest->ch.size - acc, 0, FILE_CURRENT);
//		DbgLog ((LOG_TRACE, 2, L"Skipped over %d header bytes.", dest->ch.size - acc));
		dest->bytesRemaining = 0;
	}
	else
		dest->bytesRemaining = dest->ch.size - acc;

	return ERROR_SUCCESS;
}
