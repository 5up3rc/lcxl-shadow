unit untMain;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, ExtCtrls, ShellAPI, Menus, ImgList, StdCtrls, ComCtrls,
  PlatformDefaultStyleActnCtrls, ActnList, ActnMan, RzPanel,
  RzTabs, RzCommon, RzGroupBar, RzStatus, RzLabel,
  RzButton, RzRadChk, AdvSmoothSlider, AdvOfficeHint,

  LCXLIOCPBase, LCXLIOCPCmd, LCXLWinSock2,

  untCommFunc, untCommDataType, untResString,
  untSetShadowMode, untSystemShutdown, untMyGetFileInfo, untVerifyPassword,
  untSetPassword, untShadowing, untClientSockMgr, untSerInterface, Actions;

const
  WM_SOCK_EVENT = WM_USER + $3030;

  // ��ǿ��
type
  _VolumeDiskInfoEx = record

    /// <summary>
    /// ������Ϣ
    /// </summary>
    DiskInfo: TVolumeDiskInfo;

    /// <summary>
    /// �û��Ƿ�ѡ��
    /// </summary>
    UserCheck: Boolean;

    /// <summary>
    /// Ӱ���������򷵻ص���Ϣ��ֻ���ڱ���ģʽ�²��ܻ�ȡ�ɹ�������ֻ�᷵��0
    /// </summary>
    VolumeInfo: APP_REQUEST_VOLUME_INFO;
    /// <summary>
    /// ϵͳ��ʾ�Ĵ��̿ռ�
    /// </summary>
    SysVolInfo: TSysVolumeInfo;
  end;

  TVolumeDiskInfoEx = _VolumeDiskInfoEx;
  PVolumeDiskInfoEx = ^TVolumeDiskInfoEx;

  _MainMsgData = record
    EventType: TIocpEventEnum;
    SockObj: TCmdSockObj;
    Overlapped: PIOCPOverlapped;
  end;

  TMainMsgData = _MainMsgData;
  PMainMsgData = ^TMainMsgData;

type
  TfrmMain = class(TForm)
    pgcMain: TRzPageControl;
    tsMain: TRzTabSheet;
    sbMain: TRzStatusBar;
    frmctrlMain: TRzFrameController;
    grpctrlMain: TRzGroupController;
    mnuctrlMain: TRzMenuController;
    verinfostProduct: TRzVersionInfoStatus;
    verinfoMain: TRzVersionInfo;
    lvDisk: TListView;
    tsSetting: TRzTabSheet;
    verinfostFile: TRzVersionInfoStatus;
    trycnMain: TTrayIcon;
    pmTray: TPopupMenu;
    mniExit: TMenuItem;
    tsAbout: TRzTabSheet;
    gbDriverStatus: TRzGroupBox;
    reginiMain: TRzRegIniFile;
    mniShutReset: TMenuItem;
    mniN1: TMenuItem;
    verinfostCopyright: TRzVersionInfoStatus;
    verinfostCompanyName: TRzVersionInfoStatus;
    lblInfo: TRzLabel;
    lnklblEmail: TLinkLabel;
    lblPassword: TRzLabel;
    btnPassword: TRzButton;
    chkAutorun: TRzCheckBox;
    actmgrMain: TActionManager;
    actPassword: TAction;
    gpnlLow: TRzGridPanel;
    gbDiskStatus: TRzGroupBox;
    lblShadowFreeSizeTitle: TRzLabel;
    lblShadowFreeSize: TRzLabel;
    lblIsProtectTitle: TRzLabel;
    lblIsProtect: TRzLabel;
    gbSaveData: TRzGroupBox;
    grdpnl1: TRzGridPanel;
    actStartShadowMode: TAction;
    mniStartShadowMode: TMenuItem;
    smobtnSaveData: TAdvSmoothSlider;
    advfchntMain: TAdvOfficeHint;
    actReleasePassword: TAction;
    mniReleasePassword: TMenuItem;
    mniN2: TMenuItem;
    lblReleasePassword: TRzLabel;
    btnReleasePassword: TRzButton;
    pnlSet: TRzPanel;
    cbbShadowMode: TComboBox;
    grdpnlStatus: TRzGridPanel;
    lblCurShadowMode: TRzLabel;
    lblCurShadowModeLabel: TRzLabel;
    lblNextShadowModeLabel: TRzLabel;
    lblNextShadowMode: TRzLabel;
    actNextShaodowMode: TAction;
    actCurShadowMode2: TAction;
    ilListState: TImageList;
    btnCurShadowMode: TButton;
    btnNextShadowMode: TButton;
    chkEnableSaveData: TCheckBox;
    procedure FormClose(Sender: TObject; var Action: TCloseAction);
    procedure trycnMainClick(Sender: TObject);
    procedure mniExitClick(Sender: TObject);
    procedure FormCreate(Sender: TObject);
    procedure FormDestroy(Sender: TObject);
    procedure lvDiskData(Sender: TObject; Item: TListItem);
    procedure lblNextShadowModeClick(Sender: TObject);
    procedure mniShutResetClick(Sender: TObject);
    procedure tsMainShow(Sender: TObject);
    procedure lnklblEmailLinkClick(Sender: TObject; const Link: string;
      LinkType: TSysLinkType);
    procedure lblCurShadowModeClick(Sender: TObject);
    procedure lvDiskSelectItem(Sender: TObject; Item: TListItem;
      Selected: Boolean);
    procedure actStartShadowModeExecute(Sender: TObject);
    procedure pmTrayPopup(Sender: TObject);
    procedure tsSettingShow(Sender: TObject);
    procedure chkAutorunClick(Sender: TObject);
    procedure smobtnSaveDataStateChanged(Sender: TObject;
      State: TAdvSmoothSliderState; Value: Double);
    procedure lvDiskInfoTip(Sender: TObject; Item: TListItem;
      var InfoTip: string);
    procedure actPasswordExecute(Sender: TObject);
    procedure actReleasePasswordExecute(Sender: TObject);
    procedure lvDiskDblClick(Sender: TObject);
    procedure actNextShaodowModeExecute(Sender: TObject);
    procedure actCurShadowMode2Execute(Sender: TObject);
    procedure lvDiskClick(Sender: TObject);
    procedure cbbShadowModeChange(Sender: TObject);
  private
    { Private declarations }
    FCanClose: Boolean;
    FCanShutdown: Boolean;
    FFirstCloseForm: Boolean;
    // ͼ����Ϣ
    FIconInfo: TMyGetFileInfo;
    // �������
    FIOCPMgr: TIOCPManager;
    FSockList: TIOCPClientList;
    FSockObj: TClientSockObj;

    function ProcShutdownMsg: Boolean;
    function FormatByteSize(bSize: ULONGLONG): string;

    /// <summary>
    /// �����Զ�������б�
    /// </summary>
    function SetCustomDiskListByList(): Boolean;
    /// <summary>
    /// ���´���״̬
    /// </summary>
    procedure UpdateDiskStatus(IsShow: Boolean; DiskInfo: PVolumeDiskInfoEx);
    /// <summary>
    /// ����Ӱ��ϵͳ�������
    /// </summary>
    procedure ConnectServer();
    procedure GetConnectInfo(var SerAddr: string; var SerPort: Integer;
      var IsRemoteSer: Boolean);

    /// <summary>
    /// ��Ϣ�¼�
    /// </summary>
    procedure OnMessage(var Msg: TMsg; var Handled: Boolean);
    /// <summary>
    /// Sock�¼�(���߳�)
    /// </summary>
    procedure OnIOCPEvent(EventType: TIocpEventEnum; SockObj: TCmdSockObj;
      Overlapped: PIOCPOverlapped);

    procedure OnMainEvent(var TheMsg: TMessage); message WM_SOCK_EVENT;

    procedure OnRecvEvent(SockObj: TCmdSockObj; Overlapped: PIOCPOverlapped);
    procedure OnSendEvent(SockObj: TCmdSockObj; Overlapped: PIOCPOverlapped);
  public
    { Public declarations }
    /// <summary>
    /// �����б�
    /// </summary>
    FDiskList: array of TVolumeDiskInfoEx;
    /// <summary>
    /// ��������
    /// </summary>
    FDriverSetting: TAppDriverSetting;
    /// <summary>
    /// �Զ�����̾�����
    /// </summary>
    FCustomDiskList: array of TDeviceNameType;

    function GetAuth: Boolean;

    /// <summary>
    /// ��ȡ�����б�
    /// </summary>
    function GetDiskList: Boolean;

    /// <summary>
    /// ���´����б�
    /// </summary>
    procedure UpdateDiskList(DiskInfoList: PVolumeDiskInfo; Count: Integer);
    /// <summary>
    /// ��ȡ��������
    /// </summary>
    function GetDriverSetting: Boolean;
    procedure UpdateDriverSetting(DriverSetting: PAppDriverSetting);
    /// <summary>
    /// Ӧ�ó�������
    /// </summary>
    function GetAppReuqest(AppReq: PAppRequest): Boolean;
    procedure UpdateAppReuqest(AppReq: PAppRequest);
    /// <summary>
    /// ϵͳ��ʾ�Ĵ�����Ϣ
    /// </summary>
    function GetSysVolumeInfo(const DosName: string): Boolean;
    procedure UpdateSysVolumeInfo(SysVolInfo: PLLSysVolumeInfo);

    function GetCustomDiskList(): Boolean;
    /// <summary>
    /// �����Զ�������б�
    /// </summary>
    function SetCustomDiskList(DiskList: PDeviceNameType;
      Count: Integer): Boolean;
    procedure UpdateNextRebootShadowMode(NextShadowMode: LCXL_SHADOW_MODE);
    /// <summary>
    /// �����´�����Ӱ��ģʽ
    /// </summary>
    function SetNextRebootShadowMode(NextShadowMode: LCXL_SHADOW_MODE): Boolean;

    /// <summary>
    /// ����Ӱ��ģʽ
    /// </summary>
    function StartShadowMode(IsShowSelectBox: Boolean): Boolean;
    /// <summary>
    /// ��֤����
    /// </summary>
    function VerifyPassword(const Password: string): Boolean;
    /// <summary>
    /// ��������
    /// </summary>
    /// <param name="Password">
    /// ����
    /// </param>
    /// <returns>
    /// �Ƿ����óɹ�
    /// </returns>
    function SetPassword(const Password: string): Boolean;
  end;

