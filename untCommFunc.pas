unit untCommFunc;

interface

uses
  Windows, SysUtils, Registry, JwaWinIoctl, WinSvc;

const
  RUN_VALUE_NAME = 'LCXLShadow';
  IDE_ATA_IDENTIFY = $EC; // ATA��IDָ��(IDENTIFY DEVICE)
  // #define DFP_RECEIVE_DRIVE_DATA  0x0007c088
  DFP_RECEIVE_DRIVE_DATA = ((IOCTL_DISK_BASE shl 16) or
    ((FILE_READ_ACCESS or FILE_WRITE_ACCESS) shl 14) or ($0022 shl 2) or
    METHOD_BUFFERED);

type
  _IDINFO = record
    wGenConfig: USHORT; // WORD 0: ������Ϣ��
    wNumCyls: USHORT; // WORD 1: ������
    wReserved2: USHORT; // WORD 2: ����
    wNumHeads: USHORT; // WORD 3: ��ͷ��
    wReserved4: USHORT; // WORD 4: ����
    wReserved5: USHORT; // WORD 5: ����
    wNumSectorsPerTrack: USHORT; // WORD 6: ÿ�ŵ�������
    wVendorUnique: array [0 .. 2] of USHORT; // WORD 7-9: �����趨ֵ
    sSerialNumber: array [0 .. 19] of AnsiChar; // WORD 10-19:���к�
    wBufferType: USHORT; // WORD 20: ��������
    wBufferSize: USHORT; // WORD 21: �����С
    wECCSize: USHORT; // WORD 22: ECCУ���С
    sFirmwareRev: array [0 .. 7] of AnsiChar; // WORD 23-26: �̼��汾
    sModelNumber: array [0 .. 39] of AnsiChar; // WORD 27-46: �ڲ��ͺ�
    wMoreVendorUnique: USHORT; // WORD 47: �����趨ֵ
    wReserved48: USHORT; // WORD 48: ����
    (*
      struct {
      USHORT reserved1:8;
      USHORT DMA:1;   // 1=֧��DMA
      USHORT LBA:1;   // 1=֧��LBA
      USHORT DisIORDY:1;  // 1=�ɲ�ʹ��IORDY
      USHORT IORDY:1;  // 1=֧��IORDY
      USHORT SoftReset:1;  // 1=��ҪATA������
      USHORT Overlap:1;  // 1=֧���ص�����
      USHORT Queue:1;  // 1=֧���������
      USHORT InlDMA:1;  // 1=֧�ֽ����ȡDMA
      } wCapabilities;   // WORD 49: һ������
    *)
    wCapabilities: USHORT; // WORD 49: һ������
    wReserved1: USHORT; // WORD 50: ����
    wPIOTiming: USHORT; // WORD 51: PIOʱ��
    wDMATiming: USHORT; // WORD 52: DMAʱ��
    (*
      struct {
      USHORT CHSNumber:1;  // 1=WORD 54-58��Ч
      USHORT CycleNumber:1;  // 1=WORD 64-70��Ч
      USHORT UnltraDMA:1;  // 1=WORD 88��Ч
      USHORT reserved:13;
      } wFieldValidity;   // WORD 53: �����ֶ���Ч�Ա�־
    *)
    wFieldValidity: USHORT; // WORD 53: �����ֶ���Ч�Ա�־
    wNumCurCyls: USHORT; // WORD 54: CHS��Ѱַ��������
    wNumCurHeads: USHORT; // WORD 55: CHS��Ѱַ�Ĵ�ͷ��
    wNumCurSectorsPerTrack: USHORT; // WORD 56: CHS��Ѱַÿ�ŵ�������
    wCurSectorsLow: USHORT; // WORD 57: CHS��Ѱַ����������λ��
    wCurSectorsHigh: USHORT; // WORD 58: CHS��Ѱַ����������λ��
    (*
      struct {
      USHORT CurNumber:8;  // ��ǰһ���Կɶ�д������
      USHORT Multi:1;  // 1=��ѡ���������д
      USHORT reserved1:7;
      } wMultSectorStuff;   // WORD 59: ��������д�趨
    *)
    wMultSectorStuff: USHORT; // WORD 59: ��������д�趨
    dwTotalSectors: ULONG; // WORD 60-61: LBA��Ѱַ��������
    wSingleWordDMA: USHORT; // WORD 62: ���ֽ�DMA֧������
    (*
      struct {
      USHORT Mode0:1;  // 1=֧��ģʽ0 (4.17Mb/s)
      USHORT Mode1:1;  // 1=֧��ģʽ1 (13.3Mb/s)
      USHORT Mode2:1;  // 1=֧��ģʽ2 (16.7Mb/s)
      USHORT Reserved1:5;
      USHORT Mode0Sel:1;  // 1=��ѡ��ģʽ0
      USHORT Mode1Sel:1;  // 1=��ѡ��ģʽ1
      USHORT Mode2Sel:1;  // 1=��ѡ��ģʽ2
      USHORT Reserved2:5;
      } wMultiWordDMA;   // WORD 63: ���ֽ�DMA֧������
    *)
    wMultiWordDMA: USHORT; // WORD 63: ���ֽ�DMA֧������
    (*
      struct {
      USHORT AdvPOIModes:8;  // ֧�ָ߼�POIģʽ��
      USHORT reserved:8;
      } wPIOCapacity;   // WORD 64: �߼�PIO֧������
    *)
    wPIOCapacity: USHORT; // WORD 64: �߼�PIO֧������
    wMinMultiWordDMACycle: USHORT; // WORD 65: ���ֽ�DMA�������ڵ���Сֵ
    wRecMultiWordDMACycle: USHORT; // WORD 66: ���ֽ�DMA�������ڵĽ���ֵ
    wMinPIONoFlowCycle: USHORT; // WORD 67: ��������ʱPIO�������ڵ���Сֵ
    wMinPOIFlowCycle: USHORT; // WORD 68: ��������ʱPIO�������ڵ���Сֵ
    wReserved69: array [0 .. 10] of USHORT; // WORD 69-79: ����
    (*
      struct {
      USHORT Reserved1:1;
      USHORT ATA1:1;   // 1=֧��ATA-1
      USHORT ATA2:1;   // 1=֧��ATA-2
      USHORT ATA3:1;   // 1=֧��ATA-3
      USHORT ATA4:1;   // 1=֧��ATA/ATAPI-4
      USHORT ATA5:1;   // 1=֧��ATA/ATAPI-5
      USHORT ATA6:1;   // 1=֧��ATA/ATAPI-6
      USHORT ATA7:1;   // 1=֧��ATA/ATAPI-7
      USHORT ATA8:1;   // 1=֧��ATA/ATAPI-8
      USHORT ATA9:1;   // 1=֧��ATA/ATAPI-9
      USHORT ATA10:1;  // 1=֧��ATA/ATAPI-10
      USHORT ATA11:1;  // 1=֧��ATA/ATAPI-11
      USHORT ATA12:1;  // 1=֧��ATA/ATAPI-12
      USHORT ATA13:1;  // 1=֧��ATA/ATAPI-13
      USHORT ATA14:1;  // 1=֧��ATA/ATAPI-14
      USHORT Reserved2:1;
      } wMajorVersion;   // WORD 80: ���汾
    *)
    wMajorVersion: USHORT; // WORD 80: ���汾
    wMinorVersion: USHORT; // WORD 81: ���汾
    wReserved82: array [0 .. 5] of USHORT; // WORD 82-87: ����
    (*
      struct {
      USHORT Mode0:1;  // 1=֧��ģʽ0 (16.7Mb/s)
      USHORT Mode1:1;  // 1=֧��ģʽ1 (25Mb/s)
      USHORT Mode2:1;  // 1=֧��ģʽ2 (33Mb/s)
      USHORT Mode3:1;  // 1=֧��ģʽ3 (44Mb/s)
      USHORT Mode4:1;  // 1=֧��ģʽ4 (66Mb/s)
      USHORT Mode5:1;  // 1=֧��ģʽ5 (100Mb/s)
      USHORT Mode6:1;  // 1=֧��ģʽ6 (133Mb/s)
      USHORT Mode7:1;  // 1=֧��ģʽ7 (166Mb/s) ???
      USHORT Mode0Sel:1;  // 1=��ѡ��ģʽ0
      USHORT Mode1Sel:1;  // 1=��ѡ��ģʽ1
      USHORT Mode2Sel:1;  // 1=��ѡ��ģʽ2
      USHORT Mode3Sel:1;  // 1=��ѡ��ģʽ3
      USHORT Mode4Sel:1;  // 1=��ѡ��ģʽ4
      USHORT Mode5Sel:1;  // 1=��ѡ��ģʽ5
      USHORT Mode6Sel:1;  // 1=��ѡ��ģʽ6
      USHORT Mode7Sel:1;  // 1=��ѡ��ģʽ7
      } wUltraDMA;   // WORD 88: Ultra DMA֧������
    *)
    wUltraDMA: USHORT; // WORD 88: Ultra DMA֧������
    wReserved89: array [0 .. 166] of USHORT; // WORD 89-255
  end;

  IDINFO = _IDINFO;
  PIDINFO = ^IDINFO;

