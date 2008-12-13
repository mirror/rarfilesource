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

#ifndef RFS_H
#define RFS_H

#include "OutputPin.h"

class FilePart
{
public:
	FilePart (void) : next (NULL), file (INVALID_HANDLE_VALUE),
		in_rar_offset (0), in_file_offset (0), size (0) { }
	~FilePart (void) { if (file != INVALID_HANDLE_VALUE) CloseHandle (file); }

	FilePart *next;

	HANDLE file;

	LONGLONG in_rar_offset;
	LONGLONG in_file_offset;
	LONGLONG size;
};

class File : public Node<File>
{
public:
	File (void) : size (0), parts (0), list (NULL), array (NULL), filename(NULL),
		type_known (false), unsupported(false) { }

	~File (void)
	{
		FilePart *fp = list;
		while (fp)
		{
			FilePart *tmp = fp;
			fp = fp->next;
			delete tmp;
		}
		delete [] array;
		delete [] filename;
	}

	CMediaType media_type;
	LONGLONG size;
	int parts;

	FilePart *list;
	FilePart *array;

	char *filename;
	bool type_known;
	bool unsupported;
};

typedef struct
{
	const char *extension;
	const GUID *guid;
} file_type_t;

class CRARFileSource :
	public CBaseFilter,
	public IFileSourceFilter
{
public:
	DECLARE_IUNKNOWN;

	static CUnknown * WINAPI CreateInstance (LPUNKNOWN punk, HRESULT *phr);

	// Reveals IFileSourceFilter
	STDMETHODIMP NonDelegatingQueryInterface (REFIID riid, void **ppv);

	// CBaseFilter
	int GetPinCount () { return 1; }
	CBasePin *GetPin (int n) { return n == 0 ? &m_pin : NULL; }

	// IFileSourceFilter interface
	STDMETHODIMP Load (LPCOLESTR lpwszFileName, const AM_MEDIA_TYPE *pmt);
	STDMETHODIMP GetCurFile (LPOLESTR *ppszFileName, AM_MEDIA_TYPE *pmt);

private:
	CRARFileSource (LPUNKNOWN punk, HRESULT *phr);
	~CRARFileSource ();

	int ScanArchive (wchar_t *archive_name, List<File> *file_list, int *known_files_found);
	static int CALLBACK DlgFileList (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

	CRFSOutputPin m_pin;
	CCritSec m_lock;
	LPWSTR m_file_name;
	File *m_file;

	static file_type_t s_file_types [];
};

#endif // RFS_H
