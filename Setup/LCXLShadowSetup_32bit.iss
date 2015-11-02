; �ű��� Inno Setup �ű��� ���ɡ�
; �����ĵ���ȡ���� INNO SETUP �ű��ļ�����ϸ���ϣ�

#define MyAppName "LCXLShadowӰ�ӻ�ԭϵͳ"
#define MyAppVersion "0.3"
#define MyAppPublisher "�޳���"
#define MyAppURL "http://lcxl.5166.info/"
#define MyAppExeName "LCXLShadowConsole.exe"
#define MyConfigExeName "LCXLShadowConfig.exe"
; #define MySerExeName "LCXLShadowSer.exe"
[Setup]
; ע��: AppId ��ֵ��Ψһʶ���������ı�־��
; ��Ҫ������������ʹ����ͬ�� AppId ֵ��
; (�ڱ������е���˵������� -> ���� GUID�����Բ���һ���µ� GUID)
AppId={{66986511-18B9-49B3-9AF9-F97E8EA8274A}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
;AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={pf}\LCXLShadow
DefaultGroupName={#MyAppName}
LicenseFile=.\����ļ�.rtf
InfoBeforeFile=.\LCXLShadow����.rtf
InfoAfterFile=.\��װ��.txt
OutputDir=.\Output
OutputBaseFilename=LCXLShadowSetup_32bit
SetupIconFile=.\SetupIcon.ico
Compression=lzma
SolidCompression=true
WizardImageFile=WizardImageFile.bmp
WizardSmallImageFile=.\WizardSmallImageFile.bmp
AppCopyright=Copyright (C) 2012-2013 LCXL
;ArchitecturesAllowed=x86 x64
ArchitecturesAllowed=x86
ArchitecturesInstallIn64BitMode=x64

[Languages]
;Name: "default"; MessagesFile: "compiler:Default.isl"
Name: "chinesesimp"; MessagesFile: "compiler:Languages\ChineseSimp.isl"
Name: "english"; MessagesFile: "compiler:Languages\English.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"

[Files]
;32λ�µİ�װ
Source: "..\bin\i386\DriverTest.exe"; DestDir: "{app}"; Flags: ignoreversion; Check: not Is64BitInstallMode
Source: "..\bin\i386\LCXLShadow.dll"; DestDir: "{app}"; Flags: ignoreversion; Check: not Is64BitInstallMode
Source: "..\bin\i386\LCXLShadowConsole.exe"; DestDir: "{app}"; Flags: ignoreversion; Check: not Is64BitInstallMode
Source: "..\bin\i386\LCXLMsg.exe"; DestDir: "{app}"; Flags: ignoreversion; Check: not Is64BitInstallMode
Source: "..\bin\i386\LCXLShadowConfig.exe"; DestDir: "{app}"; Flags: ignoreversion; Check: not Is64BitInstallMode
Source: "..\bin\i386\LCXLShadowSer.exe"; DestDir: "{app}"; Flags: ignoreversion; Check: not Is64BitInstallMode
Source: "..\bin\i386\LCXLShadowDriver.sys"; DestDir: "{app}"; Flags: ignoreversion; Attribs: readonly hidden system; Check: not Is64BitInstallMode
;64λ�µİ�װ
Source: "..\bin\amd64\DriverTest.exe"; DestDir: "{app}"; Flags: ignoreversion; Check: Is64BitInstallMode
Source: "..\bin\amd64\LCXLShadow.dll"; DestDir: "{app}"; Flags: ignoreversion; Check: Is64BitInstallMode
Source: "..\bin\amd64\LCXLShadowConsole.exe"; DestDir: "{app}"; Flags: ignoreversion; Check: Is64BitInstallMode
Source: "..\bin\amd64\LCXLMsg.exe"; DestDir: "{app}"; Flags: ignoreversion; Check: Is64BitInstallMode
Source: "..\bin\amd64\LCXLShadowConfig.exe"; DestDir: "{app}"; Flags: ignoreversion; Check: Is64BitInstallMode
Source: "..\bin\amd64\LCXLShadowSer.exe"; DestDir: "{app}"; Flags: ignoreversion; Check: Is64BitInstallMode
Source: "..\bin\amd64\LCXLShadowDriver.sys"; DestDir: "{app}"; Flags: ignoreversion; Attribs: readonly hidden system; Check: Is64BitInstallMode
; ע��: ��Ҫ���κι����ϵͳ�ļ�ʹ�� "Flags: ignoreversion"

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{commondesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon
[Messages]
BeveledLabel=��Ȩ���� (C) 2012-2013 �޳���
[Run]
Filename: "{app}\{#MyConfigExeName}"; Parameters: "/Install";StatusMsg: "���ڳ�ʼ����������..."
Filename: "{app}\{#MyAppExeName}"; Parameters: "/Install";StatusMsg: "���ڳ�ʼ������̨����..."
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, "&", "&&")}}"; Flags: nowait postinstall skipifsilent
[UninstallRun]
Filename: "{app}\{#MyAppExeName}"; Parameters: "/Uninstall";StatusMsg: "����ж�ؿ���̨����..."
Filename: "{app}\{#MyConfigExeName}"; Parameters: "/Uninstall";StatusMsg: "����ж����������..."
[Code]
//�����Ƿ���Ӱ��ģʽ
//function DI_InShadowMode: BOOL; external 'DI_InShadowMode@{app}\LCXLShadow.dll stdcall uninstallonly';