var
  frmMain: TfrmMain;
  MyMsg: UINT;

implementation

{$R *.dfm}

procedure TfrmMain.mniExitClick(Sender: TObject);
begin
  FCanClose := True;
  Close;
end;

procedure TfrmMain.mniShutResetClick(Sender: TObject);
begin
  ProcShutdownMsg;
end;

procedure TfrmMain.OnIOCPEvent(EventType: TIocpEventEnum; SockObj: TCmdSockObj;
  Overlapped: PIOCPOverlapped);
var
  MainData: TMainMsgData;
begin
  case EventType of
    ieAddSocket:
      ;
    ieError:
      ;
    ieRecvPart:
      ;
    ieCloseSocket:
      begin
        SockObj.DecRefCount();
      end;
    ieDelSocket, ieRecvAll, ieSendAll:
      begin
        MainData.EventType := EventType;
        MainData.SockObj := SockObj;
        MainData.Overlapped := Overlapped;
        SendMessage(frmMain.Handle, WM_SOCK_EVENT, WParam(@MainData), 0);
      end;
    ieRecvFailed:
      ;
    ieSendPart:
      ;
    ieSendFailed:
      ;
  end;
end;

procedure TfrmMain.OnMainEvent(var TheMsg: TMessage);
var
  MainData: PMainMsgData;
begin
  MainData := PMainMsgData(TheMsg.WParam);
  case MainData.EventType of
    ieRecvAll:
      begin
        OnRecvEvent(MainData.SockObj, MainData.Overlapped);
      end;
    ieSendAll:
      begin
        OnSendEvent(MainData.SockObj, MainData.Overlapped);
      end;
    ieDelSocket:
      begin
        if MainData.SockObj = FSockObj then
        begin
          if not FCanClose then
          begin
            MessageBox(Handle, PChar(RS_SERVICE_IS_CLOSED), nil, MB_ICONSTOP);
          end;
          FCanClose := True;
          Application.Terminate;
        end;
      end;
  end;

end;

procedure TfrmMain.OnMessage(var Msg: TMsg; var Handled: Boolean);
begin
  if Msg.message = MyMsg then
  begin
    Show;
    Handled := True;
  end;
end;

procedure TfrmMain.OnRecvEvent(SockObj: TCmdSockObj;
  Overlapped: PIOCPOverlapped);
var
  RecvData: TCMDDataRec;
  Data: Pointer;
  DataLen: LongWord;
  // RecvCMD: Word;
  DataCMD: Word;
  DataLastError: DWORD;
  _SockObj: TClientSockObj absolute SockObj;
  I: Integer;
