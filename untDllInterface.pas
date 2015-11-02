unit untDllInterface;

interface
uses
  Windows, untCommDataType;

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

const
  LCXL_SHADOW_DLL = 'LCXLShadow.dll';

function DI_IsDriverRunning; external LCXL_SHADOW_DLL name 'DI_IsDriverRunning';
function DI_InShadowMode; external LCXL_SHADOW_DLL name 'DI_InShadowMode';
function DI_Open; external LCXL_SHADOW_DLL name 'DI_Open';
function DI_Close; external LCXL_SHADOW_DLL name 'DI_Close';
function DI_GetRandomId; external LCXL_SHADOW_DLL name 'DI_GetRandomId';
function DI_ValidateCode; external LCXL_SHADOW_DLL name 'DI_ValidateCode';
function DI_GetDiskList; external LCXL_SHADOW_DLL name 'DI_GetDiskList';
function DI_GetDriverSetting; external LCXL_SHADOW_DLL name 'DI_GetDriverSetting';
function DI_SetNextRebootShadowMode; external LCXL_SHADOW_DLL name 'DI_SetNextRebootShadowMode';
function DI_GetCustomDiskList; external LCXL_SHADOW_DLL name 'DI_GetCustomDiskList';
function DI_SetCustomDiskList; external LCXL_SHADOW_DLL name 'DI_SetCustomDiskList';
function DI_AppRequest; external LCXL_SHADOW_DLL name 'DI_AppRequest';
function DI_StartShadowMode; external LCXL_SHADOW_DLL name 'DI_StartShadowMode';
function DI_SetSaveDataWhenShutdown; external LCXL_SHADOW_DLL name 'DI_SetSaveDataWhenShutdown';
function DI_VerifyPassword; external LCXL_SHADOW_DLL name 'DI_VerifyPassword';
function DI_SetPassword; external LCXL_SHADOW_DLL name 'DI_SetPassword';

end.
