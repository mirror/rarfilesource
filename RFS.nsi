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

!include MUI2.nsh
!include Library.nsh

!define VERSION "v0.9"

Name "RAR File Source"
OutFile "RARFileSource-${VERSION}.exe"
InstallDir "$PROGRAMFILES\RARFileSource"

!define REGKEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\RARFileSource"

InstallDirRegKey HKLM ${REGKEY} "InstallLocation"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

!insertmacro MUI_LANGUAGE "English"

Section "Install"

checkInstalled:
	StrCpy $2 'Microsoft.VC90.CRT,version="9.0.21022.8",type="win32",processorArchitecture="x86",publicKeyToken="1fc8b3b9a1e18e3b"'

	System::Call 'sxs::CreateAssemblyCache(*i .R0, i 0) i .r0'
	IntCmp $0 0 cacOK checkRegistry checkRegistry

cacOK:
	# Allocate ASSEMBLY_INFO struct
	System::Call '*(i 24, i 0, l, i 0, i 0) i .R1'
	IntCmp $R1 0 checkRegistry

	# IAssemblyCache::QueryAssemblyInfo
	System::Call "$R0->4(i 0, w '$2', i $R1) i .r0"
	IntCmp $0 0 qaiOK freeMemErr freeMemErr

qaiOK:
	# Extract ASSEMBLY_INFO.dwAssemblyFlags
	System::Call '*$R1(i, i .r0)'

	# Test ASSEMBLYINFO_FLAG_INSTALLED
	IntOp $0 $0 & 1
	IntCmp $0 1 freeMem freeMemErr freeMemErr

freeMem:
	System::Free $R1
	Goto redistInstalled

freeMemErr:
	System::Free $R1

checkRegistry:
	ReadRegDWORD $0 HKLM "Software\Microsoft\DevDiv\VC\Servicing\9.0\RED\1033" "Install"
	IntCmp $0 1 redistInstalled
	MessageBox MB_ABORTRETRYIGNORE "The Microsoft Visual C++ 2008 Redistributable Package is not installed.$\nWithout the necessary runtime DLLs the filter will not work.$\n$\nFind it at http://www.microsoft.com/downloads/ and try again." IDIGNORE redistInstalled IDRETRY checkInstalled
	Quit

redistInstalled:
	SetOutPath "$INSTDIR"
	!define LIBRARY_IGNORE_VERSION
	!insertmacro InstallLib REGDLL NOTSHARED REBOOT_NOTPROTECTED "Release\RARFileSource.ax" "$INSTDIR\RARFileSource.ax" "$INSTDIR"
	WriteUninstaller "$INSTDIR\Uninstall.exe"
	WriteRegStr HKLM ${REGKEY} "DisplayName" "RAR File Source ${VERSION}"
	WriteRegStr HKLM ${REGKEY} "DisplayVersion" "${VERSION}"
	WriteRegStr HKLM ${REGKEY} "HelpLink" "http://www.v12pwr.com/RARFileSource/"
	WriteRegStr HKLM ${REGKEY} "InstallLocation" "$INSTDIR"
	WriteRegStr HKLM ${REGKEY} "UninstallString" "$INSTDIR\Uninstall.exe"
	WriteRegStr HKLM ${REGKEY} "URLInfoAbout" "http://www.v12pwr.com/RARFileSource/"
	WriteRegDWORD HKLM ${REGKEY} "NoModify" 1
	WriteRegDWORD HKLM ${REGKEY} "NoRepair" 1
SectionEnd

Section "Uninstall"
	!insertmacro UnInstallLib REGDLL NOTSHARED REBOOT_NOTPROTECTED "$INSTDIR\RARFileSource.ax"
	Delete "$INSTDIR\Uninstall.exe"
	RMDir "$INSTDIR"
	DeleteRegKey HKLM ${REGKEY}
SectionEnd