begin
  RecvData.Assgin(SockObj.RecvData, SockObj.RecvDataLen);
  case RecvData.CMD of
    LL_SUCC:
      begin
        if RecvData.DataLen >= SizeOf(DataCMD) then
        begin
          DataCMD := PWord(RecvData.Data)^;
          Data := Pointer(DWORD_PTR(RecvData.Data) + SizeOf(DataCMD));
          DataLen := RecvData.DataLen - SizeOf(DataCMD);
          case DataCMD of
            LL_AUTH:
              begin
                // ��֤�ɹ�
                _SockObj.IsAuthed := True;
                // ��ȡ��������
                GetDriverSetting();
              end;
            LL_IS_DRIVER_RUNNING:
              begin
                // ������������
              end;
            LL_SET_NEXT_REBOOT_SHADOW_MODE:
              begin
                // ����UI
                if DataLen = SizeOf(LCXL_SHADOW_MODE) then
                begin
                  UpdateNextRebootShadowMode(PLCXL_SHADOW_MODE(Data)^);
                end
                else
                begin
                  OutputDebugStr('APP:LL_SET_NEXT_REBOOT_SHADOW_MODE���Ȳ�����');
                end;
              end;
            LL_START_SHADOW_MODE:
              begin
                MessageBox(Handle, PChar(RS_START_SHADOW_MODE_SUCESSFUL),
                  PChar(RS_MSG_INFORMATION_TITLE), MB_ICONINFORMATION);

                frmShadowing.Close();
                frmShadowing.Free();
                // ��ȡ��������
                GetDriverSetting();
              end;
            LL_VERIFY_PASSWORD:
              begin
                // ��֤����ɹ�
                _SockObj.PassIsVerified := True;
              end;
            LL_CLEAR_PASSWORD:
              begin
                // �������ɹ�
                _SockObj.PassIsVerified := False;
                _SockObj.PassIsEmpty := True;
                MessageBox(Handle, PChar(RS_PASSWORD_CLEARED),
                  PChar(RS_MSG_INFORMATION_TITLE), MB_ICONINFORMATION);
              end;
            LL_SET_CUSTOM_DISK_LIST:
              begin
                // ���óɹ��Ժ󣬻�ȡ�Զ�������б�
                GetCustomDiskList;
              end;
          end;
        end
        else
        begin
          OutputDebugStr('APP:LL_SUCC���Ȳ�����');
        end;
      end;
    LL_FAIL:
      begin
        if RecvData.DataLen = SizeOf(DataCMD) + SizeOf(DataLastError) then
        begin
          DataCMD := PWord(RecvData.Data)^;
          DataLastError := PDWord(PByte(RecvData.Data) + SizeOf(Word))^;
          case DataCMD of
            LL_AUTH:
              begin
                // ��֤ʧ��
                _SockObj.IsAuthed := False;
                if DataLastError = 0 then
                begin
                  // ���ݲ���ȷ
                  MessageBox(Handle, PChar(RS_CANT_AUTH), nil, MB_ICONSTOP);
                end
                else
                begin
                  // ����û������
                  if DataLastError = ERROR_FILE_NOT_FOUND then
                  begin
                    MessageBox(Handle, PChar(RS_DRIVER_NO_RUNNING), nil,
                      MB_ICONSTOP);
                  end
                  else
                  begin
                    // û�й���ԱȨ��
                    MessageBox(Handle, PChar(RS_NOT_ADMIN_TEXT),
                      PChar(RS_NOT_ADMIN_TITLE), MB_ICONSTOP);
                  end;

                end;
                FCanClose := True;
                Application.Terminate;
              end;
            LL_IS_DRIVER_RUNNING:
              begin
                // ����û������
              end;
            LL_GET_DRIVER_SETTING:
              begin
                // ��ȡ����ʧ��
                MessageBox(Handle, PChar(Format(RS_CANT_GET_DRIVER_SETTING,
                  [SysErrorMessage(DataLastError)])), nil, MB_ICONSTOP);
              end;
            LL_GET_DISK_LIST:
              begin
                MessageBox(Handle, PChar(Format(RS_CANT_GET_DISK_LIST,
                  [SysErrorMessage(DataLastError)])), nil, MB_ICONSTOP);

              end;
            LL_SET_NEXT_REBOOT_SHADOW_MODE:
              begin
                MessageBox(Handle,
                  PChar(Format(RS_CANT_SET_NEXT_REBOOT_SHADOW_MODE,
                  [SysErrorMessage(DataLastError)])), nil, MB_ICONSTOP);
              end;
            LL_SET_CUSTOM_DISK_LIST:
              begin
                MessageBox(Self.Handle,
                  PChar(Format(RS_CANT_SET_CUSTOM_DISK_LIST,
                  [SysErrorMessage(DataLastError)])), nil, MB_ICONSTOP);
              end;
            LL_SET_SAVE_DATA_WHEN_SHUTDOWN:
              begin
                //
              end;
            LL_START_SHADOW_MODE:
              begin

                MessageBox(Handle, PChar(Format(RS_CANT_START_SHADOW_MODE,
                  [SysErrorMessage(DataLastError)])), PChar(RS_MSG_STOP_TITLE),
                  MB_ICONSTOP);

                frmShadowing.Close();
                frmShadowing.Free();
                // ��ȡ��������
                GetDriverSetting();
              end;
            LL_VERIFY_PASSWORD:
              begin
                // ��֤ʧ��
                MessageBox(Handle, PChar(RS_PASSWORD_VERIFY_FAILURE),
                  PChar(RS_MSG_STOP_TITLE), MB_ICONSTOP);
              end;
            LL_SET_PASSWORD:
              begin
                MessageBox(Handle, PChar(Format(RS_SET_PASSWORD_FAILURE,
                  [SysErrorMessage(DataLastError)])), PChar(RS_MSG_STOP_TITLE),
                  MB_ICONSTOP);
              end;
            LL_CLEAR_PASSWORD:
              begin
                //
              end;

          end;
        end;
      end;
    LL_GET_DRIVER_SETTING:
      begin
        // ������������
        if SizeOf(TAppDriverSetting) = RecvData.DataLen then
        begin
          // ���½���
          UpdateDriverSetting(PAppDriverSetting(RecvData.Data));
        end;
      end;
    LL_GET_DISK_LIST:
      begin
        // ���´����б�
        if RecvData.DataLen mod SizeOf(TVolumeDiskInfo) = 0 then
        begin
          I := SockObj.RecvDataLen div SizeOf(TVolumeDiskInfo);
          UpdateDiskList(PVolumeDiskInfo(RecvData.Data), I);
        end;
      end;
    LL_GET_CUSTOM_DISK_LIST:
      begin
        if SockObj.RecvDataLen mod SizeOf(TDeviceNameType) = 0 then
        begin
          I := RecvData.DataLen div SizeOf(TDeviceNameType);
          SetLength(FCustomDiskList, I);
          CopyMemory(@FCustomDiskList[0], RecvData.Data, SockObj.RecvDataLen);
        end;
      end;
    LL_APP_REQUEST:
      begin
        if SizeOf(TAppRequest) = RecvData.DataLen then
        begin
          OutputDebugStr('APP:����LL_APP_REQUEST');
          UpdateAppReuqest(PAppRequest(RecvData.Data));
        end
        else
        begin
          OutputDebugStr('APP:����LL_APP_REQUESTʧ�ܣ����Ȳ�����');
        end;
      end;
    LL_VERIFY_PASSWORD:
      begin
        VerifyPasswordByMsgBox(Self);
      end;

    LL_SYS_VOLUME_INFO:
      begin
        if SizeOf(TLLSysVolumeInfo) = RecvData.DataLen then
        begin
          UpdateSysVolumeInfo(PLLSysVolumeInfo(RecvData.Data));
        end
        else
        begin
          OutputDebugStr('APP:����LL_GET_SYS_VOLUME_INFOʧ�ܣ����Ȳ�����');
        end;
      end;
  end;
