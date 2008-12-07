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
#include <initguid.h>
#include <strsafe.h>

#include "RFS.h"
#include "Utils.h"
#include "RAR.h"
#include "Anchor.h"


// {9FFE11D2-29F2-463f-AD5F-C04A5EE2E58D}
DEFINE_GUID(CLSID_RARFileSource,
0x9ffe11d2, 0x29f2, 0x463f, 0xad, 0x5f, 0xc0, 0x4a, 0x5e, 0xe2, 0xe5, 0x8d);

#define RFS_GUID_STRING "{9FFE11D2-29F2-463f-AD5F-C04A5EE2E58D}"


//{1AC0BEBD-4D2B-45AD-BCEB-F2C41C5E3788}
DEFINE_GUID(MEDIASUBTYPE_Matroska,
0x1AC0BEBD, 0x4D2B, 0x45AD, 0xBC, 0xEB, 0xF2, 0xC4, 0x1C, 0x5E, 0x37, 0x88);

// Setup information
const AMOVIESETUP_MEDIATYPE sudPinTypes =
{
	&MEDIATYPE_Stream,		// Major type
	&MEDIASUBTYPE_NULL		// Minor type
};

const AMOVIESETUP_PIN sudpPin =
{
	L"Output",		// Pin name
	FALSE,			// Is it rendered
	TRUE,			// Is it an output
	FALSE,			// Zero instances allowed
	FALSE,			// Multiple instances allowed
	&CLSID_NULL,	// Connects to filter
	NULL,			// Connects to pin
	1,				// Number of types
	&sudPinTypes	// Pin types
};

const AMOVIESETUP_FILTER sudRARFileSource =
{
	&CLSID_RARFileSource,	// Filter CLSID
	L"RAR File Source",		// Filter name
	MERIT_UNLIKELY,			// Filter merit
	1,						// Number of pins
	&sudpPin				// Pin information
};

// List of class IDs and creator functions for the class factory.

CFactoryTemplate g_Templates [] =
{
	{
		L"RAR File Source",
		&CLSID_RARFileSource,
		CRARFileSource::CreateInstance,
		NULL,
		&sudRARFileSource
	}
};

int g_cTemplates = sizeof (g_Templates) / sizeof (g_Templates[0]);


/* static */
file_type_t CRARFileSource::s_file_types [] =
{
	{ "asf", &MEDIASUBTYPE_Asf },
	{ "avi", &MEDIASUBTYPE_Avi },
	{ "mpg", &MEDIASUBTYPE_MPEG1System },
	{ "vob", &MEDIASUBTYPE_MPEG2_PROGRAM },
	{ "mkv", &MEDIASUBTYPE_Matroska },
	{ "mka", &MEDIASUBTYPE_Matroska },
	{ "mks", &MEDIASUBTYPE_Matroska },
	{ "mov", &MEDIASUBTYPE_QTMovie },
	{ "wav", &MEDIASUBTYPE_WAVE },
	{ "mp3", &MEDIASUBTYPE_MPEG1Audio },
	{ "mpa", &MEDIASUBTYPE_MPEG1Audio },
	{ "mpv", &MEDIASUBTYPE_MPEG1Video },
	{ "dat", &MEDIASUBTYPE_MPEG1VideoCD },
	{ NULL, NULL }
};


extern "C" BOOL WINAPI DllEntryPoint (HINSTANCE, ULONG, LPVOID);

BOOL WINAPI DllMain (HINSTANCE hDllHandle, DWORD dwReason, LPVOID lpReserved)
{
	return DllEntryPoint (hDllHandle, dwReason, lpReserved);
}

STDAPI DllRegisterServer ()
{
	HKEY key;
	LONG ret;

	ret = RegCreateKey (HKEY_CLASSES_ROOT, L"Media Type\\Extensions\\.rar", &key);

	if (ret != ERROR_SUCCESS)
	{
		ret = RegOpenKey (HKEY_CLASSES_ROOT, L"Media Type\\Extensions\\.rar", &key);

		if (ret != ERROR_SUCCESS)
			return S_FALSE;
	}

	ret = RegSetValueExA (key, "Source Filter", 0, REG_SZ, (BYTE *) RFS_GUID_STRING, (DWORD) strlen(RFS_GUID_STRING) + 1);

	if (ret != ERROR_SUCCESS)
	{
		RegCloseKey (key);
		return S_FALSE;
	}

	RegCloseKey (key);

	return AMovieDllRegisterServer2 (TRUE);
}

