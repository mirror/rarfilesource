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

/* 
   most of the information used in this file comes from
   http://msdn.microsoft.com/en-us/library/ms787558(VS.85).aspx
 */

#include <string.h>
#include <objbase.h>
#include <stdio.h>
#include <streams.h>

#include "Anchor.h"
#include "Utils.h"
#include "RFS.h"
#include "Mediatype.h"

// used to cast LONGLONG to LARGE_INTEGER
#define CAST_LARGE_INTEGER(X) (*(LARGE_INTEGER *)&(X))

static HRESULT SyncRead (File *file, LONGLONG llPosition, DWORD lLength, BYTE* pBuffer, LONG *cbActual);

/* getNextToken extracts the current token from the string and 
   and sets the starting point of the next token */
static void getNextToken(wchar_t **strTok,wchar_t **nextTokenStart)
{
	while(**strTok==TEXT(' ')) (*strTok)++;
	for(int i=0;i<lstrlen(*strTok);i++) {
		if((*strTok)[i]==TEXT(',')) {
			(*strTok)[i]=TEXT('\0');
			*nextTokenStart=*strTok+i+1;
			return;
		}
	}
	*nextTokenStart=NULL;
}

/* parseCheckBytes splits valueData and places the parsed values in the
   checkBytes array (allocated here) */
static int parseCheckBytes(wchar_t *valueData,CheckByteDetails **checkBytes)
{
	int tokenCount=0,byteDetailsCount=0;
	wchar_t *token,*nextTokenStart;
	for(int i=0;i<lstrlen(valueData);i++)
		if(valueData[i]==TEXT(','))
			tokenCount++;
	if((++tokenCount)%4)
		return 0;
	byteDetailsCount = tokenCount/4;
	*checkBytes = new CheckByteDetails[byteDetailsCount];

	if (!*checkBytes)
	{
		ErrorMsg (0, L"Out of memory.");
		return -1;
	}

	nextTokenStart=valueData;
	for(int i=0;i<byteDetailsCount;i++) {
		token=nextTokenStart;
		getNextToken(&token,&nextTokenStart);
		swscanf_s(token,TEXT(" %lld"),&(*checkBytes)[i].offset);			// offset in file
		token=nextTokenStart;
		getNextToken(&token,&nextTokenStart);
		swscanf_s(token,TEXT(" %u"),&(*checkBytes)[i].byteCount);			// byte count of mask and value
		(*checkBytes)[i].mask = new BYTE[(*checkBytes)[i].byteCount];
		(*checkBytes)[i].value = new BYTE[(*checkBytes)[i].byteCount];
		if (!(*checkBytes)[i].mask || !(*checkBytes)[i].value)
		{
			ErrorMsg (0, L"Out of memory.");
			return -1;
		}
		//TODO: if the next token is smaller than byteCount and not empty we could pad with zeroes
		token=nextTokenStart;
		getNextToken(&token,&nextTokenStart);
		if(*token==TEXT('\0'))														// no mask means all ones
			for(unsigned int j=0;j<(*checkBytes)[i].byteCount;j++)
				(*checkBytes)[i].mask[j]=0xFF;
		else																		// otherwise parse mask
			for(unsigned int j=0;j<(*checkBytes)[i].byteCount && j*2 < lstrlen(token);j++)
				swscanf_s(token+j*2,TEXT(" %2x"),&(*checkBytes)[i].mask[j]);
		token=nextTokenStart;
		getNextToken(&token,&nextTokenStart);
		for(unsigned int j=0;j<(*checkBytes)[i].byteCount && j*2 < lstrlen(token);j++)
			swscanf_s(token+j*2,TEXT(" %2x"),&(*checkBytes)[i].value[j]);			// parse value
	}
	return byteDetailsCount;
}

/* parses the HKEY_CLASSES_ROOT\Media Type registry key and
   fills the mediaTypeList with all valid byte marks and their
   corresponding major/subtypes */
