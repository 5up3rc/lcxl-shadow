#ifndef _DRIVER_LIB_INTERFACE_H_
#define _DRIVER_LIB_INTERFACE_H_

#include "bitmapmgr.h"
#include "driverinterface.h"
//
//  Դ�ļ���FilterInterface.h
//  ���ߣ��޳���(LCXL)
//  ��Ȩ��Copyright (c) 2011  LCXL
//  ˵�������������Ľӿڲ��֣��ӿڵ�ʵ�ֲ�������LLShadowMain.c�У����LLShadowMain.c����������ļ�

//�˵�������������LLShadowMain.c��
extern PDRIVER_OBJECT g_LCXLDriverObject;//�������������豸
extern RTL_OSVERSIONINFOEXW g_OSVerInfo;//����ϵͳ�汾��


//
//  Here is what the major and minor versions should be for the various OS versions:
//
//  OS Name                                 MajorVersion    MinorVersion
//  ---------------------------------------------------------------------
//  Windows 2000                             5                 0
//  Windows XP                               5                 1
//  Windows Server 2003                      5                 2
//

#define IS_WINDOWSXP() \
	(g_OSVerInfo.dwMajorVersion == 5 && g_OSVerInfo.dwMinorVersion == 1)

#define IS_WINDOWSSRV2003_OR_LATER() \
	((g_OSVerInfo.dwMajorVersion == 5 && g_OSVerInfo.dwMinorVersion >= 2) || \
	g_OSVerInfo.dwMajorVersion > 5)

#define IS_WINDOWSVISTA_OR_LATER() \
	(g_OSVerInfo.dwMajorVersion >= 6)

#define IS_WINDOWS2000_OR_OLDER() \
	((g_OSVerInfo.dwMajorVersion == 5 && g_OSVerInfo.dwMinorVersion == 0) || \
	(g_OSVerInfo.dwMajorVersion < 5))

#define DISKPERF_MAXSTR         64


typedef enum _DO_TYPE {
	DO_CONTROL,//�����豸
	DO_FSVOL,//������豸
	DO_FSCTRL,//�ļ�ϵͳ�����豸
	DO_DISKVOL,//���̾�����豸
	DO_DISK,//���̹����豸
} DO_TYPE;

//�ض���ṹ
typedef struct _LCXL_TABLE_MAP {
	ULONGLONG	orgIndex;	// ԭʼ�ص�ַ
	ULONGLONG	mapIndex;	// �ض����ĵ�ַ
	BOOLEAN		IsVisited;//�Ƿ���ʹ�������ڱ���Ӱ��ϵͳʱ����
	BOOLEAN		IsDiskRef;//�Ƿ��Ǵ����豸�ض���
} LCXL_TABLE_MAP, *PLCXL_TABLE_MAP;

//���ڴ��̾����������ڱ�ʾ������̾���ļ�ϵͳ�Ƿ�׼������
typedef enum _DV_INITIALIZE_STATE {
	DVIS_NONE,//û�г�ʼ����
	DVIS_INITIALIZING,//���ڳ�ʼ��
	DVIS_INITIALIZED,//�Ѿ���ʼ�����
	DVIS_WORKING,//��ʼ����
	DVIS_SAVINGDATA,//���ڱ���Ӱ��ϵͳ���ļ�
} DV_INITIALIZE_STATE, *PDV_INITIALIZE_STATE;

//���ڴ������������ڱ�ʾ������̾�
typedef enum _DISK_INITIALIZE_STATE {
	DIS_NORMAL,//Ĭ��Ϊ����ģʽ
	DIS_MEMORY,//�ڴ��ض���
	DIS_NO_VOLUME,//��ʾ�˴���û�д��̾�
} DISK_INITIALIZE_STATE, *PDISK_INITIALIZE_STATE;

typedef struct _REDIRECT_RW_MEM {
	LIST_ENTRY ListEntry;
	PVOID buffer;
	ULONGLONG offset;
	ULONG length;
} REDIRECT_RW_MEM, *PREDIRECT_RW_MEM;

typedef struct _APP_REQUEST_LIST_ENTRY {
	LIST_ENTRY ListEntry;
	NTSTATUS status;//Ӧ�ó���������
	KEVENT FiniEvent;//����¼�
	APP_REQUEST AppRequest;//Ӧ�ó�������
} APP_REQUEST_LIST_ENTRY, *PAPP_REQUEST_LIST_ENTRY;

