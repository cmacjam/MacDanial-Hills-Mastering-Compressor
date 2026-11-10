; Windows installer for MacDanial Hills Mastering Compressor (VST3).
; Built in CI with:  ISCC /DAppVersion=1.0.0 /DSourceVST3="<path to .vst3>" /DOutputDir=<dir> installer.iss

#ifndef AppVersion
  #define AppVersion "1.0.0"
#endif
#ifndef SourceVST3
  #define SourceVST3 "..\..\Combined\build\MacDanialHillsStack_artefacts\Release\VST3\MacDanial Hills Mastering Compressor.vst3"
#endif
#ifndef OutputDir
  #define OutputDir "..\..\installer-out"
#endif

[Setup]
AppId={{6E2C1A0B-6D4B-4C55-9A61-4D3F2B8A7C11}
AppName=MacDanial Hills Mastering Compressor
AppVersion={#AppVersion}
AppPublisher=Basemental
DefaultDirName={commoncf64}\VST3\MacDanial Hills Mastering Compressor.vst3
DisableDirPage=yes
DisableProgramGroupPage=yes
OutputDir={#OutputDir}
OutputBaseFilename=MacDanial Hills Mastering Compressor {#AppVersion} Windows Installer
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayName=MacDanial Hills Mastering Compressor

[Files]
Source: "{#SourceVST3}\*"; DestDir: "{commoncf64}\VST3\MacDanial Hills Mastering Compressor.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs

[Messages]
WelcomeLabel2=This installs the MacDanial Hills Mastering Compressor VST3 (Model A, Model B and the bus compressor in one plug-in) by Basemental.%n%nClose your DAW before continuing, then rescan plug-ins after installing.
