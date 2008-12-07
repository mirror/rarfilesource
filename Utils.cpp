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
#include <strsafe.h>

#include "Utils.h"

void ErrorMsg (DWORD errorCode, wchar_t *format, ...)
{
	wchar_t buffer [1024];
	wchar_t *end;
	size_t remaining;
    va_list argptr;

//	__asm int 3;

    va_start (argptr, format);
	StringCchVPrintfEx (buffer, 1024, &end, &remaining, 0, format, argptr);
    va_end (argptr);

	if (errorCode && remaining > 9)
	{
		wcscpy (end, L"\n\nError: ");
		end += 9;
		remaining -= 9;

		FormatMessage (FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
						NULL, errorCode, MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT), end, (DWORD) remaining, NULL);
	}

	DbgLog ((LOG_ERROR, 0, L"%s", buffer));
	MessageBox (NULL, buffer, L"RAR File Source", MB_OK | MB_ICONERROR);
}