typedef struct _DISK_VOL_FILTER_EXTENT {
	PDEVICE_OBJECT FilterDO;//�����ϲ�Ĵ��̾�����豸
	//
	// Specifies the offset and length of this
	// extent relative to the beginning of the
	// disk.
	//
	LARGE_INTEGER StartingOffset;
	LARGE_INTEGER ExtentLength;
} DISK_VOL_FILTER_EXTENT, *PDISK_VOL_FILTER_EXTENT;

//�豸��չ��
typedef struct _EP_DEVICE_EXTENSION {
	//�豸����
	DO_TYPE DeviceType;
	union {
		//�ǹ����豸
		struct _DE_FILTER {
			//�²������豸����
			PDEVICE_OBJECT LowerDeviceObject;
			//Ҫ�󶨵����������豸����
			PDEVICE_OBJECT PhysicalDeviceObject;
			//ע�⣺���ļ�ϵͳ�У�����������豸�����Ǵ��̾����ƣ������������������豸���ƣ��ļ�ϵͳ�豸��û�����ֵģ�
			//�����豸����
			UNICODE_STRING PhysicalDeviceName;
			//�����豸���ƻ�����
			WCHAR PhysicalDeviceNameBuffer[MAX_DEVNAME_LENGTH];
			
			union {
				/*
				//DO_FSCTRL �ļ�ϵͳ�ṹ
				struct _DE_FSCTRL {
				
				} DE_FSCTRL;
				*/
				//DO_FSVOL �ļ�ϵͳ�����豸��չ
				struct _DE_FSVOL {
					//���̾����
					PDEVICE_OBJECT DiskVolDeviceObject;
					//���̾�����
					UNICODE_STRING DiskVolDeviceName;
					//�����豸���ƻ�����
					WCHAR DiskVolDeviceNameBuffer[MAX_DEVNAME_LENGTH];
					//--------------driverlib.c�ļ���ʼ���������ˣ�������driver.c��������
					
					//�ļ����²�����Ӧ�Ĵ��̾�����豸
					PDEVICE_OBJECT DiskVolFilter;
				} DE_FSVOL;
				
				//DO_DISKVOL���̾�����豸��չ
				struct _DE_DISKVOL {
					//�ļ�ϵͳ��������Ƿ��ʼ�������������ڳ�ʼ��
					//�Ƿ��Ѿ���ʼ����
					//���ļ�ϵͳ�ɹ��Ĺ��ص��˴��̾��豸ʱ
					//�ʹ����ʼ���ɹ�
					//ע�⣺��ʱ����ļ�ϵͳ��ж���ˣ�����IsProtect=TRUE������̾������κε�д����
					DV_INITIALIZE_STATE InitializeStatus;
					//�Ƿ񱣻��˴���
					BOOLEAN IsProtect;
					//��д�Ƿ�ɹ�������д������޷��������
					BOOLEAN IsRWError;
					//�Ƿ�Ҫ����Ӱ��ϵͳ�����޸ĵ�����
					BOOLEAN IsSaveShadowData;
					//���ڱ���Ӱ��ϵͳ�ļ����¼���֪ͨ�¼���
					//  �����д�����Ѿ��������
					KEVENT SaveShadowDataFinishEvent;
					//���̾�����豸�Զ�Ӧ���ϲ��ļ�������豸
					//���ļ�ϵͳ��ж��ʱ���˶����ÿ�(NULL)
					PDEVICE_OBJECT FSVolFilter;

					//���ļ�ϵͳ�ϻ�ȡ������
					//���ļ�ϵͳ��ж��ʱ���˶�����Ȼ����
					//����ļ�ϵͳ��ж�غ��ֱ����أ���IsProtect&IsInitialized=TRUEʱ��������FsSetting�򣬷������
					struct _FS_SETTING {
						ULONG BytesPerSector;//ÿ�������������ֽ���
						ULONG SectorsPerAllocationUnit;//���䵥Ԫ�а�����������
						ULONG BytesPerAllocationUnit;//���䵥Ԫ�а������ֽ�����ΪBytesPerSector*SectorsPerAllocationUnit
						ULONGLONG TotalAllocationUnits;//�ܹ��ж��ٷ��䵥Ԫ
						ULONGLONG AvailableAllocationUnits;//���ж��ٷ��䵥Ԫ��ʵ�ʻ��ж��ٷ���ռ�
						ULONGLONG DataClusterOffset;//���ݴص�ƫ����
						//�����λͼ�����Դ�/����ռ�Ϊ��λ�����������Ϊ��λ����500G��Ӳ���п��ܻ����ĵ�128M���ҵ��ڴ棡
						//��ʹ�ô�Ϊ��λ��������ĵ�16MB���ڴ档
						LCXL_BITMAP BitmapUsed;//�Ѿ������λͼ
						LCXL_BITMAP BitmapRedirect;//�ض����λͼ�����ض������λͼΪ1
						LCXL_BITMAP BitmapDirect;//ֱ�ӷŹ���д��λͼ��ֱ�ӷŹ���λͼΪ1
						//�ص��ض����
						RTL_GENERIC_TABLE RedirectTable;
						// �ϴ�ɨ�赽�Ŀ��дص�λ�ã�Ĭ��Ϊ0������0bit��ʼɨ�裬��ΪFAT�ļ�ϵͳ���������ǴӴ��̾���ʼλ�ÿ�ʼ���㣬�����Ҫ����ע��
						ULONGLONG LastScanIndex;
					} FsSetting;//һЩ�ļ�ϵͳ������
					struct _REDIRECT_MEM{
						//�ڴ��ض����б�ͷ
						REDIRECT_RW_MEM RedirectListHead;
					} RedirectMem;//���ڴ����ض����д
					
					//�߳̾��
					HANDLE ThreadHandle;
					//�̶߳���
					PETHREAD ThreadObject;
					//�߳��¼�
					//  ���ж�д�������߳���ֹ�Լ�һϵ����Ҫ�̹߳����Ĳ���ʱ��������¼�
					KEVENT RequestEvent;
					//�Ƿ���ֹ�߳�
					BOOLEAN ToTerminateThread;
					//������ϵı���ϵͳʹ�õĶ�д�������
					LIST_ENTRY RWListHead;
					//Ӧ�ó����������
					APP_REQUEST_LIST_ENTRY AppRequsetListHead;
					//������ϵı���ϵͳʹ�õ�������е���
					KSPIN_LOCK RequestListLock;

					//����̹����豸����
					PDEVICE_OBJECT DiskFilterDO[64];//�����²�Ĵ��̹����豸
					ULONG DiskFilterNum;//�����²�Ĵ��̾�����豸����
					//BOOLEAN IsPartitionVoluem;//�Ƿ��ǻ������̵ķ�����
				} DE_DISKVOL;
				
				//DO_DISK���̹����豸��չ
				struct _DE_DISK {
					DISK_INITIALIZE_STATE InitializeStatus;
					ULONG DiskNumber;//�������
					BOOLEAN IsVolumeChanged;//���̾��Ѿ��ı䣬ͨ�����ܱ����Ĵ��̾�ɾ��ʱ�Ὣ��ֵ��ΪTRUE�������̾�����ʱ�������Ĵ��̾�������Ϊ�Ǵ��̾�
					STORAGE_BUS_TYPE BusType;//��������

					//�߳̾��
					HANDLE ThreadHandle;
					//�̶߳���
					PETHREAD ThreadObject;
					//�߳��¼�
					//  ���ж�д�������߳���ֹ�Լ�һϵ����Ҫ�̹߳����Ĳ���ʱ��������¼�
					KEVENT RequestEvent;
					//�Ƿ���ֹ�߳�
					BOOLEAN ToTerminateThread;
					//������ϵı���ϵͳʹ�õĶ�д�������
					LIST_ENTRY RWListHead;
					//��������ϵı���ϵͳʹ�õ�������е���
					KSPIN_LOCK RequestListLock;

					//����̾�����豸����
					DISK_VOL_FILTER_EXTENT DiskVolFilterDO[64];//�����ϲ�Ĵ��̾�����豸
					ULONG DiskVolFilterNum;//�����ϲ�Ĵ��̾�����豸����

					//�ڴ��ض����б�ͷ
					REDIRECT_RW_MEM RedirectListHead;
					DISK_GEOMETRY DiskGeometry;//��ѯ���̵�����������Ϣ
				} DE_DISK;
				
			};
			
		} DE_FILTER;
		//DO_CONTROL �����豸��չ
		struct _DE_CONTROL {
			//ע���ص�����Cookie
			LARGE_INTEGER  RegCookie;
		} DE_CONTROL;
	};
} EP_DEVICE_EXTENSION, *PEP_DEVICE_EXTENSION;//�豸��չ

