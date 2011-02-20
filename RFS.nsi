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
!include x64.nsh

!define VERSION "v0.9.1"

Name "RAR File Source"
OutFile "RARFileSource-${VERSION}.exe"

!define REGKEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\RARFileSource"

InstallDirRegKey HKLM ${REGKEY} "InstallLocation"

RequestExecutionLevel admin

!insertmacro MUI_PAGE_WELCOME

!define MUI_PAGE_CUSTOMFUNCTION_PRE ComponentsPre
!define MUI_PAGE_CUSTOMFUNCTION_LEAVE ComponentsLeave
!insertmacro MUI_PAGE_COMPONENTS

!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

!insertmacro MUI_LANGUAGE "English"

!define LIBRARY_IGNORE_VERSION

!macro checkRedist TARGET
	SetRegView 32
checkInstalled:
	ReadRegDWORD $0 HKLM "Software\Microsoft\VisualStudio\10.0\VC\VCRedist\${TARGET}" "Installed"
	IntCmp $0 1 redistInstalled
	ReadRegDWORD $0 HKLM "Software\Microsoft\VisualStudio\10.0\VC\Runtimes\${TARGET}" "Installed"
	IntCmp $0 1 redistInstalled
	MessageBox MB_ABORTRETRYIGNORE "The Microsoft Visual C++ 2010 Redistributable Package (${TARGET}) is not installed.$\nWithout the necessary runtime DLLs the filter will not work.$\n$\nFind it at http://www.microsoft.com/downloads/ and try again." IDIGNORE redistInstalled IDRETRY checkInstalled
	Quit
redistInstalled:
!macroend

Section "Install 32-bit filter" SEC_X86
	!insertmacro checkRedist x86
	SetOutPath "$INSTDIR"
	!insertmacro InstallLib REGDLL NOTSHARED REBOOT_NOTPROTECTED "Release\RARFileSource.ax" "$INSTDIR\RARFileSource.ax" "$INSTDIR"
SectionEnd

Section "Install 64-bit filter" SEC_X64
	!insertmacro checkRedist x64
	SetOutPath "$INSTDIR\x64"
	!define LIBRARY_X64
	!insertmacro InstallLib REGDLL NOTSHARED REBOOT_NOTPROTECTED "x64\Release\RARFileSource.ax" "$INSTDIR\x64\RARFileSource.ax" "$INSTDIR\x64"
SectionEnd

Section "-Uninstaller"
	SetRegView 32
	SetOutPath "$INSTDIR"
	WriteUninstaller "$INSTDIR\Uninstall.exe"
	WriteRegStr HKLM ${REGKEY} "DisplayName" "RAR File Source"
	WriteRegStr HKLM ${REGKEY} "DisplayVersion" "${VERSION}"
	WriteRegStr HKLM ${REGKEY} "HelpLink" "http://www.v12pwr.com/RARFileSource/"
	WriteRegStr HKLM ${REGKEY} "InstallLocation" "$INSTDIR"
	WriteRegStr HKLM ${REGKEY} "UninstallString" "$INSTDIR\Uninstall.exe"
	WriteRegStr HKLM ${REGKEY} "URLInfoAbout" "http://www.v12pwr.com/RARFileSource/"
	WriteRegDWORD HKLM ${REGKEY} "NoModify" 1
	WriteRegDWORD HKLM ${REGKEY} "NoRepair" 1
SectionEnd

Function ComponentsPre
	${IfNot} ${RunningX64}
		!insertmacro UnselectSection ${SEC_X64}
		Abort
	${EndIf}
FunctionEnd

Function ComponentsLeave
	!insertmacro SectionFlagIsSet ${SEC_X86} ${SF_SELECTED} ok_x86 check_x64

ok_x86:
	StrCpy $INSTDIR "$PROGRAMFILES\RARFileSource"
	Goto end

check_x64:
	!insertmacro SectionFlagIsSet ${SEC_X64} ${SF_SELECTED} ok_x64 abort

ok_x64:
	StrCpy $INSTDIR "$PROGRAMFILES64\RARFileSource"
	Goto end

abort:
	MessageBox MB_OK|MB_ICONEXCLAMATION "You must select at least one component."
	Abort

end:
FunctionEnd

Section "Uninstall"
	!undef LIBRARY_X64
	!insertmacro UnInstallLib REGDLL NOTSHARED REBOOT_NOTPROTECTED "$INSTDIR\RARFileSource.ax"
	!define LIBRARY_X64
	!insertmacro UnInstallLib REGDLL NOTSHARED REBOOT_NOTPROTECTED "$INSTDIR\x64\RARFileSource.ax"
	Delete "$INSTDIR\Uninstall.exe"
	RMDir "$INSTDIR\x64"
	RMDir "$INSTDIR"
	SetRegView 32
	DeleteRegKey HKLM ${REGKEY}
SectionEnd
