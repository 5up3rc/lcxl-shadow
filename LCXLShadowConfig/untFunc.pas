unit untFunc;

interface

uses
  Windows, SysUtils, WinSvc, untSerInterface;

resourcestring
  RS_HELP_TEXT =
    'LCXLShadow���ó������'#10#10'�÷���LCXLShadowConfig [/Install|/Uninstall|[/?|/help]]';

const
  LCXL_DRIVER_FILE_NAME = 'LCXLShadowDriver';
  LCXL_DRIVER_FILE_EXT = '.sys';
  LCXL_SER_FILE_NAME = 'LCXLShadowSer';
  LCXL_SER_FILE_EXT = '.exe';


const
  PROC_EXIT_CODE_NO_PARAM = 1; // û�в���
  PROC_EXIT_CODE_DRIVER_RUNNING = 2; // ������������
  PROC_EXIT_CODE_INSTALL_FAULT = 3; // ��װʧ��
  PROC_EXIT_CODE_IN_SHADOW_MODE = 4; // ����Ӱ��״̬
  PROC_EXIT_CODE_UNINSTALL_FAULT = 5; // ж��ʧ��
function GetCmdLineSwitch(ParamIndex: Integer): string;

//��װ��ж������
function InstallLCXLShadowDriver: Boolean;
function UninstallLCXLShadowDriver: Boolean;
//��װ��ж�ط���
function InstallLCXLShadowSer: Boolean;
function UninstallLCXLShadowSer: Boolean;

function SetAsDiskFilter(SerName: string): Boolean;
function SetAsDiskVolFilter(SerName: string): Boolean;
function SetAsDriverFilter(UUID: string; SerName: string): Boolean;

function DeleteDriverFilter(UUID: string; SerName: string): Boolean;
function DeleteDiskFilter(SerName: string): Boolean;
function DeleteDiskVolFilter(SerName: string): Boolean;
implementation

function GetCmdLineSwitch(ParamIndex: Integer): string;
var
  S: string;
begin
  S := ParamStr(ParamIndex);
  if (Length(S) > 0) or (SwitchChars = []) or (S[1] in SwitchChars) then
  begin
    result := Copy(S, 2, Maxint);
  end
  else
  begin
    result := S;
  end;
end;

function InstallLCXLShadowDriver: Boolean;
var
  hSCMgr: THandle;
  hSer: THandle;
  SourSysPath: string;
  DestSysPath: string;
  LastError: DWORD;
begin
  result := False;
  LastError := 0;
  SourSysPath := ExtractFilePath(ParamStr(0)) + LCXL_DRIVER_FILE_NAME +
    LCXL_DRIVER_FILE_EXT;
  DestSysPath := GetEnvironmentVariable('windir') + '\System32\Drivers\' +
    LCXL_DRIVER_FILE_NAME + LCXL_DRIVER_FILE_EXT;
  if CompareText(DestSysPath, SourSysPath) <> 0 then
  begin
    //ȡ�������ļ���ֻ������
    SetFileAttributes(PChar(DestSysPath), FILE_ATTRIBUTE_NORMAL);
    DeleteFile(DestSysPath);
    if not CopyFile(PChar(SourSysPath), PChar(DestSysPath), True) then
    begin
      Exit;
    end;
  end;
  hSCMgr := OpenSCManager(nil, nil, SC_MANAGER_ALL_ACCESS);
  if (hSCMgr = 0) then
  begin
    Exit;
  end;
  //������������
  hSer := CreateService(hSCMgr, PChar(LCXL_DRIVER_FILE_NAME),
    PChar(LCXL_DRIVER_FILE_NAME), SC_MANAGER_ALL_ACCESS, SERVICE_KERNEL_DRIVER,
    SERVICE_BOOT_START, SERVICE_ERROR_CRITICAL, PChar(DestSysPath), nil, nil,
    nil, nil, nil);
  if hSer <> 0 then
  begin
    //����Ϊ���̹��������ʹ��̾��������
    Result := SetAsDiskFilter(LCXL_DRIVER_FILE_NAME) and
      SetAsDiskVolFilter(LCXL_DRIVER_FILE_NAME);
    if not result then
    begin
      LastError := GetLastError();
      DeleteService(hSer);
    end;
    CloseServiceHandle(hSer);
  end;
  CloseServiceHandle(hSCMgr);
  SetLastError(LastError);
end;