int getMediaTypeList(List<MediaType> *mediaTypeList)
{
	//these values come from http://msdn.microsoft.com/en-us/library/ms724872(VS.85).aspx
#define MAX_VALUE_SIZE	16384
#define MAX_KEYNAME_SIZE 256
	wchar_t keyName[MAX_KEYNAME_SIZE];
	wchar_t subKeyName[MAX_KEYNAME_SIZE];
	wchar_t valueName[MAX_VALUE_SIZE];
	LONG ret,retSub,retVal;

	HKEY mTypeKey,majorTypeKey,subTypeKey;
	DWORD valueType,valueSize,valueNameSize;
	wchar_t *valueData;
	int mediaTypeCount=0;
	MediaType *newType;
	CheckByteGroup *newGroup;
	

	ret = RegOpenKey(HKEY_CLASSES_ROOT,TEXT("Media Type"),&mTypeKey);
	if(ret!=ERROR_SUCCESS)
		return -1;

	for(int i=0;ret!=ERROR_NO_MORE_ITEMS;i++) {
		ret = RegEnumKey(mTypeKey,i,keyName,MAX_KEYNAME_SIZE);
		if(ret!=ERROR_SUCCESS || !lstrcmp(keyName,TEXT("Extensions")))		// we don't want the Extensions subkey
			continue;

		retSub = RegOpenKey(mTypeKey,keyName,&majorTypeKey);
		if(retSub!=ERROR_SUCCESS)
			continue;

		for(int j=0;retSub!=ERROR_NO_MORE_ITEMS;j++) {
			retSub = RegEnumKey(majorTypeKey,j,subKeyName,MAX_KEYNAME_SIZE);
			if(retSub!=ERROR_SUCCESS)
				continue;

			retVal = RegOpenKey(majorTypeKey,subKeyName,&subTypeKey);

			newType = new MediaType;
			if (!newType)
			{
				ErrorMsg (0, L"Out of memory.");
				return -1;
			}
			CLSIDFromString(keyName,&newType->majorType);
			CLSIDFromString(subKeyName,&newType->subType);

			for(int k=0;retVal!=ERROR_NO_MORE_ITEMS;k++) {
				valueNameSize = MAX_VALUE_SIZE;
				retVal = RegEnumValue(subTypeKey,k,valueName,&valueNameSize,NULL,&valueType,NULL,&valueSize);
				if(retVal != ERROR_SUCCESS || valueType != REG_SZ || !lstrcmp(valueName,TEXT("Source Filter")))		// we don't need the source filter value
					continue;

				valueData = (wchar_t *)new BYTE[valueSize];
				valueNameSize = MAX_VALUE_SIZE;
				retVal = RegEnumValue(subTypeKey,k,valueName,&valueNameSize,NULL,&valueType,(LPBYTE)valueData,&valueSize);
				if(retVal != ERROR_SUCCESS)
					continue;

				newGroup = new CheckByteGroup;			// each value is one group
				if (!newGroup)
				{
					ErrorMsg (0, L"Out of memory.");
					return -1;
				}
				newGroup->checkByteCount = parseCheckBytes(valueData,&newGroup->checkBytes);

				if(newGroup->checkByteCount==-1)
					return -1;		// this means out of memory

				if(!newGroup->checkByteCount)
				{
					// if we get here there was a parse error or an invalid value in the registry
					delete newGroup;
				}
				else
				{
					newType->checkByteGroups.InsertLast(newGroup);
					newType->checkByteGroupCount++;					
				}

				delete valueData;
			}
			if(!newType->checkByteGroupCount)
			{
				// if we get here we were unable to parse any of the values in the current subkey
				delete newType;
			}
			else
			{
				mediaTypeList->InsertLast(newType);
				mediaTypeCount++;
			}
		}
	}
	return mediaTypeCount;
}

int checkFileForMediaType(File *file,List<MediaType> *mediaTypeList,MediaType **foundMediaType)
{
	MediaType *mt;
	CheckByteGroup *cbg;
	bool matches;
	BOOL ret;
	LONG lBytesRead;
	LONGLONG actOffset;
	BYTE *necessaryBytes;

	mt = mediaTypeList->First();
	while(mt!=NULL) {											// loop over all filetypes
		cbg = mt->checkByteGroups.First();
		while(cbg!=NULL) {										// loop over all groups of checkbytes
			matches = true;
			for(unsigned int i=0;i<cbg->checkByteCount;i++) {	// we need to match all fields in one group
				if(cbg->checkBytes[i].offset<0)
					actOffset = file->size + cbg->checkBytes[i].offset;
				else
					actOffset = cbg->checkBytes[i].offset;
				if(actOffset>file->size || actOffset < 0) {
					matches = false;
					break;
				}
				necessaryBytes = new BYTE[cbg->checkBytes[i].byteCount];
				if (!necessaryBytes)
				{
					ErrorMsg (0, L"Out of memory.");
					return 0;
				}
				// read the necessary amount of bytes to compare to value (after masking)
				ret = SyncRead(file,actOffset,cbg->checkBytes[i].byteCount,necessaryBytes,&lBytesRead);
				if(ret != S_OK) {
					matches = false;
					break;
				}
				// mask and compare all bytes in this entry
				for(unsigned int j=0;j<cbg->checkBytes[i].byteCount;j++) {
					if((necessaryBytes[j] & cbg->checkBytes[i].mask[j]) != cbg->checkBytes[i].value[j]) {
						matches = false;
						break;
					}
				}
				delete [] necessaryBytes;
				if(!matches)
					break;
			}
			if(matches)											// one group is enought to match the filetype
			{
				*foundMediaType = mt;
				return 1;
			}
			cbg = mt->checkByteGroups.Next(cbg);
		}
		mt = mediaTypeList->Next(mt);
	}
	*foundMediaType = NULL;
	return 1;
}