end;

procedure TfrmMain.OnSendEvent(SockObj: TCmdSockObj;
  Overlapped: PIOCPOverlapped);
begin

end;

procedure TfrmMain.pmTrayPopup(Sender: TObject);
begin
  mniStartShadowMode.Visible := FDriverSetting.CurShadowMode = SM_NONE;
  mniReleasePassword.Visible := FSockObj.PassIsVerified and
    not FSockObj.PassIsEmpty;
end;

procedure TfrmMain.actCurShadowMode2Execute(Sender: TObject);
begin
  if FDriverSetting.CurShadowMode = SM_NONE then
  begin
    StartShadowMode(False);
  end
  else
  begin
    if MessageBox(Handle, PChar(RS_STOP_SHADOW_RESTART_WARNNING),
      PChar(RS_MSG_WARNING_TITLE), MB_ICONWARNING or MB_YESNO) = IDYES then
    begin
      ProcShutdownMsg;
    end;
  end;
end;

procedure TfrmMain.actNextShaodowModeExecute(Sender: TObject);
begin
  // ��ʾ
  if ShowShadowModeBox(Self, FDriverSetting.NextShadowMode) then
  begin
    SetNextRebootShadowMode(FDriverSetting.NextShadowMode);
  end;
end;

procedure TfrmMain.actPasswordExecute(Sender: TObject);
begin
  SetMgrPasswordBox(Self);
end;

procedure TfrmMain.actReleasePasswordExecute(Sender: TObject);
begin
  FSockObj.SendData(LL_CLEAR_PASSWORD, nil, 0);

end;

procedure TfrmMain.actStartShadowModeExecute(Sender: TObject);
begin
  StartShadowMode(True);
end;

procedure TfrmMain.cbbShadowModeChange(Sender: TObject);
var
  I: Integer;
  tmpStr: string;
begin
  case cbbShadowMode.ItemIndex of
    0, 1: // �ޱ�����ȫ�̱���
      begin
        for I := 0 to Length(FDiskList) do
        begin
          FDiskList[I].UserCheck := cbbShadowMode.ItemIndex <> 0;
        end;
        lvDisk.Refresh;
      end;
    2: // ϵͳ�̱���
      begin
        SetLength(tmpStr, MAX_PATH);
        GetWindowsDirectory(PChar(tmpStr), MAX_PATH);
        for I := 0 to Length(FDiskList) do
        begin
          FDiskList[I].UserCheck := UpCase(FDiskList[I].DiskInfo.DosName[0])
            = UpCase(tmpStr[1]);
        end;
        lvDisk.Refresh;
      end;
  end;

end;

procedure TfrmMain.chkAutorunClick(Sender: TObject);
begin
  // ����Ϊ1�������ڴ�����ֹ����Ҫ������
  if chkAutorun.Tag = 1 then
  begin
    Exit;
  end;
  chkAutorun.Tag := 1;
  if not SetAutoRun(chkAutorun.Checked) then
  begin
    MessageBox(Handle, PChar(Format(RS_CANT_SET_AUTORUN,
      [SysErrorMessage(GetLastError)])), PChar(RS_MSG_STOP_TITLE), MB_ICONSTOP);
    chkAutorun.Checked := not chkAutorun.Checked;
  end;
  chkAutorun.Tag := 0;
end;

procedure TfrmMain.ConnectServer;
var
  SerAddr: string;
  SerPort: Integer;
  IsRemoteSer: Boolean;
begin
  // if  then
  GetConnectInfo(SerAddr, SerPort, IsRemoteSer);
  if not IsRemoteSer then
  begin
    // �жϷ����Ƿ�������
    if not StartServicebyName(LCXLSHADOW_SER_NAME) then
    begin

      MessageBox(Handle, PChar(Format(RS_CANT_START_SERVICE,
        [SysErrorMessage(GetLastError())])), nil, MB_ICONSTOP);
      Application.ShowMainForm := False;
      Application.Terminate;
      Exit;
    end;
  end;
  // ����SockObj��
  FSockObj := TClientSockObj.Create;
  // ���ӱ���������
  if not FSockObj.ConnectSer(FSockList, SerAddr, SerPort, 2) then
  begin
    MessageBox(Handle, PChar(Format(RS_CANT_CONNECT_SERVICE,
      [SysErrorMessage(WSAGetLastError)])), nil, MB_ICONSTOP);
    FSockObj.Free;

    Application.ShowMainForm := False;
    Application.Terminate;
    Exit;
  end;
  // ��ȡ��֤���
  GetAuth;

  FSockObj.DecRefCount();
end;

function TfrmMain.FormatByteSize(bSize: ULONGLONG): string;
begin
  if bSize < 1000 then
  begin
    Result := Format('%u B', [bSize]);
  end
  else if bSize < 1000 * 1024 then
  begin
    Result := Format('%f KB', [bSize / 1024]);
  end
  else if bSize < 1000 * 1024 * 1024 then
  begin
    Result := Format('%f MB', [bSize / 1024 / 1024]);
  end
  else if bSize < UInt64(1000) * 1024 * 1024 * 1024 then
  begin
    Result := Format('%f GB', [bSize / 1024 / 1024 / 1024]);
  end
  else
  begin
    Result := Format('%f TB', [bSize / 1024 / 1024 / 1024 / 1024]);
  end
end;

procedure TfrmMain.FormClose(Sender: TObject; var Action: TCloseAction);
begin
  if not FCanClose then
  begin
    if FFirstCloseForm then
    begin
      FFirstCloseForm := False;
      trycnMain.BalloonTitle := RS_APPLICATION_TITLE;
      trycnMain.BalloonHint := RS_FIRST_RUNNING_BALLOON_HINT;
      trycnMain.ShowBalloonHint;
    end;
    Action := caNone;
    Hide;
  end;
end;

procedure TfrmMain.FormCreate(Sender: TObject);
begin
  FIOCPMgr := TIOCPManager.Create;
  FSockList := TIOCPClientList.Create(FIOCPMgr);
  FSockList.IOCPEvent := OnIOCPEvent;
  FCanClose := False;
  FCanShutdown := False;
  FFirstCloseForm := True;

  FIconInfo := TMyGetFileInfo.Create;
  FIconInfo.AssignView(True, lvDisk.Handle, True);

  cbbShadowMode.Items.Clear;
  cbbShadowMode.Items.Add(RS_SHADOW_MODE_NONE);
  cbbShadowMode.Items.Add(RS_SHADOW_MODE_ALL);
  cbbShadowMode.Items.Add(RS_SHADOW_MODE_SYSTEM);
  cbbShadowMode.Items.Add(RS_SHADOW_MODE_CUSTOM);

  trycnMain.Hint := RS_APPLICATION_TITLE;
  trycnMain.Visible := True;
  pgcMain.ActivePageIndex := 0;
  Caption := Format('%s - %s', [Caption, RS_VERSION_TYPE]);
  Application.HintHidePause := 5000; // ����Ϊ5��
  // ���������������AutoRun������ʾ������
  Application.ShowMainForm := not FindCmdLineSwitch('AutoRun');
  Application.OnMessage := OnMessage;

  ConnectServer;

