/*
 * Copyright (C) 2008, OV2 <overfiend@sessionclan.de>
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

#ifndef MEDIATYPE_H
#define MEDIATYPE_H

#include "List.h"

class CheckByteDetails
{
public:
	CheckByteDetails () : offset (0), byteCount (0), mask (NULL), value (NULL) { }

	~CheckByteDetails ()
	{
		if (mask)
			delete [] mask;
		if (value)
			delete [] value;
	}

	LONGLONG offset;
	unsigned int byteCount;
	BYTE *mask;
	BYTE *value;
};

class CheckByteGroup : public Node<CheckByteGroup>
{
public:
	CheckByteGroup () : checkBytes (NULL), checkByteCount (0) { }

	~CheckByteGroup ()
	{
		if (checkBytes)
			delete [] checkBytes;
	}

	CheckByteDetails *checkBytes;
	unsigned int checkByteCount;
};

class MediaType : public Node<MediaType>
{
public:
	MediaType () : majorType (GUID_NULL), subType (GUID_NULL), checkByteGroupCount (0) { }

	~MediaType ()
	{
		checkByteGroups.Clear ();
	}

	GUID majorType;
	GUID subType;
	List<CheckByteGroup> checkByteGroups;
	unsigned int checkByteGroupCount;
};

int getMediaTypeList (List<MediaType> *mediaTypeList);
int checkFileForMediaType (File *file, List<MediaType> *mediaTypeList, MediaType **foundMediaType);

#endif // MEDIATYPE_H