function IsAdminUser: Boolean;
function EnableShutdownPrivilege: Boolean;

function IsAutoRun: Boolean; // �Ƿ񿪻�����
function SetAutoRun(AutoRun: Boolean): Boolean; // �����Ƿ񿪻�����

/// <summary>
/// ��ȡ�������кţ�PASCAL��
/// </summary>
function DiskIdentifyDevice(DiskNum: Integer; var ID_INFO: IDINFO): Boolean;

/// <summary>
/// �������
/// </summary>
/// <param name="DebugInfo">
/// ����ַ���
/// </param>
/// <param name="AddLinkBreak">
/// �Ƿ���ӻ��з�
/// </param>
procedure OutputDebugStr(const DebugInfo: string;
  AddLinkBreak: Boolean = True); inline;

/// <summary>
/// ��ȡ���������λ�ã����δ�ҵ����򷵻�0
/// </summary>
/// <param name="Switch">
/// ���������
/// </param>
/// <param name="Chars">
/// �����
/// </param>
/// <param name="IgnoreCase">
/// �Ƿ���Դ�Сд
/// </param>
/// <returns>
/// ���δ�ҵ����򷵻�0
/// </returns>
function GetCmdLineSwitchIndex(const Switch: string;
  const Chars: TSysCharSet = SwitchChars; IgnoreCase: Boolean = True): Integer;