function CreateFile(
    lpFileName             : String;
    dwDesiredAccess        : Cardinal;
    dwShareMode            : Cardinal;
    lpSecurityAttributes   : Cardinal;
    dwCreationDisposition  : Cardinal;
    dwFlagsAndAttributes   : Cardinal;
    hTemplateFile          : Integer
): THandle; external 'CreateFileA@kernel32.dll stdcall';

function CloseHandle(hHandle: THandle): Boolean; external 'CloseHandle@kernel32.dll stdcall';

function IsAppRunning(): Boolean;
begin
  Result := False;
  if CheckForMutexes('LCXLShadow_App') then
  begin
    Result := True;
  end;
end;

const
  OPEN_EXISTING = 3;
  INVALID_HANDLE_VALUE = $FFFFFFFF;
  

function InitializeSetup(): Boolean;
var
  HasRun: Boolean;
  MykeynotExist:boolean;
  ResultCode: Integer;
  uicmd: String;
  hDevice: THandle;
begin
  Result := False;
  while not Result do
  begin
    Result := not IsAppRunning();
    if not Result then
    begin
      if MsgBox('��װ�����⵽LCXLShadow����̨�����������С�'#13#10#13#10'�������ȹر���Ȼ�󵥻����ǡ�������װ���򰴡����˳���', mbConfirmation, MB_YESNO) = idNO then
      begin
        break;
      end;
    end;
  end;
  if not Result then
  begin
    Exit;
  end;
  Result := False;
  if RegQueryStringValue(HKEY_LOCAL_MACHINE, 'SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\{66986511-18B9-49B3-9AF9-F97E8EA8274A}_is1', 'UninstallString', uicmd) then
  begin
    if MsgBox('��װ�����⵽���Ѿ���װ�˱������'#13#10#13#10'�Ƿ���ж�ص�ǰ����ٰ�װ��', mbConfirmation, MB_YESNO) = idYES then
    begin
      Exec(RemoveQuotes(uicmd), '', '', SW_SHOW, ewWaitUntilTerminated, ResultCode);
      if ResultCode = 0 then
      begin
        //ж�سɹ��������������
        Result := False;
      end
      else
      begin
        if MsgBox('δ�ܳɹ�ж�أ��Ƿ�ǿ�Ƹ��ǣ�', mbError, MB_YESNO) = IdYES then
        begin
          Result := True;
        end;
      end;
    end;
  end
  else
  begin
    //�Ѿ�ж���ˣ��ٿ��������Ƿ��������У�����������У�˵����û����������
    hDevice := CreateFile('\\.\LCXLShadow', 0, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (hDevice <>  INVALID_HANDLE_VALUE) then
    begin
      MsgBox('��װ�����⵽��ж�غ�û�������������������ԡ�', mbConfirmation, MB_OK);
      CloseHandle(hDevice);
      Result := False;
    end
    else
    begin
      Result := True;
    end;
  end;
end;

const
  MF_BYPOSITION=$400;
  function DeleteMenu(HMENU: HWND; uPosition: UINT; uFlags: UINT): BOOL; external 'DeleteMenu@user32.dll stdcall';
  function GetSystemMenu(HWND: hWnd; bRevert: BOOL): HWND; external 'GetSystemMenu@user32.dll stdcall';

procedure InitializeWizard();
begin
 //ɾ��������Ϣ
  DeleteMenu(GetSystemMenu(wizardform.handle,false),8,MF_BYPOSITION);
  DeleteMenu(GetSystemMenu(wizardform.handle,false),7,MF_BYPOSITION);
end;

function InitializeUninstall(): Boolean;
var
  HasRun: Boolean;
  ResultCode: Integer;
begin
  Result := False;
  //�������ó��򣬲鿴�Ƿ���Ӱ��ģʽ
  if not Exec(RemoveQuotes(ExpandConstant('{app}\{#MyConfigExeName}')), '-InShadowMode', '', SW_SHOW, ewWaitUntilTerminated, ResultCode) then
  begin
    MsgBox('�������ó���ʧ�ܣ������°�װ������', mbError, MB_OK);
    Exit;
  end;
  if (ResultCode<>0) then
  begin
    MsgBox('��װ�����⵽��������Ӱ��ģʽ�����ȴ�Ӱ��ģʽ�˳��������б������', mbError, MB_OK);
    Exit;
  end;
  while not Result do
  begin
    Result := not IsAppRunning();
    if not Result then
    begin
      if MsgBox('��װ�����⵽LCXLShadow����̨�����������С�'#13#10#13#10'�������ȹر���Ȼ�󵥻����ǡ�������װ���򰴡����˳���', mbConfirmation, MB_YESNO) = idNO then
      begin
        break;
      end;
    end;
  end;
end;

function NeedRestart(): Boolean;
begin
  Result := True;
end;

function UninstallNeedRestart(): Boolean;
begin
  Result := True;
end;