/* the next functions are copied from Outputpin.cpp with a few small modifications
   - they should probably merged to reduce redundancy but I didn't want to change too much
   of your classes */

static int compare (const void *pos, const void *part)
{
	if (*((LONGLONG *) pos) < ((FilePart *) part)->in_file_offset)
		return -1;

	if (*((LONGLONG *) pos) >= ((FilePart *) part)->in_file_offset + ((FilePart *) part)->size)
		return 1;

	return 0;
}

static int FindStartPart (File *file,LONGLONG position)
{
	FilePart *part;

	if (position > file->size)
		return -1;

	part = (FilePart *) bsearch (&position, file->array, file->parts, sizeof (FilePart), compare);

	if (!part)
		return -1;

	return (int) (part - file->array);
}

static HRESULT SyncRead (File *file, LONGLONG llPosition, DWORD lLength, BYTE* pBuffer, LONG *cbActual)
{
	OVERLAPPED o;
	LARGE_INTEGER offset;
	DWORD to_read, read, acc = 0;
	LONGLONG offset2;
	int pos;
#ifdef _DEBUG
	static int last_pos = -1;
#endif

	if (!file)
	{
		DbgLog((LOG_TRACE, 2, L"Mediatype.cpp - SyncRead called with no file loaded."));
		return E_UNEXPECTED;
	}

	if (!pBuffer)
		return E_POINTER;

	pos = FindStartPart (file,llPosition);
	if (pos == -1)
	{
		DbgLog((LOG_TRACE, 2, L"Mediatype.cpp - FindStartPart bailed length = %lu, pos = %lld", lLength, llPosition));
		return ERROR_HANDLE_EOF;
	}

#ifdef _DEBUG
	if (pos != last_pos)
	{
		DbgLog((LOG_TRACE, 2, L"Mediatype.cpp - Now reading volume %d.", pos));
		last_pos = pos;
	}
#endif
	FilePart *part = file->array + pos;

	offset2 = llPosition - part->in_file_offset;
	offset.QuadPart = part->in_rar_offset + offset2;

	memset (&o, 0, sizeof (o));

	if (!(o.hEvent = CreateEvent (NULL, FALSE, FALSE, NULL)))
	{
		ErrorMsg (GetLastError (), L"Mediatype.cpp - SyncRead - CreateEvent)");
		return (S_FALSE);
	}

	while (true)
	{
		read = 0;
		to_read = min (lLength, part->size - offset2);

		o.Offset = offset.LowPart;
		o.OffsetHigh = offset.HighPart;

		if (!ReadFile (part->file, pBuffer + acc, to_read, NULL, &o))
		{
			DWORD err = GetLastError ();

			if (err != ERROR_IO_PENDING)
			{
				ErrorMsg (err, L"Mediatype.cpp - SyncRead - ReadFile");
				break;
			}
		}
		if (!GetOverlappedResult (part->file, &o, &read, TRUE))
		{
			ErrorMsg (GetLastError (), L"Mediatype.cpp - SyncRead - GetOverlappedResult)");
			break;
		}
		lLength -= read;
		acc += read;

		if (lLength == 0)
		{
			CloseHandle (o.hEvent);
			if (cbActual)
				*cbActual = acc;
			return S_OK;
		}

		pos ++;

		if (pos >= file->parts)
			break;

		part ++;
		offset2 = 0;
		offset.QuadPart = part->in_rar_offset;
	}

	CloseHandle (o.hEvent);
	if (cbActual)
		*cbActual = acc;
	return S_FALSE;
}
