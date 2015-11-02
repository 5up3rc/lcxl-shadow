unit untSerCore;

interface

uses
  Windows, SysUtils, WinSvc,

  untSerInterface, untDllInterface, untCommFunc,
  untDllInterfaceEx, untCommDataType, untSockMgr, LCXLIOCPBase, LCXLIOCPCmd;

type
  /// <summary>
  /// ���������
  /// </summary>
  TSerMgrBase = class(TObject)
  private
    FServiceTableEntry: array [0 .. 1] of TServiceTableEntry;
    FServiceStatusHandle: SERVICE_STATUS_HANDLE; // ������
  protected
    FSerStatus: TServiceStatus;
     /// <summary>
    /// �������״̬�����������
    /// </summary>
    /// <param name="SerStatus">
    /// ����״̬
    /// </param>
    /// <returns>
    /// �Ƿ�ɹ�
    /// </returns>
    function ReportStatusToSCMgr(): Boolean;
    procedure SerMain(dwNumServicesArgs: DWORD; lpServiceArgVectors: PPChar);
    procedure SerHandler(dwControl: DWORD); virtual; abstract;
    // ���з���
    procedure SerRun(); virtual; abstract;
  public
    constructor Create(); reintroduce; virtual;
    destructor Destroy(); override;



    /// <summary>
    /// ��ʼ������
    /// </summary>
    /// <returns>
    /// �Ƿ�ɹ�
    /// </returns>
    function Run(): Boolean;

  end;

  /// <summary>
  /// ������
  /// </summary>
  TSerCore = class(TSerMgrBase)
  private
    // �˳��¼�
    FExitEvent: THandle;
    FSerList: TIOCPSerList;
    FIOCPMgr: TIOCPManager;
    FSockLst: TSerSockLst;

    procedure IOCPEvent(EventType: TIocpEventEnum; SockObj: TCmdSockObj;
      Overlapped: PIOCPOverlapped);
    procedure RecvEvent(SockObj: TSerSockObj; Overlapped: PIOCPOverlapped);
    procedure SendEvent(SockObj: TSerSockObj; Overlapped: PIOCPOverlapped);
    // ��֤����
    function VerifyPassword(SockObj: TSerSockObj; hDevice: THandle): Boolean;
  protected
    procedure SerHandler(dwControl: DWORD); override;
    // ���з���
    procedure SerRun(); override;
  public
  end;

var
  SerCore: TSerCore;

implementation

procedure ServiceMain(dwNumServicesArgs: DWORD; lpServiceArgVectors: PPChar); stdcall;
begin
  SerCore.SerMain(dwNumServicesArgs, lpServiceArgVectors);
end;

procedure ServiceHandler(dwControl: DWORD); stdcall;
begin
  SerCore.SerHandler(dwControl);
end;
{ TSerCore }

constructor TSerMgrBase.Create;
begin
  inherited;
  FServiceTableEntry[0].lpServiceName := LCXLSHADOW_SER_NAME;
  FServiceTableEntry[0].lpServiceProc := @ServiceMain;
  FServiceTableEntry[1].lpServiceName := nil;
  FServiceTableEntry[1].lpServiceProc := nil;
end;

destructor TSerMgrBase.Destroy;
begin

  inherited;
end;

function TSerMgrBase.Run: Boolean;
begin
{$IFDEF LCXL_SHADOW_SER_TEST}
  ServiceMain(0, nil);
  Result := True;
{$ELSE}
  Result := StartServiceCtrlDispatcher(FServiceTableEntry[0]);
{$ENDIF}
end;

function TSerMgrBase.ReportStatusToSCMgr(): Boolean;
begin
  with FSerStatus do
  begin
    if (dwCurrentState = SERVICE_START_PENDING) then
    begin
      dwControlsAccepted := 0;
    end
    else
    begin
      dwControlsAccepted := SERVICE_ACCEPT_STOP;
    end;
    if (dwCurrentState = SERVICE_RUNNING) or (dwCurrentState = SERVICE_STOPPED) then
      dwCheckPoint := 0
    else
      Inc(dwCheckPoint);
  end;
  Result := SetServiceStatus(FServiceStatusHandle, FSerStatus);
