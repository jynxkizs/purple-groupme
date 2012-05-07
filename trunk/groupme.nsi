; Script based on the Skype4Pidgin and Off-the-Record Messaging NSI files


SetCompress off

; todo: SetBrandingImage
; HM NIS Edit Wizard helper defines
!define PRODUCT_NAME "purple-groupme"
!define PRODUCT_VERSION "1.1.0"
!define PRODUCT_PUBLISHER "Ken Patel"
!define PRODUCT_WEB_SITE "http://code.google.com/p/purple-groupme"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"

; MUI 1.67 compatible ------
!include "MUI.nsh"

; MUI Settings
!define MUI_ABORTWARNING
!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\modern-install.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\modern-uninstall.ico"

; Welcome page
!insertmacro MUI_PAGE_WELCOME
; License page
!insertmacro MUI_PAGE_LICENSE "COPYING"
; Instfiles page
!insertmacro MUI_PAGE_INSTFILES
!define MUI_FINISHPAGE_RUN
!define MUI_FINISHPAGE_RUN_TEXT "Run Pidgin"
!define MUI_FINISHPAGE_RUN_FUNCTION "RunPidgin"
!insertmacro MUI_PAGE_FINISH

; Uninstaller pages
;!insertmacro MUI_UNPAGE_INSTFILES

; Language files
!insertmacro MUI_LANGUAGE "English"

; MUI end ------

Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "${PRODUCT_NAME}.exe"

Var "PidginDir"

ShowInstDetails show
ShowUnInstDetails show

Section "MainSection" SEC01
    ;Check for pidgin installation
    Call GetPidginInstPath
    
    SetOverwrite try
    
	SetOutPath "$PidginDir\pixmaps\pidgin"
	File "/oname=protocols\16\groupme.png" "groupme16.png"
	File "/oname=protocols\22\groupme.png" "groupme22.png"
	File "/oname=protocols\48\groupme.png" "groupme48.png"
	File "/oname=protocols\scalable\groupme.svg" "groupme.svg"

	SetOutPath "$PidginDir\ca-certs"
	;File "login.groupme.com.pem"

    SetOverwrite try
	copy:
		ClearErrors
		Delete "$PidginDir\plugins\libgroupme.dll"
		IfErrors dllbusy
		SetOutPath "$PidginDir\plugins"
	    File "libgroupme.dll"
		Goto after_copy
	dllbusy:
		MessageBox MB_RETRYCANCEL "libgroupme.dll is busy. Please close Pidgin (including tray icon) and try again" IDCANCEL cancel
		Goto copy
	cancel:
		Abort "Installation of ${PRODUCT_NAME} aborted"
	after_copy:
	
	SetOutPath "$PidginDir"
	;File "libjson-glib-1.0.dll"
	
SectionEnd

Function GetPidginInstPath
  Push $0
  ReadRegStr $0 HKLM "Software\pidgin" ""
	IfFileExists "$0\pidgin.exe" cont
	ReadRegStr $0 HKCU "Software\pidgin" ""
	IfFileExists "$0\pidgin.exe" cont
		MessageBox MB_OK|MB_ICONINFORMATION "Failed to find Pidgin installation."
		Abort "Failed to find Pidgin installation. Please install Pidgin first."
  cont:
	StrCpy $PidginDir $0
FunctionEnd

Function RunPidgin
	ExecShell "" "$PidginDir\pidgin.exe"
FunctionEnd

