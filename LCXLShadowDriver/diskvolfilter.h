#ifndef _DISK_VOL_FILTER_H_
#define _DISK_VOL_FILTER_H_

//////////////////////////////////////////////////////////////////////////
//���̾�����豸ͷ�ļ������Ŵ��̹����豸�йصĺ���
#ifndef _DRIVER_H_
#include "driver.h"
#endif



/*
//IRP�б�
typedef struct _IRP_LIST_ENTRY {
	
}IRP_LIST_ENTRY, *PIRP_LIST_ENTRY;
*/
//���ڴ��ض��������д�뵽������
VOID VWriteMemRedirectDataToDisk(
	IN OUT PEP_DEVICE_EXTENSION voldevext,
	IN OUT PRW_BUFFER_REC RWBuffer,
	IN PVOID ClusterBuffer,
	IN REDIRECT_MODE RedirectMode//�ض���ģʽ
	);

//IRP��д������
NTSTATUS VReadWriteIrpDispose(
	IN OUT PEP_DEVICE_EXTENSION voldevext,
	ULONG MajorFunction,
	PVOID buffer,
	ULONG length,
	ULONGLONG offset,
	IN OUT PRW_BUFFER_REC RWBuffer,
	IN REDIRECT_MODE RedirectMode,//�ض���ģʽ
	IN PVOID ClusterBuffer//һ���ص��ڴ����ݿ⣬��RedirectInMem=FALSEʱ��ClusterBuffer���벻Ϊ��
	);

//����Ӱ��ϵͳ���ļ���������
NTSTATUS VSaveShadowData(PEP_DEVICE_EXTENSION voldevext);


//���̾��д�߳�
VOID VReadWriteThread (
	IN PVOID Context//�߳������ģ������Ǵ��̾������������
	);
//�����д����
NTSTATUS VHandleReadWrite(
	PEP_DEVICE_EXTENSION VolDevExt,
	IN ULONG MajorFunction,//�����ܺ�
	IN ULONGLONG ByteOffset,//ƫ����
	IN OUT PVOID Buffer,//Ҫ��ȡ/д��Ļ��������������Ǵض����
	IN ULONG Length//����������
	);

//Ӳ��û�пռ�
#define GET_CLUSTER_RESULT_NO_SPACE 0xFFFFFFFFFFFFFFFF
//���б�����Χ
#define GET_CLUSTER_RESULT_OUT_OF_BOUND 0xFFFFFFFFFFFFFFFE
//************************************
// Method:    VGetRealClusterForRead
// FullName:  VGetRealClusterForRead
// Access:    public 
// Returns:   ULONGLONG ����� GET_CLUSTER_RESULT_DENY����ʾ���������޷���ȡ
// Qualifier: ��ȡҪ��ȡд�����ʵ��
// Parameter: PEP_DEVICE_EXTENSION VolDevExt
// Parameter: ULONGLONG ClusterIndex
// Parameter: BOOLEAN IsReadOpt �Ƿ��Ƕ�����
// Parameter: BOOLEAN CanAccessProtectBitmap �Ƿ���Զ�ȡ��������λͼ
//************************************
ULONGLONG VGetRealCluster(
	PEP_DEVICE_EXTENSION VolDevExt, 
	ULONGLONG ClusterIndex, 
	BOOLEAN IsReadOpt
	);

//ֱ�Ӷ�д���̾�
NTSTATUS VDirectReadWriteDiskVolume(
	IN PDEVICE_OBJECT DiskVolumeDO,//���̾��豸���� 
	IN ULONG MajorFunction,//�����ܺ�
	IN ULONGLONG ByteOffset,//ƫ����
	IN OUT PVOID Buffer,//Ҫ��ȡ/д��Ļ�����
	IN ULONG Length,//����������
	OUT PIO_STATUS_BLOCK iostatus,
	IN BOOLEAN Wait//�Ƿ�ȴ�
	);


//////////////////////////////////////////////////////////////////////////
//˵�����Ƚ�����
//����ֵ�����غ���ִ�н��
//����˵����
//	Table��λͼ��
//	FirstStruct����һ���صĽṹָ��
//	SecondStruct���ڶ����صĽṹָ��
//////////////////////////////////////////////////////////////////////////
RTL_GENERIC_COMPARE_RESULTS NTAPI VCompareRoutine(
	PRTL_GENERIC_TABLE Table,
	PVOID FirstStruct,
	PVOID SecondStruct
	);

//////////////////////////////////////////////////////////////////////////
//˵���������ڴ����̣����ں˲��еĺ���һ�㱻��Ϊ���̣�
//����ֵ������������ڴ�ָ��
//����˵����
//	Table���ڴ��
//	ByteSize���ڴ������С��
//////////////////////////////////////////////////////////////////////////
//LCXL�޸ģ���LONG ByteSize�޸�ΪCLONG ByteSize
PVOID NTAPI VAllocateRoutine (
	PRTL_GENERIC_TABLE Table,
	CLONG ByteSize
	);
//�ͷ�����
VOID NTAPI VFreeRoutine (
	PRTL_GENERIC_TABLE Table,
	PVOID Buffer
	);
//�������̾��߳�
NTSTATUS VCreateVolumeThread(
	IN PDEVICE_OBJECT DiskVolFilterDO
	);
//�رմ��̾��߳�
NTSTATUS VCloseVolumeThread(
	IN PDEVICE_OBJECT DiskVolFilterDO
	);
//���̾�����豸ж�ع����¼�
VOID VPostDetachDiskVolDevice(
	IN PDEVICE_OBJECT FilterDeviceObject//�ļ�ϵͳ�����豸 
	);

//ʹ���̺ʹ��̾�����豸�໥����
NTSTATUS VAssociateDiskFilter(
	IN PDEVICE_OBJECT DiskVolFilterDO, 
	IN LARGE_INTEGER StartingOffset,
	IN LARGE_INTEGER ExtentLength,
	IN PDEVICE_OBJECT DiskFilterDO
	);

//ȡ���˴��̾�����豸����̹����豸�����й���
NTSTATUS VDisassociateDiskFilter(
	IN PDEVICE_OBJECT DiskVolFilterDO
	);


//��ȡ���̾��ϲ���ļ�ϵͳ����Ϣ
NTSTATUS VGetFSInformation(
	IN PDEVICE_OBJECT DiskVolDO
	);

NTSTATUS VPostProtectedFSInfo(IN PEP_DEVICE_EXTENSION VolDevExt);

//��ȡ�������õ��߳�
VOID VGetDriverSettingThread(
	IN PVOID Context//�߳������ģ��������ļ�ϵͳ������������
	);

VOID VGetFSInfoThread(
	IN PVOID Context
	);

#endif