unit untDriverInterface;

interface

uses
  SysUtils, Windows, untCommDataType;

const
  // ��������
  DRIVER_SYMBOL_LINK = '\\.\LCXLShadow';

  // ����������һ��----------------------------------------------------------------

const
  // ��ȡ�����б� $800
  IOCTL_GET_DISK_LIST = ((FILE_DEVICE_UNKNOWN shl 16) or
    (FILE_ANY_ACCESS shl 14) or ($0800 shl 2) or METHOD_BUFFERED);
  // ��ȡ�������� $801
  IOCTL_GET_DRIVER_SETTING = ((FILE_DEVICE_UNKNOWN shl 16) or
    (FILE_ANY_ACCESS shl 14) or ($0801 shl 2) or METHOD_BUFFERED);
  // �����´�������Ӱ��ģʽ $802
  IOCTL_SET_NEXT_REBOOT_SHADOW_MODE = ((FILE_DEVICE_UNKNOWN shl 16) or
    (FILE_ANY_ACCESS shl 14) or ($0802 shl 2) or METHOD_BUFFERED);
  // ��ȡ�Զ��屣�������б�
  IOCTL_GET_CUSTOM_DISK_LIST = ((FILE_DEVICE_UNKNOWN shl 16) or
    (FILE_ANY_ACCESS shl 14) or ($0803 shl 2) or METHOD_BUFFERED);
  // �����Զ��屣�������б�
  IOCTL_SET_CUSTOM_DISK_LIST = ((FILE_DEVICE_UNKNOWN shl 16) or
    (FILE_ANY_ACCESS shl 14) or ($0804 shl 2) or METHOD_BUFFERED);
  // �����Զ��屣�������б�
  IOCTL_APP_RQUESET = ((FILE_DEVICE_UNKNOWN shl 16) or
    (FILE_ANY_ACCESS shl 14) or ($0805 shl 2) or METHOD_BUFFERED);
  //����Ӱ��ģʽ
  IOCTL_START_SHADOW_MODE = ((FILE_DEVICE_UNKNOWN shl 16) or
    (FILE_ANY_ACCESS shl 14) or ($0806 shl 2) or METHOD_BUFFERED);
  //�����Ƿ񱣴�����
  IOCTL_SET_SAVE_DATA_WHEN_SHUTDOWN = ((FILE_DEVICE_UNKNOWN shl 16) or
    (FILE_ANY_ACCESS shl 14) or ($0807 shl 2) or METHOD_BUFFERED);
  //�����Ƿ���Ӱ��ģʽ
  IOCTL_IN_SHADOW_MODE = ((FILE_DEVICE_UNKNOWN shl 16) or
    (FILE_ANY_ACCESS shl 14) or ($0808 shl 2) or METHOD_BUFFERED);
  //��ȡ���ID�ţ��˴�Ϊ��ȡAES��key
  IOCTL_GET_RANDOM_ID = ((FILE_DEVICE_UNKNOWN shl 16) or
    (FILE_ANY_ACCESS shl 14) or ($0809 shl 2) or METHOD_BUFFERED);
  //��֤�������IOCTL_GET_RANDOM_ID���ܳ�������Ӳ�����кţ�����ȷ
  IOCTL_VALIDATE_CODE = ((FILE_DEVICE_UNKNOWN shl 16) or
    (FILE_ANY_ACCESS shl 14) or ($080A shl 2) or METHOD_BUFFERED);
  //��֤����
  IOCTL_VERIFY_PASSWORD = ((FILE_DEVICE_UNKNOWN shl 16) or
    (FILE_ANY_ACCESS shl 14) or ($080B shl 2) or METHOD_BUFFERED);
  //��������
  IOCTL_SET_PASSWORD = ((FILE_DEVICE_UNKNOWN shl 16) or
    (FILE_ANY_ACCESS shl 14) or ($080C shl 2) or METHOD_BUFFERED);

  // ------------------------------------------------------------------------------
//�����Ƿ���������
function DI_IsDriverRunning: BOOL; stdcall;
//�����Ƿ���Ӱ��ģʽ
function DI_InShadowMode: BOOL; stdcall;
  // ������
function DI_Open: THandle; stdcall;

// �ر��������
function DI_Close(hDevice: THandle): BOOL; stdcall;