function StartServicebyName(const ServiceName: string): Boolean;

implementation

function IsAdminUser: Boolean;
const
  SECURITY_BUILTIN_DOMAIN_RID = $20;
  DOMAIN_ALIAS_RID_ADMINS = $220;
  SECURITY_NT_AUTHORITY: TSIDIDENTIFIERAUTHORITY = (Value: (0, 0, 0, 0, 0, 5));
var
  hAccessToken: THandle;
  ptgGroups: PTokenGroups;
  dwInfoBufferSize: DWORD;
  psidAdministrators: PSID;
  x: Integer;
  bSuccess: BOOL;
begin
  Result := False;
  bSuccess := OpenThreadToken(GetCurrentThread, TOKEN_QUERY, True,
    hAccessToken);
  if not bSuccess then
  begin
    if GetLastError = ERROR_NO_TOKEN then
      bSuccess := OpenProcessToken(GetCurrentProcess, TOKEN_QUERY,
        hAccessToken);
  end;
  if bSuccess then
  begin
    GetMem(ptgGroups, 1024);
    bSuccess := GetTokenInformation(hAccessToken, TokenGroups, ptgGroups, 1024,
      dwInfoBufferSize);
    CloseHandle(hAccessToken);
    if bSuccess then
    begin
      AllocateAndInitializeSid(SECURITY_NT_AUTHORITY, 2,
        SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0,
        psidAdministrators);
{$R-}
      for x := 0 to ptgGroups.GroupCount - 1 do
        if EqualSid(psidAdministrators, ptgGroups.Groups[x].Sid) then
        begin
          Result := True;
          Break;
        end;
{$R+}
      FreeSid(psidAdministrators);
    end;
    FreeMem(ptgGroups);
  end;
end;

function EnableShutdownPrivilege: Boolean;
  function EnablePrivilege(hToken: Cardinal; PrivName: string;
    bEnable: Boolean): Boolean;
  var
    TP: TOKEN_PRIVILEGES;
    Dummy: Cardinal;
  begin
    Result := False;
    TP.PrivilegeCount := 1;
    if LookupPrivilegeValue(nil, PChar(PrivName), TP.Privileges[0].Luid) then
    begin
      if bEnable then
      begin
        TP.Privileges[0].Attributes := SE_PRIVILEGE_ENABLED;
      end
      else
      begin
        TP.Privileges[0].Attributes := 0;
      end;
      Result := AdjustTokenPrivileges(hToken, False, TP, SizeOf(TP),
        nil, Dummy);
    end;
  end;