STDAPI DllUnregisterServer ()
{
	HKEY key;
	LONG ret;
	char value [40];
	DWORD len = sizeof (value);

	ret = RegOpenKey (HKEY_CLASSES_ROOT, L"Media Type\\Extensions\\.rar", &key);
	if (ret == ERROR_SUCCESS)
	{
		ret = RegQueryValueExA (key, "Source Filter", NULL, NULL, (BYTE *) value, &len);
		RegCloseKey (key);

		if (ret == ERROR_SUCCESS)
		{
			if (!_stricmp (value, RFS_GUID_STRING))
				RegDeleteKey (HKEY_CLASSES_ROOT, L"Media Type\\Extensions\\.rar");
		}
	}

	return AMovieDllRegisterServer2 (FALSE);
}

CRARFileSource::CRARFileSource(LPUNKNOWN punk, HRESULT *phr) :
	CBaseFilter (L"RAR File Source", punk, &m_lock, CLSID_RARFileSource),
	m_pin (this, &m_lock, phr),
	m_file_name (NULL),
	m_file (NULL)
{
	if (phr)
		*phr = S_OK;
} 

CRARFileSource::~CRARFileSource() 
{
	delete m_file_name;
	delete m_file;
}

CUnknown *CRARFileSource::CreateInstance (LPUNKNOWN punk, HRESULT *phr) // OK
{
	CRARFileSource *pNewObject = new CRARFileSource (punk, phr);

	if (pNewObject == NULL)
		*phr = E_OUTOFMEMORY;

	DbgSetModuleLevel (LOG_TRACE, 2);
	DbgLog((LOG_TRACE, 2, L"CRARFileSource::CreateInstance() succeded."));

	return pNewObject;
}

STDMETHODIMP CRARFileSource::NonDelegatingQueryInterface (REFIID riid, void **ppv)
{
	if (riid == IID_IFileSourceFilter)
		return GetInterface ((IFileSourceFilter *) this, ppv);
	else
		return CBaseFilter::NonDelegatingQueryInterface (riid, ppv);
}


//  IFileSourceFilter methods