end;

procedure TSerMgrBase.SerMain(dwNumServicesArgs: DWORD; lpServiceArgVectors: PPChar);
begin
{$IFDEF LCXL_SHADOW_SER_TEST}
  SerRun();
{$ELSE}
  // ע�����
  FServiceStatusHandle := RegisterServiceCtrlHandler(LCXLSHADOW_SER_NAME,
    @ServiceHandler);
  FSerStatus.dwServiceType := SERVICE_WIN32_OWN_PROCESS;
  FSerStatus.dwServiceSpecificExitCode := 0;
  FSerStatus.dwCheckPoint := 1;
  FSerStatus.dwWaitHint := 0;
  FSerStatus.dwWin32ExitCode := 0;
  // ������������
  FSerStatus.dwCurrentState := SERVICE_START_PENDING;
  ReportStatusToSCMgr();
  // ���������ɹ�
  FSerStatus.dwCurrentState := SERVICE_RUNNING;
  ReportStatusToSCMgr();

  SerRun();
  // �������ǰ��״̬��������ƹ�����
  FSerStatus.dwCurrentState := SERVICE_STOP_PENDING;
  ReportStatusToSCMgr();
  FSerStatus.dwCurrentState := SERVICE_STOPPED;
  ReportStatusToSCMgr();
{$ENDIF}
end;

{ TSerCore }

procedure TSerCore.IOCPEvent(EventType: TIocpEventEnum; SockObj: TCmdSockObj;
  Overlapped: PIOCPOverlapped);
var
  _SockObj: TSerSockObj absolute SockObj;
begin
  case EventType of
    ieAddSocket:
      ;
    ieDelSocket:
      ;
    ieError:
      ;
    ieRecvPart:
      ;
    ieRecvAll:
      begin
        RecvEvent(_SockObj, Overlapped);
      end;
    ieRecvFailed:
      ;
    ieSendPart:
      ;
    ieSendAll:
      begin
        SendEvent(_SockObj, Overlapped);
      end;
    ieSendFailed:
      ;
  end;
end;

procedure TSerCore.RecvEvent(SockObj: TSerSockObj; Overlapped: PIOCPOverlapped);
var
  hDevice: THandle;
  RecvData: TCMDDataRec;
  //RecvCMD: Word;

  DriverSetting: TAppDriverSetting;
  NextShadowMode: LCXL_SHADOW_MODE; // ���̱���ģʽ
  IsShadowMode: Boolean;
  Password: string;

  DiskListLen: DWORD;
  DriverDiskList: array of TVolumeDiskInfo;

  CustomDiskList: array of TDeviceNameType;
  CustomDiskListLen: DWORD;

  AppReq: PAppRequest;

  SaveData: TSaveDataWhenShutdown;
  //LL_SYS_VOLUME_INFO
  DosName: string;
  SysVolInfo: TLLSysVolumeInfo;
  VolumeSerialNumber: DWORD;
  MaximumComponentLength: DWORD;
  FileSystemFlags: DWORD;
  TmpInt641: Int64;

  //LL_SYS_SHUTDOWN
  IsSucc: Boolean;
  //У����Ϣ
  AuthData: DWORD;