var
  hToken: THandle;
begin
  Result := False;
  if OpenProcessToken(GetCurrentProcess, TOKEN_ADJUST_PRIVILEGES, hToken) then
  begin
    Result := EnablePrivilege(hToken, 'SeShutdownPrivilege', True);
    CloseHandle(hToken);
  end;
end;

function IsAutoRun: Boolean; // �Ƿ񿪻�����
var
  Reg: TRegistry;
  FilePath: string;
  CurFilePath: string;
begin
  Result := False;
  Reg := TRegistry.Create;
  Reg.RootKey := HKEY_LOCAL_MACHINE;
  if Reg.OpenKey('SOFTWARE\Microsoft\Windows\CurrentVersion\Run', True) then
  begin
    try
      FilePath := Reg.ReadString(RUN_VALUE_NAME);
      CurFilePath := Format('"%s" /autorun', [ParamStr(0)]);
      if CompareText(CurFilePath, FilePath) = 0 then
      begin
        Result := True;
      end;
    except

    end;
    Reg.CloseKey;
  end;
  Reg.Free;
end;

function SetAutoRun(AutoRun: Boolean): Boolean; // �����Ƿ񿪻�����
var
  Reg: TRegistry;
  CurFilePath: string;
  LastErrorCode: DWORD;
begin
  Result := False;
  LastErrorCode := 0;
  Reg := TRegistry.Create;
  Reg.RootKey := HKEY_LOCAL_MACHINE;
  if Reg.OpenKey('SOFTWARE\Microsoft\Windows\CurrentVersion\Run', True) then
  begin
    if AutoRun then
    begin
      CurFilePath := Format('"%s" /autorun', [ParamStr(0)]);
      try
        Reg.WriteString(RUN_VALUE_NAME, CurFilePath);
        Result := True;
      except
        LastErrorCode := GetLastError;
        OutputDebugStr(Format('SetAutoRun Failed:%d', [LastErrorCode]));
      end;
    end
    else
    begin
      Reg.DeleteValue(RUN_VALUE_NAME);
      Result := True;
    end;
    Reg.CloseKey;
  end;
  Reg.Free;
  SetLastError(LastErrorCode);
end;

// ��ȡ�������кţ�PASCAL��
function DiskIdentifyDevice(DiskNum: Integer; var ID_INFO: IDINFO): Boolean;
var
  pSCIP: PSendCmdInParams;
  pSCOP: PSENDCMDOUTPARAMS;
  hDevice: THandle;
  retSize: DWORD;
  LastError: DWORD;
