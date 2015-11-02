unit untSerInterface;

interface
uses
  Windows, untCommDataType;
const
  LCXLSHADOW_SER_NAME = 'LCXLShadowSer';

const
  //NONE
  LL_NONE = $0000;
  //��֤
  LL_AUTH = $0001;
  //�����Ƿ�������
  LL_IS_DRIVER_RUNNING = $0002;
  //��ȡ��������
  LL_GET_DRIVER_SETTING = $0003;
  //�����´�������Ӱ��ģʽ
  LL_SET_NEXT_REBOOT_SHADOW_MODE = $0004;
  //�Ƿ���Ӱ��ģʽ��
  LL_IN_SHADOW_MODE = $0005;
  //��������
  LL_SET_PASSWORD = $0006;
  //��֤����
  LL_VERIFY_PASSWORD = $0007;
  //�����������
  LL_CLEAR_PASSWORD = $0008;
  //��ȡ�����б�
  LL_GET_DISK_LIST = $0009;
  //��ȡ�����б�
  LL_GET_CUSTOM_DISK_LIST = $000A;
  //��ȡ�����б�
  LL_SET_CUSTOM_DISK_LIST = $000B;
  //��������
  LL_APP_REQUEST= $000C;
  //����Ӱ��ģʽ
  LL_START_SHADOW_MODE = $000D;
  //���õ��ػ�ʱ��������
  LL_SET_SAVE_DATA_WHEN_SHUTDOWN = $000E;

  //�ɹ� ���ɹ�������
  LL_SUCC = $FF01;
  //ʧ�ܣ�ʧ�ܵ����ʧ����
  LL_FAIL = $FF02;

  //���������ֻ�ͷ�������йأ������������򽻻�
  //��ȡϵͳ�Ĵ�����Ϣ
  LL_SYS_VOLUME_INFO = $0101;
  //���ùػ�ģʽ
  LL_SYS_SHUTDOWN = $0102;
type
  //LL_SYS_VOLUME_INFO
  //ϵͳ��ʾ�Ĵ�����Ϣ
  TSysVolumeInfo = record
    ///	<summary>
    ///	  ���
    ///	</summary>
    VolumeName: array [0 .. MAX_DEVNAME_LENGTH-1] of Char;
    ///	<summary>
    ///	  ʣ����̿ռ�
    ///	</summary>
    AvaliableSize: ULONGLONG;
    ///	<summary>
    ///	  �ܹ����̿ռ�
    ///	</summary>
    TotalSize: TLargeInteger;
  end;
  PSysVolumeInfo = ^TSysVolumeInfo;

  //����
  TLLSysVolumeInfo = record
    DosName: array [0..MAX_DEVNAME_LENGTH-1] of Char;
    SysVolInfo: TSysVolumeInfo;
  end;
  PLLSysVolumeInfo = ^TLLSysVolumeInfo;
  //LL_SYS_SHUTDOWN
const
  //TSYS_SHUTDOWN_EMUN
  SS_NONE = $00;
  SS_SHUTDOWN = $01;
  SS_REBOOT = $02;

implementation

end.