end;

procedure TfrmMain.FormDestroy(Sender: TObject);
begin
  FIconInfo.Free;

  FSockList.Free;
  FIOCPMgr.Free;
end;

function TfrmMain.GetAppReuqest(AppReq: PAppRequest): Boolean;
begin
  Result := FSockObj.SendData(LL_APP_REQUEST, AppReq, SizeOf(TAppRequest));

end;

function TfrmMain.GetAuth: Boolean;
var
  // FirstIDInfo: IDINFO;
  AuthData: DWORD;
begin
  // ������DiskIdentifyDevice��ʵ����֤���̣���Ϊ�ڷǹ���Աģʽ�´˺���Ȩ�޲���
  // Result := DiskIdentifyDevice(0, FirstIDInfo) and
  // FSockObj.SendData(LL_AUTH, @FirstIDInfo, SizeOf(FirstIDInfo));
  AuthData := MakeLong(FSockObj.GetRemotePort(), FSockObj.GetLocalPort());
  OutputDebugStr(Format('APP:TfrmMain.GetAuth:AuthData=%08x', [AuthData]));
  Result := FSockObj.SendData(LL_AUTH, @AuthData, SizeOf(AuthData));
end;

procedure TfrmMain.GetConnectInfo(var SerAddr: string; var SerPort: Integer;
  var IsRemoteSer: Boolean);
var
  RemoteSwitchIndex: Integer;
  SerInfo: string;
  I: Integer;
  SerPortSer: string;
begin
  IsRemoteSer := False;
  RemoteSwitchIndex := GetCmdLineSwitchIndex('Remote');
  // ����ҵ�Զ�����ӵ��������
  if RemoteSwitchIndex > 0 then
  begin
    // Remote����������Ҫ���ӵĵ�ַ�Ͷ˿ں� SerAddr:SerPort
    if (ParamCount > RemoteSwitchIndex) then
    begin
      SerInfo := ParamStr(RemoteSwitchIndex + 1);
      I := Pos(':', SerInfo);
      if I > 0 then
      begin
        SerAddr := Copy(SerInfo, 1, I - 1);
        SerPortSer := Copy(SerInfo, I + 1, Length(SerInfo));
        if TryStrToInt(SerPortSer, SerPort) then
        begin
          IsRemoteSer := True;
        end;
      end;
    end;
  end;
  if not IsRemoteSer then
  begin
    SerAddr := 'localhost';
    SerPort := 9999;
  end;
end;

function TfrmMain.GetCustomDiskList: Boolean;
begin
  Result := FSockObj.SendData(LL_GET_CUSTOM_DISK_LIST, nil, 0);
end;

function TfrmMain.GetDiskList: Boolean;
begin
  Result := FSockObj.SendData(LL_GET_DISK_LIST, nil, 0);
end;

function TfrmMain.GetDriverSetting: Boolean;
begin
  // ��ȡ�����б�
  Result := FSockObj.SendData(LL_GET_DRIVER_SETTING, nil, 0);
end;

function TfrmMain.GetSysVolumeInfo(const DosName: string): Boolean;
begin
  // ��ȡϵͳ��ʾ�Ĵ�����Ϣ
  Result := FSockObj.SendData(LL_SYS_VOLUME_INFO, PChar(DosName),
    Length(DosName) * SizeOf(Char));
end;

function TfrmMain.ProcShutdownMsg: Boolean;
var
  SSType: Byte;
begin
  // �����Ի���
  mniShutReset.Enabled := False;
  SSType := ShowSystemShutdownBox(Self);
  if SSType <> SS_NONE then
  begin
    FSockObj.SendData(LL_SYS_SHUTDOWN, @SSType, SizeOf(SSType));
    FCanShutdown := True;
  end;
  mniShutReset.Enabled := True;
  Result := SSType <> SS_NONE;
end;

function TfrmMain.SetCustomDiskList(DiskList: PDeviceNameType;
  Count: Integer): Boolean;
begin
  // �Զ��屣��ģʽ
  Result := FSockObj.SendData(LL_SET_CUSTOM_DISK_LIST, DiskList,
    Count * SizeOf(TDeviceNameType));
end;

function TfrmMain.SetCustomDiskListByList: Boolean;
var
  // hDevice: THandle;
  I: Integer;
  J: Integer;

begin
  // Result := False;
  // ����Ϳ��Կ�ʼ��������
  case cbbShadowMode.ItemIndex of
    3:
      begin
        for I := 0 to Length(FDiskList) - 1 do
        begin
          if FDiskList[I].UserCheck then
          begin
            J := Length(FCustomDiskList);
            SetLength(FCustomDiskList, J + 1);
            StrLCopy(FCustomDiskList[J], FDiskList[I].DiskInfo.VolumeDiskName,
              MAX_DEVNAME_LENGTH);
          end;
        end;
        // �Զ��屣��ģʽ
        Result := SetCustomDiskList(@FCustomDiskList[0],
          Length(FCustomDiskList));
      end;
  else
    Result := True;

  end;
end;

function TfrmMain.SetNextRebootShadowMode(NextShadowMode
  : LCXL_SHADOW_MODE): Boolean;
begin
  Result := FSockObj.SendData(LL_SET_NEXT_REBOOT_SHADOW_MODE, @NextShadowMode,
    SizeOf(NextShadowMode));
end;

function TfrmMain.SetPassword(const Password: string): Boolean;
begin
  Result := FSockObj.SendData(LL_SET_PASSWORD, PChar(Password),
    Length(Password) * SizeOf(Char));
end;

procedure TfrmMain.smobtnSaveDataStateChanged(Sender: TObject;
  State: TAdvSmoothSliderState; Value: Double);
var
  SaveData: TSaveDataWhenShutdown;