begin
  Result := False;
  LastError := 0;
  hDevice := CreateFile(PChar(Format('\\.\PHYSICALDRIVE%d', [DiskNum])),
    GENERIC_READ or GENERIC_WRITE, FILE_SHARE_READ or FILE_SHARE_WRITE, nil,
    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
  if hDevice = INVALID_HANDLE_VALUE then
  begin
    LastError := GetLastError();
    OutputDebugStr(Format('DiskIdentifyDevice:CreateFileʧ��:%d-%s',
      [LastError, SysErrorMessage(LastError)]));
    SetLastError(LastError);
    Exit;
  end;

  GetMem(pSCIP, SizeOf(SENDCMDINPARAMS));
  GetMem(pSCOP, SizeOf(SENDCMDOUTPARAMS) + SizeOf(ID_INFO));

  ZeroMemory(pSCIP, SizeOf(SENDCMDINPARAMS));
  ZeroMemory(pSCOP, SizeOf(SENDCMDOUTPARAMS) + SizeOf(ID_INFO));

  // ָ��ATA/ATAPI����ļĴ���ֵ
  pSCIP.irDriveRegs.bCommandReg := IDE_ATA_IDENTIFY;
  // ָ������/������ݻ�������С
  pSCOP.cBufferSize := SizeOf(ID_INFO);
  if DeviceIoControl(hDevice, DFP_RECEIVE_DRIVE_DATA, pSCIP,
    SizeOf(SENDCMDINPARAMS), pSCOP, SizeOf(SENDCMDOUTPARAMS) + SizeOf(ID_INFO),
    retSize, nil) then
  begin
    CopyMemory(@ID_INFO, @pSCOP.bBuffer[0], SizeOf(ID_INFO));
    Result := True;
  end
  else
  begin
    LastError := GetLastError();
    // fix-it:Ŀǰ��֧��SCSI���̵����кŻ�ȡ��������ʱ��0���
    ZeroMemory(@ID_INFO, SizeOf(ID_INFO));
    OutputDebugStr(Format('!!!DLL:DiskIdentifyDevice:DeviceIoControlʧ��:%d-%s',
      [LastError, SysErrorMessage(LastError)]));
    Result := True;
  end;
  CloseHandle(hDevice);
  SetLastError(LastError);
end;

// �������
procedure OutputDebugStr(const DebugInfo: string; AddLinkBreak: Boolean);
begin
{$IFDEF DEBUG}
  if AddLinkBreak then
  begin
    Windows.OutputDebugString(PChar(Format('%s'#10, [DebugInfo])));
  end
  else
  begin
    Windows.OutputDebugString(PChar(DebugInfo));
  end;
{$ENDIF}
end;

function GetCmdLineSwitchIndex(const Switch: string; const Chars: TSysCharSet;
  IgnoreCase: Boolean): Integer;
var
  I: Integer;
  S: string;
begin
  for I := 1 to ParamCount do
  begin
    S := ParamStr(I);
    if (Chars = []) or (S[1] in Chars) then
      if IgnoreCase then
      begin
        if (AnsiCompareText(Copy(S, 2, Maxint), Switch) = 0) then
        begin
          Result := I;
          Exit;
        end;
      end
      else
      begin
        if (AnsiCompareStr(Copy(S, 2, Maxint), Switch) = 0) then
        begin
          Result := I;
          Exit;
        end;
      end;
  end;
  Result := 0;
end;

function StartServicebyName(const ServiceName: string): Boolean;
var
  hSCMgr: THandle;
  hSer: THandle;
  ssStatus: SERVICE_STATUS_PROCESS;
  dwBytesNeeded: DWORD;
  SerCmd: PChar;
  LastError: DWORD;
begin
  Result := False;
  LastError := 0;
  hSCMgr := OpenSCManager(nil, nil, SC_MANAGER_ALL_ACCESS);
  if (hSCMgr = 0) then
  begin
    Exit;
  end;
  hSer := OpenService(hSCMgr, PChar(ServiceName), SERVICE_ALL_ACCESS);
  if hSer <> 0 then
  begin
    if QueryServiceStatusEx(hSer, // handle to service
      SC_STATUS_PROCESS_INFO, // information level
      LPBYTE(@ssStatus), // address of structure
      SizeOf(SERVICE_STATUS_PROCESS), // size of structure
      &dwBytesNeeded) then // size needed if buffer is too small
    begin
      if ssStatus.dwCurrentState = SERVICE_STOPPED then
      begin
        SerCmd := nil;
        WinSvc.StartService(hSer, 0, SerCmd);
        QueryServiceStatusEx(hSer, // handle to service
          SC_STATUS_PROCESS_INFO, // information level
          LPBYTE(@ssStatus), // address of structure
          SizeOf(SERVICE_STATUS_PROCESS), // size of structure
          &dwBytesNeeded);
      end;
      while ssStatus.dwCurrentState = SERVICE_START_PENDING do
      begin
        OutputDebugStr(Format('APP:StartService:�ȴ�%s��������', [ServiceName]));
        Sleep(1000);
        if not QueryServiceStatusEx(hSer, // handle to service
          SC_STATUS_PROCESS_INFO, // information level
          LPBYTE(@ssStatus), // address of structure
          SizeOf(SERVICE_STATUS_PROCESS), // size of structure
          &dwBytesNeeded) then
        begin
          LastError := GetLastError();
          OutputDebugStr
            (Format('APP:StartService:QueryServiceStatusEx(%s)����ʧ��:%d',
            [ServiceName, LastError]));
          Break;
        end;
      end;
      Result := ssStatus.dwCurrentState = SERVICE_RUNNING;
    end
    else
    begin
      LastError := GetLastError();
    end;
    CloseServiceHandle(hSer);
  end
  else
  begin
    LastError := GetLastError();
  end;
  CloseServiceHandle(hSCMgr);
  SetLastError(LastError);
end;

end.
