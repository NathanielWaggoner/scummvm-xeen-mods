; Installer for the modded ScummVM build of World of Xeen.
; Build with:  ISCC.exe xeen-mods.iss   (run stage.sh first to refresh the staged files)

#define AppName "World of Xeen Modded"
#define AppVersion "1.0"

[Setup]
AppId={{6E0B5C0E-6F2B-4C39-9B7A-3C1D7F5A9E21}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
DefaultDirName={localappdata}\Programs\WorldOfXeenModded
DefaultGroupName={#AppName}
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
LicenseFile=stage\COPYING.txt
InfoBeforeFile=README.txt
OutputDir=Output
OutputBaseFilename=WorldOfXeenModded-Setup
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
UninstallDisplayName={#AppName}
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible

[Types]
Name: "full"; Description: "All mods"
Name: "convenience"; Description: "Convenience mods only (nothing that changes difficulty)"
Name: "custom"; Description: "Choose individual mods"; Flags: iscustom

[Components]
Name: "realtime"; Description: "Real-time mode with pause (Enter toggles)"; Types: full
Name: "backpack"; Description: "Larger backpack (36 slots per category instead of 9)"; Types: full
Name: "bulktrain"; Description: "Bulk training (train every affordable level in one press)"; Types: full convenience
Name: "healall"; Description: "Temple heal-all (A heals the whole party)"; Types: full convenience
Name: "levelmark"; Description: "Level-up marker on portraits"; Types: full convenience
Name: "itemstats"; Description: "Item stats (damage and armor class shown without Identify)"; Types: full
Name: "spaceconfirm"; Description: "Space and Enter confirm prompts"; Types: full convenience
Name: "tabswitch"; Description: "Tab switches character in dialogs"; Types: full convenience
Name: "mapnotes"; Description: "Map notes (click the map to add notes)"; Types: full convenience
Name: "mapzoom"; Description: "Map zoom and drag"; Types: full convenience
Name: "autoplay"; Description: "Auto-play (P makes the party explore and fight by itself)"; Types: full
Name: "safecauldrons"; Description: "Safe cauldrons (searching vats and cauldrons does no damage)"; Types: full
Name: "searchpreview"; Description: "Search preview (prompts say what searching will do)"; Types: full
Name: "maxstats"; Description: "Maximum starting stats for new characters"; Types: full
Name: "starterkit"; Description: "Starter kit for new characters"; Types: full

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"

[Files]
Source: "stage\scummvm.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "stage\*.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "stage\COPYING.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "README.txt"; DestDir: "{app}"; Flags: ignoreversion

[Dirs]
Name: "{app}\saves"
Name: "{app}\gamedata"

[Icons]
Name: "{group}\{#AppName}"; Filename: "{app}\scummvm.exe"; Parameters: "--config=""{app}\scummvm-mod.ini"" worldofxeen"; WorkingDir: "{app}"
Name: "{group}\Read me"; Filename: "{app}\README.txt"
Name: "{group}\Uninstall {#AppName}"; Filename: "{uninstallexe}"
Name: "{userdesktop}\{#AppName}"; Filename: "{app}\scummvm.exe"; Parameters: "--config=""{app}\scummvm-mod.ini"" worldofxeen"; WorkingDir: "{app}"; Tasks: desktopicon

[UninstallDelete]
; The copied game data and the settings go; saved games are left in place
Type: filesandordirs; Name: "{app}\gamedata"
Type: files; Name: "{app}\scummvm-mod.ini"

[Code]
const
  GogKey = 'SOFTWARE\GOG.com\Games\1207661233';

var
  GamePage: TInputDirWizardPage;

function IsGameFolder(const Dir: String): Boolean;
begin
  Result := (Dir <> '') and FileExists(AddBackslash(Dir) + 'XEEN.CC')
    and FileExists(AddBackslash(Dir) + 'DARK.CC');
end;

function HaveGameData: Boolean;
begin
  Result := IsGameFolder(ExpandConstant('{app}\gamedata'));
end;

function DetectGameFolder: String;
begin
  Result := '';
  if not RegQueryStringValue(HKLM32, GogKey, 'path', Result) then
    RegQueryStringValue(HKCU, GogKey, 'path', Result);
  if not IsGameFolder(Result) then
    Result := '';
end;

procedure InitializeWizard;
begin
  GamePage := CreateInputDirPage(wpSelectDir,
    'Find your copy of World of Xeen',
    'The installer copies the game data from your own copy of the game.',
    'Select the folder World of Xeen is installed in (the one containing XEEN.CC and DARK.CC), then click Next. ' +
    'If you are updating an existing install you can leave this empty to keep the game data already copied.',
    False, '');
  GamePage.Add('');
  GamePage.Values[0] := DetectGameFolder;
end;

function NextButtonClick(CurPageID: Integer): Boolean;
begin
  Result := True;
  if CurPageID = GamePage.ID then begin
    if (GamePage.Values[0] = '') and HaveGameData then
      Exit;

    if not IsGameFolder(GamePage.Values[0]) then begin
      MsgBox('That folder does not contain XEEN.CC and DARK.CC. Please select the folder World of Xeen is installed in.',
        mbError, MB_OK);
      Result := False;
    end;
  end;
end;

procedure CopyGameData(const Source: String);
var
  Dest, Name: String;
  FindRec: TFindRec;
begin
  Dest := ExpandConstant('{app}\gamedata\');
  ForceDirectories(Dest);

  FileCopy(AddBackslash(Source) + 'XEEN.CC', Dest + 'XEEN.CC', False);
  FileCopy(AddBackslash(Source) + 'DARK.CC', Dest + 'DARK.CC', False);
  if FileExists(AddBackslash(Source) + 'INTRO.CC') then
    FileCopy(AddBackslash(Source) + 'INTRO.CC', Dest + 'INTRO.CC', False);

  { The GOG release keeps the CD audio as MUSIC\xeenNN.ogg; ScummVM looks for trackNN.ogg beside the game data }
  if FindFirst(AddBackslash(Source) + 'MUSIC\xeen*.ogg', FindRec) then begin
    try
      repeat
        Name := 'track' + Copy(FindRec.Name, 5, Length(FindRec.Name));
        FileCopy(AddBackslash(Source) + 'MUSIC\' + FindRec.Name, Dest + Name, False);
      until not FindNext(FindRec);
    finally
      FindClose(FindRec);
    end;
  end;

  { Other releases may already have the tracks under the name ScummVM wants }
  if FindFirst(AddBackslash(Source) + 'track*.ogg', FindRec) then begin
    try
      repeat
        FileCopy(AddBackslash(Source) + FindRec.Name, Dest + FindRec.Name, False);
      until not FindNext(FindRec);
    finally
      FindClose(FindRec);
    end;
  end;
end;

procedure SetDefault(const Section, Key, Value, Ini: String);
begin
  if not IniKeyExists(Section, Key, Ini) then
    SetIniString(Section, Key, Value, Ini);
end;

procedure SetMod(const Key, Component, Ini: String);
begin
  if WizardIsComponentSelected(Component) then
    SetIniString('worldofxeen', Key, 'true', Ini)
  else
    SetIniString('worldofxeen', Key, 'false', Ini);
end;

procedure WriteSettings;
var
  Ini, App: String;
begin
  App := ExpandConstant('{app}');
  Ini := App + '\scummvm-mod.ini';

  { General settings are only given a value if there is not one already, so that
    changes made in ScummVM's own options screens survive running the installer again }
  SetDefault('scummvm', 'fullscreen', 'true', Ini);
  SetDefault('scummvm', 'aspect_ratio', 'true', Ini);
  SetDefault('scummvm', 'filtering', 'false', Ini);
  SetIniString('scummvm', 'savepath', App + '\saves', Ini);

  SetIniString('worldofxeen', 'description', 'Might and Magic: World of Xeen (Modded)', Ini);
  SetIniString('worldofxeen', 'path', App + '\gamedata\', Ini);
  SetIniString('worldofxeen', 'engineid', 'mm', Ini);
  SetIniString('worldofxeen', 'gameid', 'worldofxeen', Ini);
  SetDefault('worldofxeen', 'language', 'en', Ini);
  SetDefault('worldofxeen', 'platform', 'pc', Ini);
  SetDefault('worldofxeen', 'RealTimeExploreMs', '2000', Ini);
  SetDefault('worldofxeen', 'RealTimeCombatMs', '3000', Ini);

  SetMod('RealTime', 'realtime', Ini);
  SetMod('LargeBackpack', 'backpack', Ini);
  SetMod('BulkTraining', 'bulktrain', Ini);
  SetMod('TempleHealAll', 'healall', Ini);
  SetMod('LevelUpMarker', 'levelmark', Ini);
  SetMod('ItemStats', 'itemstats', Ini);
  SetMod('SpaceConfirms', 'spaceconfirm', Ini);
  SetMod('TabSwitchesCharacter', 'tabswitch', Ini);
  SetMod('MapNotes', 'mapnotes', Ini);
  SetMod('MapZoom', 'mapzoom', Ini);
  SetMod('AutoPlay', 'autoplay', Ini);
  SetMod('SafeCauldrons', 'safecauldrons', Ini);
  SetMod('SearchPreview', 'searchpreview', Ini);
  SetMod('MaxStartStats', 'maxstats', Ini);
  SetMod('StarterKit', 'starterkit', Ini);

  { Settings from before each mod had its own switch }
  DeleteIniEntry('worldofxeen', 'QualityOfLife', Ini);
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssPostInstall then begin
    if IsGameFolder(GamePage.Values[0]) then
      CopyGameData(GamePage.Values[0]);
    WriteSettings;
  end;
end;