begin
  if (lvDisk.ItemIndex < 0) or (lvDisk.ItemIndex >= Length(FDiskList)) then
  begin
    Exit;
  end;
  // �Ƿ񱣴�����
  if smobtnSaveData.Tag = 1 then
  begin
    Exit;
  end;
  smobtnSaveData.Tag := 1;
  case smobtnSaveData.State of
    ssOff:
      begin
        StrLCopy(SaveData.VolumeName, FDiskList[lvDisk.ItemIndex]
          .DiskInfo.VolumeDiskName, MAX_DEVNAME_LENGTH);
        SaveData.IsSaveData := False;
        FSockObj.SendData(LL_SET_SAVE_DATA_WHEN_SHUTDOWN, @SaveData,
          SizeOf(SaveData));
        (*
          hDevice := DI_OpenEx;
          if hDevice <> INVALID_HANDLE_VALUE then
          begin
          StrLCopy(SaveData.VolumeName, DiskList[lvDisk.ItemIndex]
          .DiskInfo.VolumeDiskName, MAX_DEVNAME_LENGTH);
          SaveData.IsSaveData := False;
          if VerifyPassword(Self, hDevice) then
          begin

          if not DI_SetSaveDataWhenShutdown(hDevice, SaveData) then
          begin

          end;
          end
          else
          begin
          MessageBox(Handle, PChar(RS_PASSWORD_VERIFY_FAILURE),
          PChar(RS_MSG_STOP_TITLE), MB_ICONSTOP);
          end;
          DI_Close(hDevice);
          end
          else
          begin
          MessageBox(Handle, PChar(Format(RS_CANT_OPEN_DEVICE,
          [SysErrorMessage(GetLastError)])), PChar(RS_MSG_STOP_TITLE),
          MB_ICONSTOP);
          end;
        *)
      end;

    ssOn:
      begin
        if MessageBox(Handle, PChar(RS_SAVE_DATA_WARNING),
          PChar(RS_MSG_WARNING_TITLE), MB_ICONWARNING or MB_YESNO) = IDYES then
        begin
          StrLCopy(SaveData.VolumeName, FDiskList[lvDisk.ItemIndex]
            .DiskInfo.VolumeDiskName, MAX_DEVNAME_LENGTH);
          SaveData.IsSaveData := True;
          FSockObj.SendData(LL_SET_SAVE_DATA_WHEN_SHUTDOWN, @SaveData,
            SizeOf(SaveData));
        end
        else
        begin
          smobtnSaveData.State := ssOff;
        end;
        (*
          hDevice := DI_OpenEx;
          if hDevice <> INVALID_HANDLE_VALUE then
          begin
          StrLCopy(SaveData.VolumeName,
          DiskList[lvDisk.ItemIndex].DiskInfo.VolumeDiskName,
          MAX_DEVNAME_LENGTH);
          SaveData.IsSaveData := True;
          if VerifyPassword(Self, hDevice) then
          begin
          if not DI_SetSaveDataWhenShutdown(hDevice, SaveData) then
          begin

          end;
          end
          else
          begin
          MessageBox(Handle, PChar(RS_PASSWORD_VERIFY_FAILURE),
          PChar(RS_MSG_STOP_TITLE), MB_ICONSTOP);
          end;
          DI_Close(hDevice);
          end
          else
          begin
          MessageBox(Handle, PChar(Format(RS_CANT_OPEN_DEVICE,
          [SysErrorMessage(GetLastError)])), PChar(RS_MSG_STOP_TITLE),
          MB_ICONSTOP);
          smobtnSaveData.State := ssOff;
          end;
          end
          else
          begin
          smobtnSaveData.State := ssOff;
          end;
        *)
      end;
  end;
  smobtnSaveData.Tag := 0;
  GetDiskList;
end;

function TfrmMain.StartShadowMode(IsShowSelectBox: Boolean): Boolean;
begin
  Result := False;
  if FDriverSetting.CurShadowMode = SM_NONE then
  begin
    if MessageBox(Handle, PChar(RS_START_SHADOW_MODE_WARNNING),
      PChar(RS_MSG_WARNING_TITLE), MB_ICONWARNING or MB_OKCANCEL) <> IDOK then
    begin
      Exit;
    end;
    if IsShowSelectBox then
    begin
      // ��ʾ
      if not ShowShadowModeBox(Self, FDriverSetting.CurShadowMode) then
      begin
        Exit;
      end;

    end
    else
    begin
      // �����Զ��屣���б�
      if not SetCustomDiskListByList then
      begin
        Exit;
      end;
      FDriverSetting.CurShadowMode := LCXL_SHADOW_MODE(cbbShadowMode.ItemIndex);
    end;
    if FDriverSetting.CurShadowMode = SM_NONE then
    begin
      MessageBox(Handle, PChar(RS_START_SHADOW_MODE_NOCHANGE),
        PChar(RS_MSG_INFORMATION_TITLE), MB_ICONINFORMATION);
      Result := True;
      Exit;
    end;

    (*
      hDevice := DI_OpenEx;
      if hDevice = INVALID_HANDLE_VALUE then
      begin
      MessageBox(Handle, PChar(Format(RS_CANT_OPEN_DEVICE,
      [SysErrorMessage(GetLastError)])), PChar(RS_MSG_STOP_TITLE),
      MB_ICONSTOP);
      Exit;
      end;
    *)
    // if VerifyPassword(Self, hDevice) then
    begin
      frmShadowing := TfrmShadowing.Create(Self);
      frmShadowing.Show();
      frmShadowing.Update();
      FSockObj.SendData(LL_START_SHADOW_MODE, @FDriverSetting.CurShadowMode,
        SizeOf(FDriverSetting.CurShadowMode));
      (*
        if not DI_StartShadowMode(hDevice, FDriverSetting.CurShadowMode) then
        begin
        MessageBox(Handle, PChar(Format(RS_CANT_START_SHADOW_MODE,
        [SysErrorMessage(GetLastError)])), PChar(RS_MSG_STOP_TITLE),
        MB_ICONSTOP);
        end
        else
        begin
        MessageBox(Handle, PChar(RS_START_SHADOW_MODE_SUCESSFUL),
        PChar(RS_MSG_INFORMATION_TITLE), MB_ICONINFORMATION);
        end;

        frmShadowing.Close();
        frmShadowing.Free();
      *)
    end
    // else
    // begin
    // MessageBox(Handle, PChar(RS_PASSWORD_VERIFY_FAILURE),
    // PChar(RS_MSG_STOP_TITLE), MB_ICONSTOP);
    // end;
    // DI_Close(hDevice);
    // �ٴλ�ȡ����
    // GetDriverSetting;
    // ��ȡ�����б�
    // GetDiskList;
  end;
end;

procedure TfrmMain.lblCurShadowModeClick(Sender: TObject);
begin
  actStartShadowMode.Execute;
end;

procedure TfrmMain.lblNextShadowModeClick(Sender: TObject);
begin
  actNextShaodowMode.Execute();
end;

procedure TfrmMain.lnklblEmailLinkClick(Sender: TObject; const Link: string;
  LinkType: TSysLinkType);
var
  mailto: string;
begin
  mailto := 'mailto:lcx87654321@163.com?subject=BUG����%5FCXLShadowӰ�ӻ�ԭϵͳ&cc=lcxl@foxmail.com&body=';
  mailto := mailto + '�𾴵��û���%0A' + '%20%20����ʮ�ָ�л���ܳ�������ʱ����������������ǵĳ���������ϸ��д��������'
    + '���Ա��������Ǿ����ҵ�����Ĳ���֮����%0A%0A' + '��Ʒ��LCXLShadowӰ�ӻ�ԭϵͳ%0A' + '����ϵͳ��%0A' +
    '����������%0A' + '���Ľ������������У���%0A%0A�ٴ˸�л�������ǵ�֧�֣�';
  ShellExeCute(Handle, nil, PChar(mailto), '', nil, SW_SHOW);

end;