// ��ȡ�����е������ID
// ��ID������֤����ĺϷ���
// ��ID�������루��Ӳ��ID���ɣ����м����Ժ󷢸�����������֤
function DI_GetRandomId(hDevice: THandle; DriverId: Pointer;
  var DriverIdLen: DWORD)
  : BOOL; stdcall;

// ��֤���ܺ�Ļ�����
function DI_ValidateCode(hDevice: THandle; MachineCode: Pointer;
  MachineCodeLen: DWORD): BOOL; stdcall;

// ��ȡ�����б�
// DiskList: �����б�����Ĵ洢״̬Ϊ
function DI_GetDiskList(hDevice: THandle; DiskList: PVolumeDiskInfo;
  var DiskListLen: DWORD): BOOL; stdcall;

// ��ȡ��������
function DI_GetDriverSetting(hDevice: THandle;
  var DriverSetting: TAppDriverSetting): BOOL; stdcall;

// �����´�����ģʽ
function DI_SetNextRebootShadowMode(hDevice: THandle;
  NextShadowMode: LCXL_SHADOW_MODE): BOOL; stdcall;

//��ȡ�Զ���ɳ��ģʽ�Ĵ����б�
function DI_GetCustomDiskList(hDevice: THandle; CustomDiskList: PChar;
  var CustomDiskListLen: DWORD): BOOL; stdcall;

//�����Զ���ɳ��ģʽ�Ĵ����б�
function DI_SetCustomDiskList(hDevice: THandle; CustomDiskList: PChar;
  CustomDiskListLen: DWORD): BOOL; stdcall;

//����/��ȡӦ�ó�������
function DI_AppRequest(hDevice: THandle;
  var AppReq: TAppRequest): BOOL; stdcall;

//����Ӱ��ģʽ
function DI_StartShadowMode(hDevice: THandle;
  CurShadowMode: LCXL_SHADOW_MODE): BOOL; stdcall;

//�����Ƿ񱣴�����
function DI_SetSaveDataWhenShutdown(hDevice: THandle;
  const SaveData: TSaveDataWhenShutdown): BOOL; stdcall;

//��֤����
function DI_VerifyPassword(hDevice: THandle;
  PasswordMD5: PChar; PasswordMD5Len: DWORD): BOOL; stdcall;

//��������
function DI_SetPassword(hDevice: THandle;
  PasswordMD5: PChar; PasswordMD5Len: DWORD): BOOL; stdcall;

implementation

//�����Ƿ���������
function DI_IsDriverRunning: BOOL;
var
  hDevice: THandle;
begin
  hDevice := DI_Open;
  Result := hDevice <> INVALID_HANDLE_VALUE;
  if Result then
  begin
    CloseHandle(hDevice);
  end;
end;
//�����Ƿ���Ӱ��ģʽ
function DI_InShadowMode: BOOL;
var
  hDevice: THandle;
  cbout: DWORD;
begin
  hDevice := DI_Open;
  Result := hDevice <> INVALID_HANDLE_VALUE;
  if Result then
  begin
    Result := DeviceIoControl(hDevice, IOCTL_IN_SHADOW_MODE, nil, 0, nil, 0,
      cbout, nil);
    CloseHandle(hDevice);
  end;
end;

