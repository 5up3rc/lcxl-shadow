#ifndef _FILE_SYS_FILTER_H_
#define _FILE_SYS_FILTER_H_

#ifndef _DRIVER_H_
#include "driver.h"
#endif

//��ȡ��һ����������ƫ����
NTSTATUS FGetFirstDataSectorOffset(
	IN HANDLE FSDeviceHandle, 
	OUT PULONGLONG FirstDataSector
	);

//����ֱ�Ӷ�д���ļ�
NTSTATUS FSetDirectRWFile(
	IN PEP_DEVICE_EXTENSION fsdevext, 
	IN PUNICODE_STRING FilePath,//�ļ�·����������������
	IN ULONGLONG DataClusterOffset,//��������ƫ����
	OUT PLCXL_BITMAP BitmapDirect
	);

#endif