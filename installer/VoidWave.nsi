; Void Wave 1.0 - NSIS Installer
; Installs VST3 to C:\Program Files\Common Files\VST3\
; Installs Standalone to C:\Program Files\Void Audio\Void Wave\

!define PRODUCT_NAME      "Void Wave"
!define PRODUCT_VERSION   "1.0.0"
!define PRODUCT_PUBLISHER "Void Audio"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"

; Paths are relative to the repo root (where makensis.exe is invoked from)
; COPY_PLUGIN_AFTER_BUILD copies the bundle here during the CI build step
!define VST3_SRC  "C:\Program Files\Common Files\VST3\Void Wave.vst3"
!define EXE_SRC   "build\VoidWave_artefacts\Release\Standalone\Void Wave.exe"

Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "installer\VoidWave-Setup.exe"
InstallDir "$PROGRAMFILES64\${PRODUCT_PUBLISHER}\${PRODUCT_NAME}"
RequestExecutionLevel admin
SetCompressor /SOLID lzma

!include "MUI2.nsh"
!define MUI_ABORTWARNING
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "English"

; -- Sections -----------------------------------------------------------------

Section "VST3 Plugin (required)" SecVST3
    SectionIn RO

    SetOutPath "$COMMONFILES64\VST3"
    File /r "${VST3_SRC}"

    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "DisplayName"     "${PRODUCT_NAME}"
    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "DisplayVersion"  "${PRODUCT_VERSION}"
    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "Publisher"       "${PRODUCT_PUBLISHER}"
    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "UninstallString" '"$INSTDIR\Uninstall.exe"'
    WriteUninstaller "$INSTDIR\Uninstall.exe"
SectionEnd

Section "Standalone Application" SecStandalone
    SetOutPath "$INSTDIR"
    File "${EXE_SRC}"

    CreateDirectory "$SMPROGRAMS\${PRODUCT_PUBLISHER}"
    CreateShortcut "$SMPROGRAMS\${PRODUCT_PUBLISHER}\${PRODUCT_NAME}.lnk" "$INSTDIR\Void Wave.exe"
    CreateShortcut "$DESKTOP\${PRODUCT_NAME}.lnk" "$INSTDIR\Void Wave.exe"
SectionEnd

; -- Section descriptions -----------------------------------------------------

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SecVST3}       "VST3 plugin for Ableton, Reaper, FL Studio and other VST3 hosts."
  !insertmacro MUI_DESCRIPTION_TEXT ${SecStandalone} "Standalone application - run Void Wave without a DAW."
!insertmacro MUI_FUNCTION_DESCRIPTION_END

; -- Uninstaller --------------------------------------------------------------

Section "Uninstall"
    RMDir /r "$COMMONFILES64\VST3\Void Wave.vst3"
    Delete   "$INSTDIR\Void Wave.exe"
    Delete   "$INSTDIR\Uninstall.exe"
    RMDir    "$INSTDIR"
    RMDir    "$PROGRAMFILES64\${PRODUCT_PUBLISHER}"

    Delete "$SMPROGRAMS\${PRODUCT_PUBLISHER}\${PRODUCT_NAME}.lnk"
    RMDir  "$SMPROGRAMS\${PRODUCT_PUBLISHER}"
    Delete "$DESKTOP\${PRODUCT_NAME}.lnk"

    DeleteRegKey HKLM "${PRODUCT_UNINST_KEY}"
SectionEnd
