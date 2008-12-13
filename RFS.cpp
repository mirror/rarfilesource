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
#include "resource.h"
#include "Mediatype.h"


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
	int i;
	wchar_t key_name [] = L"Media Type\\Extensions\\.rar";

	ret = RegCreateKey (HKEY_CLASSES_ROOT, key_name, &key);

	if (ret != ERROR_SUCCESS)
	{
		ret = RegOpenKey (HKEY_CLASSES_ROOT, key_name, &key);

		if (ret != ERROR_SUCCESS)
			return ret;
	}

	ret = RegSetValueExA (key, "Source Filter", 0, REG_SZ, (BYTE *) RFS_GUID_STRING, (DWORD) strlen(RFS_GUID_STRING) + 1);

	if (ret != ERROR_SUCCESS)
	{
		RegCloseKey (key);
		return ret;
	}

	RegCloseKey (key);

	for (i = 0; i < 100; i ++)
	{
		key_name [24] = L'0' + i / 10;
		key_name [25] = L'0' + i % 10;

		ret = RegCreateKey (HKEY_CLASSES_ROOT, key_name, &key);

		if (ret != ERROR_SUCCESS)
		{
			ret = RegOpenKey (HKEY_CLASSES_ROOT, key_name, &key);

			if (ret != ERROR_SUCCESS)
				continue;
		}

		ret = RegSetValueExA (key, "Source Filter", 0, REG_SZ, (BYTE *) RFS_GUID_STRING, (DWORD) strlen(RFS_GUID_STRING) + 1);

		RegCloseKey (key);
	}

	return AMovieDllRegisterServer2 (TRUE);
}