begin
  RecvData.Assgin(SockObj.RecvData, SockObj.RecvDataLen);
  if not SockObj.IsAuthed then
  begin
    if RecvData.CMD <> LL_AUTH then
    begin
      // ��֤ʧ��
      SockObj.SendFail(LL_FAIL, LL_AUTH, 0);
    end
    else
    begin
      //if SockObj.GetRemoteIP = '127.0.0.1' then
      //begin
        // �������ӣ���֤��֤��Ϣ�Ƿ�ƥ��
        AuthData := MakeLong(SockObj.GetLocalPort(), SockObj.GetRemotePort());
        OutputDebugStr(Format('SER:LL_AUTH:AuthData=%08x', [AuthData]));
        if (RecvData.DataLen = Sizeof(AuthData)) and (CompareMem(RecvData.Data, @AuthData, SizeOf(AuthData))) then
        begin
          if DI_IsDriverRunning then
          begin
            SockObj.IsAuthed := True;
            // ��֤�ɹ�
            OutputDebugStr('��֤�ɹ���');
            SockObj.SendSucc(LL_SUCC, RecvData.CMD);
          end
          else
          begin
            OutputDebugStr('����δ���У�');
            SockObj.SendFail(LL_FAIL, RecvData.CMD, GetLastError);
          end;
        end
        else
        begin
          // ��֤ʧ��
          OutputDebugStr('��֤ʧ�ܣ�');
          SockObj.SendFail(LL_FAIL, RecvData.CMD, 0);
        end;
      //end
      //else
      //begin
        // Զ�����ӣ���֤�û���������
        // Ŀǰ��֧��
      //  SockObj.SendFail(LL_FAIL, RecvData.CMD, 0);
      //end;
    end;
    Exit;
  end;
  case RecvData.CMD of
    LL_IS_DRIVER_RUNNING: // ��ȡ�����Ƿ�����
      begin
        if DI_IsDriverRunning then
        begin
          OutputDebugStr('������������');
          SockObj.SendSucc(LL_SUCC, RecvData.CMD);
        end
        else
        begin
          OutputDebugStr('����δ������');
          SockObj.SendFail(LL_FAIL, RecvData.CMD, GetLastError);
        end;
      end;
    LL_GET_DRIVER_SETTING: // ��ȡ��������
      begin
        hDevice := DI_OpenEx;
        if hDevice = INVALID_HANDLE_VALUE then
        begin
          OutputDebugStr('LL_GET_DRIVER_SETTING ���豸ʧ�ܣ�');
          SockObj.SendFail(LL_FAIL, RecvData.CMD, GetLastError);
          Exit;
        end;

        if DI_GetDriverSetting(hDevice, DriverSetting) then
        begin
          OutputDebugStr('��ȡ�������óɹ���');
          SockObj.SendData(RecvData.CMD, @DriverSetting, SizeOf(DriverSetting));
        end
        else
        begin
          OutputDebugStr('��ȡ��������ʧ�ܣ�');
          SockObj.SendFail(LL_FAIL, RecvData.CMD, GetLastError);
        end;
        DI_Close(hDevice);
      end;
    LL_SET_NEXT_REBOOT_SHADOW_MODE: // �����´�������Ӱ��ģʽ
      begin
        hDevice := DI_OpenEx;
        if hDevice = INVALID_HANDLE_VALUE then
        begin
          SockObj.SendFail(LL_FAIL, RecvData.CMD, GetLastError);
          Exit;
        end;
        if RecvData.DataLen = SizeOf(NextShadowMode) then
        begin
          if VerifyPassword(SockObj, hDevice) then
          begin
            NextShadowMode := PLCXL_SHADOW_MODE(RecvData.Data)^;
            if DI_SetNextRebootShadowMode(hDevice, NextShadowMode) then
            begin
              OutputDebugStr('�����´�������Ӱ��ģʽ�ɹ���');
              SockObj.SendSucc(LL_SUCC, RecvData.CMD, RecvData.Data, RecvData.DataLen);
            end
            else
            begin
              OutputDebugStr('�����´�������Ӱ��ģʽʧ�ܣ�');
              SockObj.SendFail(LL_FAIL, RecvData.CMD, GetLastError);
            end;
          end
          else
          begin
            SockObj.SendFail(LL_FAIL, RecvData.CMD, 1);
          end;
        end
        else
        begin
          OutputDebugStr('LL_SET_NEXT_REBOOT_SHADOW_MODE ���Ȳ�����');
              SockObj.SendFail(LL_FAIL, RecvData.CMD, 2);
        end;
        DI_Close(hDevice);
      end;

    LL_IN_SHADOW_MODE: // �Ƿ���Ӱ��ģʽ��
      begin
        IsShadowMode := DI_InShadowMode;
        OutputDebugStr(Format('����Ӱ��ģʽ:%d', [Integer(IsShadowMode)]));
        SockObj.SendData(RecvData.CMD, @IsShadowMode, SizeOf(IsShadowMode));
      end;

    LL_SET_PASSWORD: // ��������
      begin
        SetString(Password, PChar(RecvData.Data),
          RecvData.DataLen div SizeOf(Char));

        hDevice := DI_OpenEx;
        if hDevice = INVALID_HANDLE_VALUE then
        begin
          SockObj.SendFail(LL_FAIL, RecvData.CMD, GetLastError);
          Exit;
        end;
        // ��֤����
        if VerifyPassword(SockObj, hDevice) then
        begin
          if DI_SetPassword(hDevice, PChar(Password), Length(Password) * SizeOf(Char))
          then
          begin
            SockObj.PassMD5 := Password;
            SockObj.SendSucc(LL_SUCC, RecvData.CMD);
          end
          else
          begin
            OutputDebugStr('��������ʧ�ܣ�');
            SockObj.SendFail(LL_FAIL, RecvData.CMD, GetLastError);
          end;
        end;

        DI_Close(hDevice);
      end;

    LL_VERIFY_PASSWORD: // ��֤����
      begin
        SetString(Password, PChar(RecvData.Data),
          RecvData.DataLen div SizeOf(Char));
        hDevice := DI_OpenEx;
        if hDevice = INVALID_HANDLE_VALUE then
        begin
          SockObj.SendFail(LL_FAIL, RecvData.CMD, GetLastError);
          Exit;
        end;
        if DI_VerifyPassword(hDevice, PChar(SockObj.PassMD5),
          SizeOf(Char) * Length(SockObj.PassMD5)) then
        begin
          SockObj.PassMD5 := Password;
          SockObj.SendSucc(LL_SUCC, RecvData.CMD);
        end
        else
        begin
          OutputDebugStr('��֤����ʧ�ܣ����벻��ȷ');
          SockObj.SendFail(LL_FAIL, RecvData.CMD, 0);
        end;
        DI_Close(hDevice);
      end;
    LL_CLEAR_PASSWORD:
      begin
        OutputDebugStr('�������');
        SockObj.PassMD5 := '';
        SockObj.SendSucc(LL_SUCC, RecvData.CMD);
      end;
    LL_GET_DISK_LIST: // ��ȡ�����б�
      begin
        // ���ó���
        DiskListLen := 64;
        SetLength(DriverDiskList, DiskListLen);

        hDevice := DI_OpenEx;
        if hDevice = INVALID_HANDLE_VALUE then
        begin
          SockObj.SendFail(LL_FAIL, RecvData.CMD, GetLastError);
          Exit;
        end;
        if DI_GetDiskList(hDevice, @DriverDiskList[0], DiskListLen) then
        begin
          SockObj.SendData(RecvData.CMD, @DriverDiskList[0],
            DiskListLen * SizeOf(TVolumeDiskInfo));
        end
        else
        begin
          OutputDebugStr('��ȡ�����б�ʧ�ܣ�');
          SockObj.SendFail(LL_FAIL, RecvData.CMD, GetLastError);
        end;
        DI_Close(hDevice);
      end;

    LL_GET_CUSTOM_DISK_LIST: // ��ȡ�Զ�������б�
      begin
        hDevice := DI_OpenEx;
        if hDevice = INVALID_HANDLE_VALUE then
        begin
          SockObj.SendFail(LL_FAIL, RecvData.CMD, GetLastError);
          Exit;
        end;
        CustomDiskListLen := 64;
        SetLength(CustomDiskList, CustomDiskListLen);
        if not DI_GetCustomDiskList(hDevice, PChar(@CustomDiskList[0]), CustomDiskListLen)
        then
        begin
          OutputDebugStr('��ȡ�Զ�������б�ʧ�ܣ�');
          SockObj.SendData(RecvData.CMD, @CustomDiskList[0],
            CustomDiskListLen * SizeOf(TDeviceNameType));
        end
        else
        begin
          SockObj.SendFail(LL_FAIL, RecvData.CMD, GetLastError);
        end;
        DI_Close(hDevice);
      end;

    LL_SET_CUSTOM_DISK_LIST: // �����Զ�������б�
      begin
        hDevice := DI_OpenEx;
        if hDevice = INVALID_HANDLE_VALUE then
        begin
          SockObj.SendFail(LL_FAIL, RecvData.CMD, GetLastError);
          Exit;
        end;
        // ��֤����
        if VerifyPassword(SockObj, hDevice) then
        begin
          if DI_SetCustomDiskList(hDevice, PChar(RecvData.Data),
            RecvData.DataLen div (SizeOf(Char) * MAX_DEVNAME_LENGTH)) then
          begin
            OutputDebugStr('�����Զ�������б�ɹ���');
            SockObj.SendSucc(LL_SUCC, RecvData.CMD);
          end
          else
          begin
            OutputDebugStr('�����Զ�������б�ʧ�ܣ�');
            SockObj.SendFail(LL_FAIL, RecvData.CMD, GetLastError());
          end;
        end;

        DI_Close(hDevice);
      end;

    LL_APP_REQUEST: // ��������
      begin
        if SizeOf(TAppRequest) <> RecvData.DataLen then
        begin
          SockObj.SendFail(LL_FAIL, RecvData.CMD, 24);
          Exit;
        end;
        AppReq := PAppRequest(RecvData.Data);

        hDevice := DI_OpenEx;
        if hDevice = INVALID_HANDLE_VALUE then
        begin

          SockObj.SendFail(LL_FAIL, RecvData.CMD, GetLastError());
          Exit;
        end;

        if DI_AppRequest(hDevice, AppReq^) then
        begin
          OutputDebugStr('SER:LL_APP_REQUEST�ɹ���');
          SockObj.SendData(RecvData.CMD, AppReq, SizeOf(TAppRequest));
        end
        else
        begin
          OutputDebugStr('SER:LL_APP_REQUESTʧ�ܣ�');
          SockObj.SendFail(LL_FAIL, RecvData.CMD, GetLastError());
        end;

        DI_Close(hDevice);
      end;

    LL_START_SHADOW_MODE: // ����Ӱ��ģʽ
      begin
        hDevice := DI_OpenEx;
        if hDevice = INVALID_HANDLE_VALUE then
        begin
          SockObj.SendFail(LL_FAIL, RecvData.CMD, GetLastError());
          Exit;
        end;

        if VerifyPassword(SockObj, hDevice) then
        begin
          if RecvData.DataLen = SizeOf(NextShadowMode) then
          begin
            NextShadowMode := PLCXL_SHADOW_MODE(RecvData.Data)^;
            if DI_StartShadowMode(hDevice, NextShadowMode) then
            begin
              OutputDebugStr('����Ӱ��ģʽ�ɹ���');
              SockObj.SendSucc(LL_SUCC, RecvData.CMD);
            end
            else
            begin

              SockObj.SendFail(LL_FAIL, RecvData.CMD, 24);
              OutputDebugStr('����Ӱ��ģʽʧ�ܣ�');
            end;
          end;
        end;
        DI_Close(hDevice);
      end;

    LL_SET_SAVE_DATA_WHEN_SHUTDOWN: // ���õ��ػ�ʱ��������
      begin
        hDevice := DI_OpenEx;
        if hDevice = INVALID_HANDLE_VALUE then
        begin
          SockObj.SendFail(LL_FAIL, RecvData.CMD, GetLastError());
          Exit;
        end;
        if RecvData.DataLen = SizeOf(SaveData) then
        begin
          SaveData := PSaveDataWhenShutdown(RecvData.Data)^;
          if VerifyPassword(SockObj, hDevice) then
          begin
            if DI_SetSaveDataWhenShutdown(hDevice, SaveData) then
            begin
              SockObj.SendSucc(LL_SUCC, RecvData.CMD);
            end
            else
            begin
              SockObj.SendFail(LL_FAIL, RecvData.CMD, GetLastError());
            end;
          end;
        end
        else
        begin
          SockObj.SendFail(LL_FAIL, RecvData.CMD, 24);
        end;
        DI_Close(hDevice);
      end;

    LL_SYS_VOLUME_INFO://��ȡϵͳ�Ĵ�����Ϣ
      begin
        ZeroMemory(@SysVolInfo, SizeOf(SysVolInfo));

        SetString(DosName, PChar(RecvData.Data), RecvData.DataLen div SizeOf(Char));

        StrPLCopy(SysVolInfo.DosName, DosName, SizeOf(SysVolInfo.DosName) div SizeOf(Char));
        //��ȡ������Ϣ
        GetVolumeInformation(PChar(DosName + '\'),
          SysVolInfo.SysVolInfo.VolumeName, SizeOf(SysVolInfo.SysVolInfo.VolumeName) div SizeOf(Char),
          @VolumeSerialNumber, MaximumComponentLength, FileSystemFlags, nil, 0);
        // ��ȡ���̿ռ�
        GetDiskFreeSpaceEx(PChar(DosName + '\'), TmpInt641, SysVolInfo.SysVolInfo.TotalSize,
          PLargeInteger(@SysVolInfo.SysVolInfo.AvaliableSize));
        SockObj.SendData(RecvData.CMD, @SysVolInfo, SizeOf(SysVolInfo));
      end;
    LL_SYS_SHUTDOWN:
      begin
        if RecvData.DataLen = SizeOf(Byte) then
        begin
          if EnableShutdownPrivilege then
          begin
            case PByte(RecvData.Data)^ of
              SS_SHUTDOWN:
              begin
                IsSucc := ExitWindowsEx(EWX_SHUTDOWN, 0);
              end;
              SS_REBOOT:
              begin
                IsSucc := ExitWindowsEx(EWX_REBOOT, 0);
              end;
            else
              //87:��������
              SetLastError(87);
              IsSucc := False;
            end;
            if not IsSucc then
            begin
              SockObj.SendFail(LL_FAIL, RecvData.CMD, GetLastError());
            end;
          end;
        end
        else
        begin
           SockObj.SendFail(LL_FAIL, RecvData.CMD, 24);
        end;
      end
  else

  end;
end;

procedure TSerCore.SendEvent(SockObj: TSerSockObj; Overlapped: PIOCPOverlapped);
begin

end;

procedure TSerCore.SerHandler(dwControl: DWORD);
begin
  // Handle the requested control code.
  case dwControl of
    SERVICE_CONTROL_STOP, SERVICE_CONTROL_SHUTDOWN:
      begin
        // �رշ���
        OutputDebugStr('����˽��յ��ر�����');
        SetEvent(FExitEvent);

      end;
    SERVICE_CONTROL_INTERROGATE:
      ;
    SERVICE_CONTROL_PAUSE:
      ;
    SERVICE_CONTROL_CONTINUE:
      ;
    // invalid control code
  else
    // update the service status.
    ReportStatusToSCMgr();
  end;

end;

procedure TSerCore.SerRun;
begin
  // ����IOCP������
  FIOCPMgr := TIOCPManager.Create();
  FSerList := TIOCPSerList.Create(FIOCPMgr);
  FSerList.IOCPEvent := IOCPEvent;
  FExitEvent := CreateEvent(nil, True, False, nil);

  FSockLst := TSerSockLst.Create;
  // ��������
  if FSockLst.StartListen(FSerList, 9999) then
  begin
    // �ȴ��˳�
    WaitForSingleObject(FExitEvent, INFINITE);
    FSockLst.Close;
  end
  else
  begin
    OutputDebugStr('��������ʧ�ܣ�');
    FSockLst.Free;
  end;

  CloseHandle(FExitEvent);
  FSerList.Free;
  FIOCPMgr.Free;
end;

function TSerCore.VerifyPassword(SockObj: TSerSockObj; hDevice: THandle): Boolean;
begin
  Result := DI_VerifyPassword(hDevice, PChar(SockObj.PassMD5),
    SizeOf(Char) * Length(SockObj.PassMD5));
  if not Result then
  begin
    OutputDebugStr('������֤ʧ��');
    SockObj.SendFail(LL_FAIL, LL_VERIFY_PASSWORD, 0);
  end;
end;

end.
