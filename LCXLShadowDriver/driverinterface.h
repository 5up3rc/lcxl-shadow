#ifndef _DRIVER_INTERFACE_H_
#define _DRIVER_INTERFACE_H_
//����4201���棺ʹ���˷Ǳ�׼��չ : �����ƵĽṹ/����
#pragma warning(disable:4201)
//��ȡ�����б�
#define IOCTL_GET_DISK_LIST CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
//��ȡ��������
#define IOCTL_GET_DRIVER_SETTING CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
//�����´�������Ӱ��ģʽ
#define IOCTL_SET_NEXT_REBOOT_SHADOW_MODE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
//��ȡ�Զ��屣�������б�
#define IOCTL_GET_CUSTOM_DISK_LIST CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)
//�����Զ��屣�������б�
#define IOCTL_SET_CUSTOM_DISK_LIST CTL_CODE(FILE_DEVICE_UNKNOWN, 0x804, METHOD_BUFFERED, FILE_ANY_ACCESS)
//��ȡ/����Ӧ�ó�������
#define IOCTL_APP_RQUESET CTL_CODE(FILE_DEVICE_UNKNOWN, 0x805, METHOD_BUFFERED, FILE_ANY_ACCESS)
//����Ӱ��ģʽ
#define IOCTL_START_SHADOW_MODE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x806, METHOD_BUFFERED, FILE_ANY_ACCESS)
//�����Ƿ񱣴�����
#define IOCTL_SET_SAVE_DATA_WHEN_SHUTDOWN CTL_CODE(FILE_DEVICE_UNKNOWN, 0x807, METHOD_BUFFERED, FILE_ANY_ACCESS)
//�����Ƿ���Ӱ��ģʽ
#define IOCTL_IN_SHADOW_MODE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x808, METHOD_BUFFERED, FILE_ANY_ACCESS)
//��ȡ���ID�ţ��˴�Ϊ��ȡAES��key
#define IOCTL_GET_RANDOM_ID CTL_CODE(FILE_DEVICE_UNKNOWN, 0x809, METHOD_BUFFERED, FILE_ANY_ACCESS)
//��֤�������IOCTL_GET_RANDOM_ID���ܳ�������Ӳ�����кţ�����ȷ
#define IOCTL_VALIDATE_CODE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x80A, METHOD_BUFFERED, FILE_ANY_ACCESS)
//��֤����
#define IOCTL_VERIFY_PASSWORD CTL_CODE(FILE_DEVICE_UNKNOWN, 0x80B, METHOD_BUFFERED, FILE_ANY_ACCESS)
//��������
#define IOCTL_SET_PASSWORD CTL_CODE(FILE_DEVICE_UNKNOWN, 0x80C, METHOD_BUFFERED, FILE_ANY_ACCESS)
//����������������
#define DRIVER_ERROR_SUCCESS 0x00000000//�ɹ�
#define DRIVER_ERROR_SETTING_FILE_NOT_FOUND 0x00000001//���������ļ�������
//
//  Buffer size for local names on the stack
//
#define MAX_DEVNAME_LENGTH      64

//���̱���ģʽ
//PM_NONE:������
//SM_ALL:����ȫ������
//SM_SYSTEM:ֻ����ϵͳ��
//PM_PROTECT_CUSTOM���Զ���
typedef enum _LCXL_SHADOW_MODE {SM_NONE, SM_ALL, SM_SYSTEM, SM_CUSTOM} LCXL_SHADOW_MODE, *PLCXL_SHADOW_MODE;
//Ӧ�ó�����������
//AR_VOLUME_INFO:����Ϣ
typedef enum _APP_REQUEST_TYPE {AR_VOLUME_INFO} APP_REQUEST_TYPE, *PAPP_REQUEST_TYPE;

#include <pshpack1.h>

typedef struct _VOLUME_DISK_INFO {
	//�̷���
	WCHAR DosName[MAX_DEVNAME_LENGTH];
	//�Ƿ񱻱���
	BOOLEAN IsProtect;
	//�Ƿ��ڹػ���ʱ�򱣴�����
	BOOLEAN IsSaveShadowData;
	//���̾�����
	WCHAR VolumeDiskName[MAX_DEVNAME_LENGTH];
} VOLUME_DISK_INFO, *PVOLUME_DISK_INFO;

typedef struct _APP_DRIVER_SETTING {
	ULONG DriverErrorType;//���������й������Ƿ��д���
	LCXL_SHADOW_MODE CurShadowMode;//��ǰ���̱���ģʽ
	LCXL_SHADOW_MODE NextShadowMode;//���̱���ģʽ
} APP_DRIVER_SETTING, *PAPP_DRIVER_SETTING;

typedef struct _APP_REQUEST_VOLUME_INFO {
	ULONG BytesPerSector;//ÿ�������������ֽ���
	ULONG SectorsPerAllocationUnit;//���䵥Ԫ�а�����������
	ULONG BytesPerAllocationUnit;//���䵥Ԫ�а������ֽ�����ΪBytesPerSector*SectorsPerAllocationUnit
	ULONGLONG TotalAllocationUnits;//�ܹ��ж��ٷ��䵥Ԫ
	ULONGLONG AvailableAllocationUnits;//���ж��ٿ��õĵ�Ԫ��
} APP_REQUEST_VOLUME_INFO, *PAPP_REQUEST_VOLUME_INFO;

//Ӧ�ó�������
typedef struct _APP_REQUEST {
	//�̷���
	IN WCHAR VolumeName[MAX_DEVNAME_LENGTH];
	//��������
	IN APP_REQUEST_TYPE RequestType;
	//��������ڴ�
	union {
		OUT APP_REQUEST_VOLUME_INFO VolumeInfo;//AR_VOLUME_INFO
	};
} APP_REQUEST, *PAPP_REQUEST;

//�Ƿ��ڹػ���ʱ�򱣴�Ӱ������
typedef struct _SAVE_DATA_WHEN_SHUTDOWN {
	//�̷���
	WCHAR VolumeName[MAX_DEVNAME_LENGTH];
	//�Ƿ񱣴�����
	BOOLEAN IsSaveData;
} SAVE_DATA_WHEN_SHUTDOWN, *PSAVE_DATA_WHEN_SHUTDOWN;
#include <poppack.h>

#endif