procedure TfrmMain.lvDiskClick(Sender: TObject);
var
  // δѡ��Ĵ��̾�����
  UnCheckDriverNum: Integer;
  // ��ѡ��Ĵ��̾�����
  CheckedDriverNum: Integer;
  // �Ƿ�ѡ����ϵͳ��
  HasSysDriverCheck: Boolean;
  // ϵͳĿ¼
  WinDir: string;
  I: Integer;
begin
  if lvDisk.Selected <> nil then
  begin
    if FDriverSetting.CurShadowMode = SM_NONE then
    begin
      UnCheckDriverNum := 0;
      CheckedDriverNum := 0;
      HasSysDriverCheck := False;
      FDiskList[lvDisk.Selected.Index].UserCheck :=
        not FDiskList[lvDisk.Selected.Index].UserCheck;
      lvDisk.Refresh;
      SetLength(WinDir, MAX_PATH);
      GetWindowsDirectory(PChar(WinDir), MAX_PATH);
      for I := 0 to Length(FDiskList) - 1 do
      begin
        if FDiskList[I].UserCheck then
        begin
          Inc(CheckedDriverNum);
          if UpCase(WinDir[1]) = UpCase(FDiskList[I].DiskInfo.DosName[0]) then
          begin
            HasSysDriverCheck := True;
          end;
        end
        else
        begin
          Inc(UnCheckDriverNum);
        end;
      end;
      if CheckedDriverNum > 0 then
      begin
        if UnCheckDriverNum > 0 then
        begin
          if (CheckedDriverNum = 1) and HasSysDriverCheck then
          begin
            // ϵͳ�̱���
            cbbShadowMode.ItemIndex := 2;
          end
          else
          begin
            // �Զ��屣��
            cbbShadowMode.ItemIndex := 3;
          end;
        end
        else
        begin
          // ȫ�̱���
          cbbShadowMode.ItemIndex := 1;
        end;
      end
      else
      begin
        // δ����
        cbbShadowMode.ItemIndex := 0;
      end;

    end;
  end;

end;

procedure TfrmMain.lvDiskData(Sender: TObject; Item: TListItem);
var
  VolumeName: string;