STDMETHODIMP CRARFileSource::Load (LPCOLESTR lpwszFileName, const AM_MEDIA_TYPE *pmt)
{
	DWORD dwBytesRead;
	DWORD mem_offset = 0;
	char *filename = NULL;
	wchar_t *current_rar_filename = NULL, *rar_ext;
	bool multi_volume = false, file_complete = false, new_numbering = false;
	rar_header_t rh;
	BYTE marker [7];
	BYTE expected [7] = { 0x52, 0x61, 0x72, 0x21, 0x1A, 0x07, 0x00 };
	FilePart *new_part, *prev_part;
	LONGLONG collect = 0;
	DWORD ret;
	DWORD files = 0;

	if (!lpwszFileName)
		return E_POINTER;

	if (m_file)
	{
		DbgLog ((LOG_TRACE, 2, L"CRARFileSource::Load called with file already loaded."));
		return E_UNEXPECTED;
	}

	m_file = new File;

	if (!m_file)
	{
		ErrorMsg (0, L"Out of memory.");
		return E_OUTOFMEMORY;
	}

	Anchor<File> af (&m_file);
	ArrayAnchor<wchar_t> acrf (&current_rar_filename);

	CAutoLock lck (&m_lock);

	int cch = lstrlen (lpwszFileName) + 1;

	if (m_file_name)
		delete m_file_name;

	m_file_name = new WCHAR [cch];
	if (!m_file_name)
	{
		ErrorMsg (0, L"Out of memory.");
		return E_OUTOFMEMORY;
	}

	current_rar_filename = new WCHAR [cch];
	if (!current_rar_filename)
	{
		ErrorMsg (0, L"Out of memory.");
		return E_OUTOFMEMORY;
	}

	CopyMemory (m_file_name, lpwszFileName, cch * sizeof (WCHAR));
	CopyMemory (current_rar_filename, lpwszFileName, cch * sizeof (WCHAR));

	if (pmt)
		m_file->media_type = *pmt;
	else
	{
		m_file->media_type.InitMediaType ();
		m_file->media_type.SetType (&MEDIATYPE_Stream);
		m_file->media_type.SetSubtype (&MEDIASUBTYPE_NULL);
	}

	rar_ext = wcsrchr (current_rar_filename, '.');

	DbgLog ((LOG_TRACE, 2, L"Loading file \"%s\".", m_file_name));
	HANDLE hFile = CreateFile (m_file_name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE) 
	{
		ErrorMsg (GetLastError (), L"Could not open file \"%s\"", m_file_name);
		return S_FALSE;
	}
	Anchor<HANDLE> ha(&hFile);

	// Scan through archive(s)
	while (true)
	{
		// Read marker.
		if (!ReadFile (hFile, marker, 7, &dwBytesRead, NULL) || dwBytesRead != 7)
		{
			ErrorMsg (GetLastError (), L"Could not read RAR header.");
			return S_FALSE;
		}

		if (memcmp (marker, expected, 7))
		{
			ErrorMsg (0, L"Incorrect RAR marker.");
			return S_FALSE;
		}

		// Read archive header.
		if (ReadHeader (hFile, &rh))
			return S_FALSE;

		if (rh.ch.type != HEADER_TYPE_ARCHIVE)
		{
			ErrorMsg (0, L"Unexpected RAR header type.");
			return S_FALSE;
		}

		DbgLog ((LOG_TRACE, 2, L"Header CRC %04x  TYPE %02x  FLAGS %04x  SIZE %04x",
			rh.ch.crc, rh.ch.type, rh.ch.flags, rh.ch.size));

		if (rh.ch.flags & MHD_PASSWORD)
		{
			ErrorMsg (0, L"Encrypted RAR files are not supported.");
			return S_FALSE;
		}

		new_numbering = new_numbering || (rh.ch.flags & MHD_NEWNUMBERING);
		multi_volume = multi_volume || (rh.ch.flags & MHD_FIRSTVOLUME);

		// Find file headers
		while (true)
		{
			// Read file header.
			if (ret = ReadHeader (hFile, &rh))
			{
				if (ret == ERROR_HANDLE_EOF)
					break;
				else
					return S_FALSE;
			}

			DbgLog ((LOG_TRACE, 2, L"Header CRC %04x  TYPE %02x  FLAGS %04x  SIZE %04x",
				rh.ch.crc, rh.ch.type, rh.ch.flags, rh.ch.size));

			if (rh.ch.type == HEADER_TYPE_END)
			{
				if (!(rh.ch.flags & EARC_NEXT_VOLUME))
					multi_volume = false;

				break;
			}
			if (rh.ch.type != HEADER_TYPE_FILE)
			{
				DbgLog ((LOG_TRACE, 2,L"Skipping unknown header type %02x.", rh.ch.type));
				continue;
			}

			DbgLog ((LOG_TRACE, 2, L"SIZE %d,%d  OS %02x  CRC %08x  TIMESTAMP %08x  VERSION %d  METHOD %02x  LEN %04x  ATTRIB %08x",
				rh.fh.low_size, rh.fh.high_size, rh.fh.os, rh.fh.crc, rh.fh.timestamp, rh.fh.version, rh.fh.method, rh.fh.name_len, rh.fh.attributes));

			DbgLog ((LOG_TRACE, 2, L"FILENAME \"%S\"", rh.fh.filename));

			multi_volume = multi_volume || (rh.ch.flags & LHD_SPLIT_AFTER);

			if (filename)
			{
				if (strcmp (filename, rh.fh.filename))
				{
					delete rh.fh.filename;
					SetFilePointer (hFile, rh.bytesRemaining, 0, FILE_CURRENT);
					DbgLog ((LOG_TRACE, 2, L"Incorrect file found."));
					continue;
				}

				delete rh.fh.filename;

				if (!(rh.ch.flags & LHD_SPLIT_BEFORE))
				{
					ErrorMsg (0, L"LHD_SPLIT_BEFORE flag was not set as expected.");
					return S_FALSE;
				}
				break;
			}
			else
			{
				char *ext = strrchr (rh.fh.filename, '.');

				if (!ext)
				{
					delete rh.fh.filename;
					SetFilePointer (hFile, rh.bytesRemaining, 0, FILE_CURRENT);
					DbgLog ((LOG_TRACE, 2, L"No file extension."));
					continue;
				}

				ext ++;
				int i;

				for (i = 0; s_file_types [i].extension; i ++)
				{
					if (!_stricmp (ext, s_file_types [i].extension))
						break;
				}

				if (!s_file_types [i].extension)
				{
					delete rh.fh.filename;
					SetFilePointer (hFile, rh.bytesRemaining, 0, FILE_CURRENT);
					DbgLog ((LOG_TRACE, 2, L"Unknown file extension."));
					continue;
				}

				if (!pmt)
					m_file->media_type.SetSubtype (s_file_types [i].guid);

				if (rh.ch.flags & LHD_SPLIT_BEFORE)
				{
					delete rh.fh.filename;
					SetFilePointer (hFile, rh.bytesRemaining, 0, FILE_CURRENT);
					DbgLog ((LOG_TRACE, 2, L"Not at the beginning of the file."));
					continue;
				}

				if (rh.ch.flags & LHD_PASSWORD)
				{
					delete rh.fh.filename;
					SetFilePointer (hFile, rh.bytesRemaining, 0, FILE_CURRENT);
					DbgLog ((LOG_TRACE, 2, L"Encrypted RAR files are not supported."));
					continue;
				}

				if (rh.fh.method != 0x30)
				{
					delete rh.fh.filename;
					SetFilePointer (hFile, rh.bytesRemaining, 0, FILE_CURRENT);
					DbgLog ((LOG_TRACE, 2, L"Compressed files are not supported."));
				}

				filename = rh.fh.filename;
				break;
			}
		}

		if (filename)
		{
			new_part = new FilePart ();

			if (!new_part)
			{
				ErrorMsg (0, L"Out of memory.");
				return E_OUTOFMEMORY;
			}

			// Is this the 1st part?
			if (!m_file->parts)
			{
				m_file->size = rh.fh.size;
				m_file->parts = 1;
				m_file->list = new_part;
			}
			else
			{
				m_file->parts ++;
				prev_part->next = new_part;
			}
			prev_part = new_part;

			new_part->in_rar_offset = SetFilePointer (hFile, 0, 0, FILE_CURRENT);
			new_part->in_file_offset = mem_offset;
			new_part->size = rh.bytesRemaining;
			collect += rh.bytesRemaining;

			new_part->file = CreateFile (current_rar_filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
			if (new_part->file == INVALID_HANDLE_VALUE) 
			{
				ErrorMsg (GetLastError (), L"Could not open file \"%s\".", current_rar_filename);
				return FALSE;
			}

			mem_offset += rh.bytesRemaining;

			if (!(rh.ch.flags & LHD_SPLIT_AFTER))
			{
				ASSERT (m_file->size == mem_offset);
				file_complete = true;
				break;
			}
		}

		if (!multi_volume)
			break;

		// Open the next file.

		if (rar_ext)
		{
			if (!files)
			{
				if (multi_volume && new_numbering)
					rar_ext -= 2;
				else
					rar_ext += 2;
			}
		}
		else
		{
			ErrorMsg (0, L"Input file does not end with .rar");
			return S_FALSE;
		}

		files ++;

		StringCchPrintf (rar_ext, 3, L"%02d", new_numbering ? files + 1 : files - 1);
		if (new_numbering)
			rar_ext [2] = L'.';

		DbgLog ((LOG_TRACE, 2, L"Loading file \"%s\".", current_rar_filename));
		ha.Close();
		hFile = CreateFile (current_rar_filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
		if (hFile == INVALID_HANDLE_VALUE) 
		{
			ErrorMsg (GetLastError (), L"Could not open file \"%s\".", current_rar_filename);
			return FALSE;
		}
	}

	if (!filename)
	{
		ErrorMsg (0, L"No media files found in the archive.");
		return S_FALSE;
	}

	delete filename;

	if (!file_complete)
	{
		ErrorMsg (0, L"Couldn't find all archive volumes.");
		return S_FALSE;
	}

	if (collect != m_file->size)
	{
		DbgLog ((LOG_TRACE, 2, L"The file is not the sum of it's parts. expected = %d, actual = %d", m_file->size, collect));
		return S_FALSE;
	}

	m_file->array = new FilePart [m_file->parts];

	if (!m_file->array)
	{
		ErrorMsg (0, L"Out of memory.");
		return E_OUTOFMEMORY;
	}

	FilePart *fp = m_file->list;
	m_file->list = NULL;
	for (int i = 0; i < m_file->parts; i ++)
	{
		FilePart *tmp;
		memcpy (m_file->array + i, fp, sizeof (FilePart));
		tmp = fp;
		fp = fp->next;
		tmp->file = INVALID_HANDLE_VALUE;
		delete tmp;
	}

	af.Release ();
	m_pin.SetFile (m_file);

	return S_OK;
}

// Behaves like IPersistFile::Load
STDMETHODIMP CRARFileSource::GetCurFile (LPOLESTR *ppszFileName, AM_MEDIA_TYPE *pmt)
{
	if (ppszFileName)
		return E_POINTER;

	if (m_file_name != NULL)
	{
		DWORD n = sizeof (WCHAR) * (lstrlen (m_file_name) + 1);

		*ppszFileName = (LPOLESTR) CoTaskMemAlloc (n);

		if (*ppszFileName != NULL)
			CopyMemory (*ppszFileName, m_file_name, n);
		else
			return E_OUTOFMEMORY;
	}
	else
		*ppszFileName = NULL;

	if (pmt != NULL)
		CopyMediaType (pmt, &m_file->media_type);

	return NOERROR;
}
