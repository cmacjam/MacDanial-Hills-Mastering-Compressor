; One Windows installer with everything in it (all parts selected by default).
; ISCC /DAppVersion=1.0.0 /DStage=<dir with unzipped builds> /DOutputDir=<dir> complete.iss
; Optional defines (set by CI when the file exists): HasAAX, HasApp, HasManual, HasA, HasB

#ifndef AppVersion
  #define AppVersion "1.0.0"
#endif
#define Main "MacDanial Hills Mastering Compressor"

[Setup]
AppId={{6E2C1A0B-6D4B-4C55-9A61-4D3F2B8A7C11}
AppName={#Main}
AppVersion={#AppVersion}
AppPublisher=Basemental
DefaultDirName={commonpf64}\MacDanial Hills
DisableDirPage=yes
DefaultGroupName=MacDanial Hills
DisableProgramGroupPage=yes
OutputDir={#OutputDir}
OutputBaseFilename={#Main} {#AppVersion} Windows Installer
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayName={#Main}

[Types]
Name: "full"; Description: "Everything"
Name: "custom"; Description: "Choose what to install"; Flags: iscustom

[Components]
Name: "vst3"; Description: "{#Main} - VST3 (Ableton Live, Cubase, Studio One, Reaper, FL Studio, Bitwig)"; Types: full custom
#ifdef HasAAX
Name: "aax";  Description: "{#Main} - AAX (Pro Tools)"; Types: full custom
#endif
#ifdef HasApp
Name: "app";  Description: "Standalone app and user manual"; Types: full custom
#endif
#ifdef HasA
Name: "unita"; Description: "MacDanial Hills A (single unit) - VST3"; Types: full custom
#endif
#ifdef HasB
Name: "unitb"; Description: "MacDanial Hills B (single unit) - VST3"; Types: full custom
#endif

[Files]
Source: "{#Stage}\{#Main}.vst3\*"; DestDir: "{commoncf64}\VST3\{#Main}.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
#ifdef HasAAX
Source: "{#Stage}\{#Main}.aaxplugin\*"; DestDir: "{commoncf64}\Avid\Audio\Plug-Ins\{#Main}.aaxplugin"; Components: aax; Flags: ignoreversion recursesubdirs createallsubdirs
#endif
#ifdef HasApp
Source: "{#Stage}\{#Main}.exe"; DestDir: "{app}"; Components: app; Flags: ignoreversion
#endif
#ifdef HasManual
Source: "{#Stage}\MacDanial Hills Manual.html"; DestDir: "{app}"; Components: app; Flags: ignoreversion
#endif
#ifdef HasA
Source: "{#Stage}\MacDanial Hills A.vst3\*"; DestDir: "{commoncf64}\VST3\MacDanial Hills A.vst3"; Components: unita; Flags: ignoreversion recursesubdirs createallsubdirs
#endif
#ifdef HasB
Source: "{#Stage}\MacDanial Hills B.vst3\*"; DestDir: "{commoncf64}\VST3\MacDanial Hills B.vst3"; Components: unitb; Flags: ignoreversion recursesubdirs createallsubdirs
#endif

[Icons]
#ifdef HasApp
Name: "{group}\{#Main}"; Filename: "{app}\{#Main}.exe"; Components: app
#endif
#ifdef HasManual
Name: "{group}\MacDanial Hills Manual"; Filename: "{app}\MacDanial Hills Manual.html"; Components: app
#endif

[Messages]
WelcomeLabel2=This installs the {#Main} by Basemental: Model A, Model B and the bus compressor in one plug-in, plus the standalone app, the user manual and the single-unit plug-ins.%n%nClose your DAW before continuing, then rescan plug-ins after installing.