STDAPI DllUnregisterServer ()
{
	HKEY key;
	LONG ret;
	char value [40];
	DWORD len = sizeof (value);
	wchar_t key_name [] = L"Media Type\\Extensions\\.rar";
	int i;

	for (i = 0; i < 101; i ++)
	{
		ret = RegOpenKey (HKEY_CLASSES_ROOT, key_name, &key);
		if (ret == ERROR_SUCCESS)
		{
			ret = RegQueryValueExA (key, "Source Filter", NULL, NULL, (BYTE *) value, &len);
			RegCloseKey (key);

			if (ret == ERROR_SUCCESS)
			{
				if (!_stricmp (value, RFS_GUID_STRING))
					RegDeleteKey (HKEY_CLASSES_ROOT, key_name);
			}
		}
		key_name [24] = L'0' + i / 10;
		key_name [25] = L'0' + i % 10;
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

#define HEADER_SKIP_FILE \
	delete [] rh.fh.filename; \
	SetFilePointerEx (hFile, rh.bytesRemaining, NULL, FILE_CURRENT); \
	continue;

int CRARFileSource::ScanArchive (wchar_t *archive_name, List<File> *file_list, int *ok_files_found)
{
	DWORD dwBytesRead;
	char *filename = NULL;
	wchar_t *current_rar_filename = NULL, *rar_ext;
	bool first_archive_file = true;
	bool multi_volume = false, new_numbering = false;
	rar_header_t rh;
	BYTE marker [7];
	BYTE expected [7] = { 0x52, 0x61, 0x72, 0x21, 0x1A, 0x07, 0x00 };
	FilePart *new_part, *prev_part;
	LONGLONG collected;
	DWORD ret;
	DWORD files = 0, volumes = 0;
	int volume_digits;
	File *file = NULL;

	*ok_files_found = 0;
	LARGE_INTEGER zero = {0};

	MediaType *mType;
	List<MediaType> mediaTypeList;

	Anchor<File> af (&file);
	ArrayAnchor<wchar_t> acrf (&current_rar_filename);

	int cch = lstrlen (archive_name) + 1;

	current_rar_filename = new wchar_t [cch];
	if (!current_rar_filename)
	{
		ErrorMsg (0, L"Out of memory.");
		return FALSE;
	}

	CopyMemory (current_rar_filename, archive_name, cch * sizeof (wchar_t));

	rar_ext = wcsrchr (current_rar_filename, '.');

	DbgLog ((LOG_TRACE, 2, L"Loading file \"%s\".", current_rar_filename));
	HANDLE hFile = CreateFile (current_rar_filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE) 
	{
		ErrorMsg (GetLastError (), L"Could not open file \"%s\"", current_rar_filename);
		return FALSE;
	}
	Anchor<HANDLE> ha(&hFile);

	if (getMediaTypeList (&mediaTypeList) == -1)
		return FALSE;		// this means out of memory

	// Scan through archive volume(s)
	while (true)
	{
		// Read marker.
		if (!ReadFile (hFile, marker, 7, &dwBytesRead, NULL) || dwBytesRead != 7)
		{
			ErrorMsg (GetLastError (), L"Could not read RAR header.");
			return FALSE;
		}

		if (memcmp (marker, expected, 7))
		{
			ErrorMsg (0, L"Incorrect RAR marker.");
			return FALSE;
		}

		// Read archive header.
		if (ReadHeader (hFile, &rh))
			return FALSE;

		LOG_HEADER (&rh);

		if (rh.ch.type != HEADER_TYPE_ARCHIVE)
		{
			ErrorMsg (0, L"Unexpected RAR header type.");
			return FALSE;
		}

		if (rh.ch.flags & MHD_PASSWORD)
		{
			ErrorMsg (0, L"Encrypted RAR volumes are not supported.");
			return FALSE;
		}

		if (first_archive_file)
		{
			first_archive_file = false;

			new_numbering = rh.ch.flags & MHD_NEWNUMBERING;
			multi_volume = rh.ch.flags & MHD_VOLUME;

			if (multi_volume)
			{
				if (!rar_ext)
				{
					ErrorMsg (0, L"Input file does not end with .rar");
					return FALSE;
				}

				// Locate volume counter
				if (new_numbering)
				{
					volume_digits = 0;
					do
					{
						rar_ext --;
						volume_digits ++;
					} while (iswdigit (*(rar_ext - 1)));
				}
				else
				{
					rar_ext += 2;
					volume_digits = 2;
				}

				if (!(rh.ch.flags & MHD_FIRSTVOLUME))
				{
					DbgLog ((LOG_TRACE, 2, L"Rewinding to the first file in the set."));

					if (new_numbering)
					{
						StringCchPrintf (rar_ext, volume_digits + 1, L"%0*d", volume_digits, 1);
						rar_ext [volume_digits] = L'.';
					}
					else
					{
						rar_ext [0] = L'a';
						rar_ext [1] = L'r';
					}

					DbgLog ((LOG_TRACE, 2, L"Loading file \"%s\".", current_rar_filename));
					ha.Close();
					hFile = CreateFile (current_rar_filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
					if (hFile == INVALID_HANDLE_VALUE) 
					{
						ErrorMsg (GetLastError (), L"Could not open file \"%s\".", current_rar_filename);
						return FALSE;
					}
					continue;
				}
			}
		}
		else
		{
			ASSERT (new_numbering == (bool) (rh.ch.flags & MHD_NEWNUMBERING));
			ASSERT (rh.ch.flags & MHD_VOLUME);
		}

		// Find file headers
		while (true)
		{
			// Read file header.
			if (ret = ReadHeader (hFile, &rh))
			{
				if (ret == ERROR_HANDLE_EOF)
					break;
				else
					return FALSE;
			}

			LOG_HEADER (&rh);

			if (rh.ch.type == HEADER_TYPE_END)
			{
				// TODO: Verify that the volumne number in the header matches our volume counter.
				if (!(rh.ch.flags & EARC_NEXT_VOLUME))
					multi_volume = false;

				break;
			}
			if (rh.ch.type != HEADER_TYPE_FILE)
			{
				SetFilePointerEx (hFile, rh.bytesRemaining, NULL, FILE_CURRENT);
				DbgLog ((LOG_TRACE, 2,L"Skipping unknown header of type %02x.", rh.ch.type));
				continue;
			}

			DbgLog ((LOG_TRACE, 2, L"SIZE %08lx %08lx  OS %02x  CRC %08lx  TIMESTAMP %08lx  VERSION %d  METHOD %02x  LEN %04lx  ATTRIB %08lx",
				rh.fh.size.HighPart, rh.fh.size.LowPart, rh.fh.os, rh.fh.crc, rh.fh.timestamp, rh.fh.version, rh.fh.method, rh.fh.name_len, rh.fh.attributes));

			DbgLog ((LOG_TRACE, 2, L"FILENAME \"%S\"", rh.fh.filename));

			if (filename)
			{
				if (strcmp (filename, rh.fh.filename))
				{
					// TODO: We should probably dump the old file start fresh with this one
					// since the lazy error handling at other places may cause us to end up here.
					DbgLog ((LOG_TRACE, 2, L"Incorrect file found."));
					HEADER_SKIP_FILE
				}

				if (!(rh.ch.flags & LHD_SPLIT_BEFORE))
				{
					// TODO: Decide if it's better to ignore the missing flag.
					DbgLog ((LOG_TRACE, 2, L"LHD_SPLIT_BEFORE flag was not set as expected."));
					HEADER_SKIP_FILE
				}

				delete [] rh.fh.filename;
			}
			else
			{
				if (rh.ch.flags & LHD_SPLIT_BEFORE)
				{
					// TODO: Decide if it's better to just abort the entire scanning process here.
					DbgLog ((LOG_TRACE, 2, L"Not at the beginning of the file."));
					HEADER_SKIP_FILE
				}

				files ++;
				collected = 0;

				ASSERT (!file);

				file = new File ();

				if (!file)
				{
					ErrorMsg (0, L"Out of memory.");
					return FALSE;
				}

				file->media_type.SetType (&MEDIATYPE_Stream);
				file->media_type.SetSubtype (&MEDIASUBTYPE_NULL);
				file->filename = rh.fh.filename;
				file->size = rh.fh.size.QuadPart;
				filename = rh.fh.filename;

				if (rh.ch.flags & LHD_PASSWORD)
				{
					DbgLog ((LOG_TRACE, 2, L"Encrypted files are not supported."));
					file->unsupported = true;
				}

				if (rh.fh.method != 0x30)
				{
					DbgLog ((LOG_TRACE, 2, L"Compressed files are not supported."));
					file->unsupported = true;
				}
			}

			if (!file->unsupported)
			{
				new_part = new FilePart ();

				if (!new_part)
				{
					ErrorMsg (0, L"Out of memory.");
					return FALSE;
				}

				// Is this the 1st part?
				if (!file->parts)
				{
					file->parts = 1;
					file->list = new_part;
				}
				else
				{
					file->parts ++;
					prev_part->next = new_part;
				}
				prev_part = new_part;

				SetFilePointerEx (hFile, zero, (LARGE_INTEGER*) &new_part->in_rar_offset, FILE_CURRENT);
				new_part->in_file_offset = collected;
				new_part->size = rh.bytesRemaining.QuadPart;

				new_part->file = CreateFile (current_rar_filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
				if (new_part->file == INVALID_HANDLE_VALUE)
				{
					ErrorMsg (GetLastError (), L"Could not open file \"%s\".", current_rar_filename);
					return FALSE;
				}
			}

			collected += rh.bytesRemaining.QuadPart;
			SetFilePointerEx (hFile, rh.bytesRemaining, NULL, FILE_CURRENT);

			// Is file complete?
			if (!(rh.ch.flags & LHD_SPLIT_AFTER))
			{
				if (!file->unsupported && file->size != collected)
					DbgLog ((LOG_TRACE, 2, L"The file is not the sum of it's parts. expected = %lld, actual = %lld", file->size, collected));

				if (file->parts)
				{
					file->array = new FilePart [file->parts];

					if (!file->array)
					{
						ErrorMsg (0, L"Out of memory.");
						return FALSE;
					}

					FilePart *fp = file->list;
					file->list = NULL;
					for (int i = 0; i < file->parts; i ++)
					{
						FilePart *tmp;
						memcpy (file->array + i, fp, sizeof (FilePart));
						tmp = fp;
						fp = fp->next;
						tmp->file = INVALID_HANDLE_VALUE;
						delete tmp;
					}
				}

				if(!file->unsupported)
				{
					if (!checkFileForMediaType (file,&mediaTypeList,&mType))
						return FALSE;		// this means out of memory

					if(mType)
					{
#ifdef _DEBUG
						LPOLESTR majorType, subType;
						StringFromCLSID (mType->majorType, &majorType);
						StringFromCLSID (mType->subType, &subType);
						DbgLog ((LOG_TRACE, 2, L"Filetype detection determined:\nMajor type: %s\nSubtype: %s", majorType, subType));
						CoTaskMemFree(majorType);
						CoTaskMemFree(subType);
#endif
						file->media_type.SetType(&mType->majorType);
						file->media_type.SetSubtype(&mType->subType);
						file->type_known = true;
						(*ok_files_found)++;
					}
				}

				//TODO: should we fall back to extensions if automatic detection does not work ?
				/* 
				if(!file->type_known)
				{
					char *ext = strrchr (file->filename, '.');

					if (ext)
					{
						ext ++;
						int i;

						for (i = 0; s_file_types [i].extension; i ++)
						{
							if (!_stricmp (ext, s_file_types [i].extension))
								break;
						}

						if (s_file_types [i].extension)
						{
							file->media_type.SetSubtype (s_file_types [i].guid);
							file->type_known = true;
							if (!file->unsupported)
								(*ok_files_found) ++;
						}
						else
							DbgLog ((LOG_TRACE, 2, L"Unknown file extension."));
					}
					else
						DbgLog ((LOG_TRACE, 2, L"No file extension."));
				}*/

				if (filename != file->filename)
					delete filename;

				filename = NULL;

				file_list->InsertLast (file);
				file = NULL;
			}
		}

		if (!multi_volume)
			break;

		// Open the next file.

		volumes ++;

		StringCchPrintf (rar_ext, volume_digits + 1, L"%0*d", volume_digits, new_numbering ? volumes + 1 : volumes - 1);
		if (new_numbering)
			rar_ext [volume_digits] = L'.';

		DbgLog ((LOG_TRACE, 2, L"Loading file \"%s\".", current_rar_filename));
		ha.Close();
		hFile = CreateFile (current_rar_filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
		if (hFile == INVALID_HANDLE_VALUE) 
		{
			ErrorMsg (GetLastError (), L"Could not open file \"%s\".", current_rar_filename);
			return FALSE;
		}
	}

	if (!files)
	{
		ErrorMsg (0, L"No files found in the archive.");
		return FALSE;
	}

	if (file)
	{
		// TODO: Decide if we should allow playback of truncated files.
		files --;
		delete file;
		ErrorMsg (0, L"Couldn't find all archive volumes.");
	}

	return files;
}

/* static */
int CALLBACK CRARFileSource::DlgFileList (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int index;

	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
		int len;
		List<File> *file_list = (List<File> *) lParam;
		File *file = file_list->First ();
		wchar_t *tempString;

		while (file)
		{
			if (file->unsupported)
				continue;

			len = strlen (file->filename) + 1;
			tempString = new wchar_t [len];
			MultiByteToWideChar (CP_ACP, 0, file->filename, -1, tempString, len);
			index = ListBox_AddString (GetDlgItem (hwndDlg, IDC_FILELIST), tempString);
			ListBox_SetItemData(GetDlgItem (hwndDlg, IDC_FILELIST), index, file);
			delete [] tempString;

			file = file_list->Next (file);
		}
		ListBox_SetCurSel (GetDlgItem (hwndDlg, IDC_FILELIST), 0);
		return TRUE;
	}
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
				index = ListBox_GetCurSel (GetDlgItem (hwndDlg, IDC_FILELIST));
				EndDialog (hwndDlg, ListBox_GetItemData(GetDlgItem (hwndDlg, IDC_FILELIST), index));
				return TRUE;

		case IDCANCEL:
				EndDialog (hwndDlg, NULL);
				return TRUE;

		case IDC_FILELIST:
			switch (HIWORD (wParam))
			{
				case LBN_DBLCLK:
					index = ListBox_GetCurSel ((HWND) lParam);
					EndDialog (hwndDlg, ListBox_GetItemData((HWND) lParam, index));
					return TRUE;
			}
		}
		break;

	case WM_CLOSE:
		EndDialog (hwndDlg, NULL);
		return TRUE;
	}

	return FALSE;
}

//  IFileSourceFilter methods

STDMETHODIMP CRARFileSource::Load (LPCOLESTR lpwszFileName, const AM_MEDIA_TYPE *pmt)
{
	List <File> file_list;
	int num_files, num_ok_files;
	CAutoLock lck (&m_lock);

	if (!lpwszFileName)
		return E_POINTER;

	if (m_file)
	{
		DbgLog ((LOG_TRACE, 2, L"CRARFileSource::Load called with file already loaded."));
		return E_UNEXPECTED;
	}

	int cch = lstrlen (lpwszFileName) + 1;

	if (m_file_name)
		delete m_file_name;

	m_file_name = new WCHAR [cch];
	if (!m_file_name)
	{
		ErrorMsg (0, L"Out of memory.");
		return E_OUTOFMEMORY;
	}

	CopyMemory (m_file_name, lpwszFileName, cch * sizeof (WCHAR));

	num_files = ScanArchive ((wchar_t *) lpwszFileName, &file_list, &num_ok_files);

	DbgLog ((LOG_TRACE, 2, L"Found %d files out of which %d are media files.", num_files, num_ok_files));

	if (!num_files || !num_ok_files)
		return E_UNEXPECTED; // TODO: Figure out a better error code.

	if (num_ok_files == 1)
	{
		m_file = file_list.First ();

		while (m_file->unsupported)
			m_file = file_list.Next (m_file);
	}
	else
	{
		m_file = (File *) DialogBoxParam (g_hInst, MAKEINTRESOURCE(IDD_FILELIST), 0, DlgFileList, (LPARAM)&file_list);

		if (!m_file)
			return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
	}

	if (pmt)
		m_file->media_type = *pmt;

	m_file->Unlink ();
	m_pin.SetFile (m_file);

	file_list.Clear ();

	return S_OK;
}

// Behaves like IPersistFile::Load
STDMETHODIMP CRARFileSource::GetCurFile (LPOLESTR *ppszFileName, AM_MEDIA_TYPE *pmt)
{
	if (!ppszFileName)
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