//���к����Ķ�����driver.c��

//�豸��ͬ����
NTSTATUS LCXLDriverEntry(
	IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING RegistryPath
	);

//����ж������
VOID LCXLDriverUnload(
	IN PDRIVER_OBJECT DriverObject
	);

//���̾�����豸
VOID VPostAttachDiskVolDevice(
	IN PDEVICE_OBJECT FilterDeviceObject//�ļ�ϵͳ�����豸 
	);

//���̾��ȡ����
NTSTATUS VDeviceRead(
	IN PDEVICE_OBJECT DeviceObject, 
	IN PIRP Irp
	);
NTSTATUS VDeviceWrite(
	IN PDEVICE_OBJECT DeviceObject, 
	IN PIRP Irp
	);

NTSTATUS VDevicePnp(
	IN PDEVICE_OBJECT DeviceObject, 
	IN PIRP Irp
	);

NTSTATUS VDevicePower(
	IN PDEVICE_OBJECT DeviceObject, 
	IN PIRP Irp
	);

NTSTATUS VDeviceControl(
	IN PDEVICE_OBJECT DeviceObject, 
	IN PIRP Irp
	);

//���̹����豸

VOID DPostAttachDiskDevice(
	IN PDEVICE_OBJECT FilterDeviceObject//�ļ�ϵͳ�����豸 
	);

