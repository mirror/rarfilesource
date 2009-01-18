/*
 * Copyright (C) 2008-2009, OctaneSnail <os@v12pwr.com>
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

#include "File.h"

static int compare (const void *pos, const void *part)
{
	if (*((LONGLONG *) pos) < ((FilePart *) part)->in_file_offset)
		return -1;

	if (*((LONGLONG *) pos) >= ((FilePart *) part)->in_file_offset + ((FilePart *) part)->size)
		return 1;

	return 0;
}

int File::FindStartPart (LONGLONG position)
{
	if (position > size)
		return -1;

	// Check if the previous lookup up still matches.
	if (m_prev_part && !compare (&position, m_prev_part))
		return (int) (m_prev_part - array);

	m_prev_part = (FilePart *) bsearch (&position, array, parts, sizeof (FilePart), compare);

	if (!m_prev_part)
		return -1;

	return (int) (m_prev_part - array);
}