begin
  if Item.Index < Length(FDiskList) then
  begin
    // ��ȡ����ͼ��
    if FDiskList[Item.Index].DiskInfo.DosName[0] = '\' then
    begin
      Item.ImageIndex := FIconInfo.GetFileIcon('AA:\');
    end
    else
    begin
      Item.ImageIndex := FIconInfo.GetFileIcon
        (string(FDiskList[Item.Index].DiskInfo.DosName) + '\');
    end;

    if FDiskList[Item.Index].DiskInfo.IsProtect then
    begin
      Item.Caption := RS_SHADOW_MODE;
      if FDiskList[Item.Index].DiskInfo.IsSaveShadowData then
      begin
        Item.Caption := Item.Caption + ', ' + RS_SHADOW_SAVE_DATA;
      end;
      Item.GroupID := 0;
    end
    else
    begin
      Item.Caption := RS_NORMAL_MODE;
      Item.GroupID := 1;
    end;
    VolumeName := PChar(@FDiskList[Item.Index].SysVolInfo.VolumeName[0]);
    if VolumeName = '' then
    begin
      VolumeName := RS_DEFAULT_VOLUME_NAME;
    end;
    Item.SubItems.Add(Format(RS_MAIN_LIST_SUB0,
      [string(FDiskList[Item.Index].DiskInfo.DosName), VolumeName]));
    Item.SubItems.Add(FormatByteSize(FDiskList[Item.Index]
      .SysVolInfo.AvaliableSize) + '/' + FormatByteSize(FDiskList[Item.Index]
      .SysVolInfo.TotalSize));
    Item.StateIndex := Integer(FDiskList[Item.Index].UserCheck);
  end;
end;

procedure TfrmMain.lvDiskDblClick(Sender: TObject);
var
  SelItem: TListItem;
begin
  SelItem := lvDisk.Selected;
  if SelItem <> nil then
  begin
    ShellExeCute(Handle, nil, FDiskList[SelItem.Index].DiskInfo.DosName, nil,
      nil, SW_SHOW);
  end;
end;

procedure TfrmMain.lvDiskInfoTip(Sender: TObject; Item: TListItem;
  var InfoTip: string);
var
  I: Integer;
  MinNum: Integer;
begin
  if Item.Index < Length(FDiskList) then
  begin
    InfoTip := Format('%s: %s', [lvDisk.Column[0].Caption, Item.Caption]);
    MinNum := Item.SubItems.Count + 1;
    if lvDisk.Columns.Count < MinNum then
    begin
      MinNum := lvDisk.Columns.Count;
    end;
    for I := 1 to MinNum - 1 do
    begin
      InfoTip := InfoTip + Format(#13#10'%s: %s', [lvDisk.Column[I].Caption,
        Item.SubItems[I - 1]]);
    end;
    /// /��TMS����ʾ��Ϣ
    // lvDisk.Hint := InfoTip;
    // InfoTip := '';
  end;
end;

procedure TfrmMain.lvDiskSelectItem(Sender: TObject; Item: TListItem;
  Selected: Boolean);
var
  AppReq: TAppRequest;
begin
  if Selected then
  begin
    if (Item.Index >= Low(FDiskList)) and (Item.Index <= High(FDiskList)) then
    begin
      if FDiskList[Item.Index].DiskInfo.IsProtect then
      begin
        StrLCopy(AppReq.VolumeName, FDiskList[Item.Index].DiskInfo.VolumeDiskName,
          MAX_DEVNAME_LENGTH);
        AppReq.RequestType := AR_VOLUME_INFO;
        GetAppReuqest(@AppReq);
      end;
      UpdateDiskStatus(True, @FDiskList[Item.Index]);
    end;
  end
  else
  begin
    UpdateDiskStatus(False, nil);
  end;
end;

procedure TfrmMain.trycnMainClick(Sender: TObject);
begin
  Show;
end;

procedure TfrmMain.tsMainShow(Sender: TObject);
begin
  if FSockObj.IsAuthed then
  begin
    GetDiskList;
  end;

end;

procedure TfrmMain.tsSettingShow(Sender: TObject);
begin
  chkAutorun.Tag := 1;
  chkAutorun.Checked := IsAutoRun;
  chkAutorun.Tag := 0;
  lblReleasePassword.Enabled := FSockObj.PassIsVerified and
    not FSockObj.PassIsEmpty;
  btnReleasePassword.Enabled := lblReleasePassword.Enabled;
end;

procedure TfrmMain.UpdateDiskList(DiskInfoList: PVolumeDiskInfo;
  Count: Integer);
var
  I: Integer;
  AppReq: TAppRequest;
begin

  SetLength(FDiskList, Count);
  for I := 0 to Count - 1 do
  begin
    FDiskList[I].DiskInfo := DiskInfoList[I];;
    if DiskInfoList[I].IsProtect then
    begin
      // ֻ���ڱ���״̬�²���Ч
      StrLCopy(AppReq.VolumeName, DiskInfoList[I].VolumeDiskName,
        MAX_DEVNAME_LENGTH);
      AppReq.RequestType := AR_VOLUME_INFO;
      GetAppReuqest(@AppReq);
    end;
    GetSysVolumeInfo(string(DiskInfoList[I].DosName));
  end;
  lvDisk.Items.Count := Count;
  lvDisk.Refresh;
end;

procedure TfrmMain.UpdateDiskStatus(IsShow: Boolean;
  DiskInfo: PVolumeDiskInfoEx);
begin
  smobtnSaveData.Tag := 1;
  gbDiskStatus.Enabled := IsShow;
  if IsShow then
  begin
    lblShadowFreeSizeTitle.Enabled := DiskInfo.DiskInfo.IsProtect;
    lblShadowFreeSize.Enabled := DiskInfo.DiskInfo.IsProtect;
    gbSaveData.Enabled := DiskInfo.DiskInfo.IsProtect and
      chkEnableSaveData.Checked;
    if DiskInfo.DiskInfo.IsProtect then
    begin
      if DiskInfo.DiskInfo.IsSaveShadowData then
      begin
        smobtnSaveData.State := ssOn;
      end
      else
      begin
        smobtnSaveData.State := ssOff;
      end;
      lblIsProtect.Caption := RS_YES;
      lblShadowFreeSize.Caption :=
        FormatByteSize(DiskInfo.VolumeInfo.AvailableAllocationUnits *
        DiskInfo.VolumeInfo.BytesPerAllocationUnit);
    end
    else
    begin
      lblIsProtect.Caption := RS_NO;
      lblShadowFreeSize.Caption := '';
    end;
  end
  else
  begin
    lblIsProtect.Caption := '';
    lblShadowFreeSizeTitle.Enabled := False;
    lblShadowFreeSize.Enabled := False;
    gbSaveData.Enabled := False;
    lblShadowFreeSize.Caption := '';
  end;
  smobtnSaveData.Tag := 0;
end;

procedure TfrmMain.UpdateDriverSetting(DriverSetting: PAppDriverSetting);
begin
  FDriverSetting := DriverSetting^;
  // ��ǰģʽ
  case FDriverSetting.CurShadowMode of
    SM_NONE:
      begin
        lblCurShadowMode.Caption := RS_SHADOW_MODE_NONE;
      end;
    SM_SYSTEM:
      begin
        lblCurShadowMode.Caption := RS_SHADOW_MODE_SYSTEM;
      end;
    SM_ALL:
      begin
        lblCurShadowMode.Caption := RS_SHADOW_MODE_ALL;
      end;
    SM_CUSTOM:
      begin
        lblCurShadowMode.Caption := RS_SHADOW_MODE_CUSTOM;
      end;
  else
    OutputDebugString('FDriverSetting.CurShadowMode else'#10);
    lblCurShadowMode.Caption := RS_SHADOW_MODE_NONE;
  end;
  if FDriverSetting.CurShadowMode = SM_NONE then
  begin
    lblCurShadowMode.Caption := lblCurShadowMode.Caption + '(' +
      RS_CLICK_TO_CHANGE + ')';
    actCurShadowMode2.Caption := RS_START_SHADOW;
    lvDisk.StateImages := ilListState;
  end
  else
  begin
    actCurShadowMode2.Caption := RS_STOP_SHADOW;
    lvDisk.StateImages := nil;
  end;
  lblCurShadowMode.FlyByEnabled := FDriverSetting.CurShadowMode = SM_NONE;
  lblCurShadowMode.Blinking := FDriverSetting.CurShadowMode = SM_NONE;

  cbbShadowMode.ItemIndex := Integer(FDriverSetting.CurShadowMode);
  cbbShadowMode.Enabled := FDriverSetting.CurShadowMode = SM_NONE;
  // ������ģʽ
  UpdateNextRebootShadowMode(FDriverSetting.NextShadowMode);
  GetDiskList;
  GetCustomDiskList;
end;

procedure TfrmMain.UpdateNextRebootShadowMode(NextShadowMode: LCXL_SHADOW_MODE);
begin
  FDriverSetting.NextShadowMode := NextShadowMode;
  // ������ģʽ
  case NextShadowMode of
    SM_NONE:
      begin
        lblNextShadowMode.Caption := RS_SHADOW_MODE_NONE;
      end;
    SM_SYSTEM:
      begin
        lblNextShadowMode.Caption := RS_SHADOW_MODE_SYSTEM;
      end;
    SM_ALL:
      begin
        lblNextShadowMode.Caption := RS_SHADOW_MODE_ALL;
      end;
    SM_CUSTOM:
      begin
        lblNextShadowMode.Caption := RS_SHADOW_MODE_CUSTOM;
      end;
  else
    OutputDebugString('FDriverSetting.NextShadowMode else'#10);
    lblNextShadowMode.Caption := RS_SHADOW_MODE_NONE;
  end;
  lblNextShadowMode.Caption := lblNextShadowMode.Caption + '(' +
    RS_CLICK_TO_CHANGE + ')';
end;

procedure TfrmMain.UpdateSysVolumeInfo(SysVolInfo: PLLSysVolumeInfo);
var
  I: Integer;
begin
  for I := 0 to Length(FDiskList) - 1 do
  begin
    OutputDebugStr(Format('APP:UpdateSysVolumeInfo, FDiskList[%d]:%s',
      [I, FDiskList[I].DiskInfo.DosName]));
    if StrLComp(FDiskList[I].DiskInfo.DosName, SysVolInfo.DosName,
      SizeOf(SysVolInfo.DosName) div SizeOf(Char)) = 0 then
    begin
      FDiskList[I].SysVolInfo := SysVolInfo.SysVolInfo;
      lvDisk.Refresh;
      break;
    end;
  end;
end;

function TfrmMain.VerifyPassword(const Password: string): Boolean;
begin
  Result := FSockObj.SendData(LL_VERIFY_PASSWORD, PChar(Password),
    Length(Password) * SizeOf(Char));
  if Result then
  begin
    FSockObj.PassIsEmpty := Password = '';
  end;
end;

procedure TfrmMain.UpdateAppReuqest(AppReq: PAppRequest);
var
  I: Integer;
begin
  case AppReq.RequestType of
    AR_VOLUME_INFO:
      begin
        OutputDebugStr(Format('APP:UpdateAppReuqest, AppReq:%s',
          [AppReq.VolumeName]));
        for I := 0 to Length(FDiskList) - 1 do
        begin
          OutputDebugStr(Format('APP:UpdateAppReuqest, FDiskList[%d]:%s',
            [I, FDiskList[I].DiskInfo.VolumeDiskName]));
          if StrLComp(FDiskList[I].DiskInfo.VolumeDiskName, AppReq.VolumeName,
            SizeOf(AppReq.VolumeName) div SizeOf(Char)) = 0 then
          begin
            FDiskList[I].VolumeInfo := AppReq.VolumeInfo;
            lvDisk.Refresh;
            break;
          end;
        end;
      end;
  end;
end;

end.
