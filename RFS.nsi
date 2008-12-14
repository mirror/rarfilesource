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

!define VERSION "v0.8.1"

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
# TODO: Verify that the VC2008 redist is installed by checking DWORD key
# HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\DevDiv\VC\Servicing\9.0\RED\1033\Install
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
