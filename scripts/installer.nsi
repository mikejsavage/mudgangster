!include "MUI2.nsh"

Name "Mud Gangster"
Outfile "MudGangsterInstaller.exe"

InstallDir "$PROGRAMFILES64\Mud Gangster"
RequestExecutionLevel admin

!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

Section "Install" SectionInstall
	# Install stuff
	SetOutPath $INSTDIR
	File ..\release\mudgangster.exe

	# Start menu shortcut
	CreateDirectory "$SMPROGRAMS\Mud Gangster"
	CreateShortCut "$SMPROGRAMS\Mud Gangster\Mud Gangster.lnk" "$INSTDIR\mudgangster.exe"

	# Uninstaller
	WriteUninstaller "$INSTDIR\uninstall.exe"

	# Registry keys
	SetRegView 64
	WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\MudGangster" "DisplayName" "Mud Gangster"
	WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\MudGangster" "UninstallString" "$INSTDIR\uninstall.exe"
	WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\MudGangster" "DisplayIcon" "$INSTDIR\mudgangster.exe"
	WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\MudGangster" "Publisher" "Yes Son"
	WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\MudGangster" "DisplayVersion" "0.0.0.0"
	WriteRegDWORD HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\MudGangster" "NoModify" 1
	WriteRegDWORD HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\MudGangster" "NoRepair" 1
	
	SectionGetSize ${SectionInstall} $0
	IntFmt $1 "0x%08X" $0
	WriteRegDWORD HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\MudGangster" "EstimatedSize" $1
SectionEnd

Section "Uninstall"
	# Files
	Delete "$INSTDIR\MudGangster.exe"
	Delete "$INSTDIR\uninstall.exe"
	RMDir "$INSTDIR"

	# Start menu shortcut
	Delete "$SMPROGRAMS\Mud Gangster\Mud Gangster.lnk"
	RMDir "$SMPROGRAMS\Mud Gangster"

	# Registry keys
	SetRegView 64
	DeleteRegKey HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\MudGangster"
SectionEnd
