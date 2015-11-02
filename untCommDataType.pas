unit untCommDataType;

interface
uses
  Windows;

const
  //����������������
  DRIVER_ERROR_SUCCESS = $00000000;//�ɹ�
  DRIVER_ERROR_SETTING_FILE_NOT_FOUND = $00000001;//���������ļ�������
  MAX_DEVNAME_LENGTH = 64;
type
  TDeviceNameType = array [0 .. MAX_DEVNAME_LENGTH-1] of Char;
  {$POINTERMATH ON}
  PDeviceNameType = ^TDeviceNameType;
  {$POINTERMATH OFF}
type
{$Z+}// 4�ֽ�
  LCXL_SHADOW_MODE = (SM_NONE, SM_ALL, SM_SYSTEM, SM_CUSTOM);
  PLCXL_SHADOW_MODE = ^LCXL_SHADOW_MODE;
  //Ӧ�ó�����������
  //AR_VOLUME_INFO:����Ϣ
  APP_REQUEST_TYPE = (AR_VOLUME_INFO);
  PAPP_REQUEST_TYPE = APP_REQUEST_TYPE;
{$Z-}
type
  _VOLUME_DISK_INFO = packed record
    ///	<summary>
    ///	  �̷���
    ///	</summary>
    DosName: array [0 .. MAX_DEVNAME_LENGTH-1] of Char;
    ///	<summary>
    ///	  �˴����Ƿ񱻱���
    ///	</summary>
    IsProtect: Boolean;
    ///	<summary>
    ///	  �Ƿ��ڹػ���ʱ�򱣴�����
    ///	</summary>
    IsSaveShadowData: Boolean;
    ///	<summary>
    ///	  ���̾�����
    ///	</summary>
    VolumeDiskName: array [0 .. MAX_DEVNAME_LENGTH-1] of Char;
  end;

  VOLUME_DISK_INFO = _VOLUME_DISK_INFO;
  PVOLUME_DISK_INFO = ^VOLUME_DISK_INFO;
  TVolumeDiskInfo = VOLUME_DISK_INFO;
  {$POINTERMATH ON}
  PVolumeDiskInfo = ^TVolumeDiskInfo;
  {$POINTERMATH OFF}
  // ��ȡ��������
  _APP_DRIVER_SETTING = packed record
    DriverErrorType: ULONG;//���������й������Ƿ��д���
    CurShadowMode: LCXL_SHADOW_MODE; // ��ǰ���̱���ģʽ
    NextShadowMode: LCXL_SHADOW_MODE; // ���̱���ģʽ
  end;

  APP_DRIVER_SETTING = _APP_DRIVER_SETTING;
  PAPP_DRIVER_SETTING = ^APP_DRIVER_SETTING;
  TAppDriverSetting = APP_DRIVER_SETTING;
  PAppDriverSetting = ^TAppDriverSetting;

  _APP_REQUEST_VOLUME_INFO = packed record
	  BytesPerSector: ULONG;//ÿ�������������ֽ���
	  SectorsPerAllocationUnit: ULONG;//���䵥Ԫ�а�����������
  	BytesPerAllocationUnit: ULONG;//���䵥Ԫ�а������ֽ�����ΪBytesPerSector*SectorsPerAllocationUnit
	  TotalAllocationUnits: ULONGLONG;//�ܹ��ж��ٷ��䵥Ԫ
	  AvailableAllocationUnits: ULONGLONG;//���ж��ٿ��õĵ�Ԫ��
  end;
  APP_REQUEST_VOLUME_INFO = _APP_REQUEST_VOLUME_INFO;
  PAPP_REQUEST_VOLUME_INFO = ^APP_REQUEST_VOLUME_INFO;

  //Ӧ�ó�������
  _APP_REQUEST = packed record
	  //IN �̷���
	  VolumeName: array [0..MAX_DEVNAME_LENGTH-1] of Char;
	  //IN ��������
	  RequestType: APP_REQUEST_TYPE ;
	  //��������ڴ�
    case Integer of
    0:
    (
      VolumeInfo: APP_REQUEST_VOLUME_INFO ;//AR_VOLUME_INFO
    );
 end;
 APP_REQUEST = _APP_REQUEST;
 PAPP_REQUEST = ^APP_REQUEST;
 TAppRequest = APP_REQUEST;
 PAppRequest = ^TAppRequest;

 //�Ƿ��ڹػ���ʱ�򱣴�Ӱ������
 _SAVE_DATA_WHEN_SHUTDOWN = packed record
   //�̷���
	VolumeName: array [0..MAX_DEVNAME_LENGTH-1] of Char;
	//�Ƿ񱣴�����
	IsSaveData: Boolean;
 end;
 SAVE_DATA_WHEN_SHUTDOWN = _SAVE_DATA_WHEN_SHUTDOWN;
 PSAVE_DATA_WHEN_SHUTDOWN = ^SAVE_DATA_WHEN_SHUTDOWN;
 TSaveDataWhenShutdown = SAVE_DATA_WHEN_SHUTDOWN;
 PSaveDataWhenShutdown = ^TSaveDataWhenShutdown;



implementation

end.
