unit untResString;

interface

resourcestring
  RS_MSG_INFORMATION_TITLE = '��Ϣ';
  RS_MSG_WARNING_TITLE = '����';
  RS_MSG_STOP_TITLE = '����';
  RS_YES = '��';
  RS_NO = '��';
  RS_VERSION_TYPE = 'Alpha���˰�';
//LCXLShadowConsole.dpr
  RS_CONSOLE_IS_RUNNING = 'LCXLShadow����̨�Ѿ���ǰ̨���С�';
  RS_SERVICE_IS_CLOSED = 'LCXLShadow�����ѹرգ�������̨�����˳���';
//untMain.pas
  RS_NOT_ADMIN_TEXT = 'ʮ�ֱ�Ǹ�������ǹ���Ա�û������Թ���Ա������б������';
  RS_NOT_ADMIN_TITLE = 'Ȩ�޲���';
  RS_APPLICATION_TITLE = 'LCXLShadow Ӱ��ϵͳ���ƶ�';
  RS_SHADOW_MODE = 'Ӱ��ģʽ';
  RS_NORMAL_MODE = 'δ����';
  RS_SHADOW_SAVE_DATA = '�ػ�����';
  RS_FIRST_RUNNING_BALLOON_HINT = '����̨�����������У�����˴���ʾ�����򣬻��ߵ���Ҽ�ѡ���˳�����ť�˳���';
  RS_CANT_GET_DISK_LIST = 'ʮ�ֱ�Ǹ�������޷���ȡ�����б�'#10'������Ϣ��%s';
  RS_CANT_GET_DRIVER_SETTING = 'ʮ�ֱ�Ǹ�������޷���ȡ������Ϣ��'#10'������Ϣ��%s';
  RS_CANT_OPEN_DEVICE = 'ʮ�ֱ�Ǹ���޷��������豸��'#10'������Ϣ��%s';
  RS_CANT_SET_NEXT_REBOOT_SHADOW_MODE = 'ʮ�ֱ�Ǹ���޷������´�������Ӱ��ģʽ��'#10'������Ϣ��%s';
  RS_CANT_START_SHADOW_MODE = 'ʮ�ֱ�Ǹ���޷�����Ӱ��ģʽ��'#10'������Ϣ��%s';
  RS_CANT_SET_AUTORUN = 'ʮ�ֱ�Ǹ���޷����ÿ�������'#10'������Ϣ��%s';
  RS_CANT_CONNECT_SERVICE = 'ʮ�ֱ�Ǹ���޷����ӵ�Ӱ��ϵͳ�������'#10'������Ϣ��%s';
  RS_CANT_START_SERVICE = 'ʮ�ֱ�Ǹ���޷�����Ӱ��ϵͳ�������'#10'������Ϣ��%s';
  RS_CANT_AUTH = 'ʮ�ֱ�Ǹ��Ӱ��ϵͳ��������޷���֤������İ�ȫ�ԣ������°�װ�������';
  RS_START_SHADOW_MODE_WARNNING = '��ע�⣬�ڿ���Ӱ��ģʽ֮ǰ������Ҫ�ȱ������Ĺ��������ر������޹س���'#13#10#13#10'�����ȷ������ť�����������ȡ������ťȡ����������';
  RS_START_SHADOW_MODE_SUCESSFUL = '����Ӱ��ģʽ�ɹ���';
  RS_START_SHADOW_MODE_NOCHANGE = '���Ѿ����ڴ˱���ģʽ���ˡ�';
  RS_STOP_SHADOW_RESTART_WARNNING = 'Ҫ�˳�Ӱ��ģʽ���������ĵ��ԣ�����Ҫ������';
  RS_SAVE_DATA_WARNING = '��ע�⣬������ᱣ��˴���Ӱ��ģʽ�������޸ĵ����ݣ������������'#13#10'�Ƿ������';
  RS_DRIVER_NO_RUNNING = 'ʮ�ֱ�Ǹ��Ӱ��ϵͳδ���������������װ�������û�����������������ĵ��ԡ�';
  RS_MAIN_LIST_SUB0 = '%s(%s)';
  RS_DEFAULT_VOLUME_NAME = '���ش���';
  RS_CLICK_TO_CHANGE = '�������';
  RS_SHADOW_MODE_NONE = 'δ����';
  RS_SHADOW_MODE_SYSTEM = 'ϵͳ�̱���';
  RS_SHADOW_MODE_ALL = 'ȫ�̱���';
  RS_SHADOW_MODE_CUSTOM = '�Զ��屣��';
  RS_START_SHADOW = '��ʼ����(&S)';
  RS_STOP_SHADOW = 'ֹͣ����(&S)';
  RS_PASSWORD_CLEARED = '����Ȩ�����ͷţ�����Ҫ���������������ִ�ж�Ӱ��ϵͳ�Ĺ������';

//untSetShadowMode
  RS_DOS_NAME = '%s��';
  RS_CANT_GET_CUSTOM_DISK_LIST = 'ʮ�ֱ�Ǹ���޷���ȡ�Զ��屣�������б�'#10'������Ϣ��%s';
  RS_CANT_SET_CUSTOM_DISK_LIST = 'ʮ�ֱ�Ǹ���޷������Զ��屣�������б�'#10'������Ϣ��%s';
 //untSystemShutdown.pas
  RS_NEXT_REBOOT_SHADOW_MODE = '�´�����ģʽ';
  //untSetPassword
  RS_PASSWORD_NOT_MATCH = '����������벻һ�£����������롣';
  RS_NO_PASSWORD_WARNING = '�������˿����룬ϵͳ���������뱣�����Ƿ������';
  RS_SET_PASSWORD_FAILURE = 'ʮ�ֱ�Ǹ����������ʧ�ܣ�'#10'������Ϣ��%s';
  //untVerifyPassword
  RS_PASSWORD_VERIFY_FAILURE = '������֤ʧ�ܣ�';
implementation

end.