function DI_Open: THandle;
begin
  Result := CreateFile(DRIVER_SYMBOL_LINK, GENERIC_READ or GENERIC_WRITE, 0,
    nil, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
end;

function DI_Close(hDevice: THandle): BOOL;
begin
  Result := CloseHandle(hDevice);
end;

function DI_GetRandomId(hDevice: THandle; DriverId: Pointer;
  var DriverIdLen: DWORD): BOOL;
var
  cbout: DWORD;
begin
  Result := DeviceIoControl(hDevice, IOCTL_GET_RANDOM_ID, nil, 0, DriverId,
    DriverIdLen, cbout, nil);
  if Result then
  begin
    DriverIdLen := cbout;
  end;
end;

function DI_ValidateCode(hDevice: THandle; MachineCode: Pointer;
  MachineCodeLen: DWORD): BOOL;
var
  cbout: DWORD;
begin
  Result := DeviceIoControl(hDevice, IOCTL_VALIDATE_CODE, MachineCode,
    MachineCodeLen, nil, 0, cbout, nil);
end;

function DI_GetDiskList(hDevice: THandle; DiskList: PVolumeDiskInfo;
  var DiskListLen: DWORD): BOOL;
var
  cbout: DWORD;
begin
  Result := DeviceIoControl(hDevice, IOCTL_GET_DISK_LIST, nil, 0, DiskList,
    DiskListLen * sizeof(TVolumeDiskInfo), cbout, nil);
  if Result then
  begin
    cbout := cbout div sizeof(TVolumeDiskInfo);
  end;
  DiskListLen := cbout;
end;

function DI_GetDriverSetting(hDevice: THandle;
  var DriverSetting: TAppDriverSetting): BOOL;
var
  outlen: DWORD;
begin
  outlen := sizeof(DriverSetting);
  Result := DeviceIoControl(hDevice, IOCTL_GET_DRIVER_SETTING, nil, 0,
    @DriverSetting, outlen, outlen, nil);
end;

function DI_SetNextRebootShadowMode(hDevice: THandle;
  NextShadowMode: LCXL_SHADOW_MODE): BOOL;
var
  outlen: DWORD;
begin
  Result := DeviceIoControl(hDevice, IOCTL_SET_NEXT_REBOOT_SHADOW_MODE,
    @NextShadowMode, sizeof(NextShadowMode), nil, 0, outlen, nil);
end;

//��ȡ�Զ���ɳ��ģʽ�Ĵ����б�
function DI_GetCustomDiskList(hDevice: THandle; CustomDiskList: PChar;
  var CustomDiskListLen: DWORD): BOOL;
var
  outlen: DWORD;
begin
  Result := DeviceIoControl(hDevice, IOCTL_GET_CUSTOM_DISK_LIST,
    nil, 0, CustomDiskList, CustomDiskListLen*MAX_DEVNAME_LENGTH *sizeof(Char),
    outlen, nil);
  if Result then
  begin
    CustomDiskListLen := outlen div (MAX_DEVNAME_LENGTH * sizeof(Char));
  end;
end;

//�����Զ���ɳ��ģʽ�Ĵ����б�
function DI_SetCustomDiskList(hDevice: THandle; CustomDiskList: PChar;
  CustomDiskListLen: DWORD): BOOL;
var
  outlen: DWORD;
begin
  Result := DeviceIoControl(hDevice, IOCTL_SET_CUSTOM_DISK_LIST,
    CustomDiskList, CustomDiskListLen*MAX_DEVNAME_LENGTH *sizeof(Char),
    nil, 0, outlen, nil);
end;

//����/��ȡӦ�ó�������
function DI_AppRequest(hDevice: THandle; var AppReq: TAppRequest): BOOL;
var
  outlen: DWORD;
begin
  Result := DeviceIoControl(hDevice, IOCTL_APP_RQUESET,
    @AppReq, sizeof(AppReq), @AppReq, sizeof(AppReq), outlen, nil);
end;

//����Ӱ��ģʽ
function DI_StartShadowMode(hDevice: THandle;
  CurShadowMode: LCXL_SHADOW_MODE): BOOL;
var
  outlen: DWORD;
begin
  Result := DeviceIoControl(hDevice, IOCTL_START_SHADOW_MODE,
    @CurShadowMode, sizeof(CurShadowMode), nil, 0, outlen, nil);
end;

function DI_SetSaveDataWhenShutdown(hDevice: THandle;
  const SaveData: TSaveDataWhenShutdown): BOOL;
var
  outlen: DWORD;
begin
  Result := DeviceIoControl(hDevice, IOCTL_SET_SAVE_DATA_WHEN_SHUTDOWN,
    @SaveData, sizeof(SaveData), nil, 0, outlen, nil);
end;

//��֤����
function DI_VerifyPassword(hDevice: THandle;
  PasswordMD5: PChar; PasswordMD5Len: DWORD): BOOL;
var
  outlen: DWORD;
begin
  Result := DeviceIoControl(hDevice, IOCTL_VERIFY_PASSWORD,
    PasswordMD5, PasswordMD5Len, nil, 0, outlen, nil);
end;

//��������
function DI_SetPassword(hDevice: THandle;
  PasswordMD5: PChar; PasswordMD5Len: DWORD): BOOL;
var
  outlen: DWORD;
begin
  Result := DeviceIoControl(hDevice, IOCTL_SET_PASSWORD,
    PasswordMD5, PasswordMD5Len, nil, 0, outlen, nil);
end;

end.


