#ifndef _DISK_FILTER_H_
#define _DISK_FILTER_H_
//////////////////////////////////////////////////////////////////////////
//���̹����豸ͷ�ļ������Ŵ��̹����豸�йصĺ���
#ifndef _DRIVER_H_
#include "driver.h"
#endif

//���������߳�
NTSTATUS DCreateDiskThread(
	IN PDEVICE_OBJECT DiskFilterDO
	);
//�رմ����߳�
NTSTATUS DCloseDiskThread(
	IN PDEVICE_OBJECT DiskFilterDO
	);

VOID DReadWriteThread(
	IN PVOID Context//�߳������ģ������Ǵ��̾������������
	);

NTSTATUS DHookDeviceWrite(
	IN PDEVICE_OBJECT DeviceObject, 
	IN PIRP Irp
	);

#endif