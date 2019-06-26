!include "MUI2.nsh"

Name "Mud Gangster Example Scripts"
Outfile "MudGangsterExampleScriptsInstaller.exe"

InstallDir "$APPDATA\Mud Gangster\scripts"
RequestExecutionLevel admin

!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES

Section "Install" SectionInstall
	# Install stuff
	SetOutPath $INSTDIR
	FILE /r ..\example_scripts\*
SectionEnd