function UninstallLCXLShadowDriver: Boolean;
var
  hSCMgr: THandle;
  hSer: THandle;
  SysPath: string;
  LastError: DWORD;
begin
  result := False;
  if not DeleteDiskFilter(LCXL_DRIVER_FILE_NAME) or not DeleteDiskVolFilter(LCXL_DRIVER_FILE_NAME) then
  begin
    Exit;
  end;
  SysPath := GetEnvironmentVariable('windir') + '\System32\Drivers\' +
    LCXL_DRIVER_FILE_NAME + LCXL_DRIVER_FILE_EXT;
  hSCMgr := OpenSCManager(nil, nil, SC_MANAGER_ALL_ACCESS);
  if (hSCMgr = 0) then
  begin
    Exit;
  end;
  LastError := ERROR_SUCCESS;
  hSer := OpenService(hSCMgr, PChar(LCXL_DRIVER_FILE_NAME), SERVICE_ALL_ACCESS);
  if hSer <> 0 then
  begin
    // ɾ������
    if DeleteService(hSer) then
    begin
      result := True;
      //ȡ�������ļ���ֻ������
      SetFileAttributes(PChar(SysPath), FILE_ATTRIBUTE_NORMAL);
      // ɾ�������ļ�
      DeleteFile(SysPath);
    end
    else
    begin
      LastError := GetLastError();
    end;
    CloseServiceHandle(hSer);
  end;
  CloseServiceHandle(hSCMgr);
  SetLastError(LastError);
end;

function InstallLCXLShadowSer: Boolean;
var
  SerPath: string;
  hSCMgr: THandle;
  hSer: THandle;
  LastError: DWORD;
begin
  Result := False;
  LastError := 0;
  hSCMgr := OpenSCManager(nil, nil, SC_MANAGER_ALL_ACCESS);
  if (hSCMgr = 0) then
  begin
    Exit;
  end;
  SerPath := ExtractFilePath(ParamStr(0))+LCXL_SER_FILE_NAME+LCXL_SER_FILE_EXT;
  //������������
  hSer := CreateService(hSCMgr, PChar(LCXLSHADOW_SER_NAME),
    PChar(LCXLSHADOW_SER_NAME), SC_MANAGER_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS,
    SERVICE_AUTO_START, SERVICE_ERROR_CRITICAL, PChar(SerPath), nil, nil,
    nil, nil, nil);
  if hSer <> 0 then
  begin
    Result := True;
    CloseServiceHandle(hSer);
  end
  else
  begin
    LastError := GetLastError();
  end;
  CloseServiceHandle(hSCMgr);
  SetLastError(LastError);
end;

function UninstallLCXLShadowSer: Boolean;
var
  hSCMgr: THandle;
  hSer: THandle;
  LastError: DWORD;
begin
  Result := False;
  LastError := 0;
  hSCMgr := OpenSCManager(nil, nil, SC_MANAGER_ALL_ACCESS);
  if (hSCMgr = 0) then
  begin
    Exit;
  end;
  hSer := OpenService(hSCMgr, PChar(LCXLSHADOW_SER_NAME), SERVICE_ALL_ACCESS);
  if hSer <> 0 then
  begin
    // ɾ������
    if DeleteService(hSer) then
    begin
      result := True;
    end
    else
    begin
      LastError := GetLastError();
    end;
    CloseServiceHandle(hSer);
  end;
  CloseServiceHandle(hSCMgr);
  SetLastError(LastError);
end;

function SetAsDiskFilter(SerName: string): Boolean;
begin
  result := SetAsDriverFilter('{4D36E967-E325-11CE-BFC1-08002BE10318}',
    SerName);
end;

function SetAsDiskVolFilter(SerName: string): Boolean;
begin
  result := SetAsDriverFilter('{71A27CDD-812A-11D0-BEC7-08002BE2092F}',
    SerName);
end;

function SetAsDriverFilter(UUID: string; SerName: string): Boolean;
var
  hReg: HKEY;
  keyresu: Integer;
  keytype: DWORD;
  keybuf: array [0 .. 2048] of Char;
  bufpos: PChar;
  isserisexist: Boolean;
  keybufsize: DWORD;