NTSTATUS DDeviceRead(
	IN PDEVICE_OBJECT DeviceObject, 
	IN PIRP Irp
	);
NTSTATUS DDeviceWrite(
	IN PDEVICE_OBJECT DeviceObject, 
	IN PIRP Irp
	);

NTSTATUS DDevicePnp(
	IN PDEVICE_OBJECT DeviceObject, 
	IN PIRP Irp
	);

NTSTATUS DDeviceControl(
	IN PDEVICE_OBJECT DeviceObject, 
	IN PIRP Irp
	);

//�ļ������豸
NTSTATUS FDeviceCreate(
	IN PDEVICE_OBJECT DeviceObject, 
	IN PIRP Irp
	);

NTSTATUS FDeviceCleanup(
	IN PDEVICE_OBJECT DeviceObject, 
	IN PIRP Irp
	);
NTSTATUS FDeviceClose(
	IN PDEVICE_OBJECT DeviceObject, 
	IN PIRP Irp
	);

//�Ƿ���Ҫ����������豸
//�������False���򲻹��ش��豸
//ע�⣺�п��ܻ�����ѹ��ص��豸����������أ���Ҫ���б��
BOOL FPreAttachVolumeDevice(
	IN PDEVICE_OBJECT VolumeDeviceObject,//���豸����
	IN PDEVICE_OBJECT DiskDeviceObject,//�����豸����
	IN PUNICODE_STRING DiskDeviceName///�����豸����
	);
//���سɹ�ʱ�����ô˺���
VOID FPostAttachVolumeDevice(
	IN PDEVICE_OBJECT FilterDeviceObject//�ļ�ϵͳ�����豸
	);

//ж�ؾ��豸
VOID FPostDetachVolumeDevice(
	IN PDEVICE_OBJECT DeviceObject,//�ҵ��豸
	IN PDEVICE_OBJECT VolumeDeviceObject//�豸��
	);

//�����豸
NTSTATUS CDeviceCreate(
	IN PDEVICE_OBJECT pDevObj, 
	IN PIRP pIrp
	);

NTSTATUS CDeviceControl(
	IN PDEVICE_OBJECT pDevObj, 
	IN PIRP pIrp
	);

NTSTATUS CDeviceClose(
	IN PDEVICE_OBJECT pDevObj, 
	IN PIRP pIrp
	);

NTSTATUS CShutdown(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
	);

#endif