begin
  result := True;
  keyresu := RegOpenKeyEx(HKEY_LOCAL_MACHINE,
    PChar('SYSTEM\CurrentControlSet\Control\Class\' + UUID),
    0, KEY_READ or KEY_WRITE, hReg);
  if keyresu = ERROR_SUCCESS then
  begin
    keybufsize := SizeOf(keybuf);
    keytype := REG_MULTI_SZ;
    keyresu := RegQueryValueEx(hReg, 'UpperFilters', nil, @keytype,
      Pbyte(@(keybuf[0])), @keybufsize);
    if keyresu <> ERROR_SUCCESS then
    begin
      keybufsize := 0;
      keytype := REG_MULTI_SZ;
    end;
    bufpos := @(keybuf[0]);
    isserisexist := False;
    while (DWORD_PTR(bufpos) - DWORD_PTR(@keybuf[0]) < keybufsize) and
      (bufpos^ <> #0) do
    begin
      if lstrcmpi(bufpos, PChar(SerName)) = 0 then
      begin
        isserisexist := True;
        Break;
      end;
      bufpos := bufpos + lstrlen(bufpos) + 1;
    end;
    if not isserisexist then
    begin
      CopyMemory(bufpos, PChar(SerName), Length(SerName) * SizeOf(Char));
      bufpos := bufpos + Length(SerName);
      bufpos^ := #0;
      Inc(bufpos);
      bufpos^ := #0;
      keyresu := RegSetValueEx(hReg, 'UpperFilters', 0, REG_MULTI_SZ,
        Pbyte(@(keybuf[0])), keybufsize + ((Length(SerName) + 1) *
        SizeOf(Char)));
      RegFlushKey(hReg);
      result := keyresu = ERROR_SUCCESS;
    end
    else
    begin
      result := True;
    end;
    RegCloseKey(hReg);
  end;
  SetLastError(keyresu);
end;

function DeleteDriverFilter(UUID: string; SerName: string): Boolean;
var
  hReg: HKEY;
  keyresu: Integer;
  keytype: DWORD;
  keybuf: array [0 .. 2048] of Char;
  bufpos: PChar;
  bufpos2: PChar;
  serstrlen: Integer;
  isserisexist: Boolean;
  keybufsize: DWORD;
begin
  result := True;
  keyresu := RegOpenKeyEx(HKEY_LOCAL_MACHINE,
    PChar('SYSTEM\CurrentControlSet\Control\Class\' + UUID),
    0, KEY_READ or KEY_WRITE, hReg);
  if keyresu = ERROR_SUCCESS then
  begin
    keybufsize := SizeOf(keybuf);
    keytype := REG_MULTI_SZ;
    keyresu := RegQueryValueEx(hReg, 'UpperFilters', nil, @keytype,
      Pbyte(@(keybuf[0])), @keybufsize);
    if keyresu <> ERROR_SUCCESS then
    begin
      keybufsize := 0;
      keytype := REG_MULTI_SZ;
    end;
    bufpos := @(keybuf[0]);
    isserisexist := False;
    while (DWORD_PTR(bufpos) - DWORD_PTR(@keybuf[0]) < keybufsize) and
      (bufpos^ <> #0) do
    begin
      //��ѯ��û��ͬ���Ĺ�����������
      if lstrcmpi(bufpos, PChar(SerName)) = 0 then
      begin
        //�Ƴ���������
        isserisexist := True;
        serstrlen := (lstrlen(bufpos) + 1);

        bufpos2 := bufpos+serstrlen;
        MoveMemory(bufpos, bufpos2,
          keybufsize - (DWORD_PTR(bufpos2)- DWORD_PTR(@keybuf[0])));
        keybufsize := keybufsize-serstrlen*sizeof(Char);
        Break;
      end;
      bufpos := bufpos + lstrlen(bufpos) + 1;
    end;
    if isserisexist then
    begin
      //���޸Ĺ���ע�������д��ȥ
      keyresu := RegSetValueEx(hReg, 'UpperFilters', 0, REG_MULTI_SZ,
        @(keybuf[0]), keybufsize);
      RegFlushKey(hReg);
      result := keyresu = ERROR_SUCCESS;
    end;
  end;
end;

function DeleteDiskFilter(SerName: string): Boolean;
begin
  result := DeleteDriverFilter('{4D36E967-E325-11CE-BFC1-08002BE10318}',
    SerName);
end;

function DeleteDiskVolFilter(SerName: string): Boolean;
begin
  result := DeleteDriverFilter('{71A27CDD-812A-11D0-BEC7-08002BE2092F}',
    SerName);
end;

end.
