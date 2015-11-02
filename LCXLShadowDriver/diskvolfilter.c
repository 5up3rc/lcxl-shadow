#include "winkernel.h"
#include "driver.h"
#include "diskvolfilter.h"
#include "filesysfilter.h"
//���̾�����豸


NTSTATUS VCreateVolumeThread(
	IN PDEVICE_OBJECT DiskVolFilterDO
	)
{
	PEP_DEVICE_EXTENSION devext;
	NTSTATUS status;

	PAGED_CODE();
	devext = (PEP_DEVICE_EXTENSION)DiskVolFilterDO->DeviceExtension;
	//�ر��̱߳�ʾΪFALSE
	devext->DE_FILTER.DE_DISKVOL.ToTerminateThread = FALSE;
	//��д����Ĭ��ΪFALSE��
	devext->DE_FILTER.DE_DISKVOL.IsRWError = FALSE;
	//��ʼ�����������������
	InitializeListHead(&devext->DE_FILTER.DE_DISKVOL.RWListHead);
	//��ʼ��Ӧ�ó����������
	InitializeListHead(&devext->DE_FILTER.DE_DISKVOL.AppRequsetListHead.ListEntry);
	//��ʼ���ڴ��ض����б�
	InitializeListHead(&devext->DE_FILTER.DE_DISKVOL.RedirectMem.RedirectListHead.ListEntry);
	//��ʼ����������е���
	KeInitializeSpinLock(&devext->DE_FILTER.DE_DISKVOL.RequestListLock);
	//��ʼ����д�����¼���ͬ���¼���
	KeInitializeEvent(&devext->DE_FILTER.DE_DISKVOL.RequestEvent, SynchronizationEvent, FALSE);
	//��ʼ��Ӱ�ӱ����¼���֪ͨ�¼���
	KeInitializeEvent(&devext->DE_FILTER.DE_DISKVOL.SaveShadowDataFinishEvent, NotificationEvent, FALSE);
	//���ʼ����־
	devext->DE_FILTER.DE_DISKVOL.InitializeStatus = DVIS_NONE;
	/*
	//�鿴��ʱ����������û�г�ʼ�����
	if (g_DriverSetting.InitState =  DS_INITIALIZED) {
		//�������Ҫ����
		if (!LCXLVolumeNeedProtect(DiskVolFilterDO)) {
			//ֱ���趨��ʼ�����
			devext->DE_FILTER.DE_DISKVOL.InitializeStatus = DVIS_INITIALIZED;
			devext->DE_FILTER.DE_DISKVOL.IsProtect = FALSE;
		}
	}
	*/
	//����ϵͳ�̣߳������Ĵ�����̾�����豸���豸����
	devext->DE_FILTER.DE_DISKVOL.ThreadHandle = NULL;
	status = PsCreateSystemThread(&devext->DE_FILTER.DE_DISKVOL.ThreadHandle, 
		(ACCESS_MASK)0L,
		NULL,
		NULL,
		NULL,
		VReadWriteThread, 
		DiskVolFilterDO);

	if (NT_SUCCESS(status)) {
		//��ȡ�߳̾��
		status = ObReferenceObjectByHandle(devext->DE_FILTER.DE_DISKVOL.ThreadHandle,
			0,
			*PsThreadType,
			KernelMode,
			(PVOID *)&devext->DE_FILTER.DE_DISKVOL.ThreadObject,
			NULL
			);
		if (!NT_SUCCESS(status)) {
			//���ɹ���
			devext->DE_FILTER.DE_DISKVOL.ToTerminateThread = TRUE;
			//�����߳�
			KeSetEvent(&devext->DE_FILTER.DE_DISKVOL.RequestEvent, 0, TRUE);
			ASSERT(0);
		}
		KdPrint(("SYS:VCreateVolumeThread:PsCreateSystemThread succeed.(handle = 0x%p)\n", devext->DE_FILTER.DE_DISKVOL.ThreadHandle));
	} else {
		KdPrint(("SYS:VCreateVolumeThread:PsCreateSystemThread failed.(0x%08x)\n", status));
	}
	return status;
}

NTSTATUS VCloseVolumeThread(
	IN PDEVICE_OBJECT DiskVolFilterDO
	)
{
	NTSTATUS status;
	PEP_DEVICE_EXTENSION devext;

	devext = (PEP_DEVICE_EXTENSION)DiskVolFilterDO->DeviceExtension;
	if (devext->DE_FILTER.DE_DISKVOL.ThreadHandle!=NULL) {
		//������־��ΪTRUE
		devext->DE_FILTER.DE_DISKVOL.ToTerminateThread = TRUE;
		//�����߳�
		KeSetEvent(&devext->DE_FILTER.DE_DISKVOL.RequestEvent, 0, TRUE);
		
		//�ȴ��߳��˳�
		status = KeWaitForSingleObject(devext->DE_FILTER.DE_DISKVOL.ThreadObject, UserRequest, KernelMode, FALSE, NULL);
		if (!NT_SUCCESS(status)) {
			KdPrint(("SYS:VPostDetachDiskVolDevice:KeWaitForSingleObject failed(0x%08x).\n", status));
		}
		//��������
		ObDereferenceObject(devext->DE_FILTER.DE_DISKVOL.ThreadObject);
		//�رվ��
		ZwClose(devext->DE_FILTER.DE_DISKVOL.ThreadHandle);
		devext->DE_FILTER.DE_DISKVOL.ThreadHandle = NULL;
	}
	return STATUS_SUCCESS;
}

//��ȡ���̾��ϲ���ļ�ϵͳ����Ϣ
NTSTATUS VGetFSInformation(
	IN PDEVICE_OBJECT DiskVolFilterDO
	)
{
	NTSTATUS status;
	//��ȡ����Ϣ
	HANDLE hFile;
	IO_STATUS_BLOCK ioStatus;
	OBJECT_ATTRIBUTES oa;

	PEP_DEVICE_EXTENSION voldevext;
	PEP_DEVICE_EXTENSION fsdevext;
	PDEVICE_OBJECT FileSyeFilterDO;//�ļ�ϵͳ���˺���

	PAGED_CODE();
	
	voldevext = (PEP_DEVICE_EXTENSION)DiskVolFilterDO->DeviceExtension;
	//������û�г�ʼ������
	ASSERT(voldevext->DE_FILTER.DE_DISKVOL.InitializeStatus==DVIS_NONE);
	FileSyeFilterDO = voldevext->DE_FILTER.DE_DISKVOL.FSVolFilter;
	ASSERT(FileSyeFilterDO!=NULL);
	fsdevext = (PEP_DEVICE_EXTENSION)FileSyeFilterDO->DeviceExtension;

	//���ñ����ʼ�����
	ASSERT(g_DriverSetting.InitState==DS_INITIALIZED);
	if (g_DriverSetting.InitState!=DS_INITIALIZED) {
		//����������û�г�ʼ��
		return STATUS_FLT_NOT_INITIALIZED;
	}
	//��ȡ�˴����Ƿ���Ҫ����
	voldevext->DE_FILTER.DE_DISKVOL.IsProtect = LCXLVolumeNeedProtect(FileSyeFilterDO);
	//�������Ҫ������ֱ�ӷ��سɹ�
	if (!voldevext->DE_FILTER.DE_DISKVOL.IsProtect) {
		voldevext->DE_FILTER.DE_DISKVOL.InitializeStatus = DVIS_INITIALIZED;
		KeSetEvent(&voldevext->DE_FILTER.DE_DISKVOL.RequestEvent, 0, FALSE);
		return STATUS_SUCCESS;
	}
	//�����Ҫ����������ʾ�ĳɡ����ڳ�ʼ����
	voldevext->DE_FILTER.DE_DISKVOL.InitializeStatus = DVIS_INITIALIZING;
	KdPrint(("SYS:VGetFSInformation: Protect this volume(%wZ)\n", &voldevext->DE_FILTER.PhysicalDeviceName));

	InitializeObjectAttributes(&oa, &voldevext->DE_FILTER.PhysicalDeviceName, OBJ_CASE_INSENSITIVE|OBJ_KERNEL_HANDLE, NULL, NULL);
	//��������
	status = IoCreateFileSpecifyDeviceObjectHint(
		&hFile, 
		GENERIC_READ | SYNCHRONIZE,
		&oa, 
		&ioStatus, 
		NULL, 
		FILE_ATTRIBUTE_NORMAL, 
		FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, 
		FILE_OPEN, 
		FILE_SYNCHRONOUS_IO_NONALERT, 
		NULL, 
		0, 
		CreateFileTypeNone, 
		NULL, 
		0,  
		fsdevext->DE_FILTER.PhysicalDeviceObject
		);
	if (NT_SUCCESS(status)) {
		FILE_FS_SIZE_INFORMATION fssizeInfo;
		IO_STATUS_BLOCK IoStatusBlock;
		//�ͷ�λͼ
		LCXLBitmapFina(&voldevext->DE_FILTER.DE_DISKVOL.FsSetting.BitmapRedirect);
		LCXLBitmapFina(&voldevext->DE_FILTER.DE_DISKVOL.FsSetting.BitmapDirect);
		LCXLBitmapFina(&voldevext->DE_FILTER.DE_DISKVOL.FsSetting.BitmapUsed);
		//�Ȼ�ȡ��(���䵥Ԫ)��Ϣ//��ȡ���̾���Ϣ
		status = ZwQueryVolumeInformationFile(hFile,
			&IoStatusBlock,
			&fssizeInfo,
			sizeof(fssizeInfo),
			FileFsSizeInformation);
		if (NT_SUCCESS(status)) {
			STARTING_LCN_INPUT_BUFFER StartingLCN;
			ULONG	BitmapSize;
			PVOLUME_BITMAP_BUFFER BitmapInUse;

			KdPrint(("SYS:VGetFSInformation:fssizeInfo.TotalAllocationUnits=%I64d\n", fssizeInfo.TotalAllocationUnits.QuadPart));
			//��һ���дص�Ѱ��λ��Ĭ��Ϊ0
			voldevext->DE_FILTER.DE_DISKVOL.FsSetting.LastScanIndex = 0;
			// ��ʼ���ض����
			RtlInitializeGenericTable(&voldevext->DE_FILTER.DE_DISKVOL.FsSetting.RedirectTable, VCompareRoutine, VAllocateRoutine, VFreeRoutine, NULL);
			

			KdPrint(("SYS:VGetFSInformation:fssizeInfo.AvailableAllocationUnits=%I64u\n", fssizeInfo.AvailableAllocationUnits.QuadPart));
			KdPrint(("SYS:VGetFSInformation:fssizeInfo.BytesPerSector=%u\n", fssizeInfo.BytesPerSector));
			KdPrint(("SYS:VGetFSInformation:fssizeInfo.SectorsPerAllocationUnit=%u\n", fssizeInfo.SectorsPerAllocationUnit));
			//��ȡ���������䵥Ԫ����Ϣ

			voldevext->DE_FILTER.DE_DISKVOL.FsSetting.BytesPerSector = fssizeInfo.BytesPerSector;
			voldevext->DE_FILTER.DE_DISKVOL.FsSetting.SectorsPerAllocationUnit = fssizeInfo.SectorsPerAllocationUnit;
			voldevext->DE_FILTER.DE_DISKVOL.FsSetting.TotalAllocationUnits = (ULONGLONG)fssizeInfo.TotalAllocationUnits.QuadPart;
			voldevext->DE_FILTER.DE_DISKVOL.FsSetting.AvailableAllocationUnits = (ULONGLONG)fssizeInfo.AvailableAllocationUnits.QuadPart;
			voldevext->DE_FILTER.DE_DISKVOL.FsSetting.BytesPerAllocationUnit = fssizeInfo.BytesPerSector*fssizeInfo.SectorsPerAllocationUnit;

			//�����Ĵ�λͼ��С
			BitmapSize = (ULONG)(fssizeInfo.TotalAllocationUnits.QuadPart/8+sizeof(VOLUME_BITMAP_BUFFER));
			KdPrint(("SYS:VGetFSInformation:BitmapSize = %d\n", BitmapSize));
			StartingLCN.StartingLcn.QuadPart = 0;
			//��ȡ��λͼ
			do  {
				BitmapInUse = (PVOLUME_BITMAP_BUFFER)ExAllocatePoolWithTag(PagedPool, BitmapSize, 'BMPS');
				// �ڴ治����
				if (BitmapInUse==NULL) {
					//LCXL�����ش�����Ϣ
					status = STATUS_INSUFFICIENT_RESOURCES;
					break;
				}
				//LCXL����þ�λʾͼ
				status = ZwFsControlFile( hFile, 
					NULL, 
					NULL, 
					NULL, 
					&IoStatusBlock, 
					FSCTL_GET_VOLUME_BITMAP, 
					&StartingLCN,
					sizeof (StartingLCN),
					BitmapInUse, 
					BitmapSize
					);
				//LCXL���������Ļ�����װ����λʾͼ
				if (STATUS_BUFFER_OVERFLOW == status) {
					//������Ӧ���ǲ������STATUS_BUFFER_OVERFLOW�����
					ASSERT(0);
					//LCXL���ͷŻ������������������Ļ�����
					ExFreePoolWithTag(BitmapInUse, 'BMPS');
					//8192
					BitmapSize += 1<<13;
					KdPrint(("SYS:VGetFSInformation:ZwFsControlFile:BitmapSize is too small,realloc memory(%u)\n", BitmapSize));
				}
			} while(STATUS_BUFFER_OVERFLOW == status);
			//�ɹ���
			if (NT_SUCCESS(status)) {
				KdPrint(("SYS:VGetFSInformation:BitmapInUse=%I64d, IoStatusBlock.Information=%d\n", BitmapInUse->BitmapSize.QuadPart, IoStatusBlock.Information));
				//��ȡ����������ƫ����
				status = FGetFirstDataSectorOffset(hFile, &voldevext->DE_FILTER.DE_DISKVOL.FsSetting.DataClusterOffset);
				if (NT_SUCCESS(status)) {
					KdPrint(("SYS:VGetFSInformation:FGetFirstDataSectorOffset=%I64d\n", voldevext->DE_FILTER.DE_DISKVOL.FsSetting.DataClusterOffset));
					//��ȡ���ݴص�ƫ����
					voldevext->DE_FILTER.DE_DISKVOL.FsSetting.DataClusterOffset = 
						voldevext->DE_FILTER.DE_DISKVOL.FsSetting.DataClusterOffset/fssizeInfo.SectorsPerAllocationUnit
						+(voldevext->DE_FILTER.DE_DISKVOL.FsSetting.DataClusterOffset%fssizeInfo.SectorsPerAllocationUnit>0);
					//ͨ��Bitmap����LCXLλͼ
					LCXLBitmapCreateFromBitmap(BitmapInUse->Buffer, (ULONGLONG)BitmapInUse->BitmapSize.QuadPart, voldevext->DE_FILTER.DE_DISKVOL.FsSetting.DataClusterOffset, 2<<20, &voldevext->DE_FILTER.DE_DISKVOL.FsSetting.BitmapUsed);
					//���ݴ�֮ǰ�Ķ���־Ϊ��ʹ��
					LCXLBitmapSet(&voldevext->DE_FILTER.DE_DISKVOL.FsSetting.BitmapUsed, 0, voldevext->DE_FILTER.DE_DISKVOL.FsSetting.DataClusterOffset, TRUE);
					//�����Ѿ�ɨ�赽���ݴص�λ��
					voldevext->DE_FILTER.DE_DISKVOL.FsSetting.LastScanIndex = voldevext->DE_FILTER.DE_DISKVOL.FsSetting.DataClusterOffset;
					//��ʼ��Ĭ�ϵ��ض���λͼ����Ϊ2M
					LCXLBitmapInit(&voldevext->DE_FILTER.DE_DISKVOL.FsSetting.BitmapRedirect, voldevext->DE_FILTER.DE_DISKVOL.FsSetting.BitmapUsed.BitmapSize, 2<<20);
					//��ʼ��Ĭ�ϵ�ֱ�Ӷ�дλͼ����Ϊ2M
					LCXLBitmapInit(&voldevext->DE_FILTER.DE_DISKVOL.FsSetting.BitmapDirect, voldevext->DE_FILTER.DE_DISKVOL.FsSetting.BitmapUsed.BitmapSize, 2<<20);
					
					//�����ϵͳ��
					if (g_DriverSetting.SettingDO== FileSyeFilterDO) {
						UNICODE_STRING FilePath;

						RtlInitUnicodeString(&FilePath, L"\\Windows\\bootstat.dat");
						//������ֱ�Ӷ�д���ļ�
						FSetDirectRWFile(
							fsdevext, 
							&FilePath,
							voldevext->DE_FILTER.DE_DISKVOL.FsSetting.DataClusterOffset,
							&voldevext->DE_FILTER.DE_DISKVOL.FsSetting.BitmapDirect
							);
					}
				}
				ExFreePool(BitmapInUse);
			} else {
				KdPrint(("SYS:VGetFSInformation:ZwFsControlFile:Fail!0x08x\n", status));
			}
		}//ZwQueryVolumeInformationFile
		//�ر��ļ�
		ZwClose(hFile);
	}//IoCreateFileSpecifyDeviceObjectHint
	if (!NT_SUCCESS(status)){
		KdPrint(("SYS(%d):VGetFSInformation (%wZ) is Fail!0x%08x\n", PsGetCurrentProcessId(), &voldevext->DE_FILTER.PhysicalDeviceName, status));
		voldevext->DE_FILTER.DE_DISKVOL.IsProtect = FALSE;
	} else {
		ULONG i;
		//�������ڴ���Ϊ�ض��򱣻�ģʽ
		for (i = 0; i < voldevext->DE_FILTER.DE_DISKVOL.DiskFilterNum; i++) {
			PEP_DEVICE_EXTENSION diskdevext;

			diskdevext = (PEP_DEVICE_EXTENSION)voldevext->DE_FILTER.DE_DISKVOL.DiskFilterDO[i]->DeviceExtension;
			diskdevext->DE_FILTER.DE_DISK.InitializeStatus = DIS_MEMORY;
		}
	}
	voldevext->DE_FILTER.DE_DISKVOL.InitializeStatus = DVIS_INITIALIZED;
	//�����¼�,֪ͨ��д�߳̿��԰�
	KeSetEvent(&voldevext->DE_FILTER.DE_DISKVOL.RequestEvent, 0, FALSE);
	
	return status;
}

NTSTATUS VPostProtectedFSInfo(IN PEP_DEVICE_EXTENSION VolDevExt)
{
	NTSTATUS localstatus;

	PAGED_CODE();

	do {
		//�ȴ�ϵͳ��ʼ�����
		//LCXLChangeDriverIcon��Ҫ�ȴ�ϵͳ��ʼ����ɲ���ִ�гɹ��������Ҫ�ȴ�ϵͳ��ʼ������¼�
		KeWaitForSingleObject(&g_SysInitEvent, UserRequest, KernelMode, FALSE, NULL);
		//����ͼ��
		localstatus = LCXLChangeDriverIcon(VolDevExt->DE_FILTER.PhysicalDeviceObject);
		if (NT_SUCCESS(localstatus)) {
			KdPrint(("SYS:VGetFSInfoThread:Change driver(%wZ)icon succeed!\n", &VolDevExt->DE_FILTER.PhysicalDeviceName));
		} else {
			KdPrint(("SYS:VGetFSInfoThread:Change driver(%wZ)icon failed!(0x%08x)\n", &VolDevExt->DE_FILTER.PhysicalDeviceName, localstatus));
			//���������������Ǳ���ע���û�г�ʼ�����//Windows Vista��BootReinitilize֮��û�г�ʼ���˴���ע���
			if (localstatus == STATUS_OBJECT_NAME_NOT_FOUND) {
				LARGE_INTEGER sleeptime;
				//ע��������ֵ�޷����ʣ�����ע��������ʼ������
				KeClearEvent(&g_SysInitEvent);
				//�ȵȴ�3s����ȥ��ȡ����Ȼ�����������Windows XP�»�ѭ���ܶ��
				sleeptime.QuadPart = -30000000;//3s
				KeDelayExecutionThread(KernelMode, FALSE, &sleeptime);
				//����ע��
				IoRegisterDriverReinitialization(g_CDO->DriverObject, LCXLDriverReinitialize, NULL);
				KdPrint(("SYS:VGetFSInfoThread(%wZ):Register Boot Reinitilize roution again\n", &VolDevExt->DE_FILTER.PhysicalDeviceName));
			}
		}
	} while (localstatus == STATUS_OBJECT_NAME_NOT_FOUND);
	return localstatus;
}

VOID VGetDriverSettingThread(
	IN PVOID Context
	)
{
	NTSTATUS status;
	PDEVICE_OBJECT FileSyeFilterDO;
	
	PAGED_CODE();
	//��������̵߳����ȼ�Ϊ��ʵʱ��
	KeSetPriorityThread(KeGetCurrentThread(), HIGH_PRIORITY);
	FileSyeFilterDO = (PDEVICE_OBJECT)Context;

	status = LCXLGetDriverSetting(FileSyeFilterDO);
	if (NT_SUCCESS(status)) {
		
	} else {
		KdPrint(("SYS:VGetFSInformation:LCXLGetDriverSetting failed(0x%08x)\n", status));
	}
	//�����߳�
	PsTerminateSystemThread(status);
}

VOID VGetFSInfoThread(
	IN PVOID Context
	)
{
	NTSTATUS status;
	PDEVICE_OBJECT FileSyeFilterDO;
	PDEVICE_OBJECT DiskVolFilterDO;
	PEP_DEVICE_EXTENSION FsDevExt;
	PEP_DEVICE_EXTENSION VolDevExt;
	PAGED_CODE();
	//��������̵߳����ȼ�Ϊ��ʵʱ��
	KeSetPriorityThread(KeGetCurrentThread(), HIGH_PRIORITY);
	FileSyeFilterDO = (PDEVICE_OBJECT)Context;
	FsDevExt = (PEP_DEVICE_EXTENSION)FileSyeFilterDO->DeviceExtension;
	DiskVolFilterDO = FsDevExt->DE_FILTER.DE_FSVOL.DiskVolFilter;
	VolDevExt = (PEP_DEVICE_EXTENSION)DiskVolFilterDO->DeviceExtension;
	KdPrint(("SYS:VGetFSInfoThread(%wZ)\n", &VolDevExt->DE_FILTER.PhysicalDeviceName));
	//�ȴ����û�ȡ���
	KeWaitForSingleObject(&g_DriverSetting.SettingInitEvent, UserRequest, KernelMode, FALSE, NULL);
	
	//��ʼ��ȡ�ļ�ϵͳ����
	status = VGetFSInformation(DiskVolFilterDO);
	if (!NT_SUCCESS(status)) {
		KdPrint(("SYS:VGetFSInfoThread:VGetFSInformation(%wZ)failed.(0x%08x)\n", &VolDevExt->DE_FILTER.PhysicalDeviceName, status));
	}
	//�����Ҫ����
	if (VolDevExt->DE_FILTER.DE_DISKVOL.IsProtect) {
		VPostProtectedFSInfo(VolDevExt);
	}
	KdPrint(("SYS:VGetFSInfoThread(%wZ) ended\n", &VolDevExt->DE_FILTER.PhysicalDeviceName));
	//�����߳�
	PsTerminateSystemThread(status);
}

//���̾�����豸�����¼�
VOID VPostAttachDiskVolDevice(
	IN PDEVICE_OBJECT FilterDeviceObject//���̾�����豸 
	)
{
	PEP_DEVICE_EXTENSION devext;

	PAGED_CODE();
	devext = (PEP_DEVICE_EXTENSION)FilterDeviceObject->DeviceExtension;
	//����ϵͳ�߳�
	VCreateVolumeThread(FilterDeviceObject);
	KdPrint(("SYS:VPostAttachDiskVolDevice:FilterDeviceObject=%wZ\n", &devext->DE_FILTER.PhysicalDeviceName));
}
//���̾�����豸ж�ع����¼�
VOID VPostDetachDiskVolDevice(
	IN PDEVICE_OBJECT FilterDeviceObject//�ļ�ϵͳ�����豸 
	)
{
	PEP_DEVICE_EXTENSION devext;
	NTSTATUS status;

	PAGED_CODE();
	devext = (PEP_DEVICE_EXTENSION)FilterDeviceObject->DeviceExtension;
	KdPrint(("SYS:VPostDetachDiskVolDevice:FilterDeviceObject=%wZ\n", &devext->DE_FILTER.PhysicalDeviceName));
	
	status = VCloseVolumeThread(FilterDeviceObject);
	if (!NT_SUCCESS(status)) {
		KdPrint(("SYS:VPostDetachDiskVolDevice:VCloseVolumeThread failed(0x%08x)\n"));
	}
}

//ʹ���̺ʹ��̾�����豸�໥����
NTSTATUS VAssociateDiskFilter(
	IN PDEVICE_OBJECT DiskVolFilterDO, 
	IN LARGE_INTEGER StartingOffset,
	IN LARGE_INTEGER ExtentLength,
	IN PDEVICE_OBJECT DiskFilterDO
	)
{
	PEP_DEVICE_EXTENSION voldevext;
	PEP_DEVICE_EXTENSION diskdevext;
	NTSTATUS status;

	PAGED_CODE();

	voldevext = (PEP_DEVICE_EXTENSION)DiskVolFilterDO->DeviceExtension;
	diskdevext = (PEP_DEVICE_EXTENSION)DiskFilterDO->DeviceExtension;
	ASSERT(voldevext->DeviceType==DO_DISKVOL);
	ASSERT(diskdevext->DeviceType == DO_DISK);
	
	if (voldevext->DE_FILTER.DE_DISKVOL.DiskFilterNum<64 && diskdevext->DE_FILTER.DE_DISK.DiskVolFilterNum<64) {
		voldevext->DE_FILTER.DE_DISKVOL.DiskFilterDO[voldevext->DE_FILTER.DE_DISKVOL.DiskFilterNum] = DiskFilterDO;
		
		diskdevext->DE_FILTER.DE_DISK.DiskVolFilterDO[diskdevext->DE_FILTER.DE_DISK.DiskVolFilterNum].FilterDO = DiskVolFilterDO;
		diskdevext->DE_FILTER.DE_DISK.DiskVolFilterDO[diskdevext->DE_FILTER.DE_DISK.DiskVolFilterNum].StartingOffset = StartingOffset;
		diskdevext->DE_FILTER.DE_DISK.DiskVolFilterDO[diskdevext->DE_FILTER.DE_DISK.DiskVolFilterNum].ExtentLength = ExtentLength;

		voldevext->DE_FILTER.DE_DISKVOL.DiskFilterNum++;
		diskdevext->DE_FILTER.DE_DISK.DiskVolFilterNum++;
		status = STATUS_SUCCESS;
	} else {
		//���
		status = STATUS_BUFFER_OVERFLOW;
	}
	return status;
}

//ȡ���˴��̾�����豸����̹����豸�����й���
NTSTATUS VDisassociateDiskFilter(
	IN PDEVICE_OBJECT DiskVolFilterDO
	)
{
	PEP_DEVICE_EXTENSION voldevext;
	ULONG i;
	PAGED_CODE();

	voldevext = (PEP_DEVICE_EXTENSION)DiskVolFilterDO->DeviceExtension;
	ASSERT(voldevext->DeviceType==DO_DISKVOL);
	for (i = 0; i < voldevext->DE_FILTER.DE_DISKVOL.DiskFilterNum; i++) {
		PDEVICE_OBJECT DiskFilterDO;
		PEP_DEVICE_EXTENSION diskdevext;
		ULONG j;

		DiskFilterDO = voldevext->DE_FILTER.DE_DISKVOL.DiskFilterDO[i];
		diskdevext = (PEP_DEVICE_EXTENSION)DiskFilterDO->DeviceExtension;

		if (voldevext->DE_FILTER.DE_DISKVOL.IsProtect) {
			//֪ͨ�����豸�������ϵĴ��̾��Ѿ�����
			diskdevext->DE_FILTER.DE_DISK.IsVolumeChanged = TRUE;
		}
		
		for (j = 0; j <diskdevext->DE_FILTER.DE_DISK.DiskVolFilterNum; j++) {
			if (diskdevext->DE_FILTER.DE_DISK.DiskVolFilterDO[j].FilterDO == DiskVolFilterDO) {
				//�������
				RtlMoveMemory(&diskdevext->DE_FILTER.DE_DISK.DiskVolFilterDO[j], &diskdevext->DE_FILTER.DE_DISK.DiskVolFilterDO[j+1], sizeof(DISK_VOL_FILTER_EXTENT)*(diskdevext->DE_FILTER.DE_DISK.DiskVolFilterNum-j-1));
				diskdevext->DE_FILTER.DE_DISK.DiskVolFilterNum--;
				break;
			}
		}
	}
	voldevext->DE_FILTER.DE_DISKVOL.DiskFilterNum = 0;
	return STATUS_SUCCESS;
}

static PCHAR MAJOR_FUNCTION_STR[] = {
	"IRP_MJ_CREATE", "IRP_MJ_CREATE_NAMED_PIPE", "IRP_MJ_CLOSE", "IRP_MJ_READ",
	"IRP_MJ_WRITE", "IRP_MJ_QUERY_INFORMATION", "IRP_MJ_SET_INFORMATION", "IRP_MJ_QUERY_EA",
	"IRP_MJ_SET_EA", "IRP_MJ_FLUSH_BUFFERS", "IRP_MJ_QUERY_VOLUME_INFORMATION", "IRP_MJ_SET_VOLUME_INFORMATION",
	"IRP_MJ_DIRECTORY_CONTROL", "IRP_MJ_FILE_SYSTEM_CONTROL", "IRP_MJ_DEVICE_CONTROL", "IRP_MJ_INTERNAL_DEVICE_CONTROL",
	"IRP_MJ_SHUTDOWN", "IRP_MJ_LOCK_CONTROL", "IRP_MJ_CLEANUP", "IRP_MJ_CREATE_MAILSLOT", 
	"IRP_MJ_QUERY_SECURITY", "IRP_MJ_SET_SECURITY", "IRP_MJ_POWER", "IRP_MJ_SYSTEM_CONTROL",
	"IRP_MJ_DEVICE_CHANGE", "IRP_MJ_QUERY_QUOTA", "IRP_MJ_SET_QUOTA", "IRP_MJ_PNP",
};

//inlineһ��
__inline NTSTATUS VDeviceReadWrite (
	IN PDEVICE_OBJECT DeviceObject, 
	IN PIRP Irp
	)
{
	PEP_DEVICE_EXTENSION devext = (PEP_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

	if (devext->DE_FILTER.DE_DISKVOL.InitializeStatus==DVIS_SAVINGDATA) {
		KdPrint(("SYS:VDeviceReadWrite:Another process read/write disk when %wZ is saving data.\n", &devext->DE_FILTER.PhysicalDeviceName));
	}
	//��IRP����Ϊ�ȴ�״̬
	IoMarkIrpPending(Irp);
	//Ȼ�����irp�Ž���д���������
	ExInterlockedInsertTailList(
		&devext->DE_FILTER.DE_DISKVOL.RWListHead,
		&Irp->Tail.Overlay.ListEntry,
		&devext->DE_FILTER.DE_DISKVOL.RequestListLock
		);
	//������еĵȴ��¼���֪ͨ���ж����irp���д���
	KeSetEvent(&devext->DE_FILTER.DE_DISKVOL.RequestEvent, (KPRIORITY)0, FALSE);
	//����pending״̬�����irp���㲿�ִ�������
	return STATUS_PENDING;
};

NTSTATUS VDeviceRead(
	IN PDEVICE_OBJECT DeviceObject, 
	IN PIRP Irp
	)
{
	PAGED_CODE();

	return VDeviceReadWrite(DeviceObject, Irp);
}

NTSTATUS VDeviceWrite(
	IN PDEVICE_OBJECT DeviceObject, 
	IN PIRP Irp
	)
{
	PAGED_CODE();
	
	return VDeviceReadWrite(DeviceObject, Irp);
}

NTSTATUS VDevicePnp(
	IN PDEVICE_OBJECT DeviceObject, 
	IN PIRP Irp
	)
{
	PEP_DEVICE_EXTENSION devext = (PEP_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	NTSTATUS status;
	PIO_STACK_LOCATION isp;
	BOOLEAN IsLocalVolume;//1.�Ƿ��Ǳ��ؾ����������ƶ������еľ�������Ӱ��ģʽ�������Ĵ��̾�
		
	PAGED_CODE();
	
	isp = IoGetCurrentIrpStackLocation( Irp );
	switch(isp->MinorFunction) 
	{
	case IRP_MN_START_DEVICE:
		KdPrint(("SYS:VDevicePnp:IRP_MN_START_DEVICE(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		status = LLWaitIRPCompletion(devext->DE_FILTER.LowerDeviceObject, Irp);
		
		IsLocalVolume = FALSE;
		//�ɹ������ˣ�
		if (NT_SUCCESS(status)) {
			PVOLUME_DISK_EXTENTS VolumeDiskExtents;//������Ϣ
			ULONG VolumeDiskExtentsSize;

			VolumeDiskExtentsSize = sizeof(VOLUME_DISK_EXTENTS ) + ( 32 - 1) * sizeof(DISK_EXTENT );
			VolumeDiskExtents = (PVOLUME_DISK_EXTENTS)ExAllocatePoolWithTag(PagedPool, VolumeDiskExtentsSize, 'DEXT'); 
			if (VolumeDiskExtents != NULL) {
				KEVENT event;
				PIRP QueryIrp;
				NTSTATUS localstatus;
				IO_STATUS_BLOCK iostatus;

				KeInitializeEvent(&event, NotificationEvent, FALSE);

				//
				// ��ѯ������Ӧ�Ĵ���
				//
				QueryIrp = IoBuildDeviceIoControlRequest(
					IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
					devext->DE_FILTER.LowerDeviceObject,
					NULL,
					0,
					VolumeDiskExtents,
					VolumeDiskExtentsSize,
					FALSE,
					&event,
					&iostatus);
				if (QueryIrp != NULL) {
					localstatus = IoCallDriver(devext->DE_FILTER.LowerDeviceObject, QueryIrp);
					if (localstatus == STATUS_PENDING) {
						KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
						localstatus = iostatus.Status;
					}
					//��ȡ����Ϣ�ɹ�
					if (NT_SUCCESS(localstatus)) {
						ULONG i;
						
						//�鿴������Ƿ������������
						for (i = 0; i <VolumeDiskExtents->NumberOfDiskExtents; i++) {
							PDEVICE_OBJECT DiskFilterDO;
							

							DiskFilterDO = DeviceObject->DriverObject->DeviceObject;
							while (DiskFilterDO != NULL) {
								PEP_DEVICE_EXTENSION devobjext = NULL;
								devobjext = (PEP_DEVICE_EXTENSION)DiskFilterDO->DeviceExtension;
								//����Ǵ����豸
								if (devobjext->DeviceType == DO_DISK) {
									//�ҵ�����������ڵĴ���
									if (VolumeDiskExtents->Extents[i].DiskNumber == devobjext->DE_FILTER.DE_DISK.DiskNumber) {
										break;
									}
								}
								//��ѯ��һ���豸
								DiskFilterDO = DiskFilterDO->NextDevice;
							}
							if (DiskFilterDO != NULL) {
								PEP_DEVICE_EXTENSION devobjext;

								devobjext = (PEP_DEVICE_EXTENSION)DiskFilterDO->DeviceExtension;
								ASSERT(devobjext->DeviceType == DO_DISK);
								if (!devobjext->DE_FILTER.DE_DISK.IsVolumeChanged) {
									//�������̹�������
									VAssociateDiskFilter(DeviceObject, VolumeDiskExtents->Extents[i].StartingOffset, VolumeDiskExtents->Extents[i].ExtentLength, DiskFilterDO);
									IsLocalVolume = DIS_LOCAL_DISK(devobjext->DE_FILTER.DE_DISK.BusType);
								} else {
									//����Ӱ��ģʽ�������Ĵ��̾����ܱ����Σ���˲��Ǳ��ش��̾�
									IsLocalVolume = FALSE;
									KdPrint(("SYS: This volume %wZ can't be believed\n", &devext->DE_FILTER.PhysicalDeviceName));
								}
							} else {
								IsLocalVolume = FALSE;
							}
							if (!IsLocalVolume) {
								break;
							}
						}
					} else {
						KdPrint(("SYS:DDevicePnp(%wZ) IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS failed(0x%08x)\n", &devext->DE_FILTER.PhysicalDeviceName, localstatus));
					}
				}
				ExFreePool(VolumeDiskExtents);
			}
		}
		if (!IsLocalVolume) {
			//���Ǳ��ؾ���ɾ�������豸
			IoDetachDevice(devext->DE_FILTER.LowerDeviceObject);
			//�������̹����豸��ӳ��
			VDisassociateDiskFilter(DeviceObject);

			VPostDetachDiskVolDevice(DeviceObject);
			//������ڹ����豸����Ҫɾ�������Ⲣ����������ɾ����ֻ�Ǽ��������ã����Է���devext�ǰ�ȫ�ġ�
			IoDeleteDevice(DeviceObject);
		} else {
			
		}
		return LLCompleteRequest(status, Irp, Irp->IoStatus.Information);
		break;
		
	case IRP_MN_QUERY_REMOVE_DEVICE:
		KdPrint(("SYS:VDevicePnp:IRP_MN_QUERY_REMOVE_DEVICE(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
	case IRP_MN_REMOVE_DEVICE:
		KdPrint(("SYS:VDevicePnp:IRP_MN_REMOVE_DEVICE(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		/*
		//�����ﷵ�ش���ֵ���ã���Ϊ�������Ѿ����ˣ�������൱���Ѿ�û����
		if (devext->DE_FILTER.DE_DISKVOL.IsProtect) {
			//�ܱ���������ѹ����
			KdPrint(("SYS:VDevicePnp:IRP_MN_REMOVE_DEVICE(%wZ) is protected, deny to remove\n", &devext->DE_FILTER.PhysicalDeviceName));
			return LLCompleteRequest(STATUS_ACCESS_DENIED, Irp, 0);
		}
		*/
		IoDetachDevice(devext->DE_FILTER.LowerDeviceObject);
		
		//�������̹����豸��ӳ��
		VDisassociateDiskFilter(DeviceObject);

		VPostDetachDiskVolDevice(DeviceObject);
		//������ڹ����豸����Ҫɾ�������Ⲣ����������ɾ����ֻ�Ǽ��������ã����Է���devext�ǰ�ȫ�ġ�
		IoDeleteDevice(DeviceObject);
		break;
		/*
	case IRP_MN_CANCEL_REMOVE_DEVICE:
		KdPrint(("SYS:VDevicePnp:IRP_MN_CANCEL_REMOVE_DEVICE(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
		*/
	case IRP_MN_STOP_DEVICE:
		KdPrint(("SYS:VDevicePnp:IRP_MN_STOP_DEVICE(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		/*
		if (devext->DE_FILTER.DE_DISKVOL.IsProtect) {
			//�ܱ���������ѹ����
			KdPrint(("SYS:VDevicePnp:IRP_MN_STOP_DEVICE(%wZ) is protected, deny to remove\n", &devext->DE_FILTER.PhysicalDeviceName));
			return LLCompleteRequest(STATUS_ACCESS_DENIED, Irp, 0);
		}
		*/
		break;
		/*
	case IRP_MN_QUERY_STOP_DEVICE:
		KdPrint(("SYS:VDevicePnp:IRP_MN_QUERY_STOP_DEVICE(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
	case IRP_MN_CANCEL_STOP_DEVICE:
		KdPrint(("SYS:VDevicePnp:IRP_MN_CANCEL_STOP_DEVICE(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;

	case IRP_MN_QUERY_DEVICE_RELATIONS:
		KdPrint(("SYS:VDevicePnp:IRP_MN_QUERY_DEVICE_RELATIONS-%s(%wZ)\n", qrydevtypestrlst[isp->Parameters.QueryDeviceRelations.Type], &devext->DE_FILTER.PhysicalDeviceName));
		break;
	case IRP_MN_QUERY_INTERFACE:
		KdPrint(("SYS:VDevicePnp:IRP_MN_QUERY_INTERFACE(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
	case IRP_MN_QUERY_CAPABILITIES:
		KdPrint(("SYS:VDevicePnp:IRP_MN_QUERY_CAPABILITIES(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
	case IRP_MN_QUERY_RESOURCES:
		KdPrint(("SYS:VDevicePnp:IRP_MN_QUERY_RESOURCES(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
	case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
		KdPrint(("SYS:VDevicePnp:IRP_MN_QUERY_RESOURCE_REQUIREMENTS(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
	case IRP_MN_QUERY_DEVICE_TEXT:
		KdPrint(("SYS:VDevicePnp:IRP_MN_QUERY_DEVICE_TEXT(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
	case IRP_MN_FILTER_RESOURCE_REQUIREMENTS:
		KdPrint(("SYS:VDevicePnp:IRP_MN_FILTER_RESOURCE_REQUIREMENTS(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;

	case IRP_MN_READ_CONFIG:
		KdPrint(("SYS:VDevicePnp:IRP_MN_READ_CONFIG(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
	case IRP_MN_WRITE_CONFIG:
		KdPrint(("SYS:VDevicePnp:IRP_MN_WRITE_CONFIG(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
	case IRP_MN_EJECT:
		KdPrint(("SYS:VDevicePnp:IRP_MN_EJECT(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
	case IRP_MN_SET_LOCK:
		KdPrint(("SYS:VDevicePnp:IRP_MN_SET_LOCK(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
	case IRP_MN_QUERY_ID:
		KdPrint(("SYS:VDevicePnp:IRP_MN_QUERY_ID(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
	case IRP_MN_QUERY_PNP_DEVICE_STATE:
		KdPrint(("SYS:VDevicePnp:IRP_MN_QUERY_PNP_DEVICE_STATE(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
	case IRP_MN_QUERY_BUS_INFORMATION:
		KdPrint(("SYS:VDevicePnp:IRP_MN_QUERY_BUS_INFORMATION(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
	case IRP_MN_DEVICE_USAGE_NOTIFICATION:
		KdPrint(("SYS:VDevicePnp:IRP_MN_DEVICE_USAGE_NOTIFICATION(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
	case IRP_MN_SURPRISE_REMOVAL:
		KdPrint(("SYS:VDevicePnp:IRP_MN_SURPRISE_REMOVAL(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
		*/
	}

	return LLSendToNextDriver(devext->DE_FILTER.LowerDeviceObject, Irp);
}

NTSTATUS VDevicePower(
	IN PDEVICE_OBJECT DeviceObject, 
	IN PIRP Irp
	)
{
	PEP_DEVICE_EXTENSION devExt;
	PIO_STACK_LOCATION irpsp;
	NTSTATUS status;

	PAGED_CODE();

	devExt = (PEP_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	irpsp = IoGetCurrentIrpStackLocation(Irp);
	if (irpsp->MinorFunction == IRP_MN_SET_POWER) {
		
		switch (irpsp->Parameters.Power.ShutdownType) {
		case PowerActionShutdown://�ػ�
		case PowerActionShutdownOff://�ػ��͹ص�Դ
		case PowerActionShutdownReset://����

			break;
		}
	}
	PoStartNextPowerIrp(Irp);
	IoSkipCurrentIrpStackLocation(Irp);
	status = PoCallDriver(devExt->DE_FILTER.LowerDeviceObject, Irp);
	return status;
}

VOID VWriteMemRedirectDataToDisk(
	IN OUT PEP_DEVICE_EXTENSION voldevext,
	IN OUT PRW_BUFFER_REC RWBuffer,
	IN PVOID ClusterBuffer,
	IN REDIRECT_MODE RedirectMode//�ض���ģʽ
	)
{
	PREDIRECT_RW_MEM ListEntry;//�ض������
	PREDIRECT_RW_MEM TmpListEntry;//�ض������
#ifdef DBG
	ULONG TotalDataSize = 0;
#endif
	
	PAGED_CODE();
	//���벻�����ڴ��ض���ģʽ����ΪҪд����
	ASSERT(RedirectMode != RM_IN_MEMORY);

	ListEntry = &voldevext->DE_FILTER.DE_DISKVOL.RedirectMem.RedirectListHead;
	//��д���ض��򣬲������ܵĺϲ��б�
	for (ListEntry = (PREDIRECT_RW_MEM)ListEntry->ListEntry.Flink; ListEntry != &voldevext->DE_FILTER.DE_DISKVOL.RedirectMem.RedirectListHead;) {
		TmpListEntry = (PREDIRECT_RW_MEM)ListEntry->ListEntry.Flink;
		//ժ������
		ListEntry->ListEntry.Blink->Flink = ListEntry->ListEntry.Flink;
		ListEntry->ListEntry.Flink->Blink = ListEntry->ListEntry.Blink;

		VReadWriteIrpDispose(voldevext, IRP_MJ_WRITE, ListEntry->buffer, ListEntry->length, ListEntry->offset, RWBuffer, RedirectMode, ClusterBuffer);
#ifdef DBG
		TotalDataSize += ListEntry->length;
#endif
		//�ͷ��ڴ�
		FreeRedirectIrpListEntry(ListEntry);
		ListEntry = TmpListEntry;
	}
#ifdef DBG
	DbgPrint("SYS:VWriteMemRedirectDataToDisk:TotalDataSize=%d\n", TotalDataSize);
#endif
}

NTSTATUS VReadWriteIrpDispose(
	IN OUT PEP_DEVICE_EXTENSION voldevext,
	ULONG MajorFunction,
	PVOID buffer,
	ULONG length,
	ULONGLONG offset,

	IN OUT PRW_BUFFER_REC RWBuffer,
	IN REDIRECT_MODE RedirectMode,//�ض���ģʽ
	IN PVOID ClusterBuffer
	)
{
	
	NTSTATUS	status;

	PAGED_CODE();

	status = STATUS_SUCCESS;
	//�����￪ʼ���ж��Ƿ������ڴ����ض���
	switch (RedirectMode) {
	case RM_IN_DISK://�ڴ������ض���
		{
			ULONG				RealLength;
			//��һ�������
			ULONGLONG			FirstClusterIndex;
			//δ�ʹض�����ֽ���
			ULONG				FirstUnalignedBytes;
			//���һ�������
			ULONGLONG			LastClusterIndex;
			//δ�ʹض�����ֽ���
			ULONG				LastUnalignedBytes;
			//������
			ULONG				ClusterNum;

			ASSERT(ClusterBuffer!=NULL);
			//һ���ǵð�������С�ı�����д
			if (length%voldevext->DE_FILTER.DE_DISKVOL.FsSetting.BytesPerSector!=0) {
				KdPrint(("SYS:VReadWriteThread:bad length=%d, offset=%I64d, IRP_MJ_WRITE=%d\n", length, offset, MajorFunction));
				return STATUS_UNSUCCESSFUL;
			}
			//��ȡ��һ���ص����
			FirstClusterIndex = offset/voldevext->DE_FILTER.DE_DISKVOL.FsSetting.BytesPerAllocationUnit;
			FirstUnalignedBytes = offset%voldevext->DE_FILTER.DE_DISKVOL.FsSetting.BytesPerAllocationUnit;
			LastClusterIndex = (offset+length)/voldevext->DE_FILTER.DE_DISKVOL.FsSetting.BytesPerAllocationUnit;
			LastUnalignedBytes = (offset+length)%voldevext->DE_FILTER.DE_DISKVOL.FsSetting.BytesPerAllocationUnit;
			//��ȡ������
			ClusterNum = (ULONG)(LastClusterIndex-FirstClusterIndex)+(LastUnalignedBytes>0);
			//��ȡ���ض�д����ʵ����
			RealLength = ClusterNum*voldevext->DE_FILTER.DE_DISKVOL.FsSetting.BytesPerAllocationUnit;
			//������������Ȳ��������������ڴ�
			RW_ADJUST_MEM(RWBuffer, RealLength);

			switch (MajorFunction) {
			case IRP_MJ_READ:
				//���ض�д
				status = VHandleReadWrite(
					voldevext, 
					IRP_MJ_READ, 
					offset-FirstUnalignedBytes, 
					RWBuffer->Buffer, 
					RealLength);
				if (NT_SUCCESS(status)) {
					//��������������������Ҫ�ĸ��Ƶ���������
					RtlCopyMemory(buffer, &((PBYTE)RWBuffer->Buffer)[FirstUnalignedBytes], length);
				} else {
					length = 0;
				}

				break;
			case IRP_MJ_WRITE:
				status = STATUS_SUCCESS;
				//����д����������Ҫ������������
				RtlCopyMemory(&((PBYTE)RWBuffer->Buffer)[FirstUnalignedBytes], buffer, length);
				//������ݵ�ͷ�������ض��룬���ȶ�ȡ����ص�ȫ�����ݣ�Ȼ���Ƶ�����������ɰ��ض��������
				if (FirstUnalignedBytes>0) {
					
					//��ȡ��һ�ص�����
					status = VHandleReadWrite(
						voldevext, 
						IRP_MJ_READ, 
						offset-FirstUnalignedBytes, 
						ClusterBuffer, 
						voldevext->DE_FILTER.DE_DISKVOL.FsSetting.BytesPerAllocationUnit
						);
					if (NT_SUCCESS(status)) {
						//��������������������Ҫ�ĸ��Ƶ���������
						RtlCopyMemory(RWBuffer->Buffer, ClusterBuffer, FirstUnalignedBytes);
					} else {
						length = 0;
					}
				}
				//�������β�������ز����룬���ȡβ�������ص�����
				if (LastUnalignedBytes>0) {
					//���������Ҫ��ȡ����
					if (FirstUnalignedBytes==0||ClusterNum > 1) {
						//��ȡ���һ�ص�����
						status = VHandleReadWrite(
							voldevext, 
							IRP_MJ_READ, 
							offset+length-LastUnalignedBytes, 
							ClusterBuffer, 
							voldevext->DE_FILTER.DE_DISKVOL.FsSetting.BytesPerAllocationUnit
							);
					}
					if (NT_SUCCESS(status)) {
						//��������������������Ҫ�ĸ��Ƶ���������
						RtlCopyMemory(&((PBYTE)RWBuffer->Buffer)[FirstUnalignedBytes+length], ClusterBuffer, voldevext->DE_FILTER.DE_DISKVOL.FsSetting.BytesPerAllocationUnit-LastUnalignedBytes);
					} else {
						length = 0;
					}
				}
				if (NT_SUCCESS(status)) {
					status = VHandleReadWrite(
						voldevext, 
						IRP_MJ_WRITE, 
						offset-FirstUnalignedBytes, 
						RWBuffer->Buffer, 
						RealLength
						);
					if (!NT_SUCCESS(status)) {
						length = 0;
					}
				}
				break;
			default:
				ASSERT(0);
				status = STATUS_UNSUCCESSFUL;
				length = 0;
				break;
			}
		}
		break;
	case RM_IN_MEMORY://���ڴ����ض���
		{
			IO_STATUS_BLOCK iostatus;

			//������ڴ����ض���
			switch (MajorFunction) {
			case IRP_MJ_READ:
				//�ж��ڴ湻����
				RW_ADJUST_MEM(RWBuffer, length);
				status = VDirectReadWriteDiskVolume(voldevext->DE_FILTER.PhysicalDeviceObject, 
					IRP_MJ_READ,
					offset,
					RWBuffer->Buffer,
					length,
					&iostatus,
					TRUE);
				if (NT_SUCCESS(status)) {
					//��ȡ�ɹ�

					LCXLReadFromRedirectRWMemList(&voldevext->DE_FILTER.DE_DISKVOL.RedirectMem.RedirectListHead, RWBuffer->Buffer, offset, length);
					RtlCopyMemory(buffer, RWBuffer->Buffer, length);
				}
				break;
			case IRP_MJ_WRITE:
				//�ڴ��ض�����Ҫ������д�Ĳ���
				//Ҳ���ǵ��ڴ��ض������֮�󣬲ſ�ʼ������д�����
				LCXLInsertToRedirectRWMemList(&voldevext->DE_FILTER.DE_DISKVOL.RedirectMem.RedirectListHead, buffer, offset, length);
				status = STATUS_SUCCESS;
				break;
			default:
				ASSERT(0);
				status = STATUS_INVALID_DEVICE_REQUEST;
				length = 0;
				break;
			}
		}
		break;
	case RM_DIRECT://ֱ�Ӷ�д
		{
			IO_STATUS_BLOCK iostatus;
			//������������Ȳ��������������ڴ�
			RW_ADJUST_MEM(RWBuffer, length);
			switch (MajorFunction) {
			case IRP_MJ_READ:
				//���ض�д

				status = VDirectReadWriteDiskVolume(
					voldevext->DE_FILTER.PhysicalDeviceObject, 
					IRP_MJ_READ, 
					offset, 
					RWBuffer->Buffer, 
					length, 
					&iostatus,
					TRUE);
				if (NT_SUCCESS(status)) {
					//��������������������Ҫ�ĸ��Ƶ���������
					RtlCopyMemory(buffer, RWBuffer->Buffer, length);
				} else {
					length = 0;
				}

				break;
			case IRP_MJ_WRITE:
				status = STATUS_SUCCESS;
				//����д����������Ҫ������������
				RtlCopyMemory(RWBuffer->Buffer, buffer, length);
				//
				status = VDirectReadWriteDiskVolume(
					voldevext->DE_FILTER.PhysicalDeviceObject, 
					IRP_MJ_WRITE, 
					offset, 
					RWBuffer->Buffer, 
					length, 
					&iostatus,
					TRUE);
				break;
			default:
				ASSERT(0);
				status = STATUS_UNSUCCESSFUL;
				length = 0;
				break;
			}
		}
		break;
	}
	return status;
}

//����MAPINDEX������
PLCXL_TABLE_MAP VFindMapIndex(IN PRTL_GENERIC_TABLE Table, IN ULONGLONG mapIndex)
{
	PVOID RestartKey;
	PLCXL_TABLE_MAP ptr;

	PAGED_CODE();

	RestartKey = NULL;
	//��ȡ�ض����
	for (ptr = (PLCXL_TABLE_MAP)RtlEnumerateGenericTableWithoutSplaying(Table, &RestartKey);
		ptr != NULL;
		ptr = (PLCXL_TABLE_MAP)RtlEnumerateGenericTableWithoutSplaying(Table, &RestartKey)) {
			if (ptr->mapIndex==mapIndex) {
				return ptr;
			}
	}
	return NULL;
}

NTSTATUS VSaveShadowData(PEP_DEVICE_EXTENSION voldevext)
{

	NTSTATUS status;
	PVOID RestartKey;
	PLCXL_TABLE_MAP ptr;
	//�ض������ģ���ջ
	PLCXL_TABLE_MAP *TableMapStack;
	ULONG			TableMapStackSize;//ģ���ջ�Ĵ�С
	ULONG			TableMapStackPos;//��ջλ��
	PVOID Buffer;//��д������
	ULONG BufferLen;//���������ȣ�Ҳ��ÿ���صĴ�С
	IO_STATUS_BLOCK iostatus;
	ULONG DataNum;
	ULONG DataI;
	ULONG SavingProgress;

	CHAR strbuf[MAX_PATH];
	PAGED_CODE();
	ASSERT(voldevext != NULL);
	ASSERT(voldevext->DE_FILTER.DE_DISKVOL.IsProtect&&voldevext->DE_FILTER.DE_DISKVOL.IsSaveShadowData);
	status = STATUS_SUCCESS;
	KdPrint(("SYS:VSaveShadowData begin\n"));
	//if (!IS_WINDOWSVISTA_OR_LATER()) {
		RtlStringCbPrintfA(strbuf, sizeof(strbuf), "Disk Volume:%wZ\nPlease wait while getting data information...", &voldevext->DE_FILTER.PhysicalDeviceName);
		InbvDisplayString(strbuf);
	//}
	BufferLen = voldevext->DE_FILTER.DE_DISKVOL.FsSetting.BytesPerAllocationUnit;
	Buffer = ExAllocatePoolWithTag(PagedPool, BufferLen, 'DATA');
	if (Buffer == NULL) {
		//��Դ����
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	DataNum = 0;
	RestartKey = NULL;
	//��ʼ���ض�����еķ�����IsVisited
	for (ptr = (PLCXL_TABLE_MAP)RtlEnumerateGenericTableWithoutSplaying(&voldevext->DE_FILTER.DE_DISKVOL.FsSetting.RedirectTable, &RestartKey);
		ptr != NULL;
		ptr = (PLCXL_TABLE_MAP)RtlEnumerateGenericTableWithoutSplaying(&voldevext->DE_FILTER.DE_DISKVOL.FsSetting.RedirectTable, &RestartKey)) {
			ptr->IsVisited = FALSE;
			DataNum++;
	}
	//if (!IS_WINDOWSVISTA_OR_LATER()) {
		RtlStringCbPrintfA(strbuf, sizeof(strbuf), "OK!Data Number=%d\n0%%-----------------50%%-----------------100%%\n", DataNum);
		InbvDisplayString(strbuf);
	//}
	//��ʼ������
	SavingProgress = 0;
	//��ʼ��ģ���ջ
	TableMapStack = NULL;
	TableMapStackSize = 0;
	TableMapStackPos = 0;

	DataI = 0;
	RestartKey = NULL;
	//��ȡ�ض����
	for (ptr = (PLCXL_TABLE_MAP)RtlEnumerateGenericTableWithoutSplaying(&voldevext->DE_FILTER.DE_DISKVOL.FsSetting.RedirectTable, &RestartKey);
		ptr != NULL;
		ptr = (PLCXL_TABLE_MAP)RtlEnumerateGenericTableWithoutSplaying(&voldevext->DE_FILTER.DE_DISKVOL.FsSetting.RedirectTable, &RestartKey)) {
			ULONG j;
			if (ptr->mapIndex != ptr->orgIndex) {
				if (ptr->IsVisited) {
					continue;
				}
				do {
					//ѹ���ջ��
					if (TableMapStackPos>=TableMapStackSize) {
						PLCXL_TABLE_MAP *tmpstack;

						TableMapStackSize += 1024;
						tmpstack = (PLCXL_TABLE_MAP*)ExAllocatePoolWithTag(PagedPool, TableMapStackSize*sizeof(PLCXL_TABLE_MAP), 'SSSS');
						if (TableMapStackPos!=0) {
							RtlCopyMemory(tmpstack, TableMapStack, TableMapStackPos*sizeof(PLCXL_TABLE_MAP));	
						}
						if (TableMapStack != NULL) {
							ExFreePool(TableMapStack);
						}
						TableMapStack = tmpstack;
						KdPrint(("SAVE TableMapStackPos=%d\n", TableMapStackPos+1));
					}
					//ѹ���ջ
					TableMapStack[TableMapStackPos] = ptr;
					TableMapStackPos++;
					//������һ��
					ptr = VFindMapIndex(&voldevext->DE_FILTER.DE_DISKVOL.FsSetting.RedirectTable, ptr->orgIndex);
				} while (ptr != NULL && !ptr->IsVisited);
				for (;TableMapStackPos>0; TableMapStackPos--) {
					//����Ϊ�ѷ���
					ASSERT(TableMapStack[TableMapStackPos-1]!=NULL);
					TableMapStack[TableMapStackPos-1]->IsVisited = TRUE;
					//���ض���Ĵ��̶�дд�ص�ԭ����λ��
					status = VDirectReadWriteDiskVolume(voldevext->DE_FILTER.PhysicalDeviceObject, IRP_MJ_READ, TableMapStack[TableMapStackPos-1]->mapIndex*BufferLen, Buffer, BufferLen, &iostatus, TRUE);
					if (NT_SUCCESS(status)) {
						status = VDirectReadWriteDiskVolume(voldevext->DE_FILTER.PhysicalDeviceObject, IRP_MJ_WRITE, TableMapStack[TableMapStackPos-1]->orgIndex*BufferLen, Buffer, BufferLen, &iostatus, TRUE);
						if (!NT_SUCCESS(status)) {
							KdPrint(("SYS:VSaveShadowData:VDirectReadWriteDiskVolume(WRITE)Error(0x%08x)\n", status));
						}
					} else {
						KdPrint(("SYS:VSaveShadowData:VDirectReadWriteDiskVolume(READ)Error(0x%08x)\n", status));
					}
					DataI++;
				}
				
			} else {
				if (!ptr->IsVisited) {
					DataI++;
				}
			}
			//���½���
			
			j = (ULONG)((ULONGLONG)DataI*40/DataNum);
			if (SavingProgress < j) {
				for (; SavingProgress < j; SavingProgress++) {
					//if (!IS_WINDOWSVISTA_OR_LATER()) {
						InbvDisplayString(".");
					//}
				}
			}
			//----
	}
	if (TableMapStack != NULL) {
		ExFreePool(TableMapStack);
	}
	ExFreePool(Buffer);
	InbvDisplayString("OK!\n\n");
	return status;
}

//���̾��д�߳�
VOID VReadWriteThread (
	IN PVOID Context//�߳������ģ������Ǵ��̾������������
	)
{
	//�豸����ָ��
	PDEVICE_OBJECT DeviceObject;
	//�豸��չ
	PEP_DEVICE_EXTENSION voldevext;
	RW_BUFFER_REC RWBuffer;//��д�������ṹ
	PVOID ClusterBuffer;//�ش�С�Ļ�����
	//�ض���ģʽ
	REDIRECT_MODE		RedirectMode;

	PAGED_CODE();
	//��������̵߳����ȼ�Ϊ��ʵʱ��
	KeSetPriorityThread(KeGetCurrentThread(), LOW_REALTIME_PRIORITY);

	DeviceObject = (PDEVICE_OBJECT)Context;
	voldevext = (PEP_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	ASSERT(voldevext->DeviceType == DO_DISKVOL);

	RedirectMode = RM_DIRECT;
	RW_INIT_MEM(&RWBuffer);
	RW_ADJUST_MEM(&RWBuffer, DefaultReadWriteBufferSize);
	if (RWBuffer.Buffer == NULL) {
		KdPrint(("SYS:!!!!!!!!!!!!!!!!VReadWriteThread(%wZ)ReadWriteBuffer=NULL\n", &voldevext->DE_FILTER.PhysicalDeviceName));
		PsTerminateSystemThread(STATUS_INSUFFICIENT_RESOURCES);
	}
	ClusterBuffer = NULL;
	//����߳̽�����־ΪTRUE�����˳�
	while (!voldevext->DE_FILTER.DE_DISKVOL.ToTerminateThread) {
		//������е����
		PLIST_ENTRY	ReqEntry;
		
		//�ȵȴ��������ͬ���¼������������û��irp��Ҫ�������ǵ��߳̾͵ȴ�������ó�cpuʱ��������߳�
		KeWaitForSingleObject(
			&voldevext->DE_FILTER.DE_DISKVOL.RequestEvent,
			Executive,
			KernelMode,
			FALSE,
			NULL
			);
		switch (voldevext->DE_FILTER.DE_DISKVOL.InitializeStatus) {
		case DVIS_INITIALIZED:
			voldevext->DE_FILTER.DE_DISKVOL.InitializeStatus = DVIS_WORKING;
			//��ʼ���ض����ڴ������д�뵽������
			if (voldevext->DE_FILTER.DE_DISKVOL.IsProtect) {
				ClusterBuffer = ExAllocatePoolWithTag(PagedPool, voldevext->DE_FILTER.DE_DISKVOL.FsSetting.BytesPerAllocationUnit, MEM_TAG_RWBUFFER);
				RedirectMode = RM_IN_DISK;
			} else {
				RedirectMode = RM_DIRECT;
			}
			VWriteMemRedirectDataToDisk(voldevext, &RWBuffer, ClusterBuffer, RedirectMode);
			break;
			//��������ģʽ
		case DVIS_SAVINGDATA:
			VSaveShadowData(voldevext);
			//д�����ݺ��ܱ���
			voldevext->DE_FILTER.DE_DISKVOL.IsProtect = FALSE;
			//����Ϊֱ�Ӷ�дģʽ
			RedirectMode = RM_DIRECT;
			//��������Ϊ����״̬
			voldevext->DE_FILTER.DE_DISKVOL.InitializeStatus = DVIS_WORKING;
			//��������¼�
			KeSetEvent(&voldevext->DE_FILTER.DE_DISKVOL.SaveShadowDataFinishEvent, 0, FALSE);
			break;
		}
		//��������е��ײ��ó�һ��������׼����������ʹ�������������ƣ����Բ����г�ͻ
		while (ReqEntry = ExInterlockedRemoveHeadList(&voldevext->DE_FILTER.DE_DISKVOL.RWListHead, &voldevext->DE_FILTER.DE_DISKVOL.RequestListLock), ReqEntry != NULL) {
			NTSTATUS			status;
			//irpָ��
			PIRP				Irp;
			//irp�е����ݳ���
			ULONG				length;
			//irpҪ�����ƫ����
			ULONGLONG			offset;
			//irp�а����Ķ�д���ݻ�������ַ
			PVOID				buffer;
			//IRP��ջ
			PIO_STACK_LOCATION	IrpSp;

			//�Ӷ��е�������ҵ�ʵ�ʵ�irp�ĵ�ַ
			Irp = CONTAINING_RECORD(ReqEntry, IRP, Tail.Overlay.ListEntry);
			IrpSp = IoGetCurrentIrpStackLocation(Irp);
			switch (IrpSp->MajorFunction) {
			case IRP_MJ_READ:
				//����Ƕ���irp����������irp stack��ȡ����Ӧ�Ĳ�����Ϊoffset��length
				offset = (ULONGLONG)IrpSp->Parameters.Read.ByteOffset.QuadPart;
				length = IrpSp->Parameters.Read.Length;
				break;
			case IRP_MJ_WRITE:
				//�����д��irp����������irp stack��ȡ����Ӧ�Ĳ�����Ϊoffset��length
				offset = (ULONGLONG)IrpSp->Parameters.Write.ByteOffset.QuadPart;
				length = IrpSp->Parameters.Write.Length;				
				break;
			default:
				//�������������ǲ�Ӧ���е�
				ASSERT(0);
				offset = 0;
				length = 0;
				break;
			}
			if (Irp->MdlAddress) {
				buffer = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
			}else if (Irp->UserBuffer) {
				buffer = Irp->UserBuffer;
			} else {
				buffer = Irp->AssociatedIrp.SystemBuffer;
			}
			
			//�ж�Ŀǰ��ȡ���ú�λͼ��״̬
			switch (voldevext->DE_FILTER.DE_DISKVOL.InitializeStatus) {
			case DVIS_NONE:	
			case DVIS_INITIALIZING:
				//û�л����ڳ�ʼ��
				status = VReadWriteIrpDispose(voldevext, IrpSp->MajorFunction, buffer, length, offset, &RWBuffer, RM_IN_MEMORY, ClusterBuffer);
				break;
			case DVIS_INITIALIZED:
				voldevext->DE_FILTER.DE_DISKVOL.InitializeStatus = DVIS_WORKING;
				//��ʼ���ض����ڴ������д�뵽������
				if (voldevext->DE_FILTER.DE_DISKVOL.IsProtect) {
					ClusterBuffer = ExAllocatePoolWithTag(PagedPool, voldevext->DE_FILTER.DE_DISKVOL.FsSetting.BytesPerAllocationUnit, MEM_TAG_RWBUFFER);
					RedirectMode = RM_IN_DISK;
				} else {
					RedirectMode = RM_DIRECT;
				}
				VWriteMemRedirectDataToDisk(voldevext, &RWBuffer, ClusterBuffer, RedirectMode);
				status = VReadWriteIrpDispose(voldevext, IrpSp->MajorFunction, buffer, length, offset, &RWBuffer, RedirectMode, ClusterBuffer);
				break;
			case DVIS_WORKING:
				status = VReadWriteIrpDispose(voldevext, IrpSp->MajorFunction, buffer, length, offset, &RWBuffer, RedirectMode, ClusterBuffer);
				break;
			case DVIS_SAVINGDATA:
				VSaveShadowData(voldevext);
				//д�����ݺ��ܱ���
				voldevext->DE_FILTER.DE_DISKVOL.IsProtect = FALSE;
				//����Ϊֱ�Ӷ�дģʽ
				RedirectMode = RM_DIRECT;
				//��������Ϊ����״̬
				voldevext->DE_FILTER.DE_DISKVOL.InitializeStatus = DVIS_WORKING;
				//��������¼�
				KeSetEvent(&voldevext->DE_FILTER.DE_DISKVOL.SaveShadowDataFinishEvent, 0, FALSE);

				status = VReadWriteIrpDispose(voldevext, IrpSp->MajorFunction, buffer, length, offset, &RWBuffer, RedirectMode, ClusterBuffer);
				break;
			default:
				status = STATUS_INVALID_DEVICE_REQUEST;
				ASSERT(0);
				break;
			}
			if (!NT_SUCCESS(status)) {
				length = 0;	
			}
			LLCompleteRequest(status, Irp, length);
		}
		//�鿴�Ƿ����û�����
		while (ReqEntry = ExInterlockedRemoveHeadList(&voldevext->DE_FILTER.DE_DISKVOL.AppRequsetListHead.ListEntry, &voldevext->DE_FILTER.DE_DISKVOL.RequestListLock), ReqEntry != NULL) {
			PAPP_REQUEST_LIST_ENTRY  ARListEntrt;

			ARListEntrt = (PAPP_REQUEST_LIST_ENTRY)ReqEntry;
			switch (ARListEntrt->AppRequest.RequestType) {
			case AR_VOLUME_INFO://��ȡ����Ϣ
				ARListEntrt->AppRequest.VolumeInfo.AvailableAllocationUnits = voldevext->DE_FILTER.DE_DISKVOL.FsSetting.AvailableAllocationUnits;
				ARListEntrt->AppRequest.VolumeInfo.BytesPerAllocationUnit = voldevext->DE_FILTER.DE_DISKVOL.FsSetting.BytesPerAllocationUnit;
				ARListEntrt->AppRequest.VolumeInfo.BytesPerSector = voldevext->DE_FILTER.DE_DISKVOL.FsSetting.BytesPerSector;
				ARListEntrt->AppRequest.VolumeInfo.SectorsPerAllocationUnit = voldevext->DE_FILTER.DE_DISKVOL.FsSetting.SectorsPerAllocationUnit;
				ARListEntrt->AppRequest.VolumeInfo.TotalAllocationUnits = voldevext->DE_FILTER.DE_DISKVOL.FsSetting.TotalAllocationUnits;
				KdPrint(("SYS:VReadWriteThread:AR_VOLUME_INFO(%wZ) AvailableAllocationUnits=%I64u, BytesPerAllocationUnit=%u, BytesPerSector=%u, SectorsPerAllocationUnit=%u, TotalAllocationUnits=%I64u\n", 
					&voldevext->DE_FILTER.PhysicalDeviceName,
					ARListEntrt->AppRequest.VolumeInfo.AvailableAllocationUnits,
					ARListEntrt->AppRequest.VolumeInfo.BytesPerAllocationUnit,
					ARListEntrt->AppRequest.VolumeInfo.BytesPerSector,
					ARListEntrt->AppRequest.VolumeInfo.SectorsPerAllocationUnit,
					ARListEntrt->AppRequest.VolumeInfo.TotalAllocationUnits));
				ARListEntrt->status = STATUS_SUCCESS;
				break;
			default:
				//���������������
				ARListEntrt->status = STATUS_INVALID_DEVICE_REQUEST;
			}
			//�����¼�
			KeSetEvent(&ARListEntrt->FiniEvent, 0, FALSE);
		}
	}
	KdPrint(("SYS:VReadWriteThread go to terminate(%wZ)\n", &voldevext->DE_FILTER.PhysicalDeviceName));
	if (ClusterBuffer!=NULL) {
		ExFreePoolWithTag(ClusterBuffer, MEM_TAG_RWBUFFER);
	}
	RW_FREE_MEM(&RWBuffer);
	PsTerminateSystemThread(STATUS_SUCCESS);
}

NTSTATUS VHandleReadWrite(
	PEP_DEVICE_EXTENSION VolDevExt,
	IN ULONG MajorFunction,//�����ܺ�
	IN ULONGLONG ByteOffset,//ƫ����
	IN OUT PVOID Buffer,//Ҫ��ȡ/д��Ļ��������������Ǵض����
	IN ULONG Length//����������
	)
{
	//������������ǿ���������ģ���Ϊǰ��Ĳ����Ѿ�������ˣ�����ֱ�ӹ�����
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	//
	IO_STATUS_BLOCK iostatus;
	//��ǰƫ����
	ULONGLONG CurByteOffset;
	//��ǰ�ĳ���
	ULONG CurLength;
	//������Ϊ�����������ӿ��ٶ�
	//���������ݵ�ƫ����
	ULONGLONG SeriateByteOffset;
	//���������ݵĳ���
	ULONG SeriateLength;
	//����Ҫ��д��ƫ����
	ULONGLONG RealByteOffset;
	//Bufferƫ��ָ��
	PBYTE Bufpos;

	PAGED_CODE();

	CurLength = Length;
	CurByteOffset = ByteOffset;

	SeriateLength = 0;
	SeriateByteOffset = 0;
	Bufpos = (PBYTE)Buffer;
	while (CurLength > 0) {
		//��ô����
		RealByteOffset = VGetRealCluster(VolDevExt, CurByteOffset/VolDevExt->DE_FILTER.DE_DISKVOL.FsSetting.BytesPerAllocationUnit, MajorFunction==IRP_MJ_READ);
		
		if (RealByteOffset == GET_CLUSTER_RESULT_NO_SPACE||RealByteOffset == GET_CLUSTER_RESULT_OUT_OF_BOUND) {
			//û�пռ�
			return STATUS_INVALID_PARAMETER;
		}
		//�������ʵ��ƫ����
		RealByteOffset *= VolDevExt->DE_FILTER.DE_DISKVOL.FsSetting.BytesPerAllocationUnit;
		//��������Ĵ�����Ϊ�գ���д�뵱ǰ����ʵ��
		if (SeriateLength==0) {
			SeriateByteOffset = RealByteOffset;
			SeriateLength = VolDevExt->DE_FILTER.DE_DISKVOL.FsSetting.BytesPerAllocationUnit;
		} else {
			//�жϵ�ǰ����ʵ���Ƿ�������Ĵ�����
			if (SeriateByteOffset+SeriateLength==RealByteOffset) {
				//����
				SeriateLength+=VolDevExt->DE_FILTER.DE_DISKVOL.FsSetting.BytesPerAllocationUnit;
			} else {
				//���������Ƚ�֮ǰ��������д�������
				//ֱ�Ӵ���д��
				status = VDirectReadWriteDiskVolume(VolDevExt->DE_FILTER.PhysicalDeviceObject, 
					MajorFunction,
					SeriateByteOffset,
					Bufpos,
					SeriateLength,
					&iostatus,
					TRUE);
				if (!NT_SUCCESS(status)) {
					KdPrint(("SYS:VHandleReadWrite:VDirectReadWriteDiskVolume(%wZ)Error.(0x%08x)\n", &VolDevExt->DE_FILTER.PhysicalDeviceName, status));
					VolDevExt->DE_FILTER.DE_DISKVOL.IsRWError = TRUE;
					return status;
				}
				//ָ������ƶ�
				Bufpos+=SeriateLength;
				//�������������ص�ƫ�����ʹ�С
				SeriateByteOffset = RealByteOffset;
				SeriateLength = VolDevExt->DE_FILTER.DE_DISKVOL.FsSetting.BytesPerAllocationUnit;
			}
		}
		
		CurLength-=VolDevExt->DE_FILTER.DE_DISKVOL.FsSetting.BytesPerAllocationUnit;
		CurByteOffset+=VolDevExt->DE_FILTER.DE_DISKVOL.FsSetting.BytesPerAllocationUnit;
	}
	//��β��������������ػ���
	if (SeriateLength>0) {
		//ֱ�Ӵ���д��
		status = VDirectReadWriteDiskVolume(VolDevExt->DE_FILTER.PhysicalDeviceObject, 
			MajorFunction,
			SeriateByteOffset,
			Bufpos,
			SeriateLength,
			&iostatus,
			TRUE);
		if (!NT_SUCCESS(status)) {
			KdPrint(("SYS:VHandleReadWrite:VDirectReadWriteDiskVolume(%wZ)Error.(0x%08x)\n", &VolDevExt->DE_FILTER.PhysicalDeviceName, status));
			VolDevExt->DE_FILTER.DE_DISKVOL.IsRWError = TRUE;
			return status;
		}
		//ָ������ƶ�
		Bufpos+=SeriateLength;
		SeriateLength = 0;
		
	} else {
		ASSERT(0);
	}
	ASSERT((ULONG_PTR)Bufpos-(ULONG_PTR)Buffer == Length);
	//���
	return status;
}

ULONGLONG VGetRealCluster(
	PEP_DEVICE_EXTENSION VolDevExt, 
	ULONGLONG ClusterIndex, 
	BOOLEAN IsReadOpt
	)
{
	//������б�����Χ
	if (ClusterIndex >= VolDevExt->DE_FILTER.DE_DISKVOL.FsSetting.BitmapUsed.BitmapSize) {
		KdPrint(("SYS:VGetRealCluster:!!!%I64d>=BitmapSize(%I64d)\n", ClusterIndex, VolDevExt->DE_FILTER.DE_DISKVOL.FsSetting.BitmapUsed.BitmapSize));
		return GET_CLUSTER_RESULT_OUT_OF_BOUND;
	} 
	//�����ֱ�Ӷ�дλͼ���ҵ�����Щλͼ���򷵻�ԭʼֵ
	if (LCXLBitmapGet(&VolDevExt->DE_FILTER.DE_DISKVOL.FsSetting.BitmapDirect, ClusterIndex)) {
		return ClusterIndex;
	}
	//����ض����
	if (LCXLBitmapGet(&VolDevExt->DE_FILTER.DE_DISKVOL.FsSetting.BitmapRedirect, ClusterIndex)) {
		PLCXL_TABLE_MAP TableMap;
		LCXL_TABLE_MAP LookupMap;

		LookupMap.orgIndex = ClusterIndex;
		//�����ض�������ȥ��
		TableMap = (PLCXL_TABLE_MAP)RtlLookupElementGenericTable(&VolDevExt->DE_FILTER.DE_DISKVOL.FsSetting.RedirectTable, &LookupMap);
		if (TableMap!=NULL) {
			//�����ض���֮������
			return TableMap->mapIndex;
		} else {
			//��Ӧ���Ҳ�����
			ASSERT(0);
			return ClusterIndex;
		}
	} else {
		//����Ƕ�����
		if (IsReadOpt) {
			return ClusterIndex;
		} else {
			ULONGLONG i;
			//���Ƕ���������ʼ�����ض�����
			//Ѱ�ҵ�һ����������
			
			i = LCXLBitmapFindFreeBit(&VolDevExt->DE_FILTER.DE_DISKVOL.FsSetting.BitmapUsed, VolDevExt->DE_FILTER.DE_DISKVOL.FsSetting.LastScanIndex);
			
			//����ҵ���
			if (i<(ULONGLONG)VolDevExt->DE_FILTER.DE_DISKVOL.FsSetting.BitmapUsed.BitmapSize) {
				LCXL_TABLE_MAP TableMap;
				PVOID p;
				//���Ϊ��ʹ��
				LCXLBitmapSet(&VolDevExt->DE_FILTER.DE_DISKVOL.FsSetting.BitmapUsed, i, 1, TRUE);
				//���Ϊ���ض���
				LCXLBitmapSet(&VolDevExt->DE_FILTER.DE_DISKVOL.FsSetting.BitmapRedirect, ClusterIndex, 1, TRUE);
				//���뵽�ض������
				TableMap.orgIndex = ClusterIndex;
				TableMap.mapIndex = i;
				p = RtlInsertElementGenericTable(&VolDevExt->DE_FILTER.DE_DISKVOL.FsSetting.RedirectTable, &TableMap, sizeof(TableMap), NULL);
				ASSERT(p!=NULL);
				//KdPrint(("R %wZ:%016I64x->%016I64x\n", &VolDevExt->DE_FILTER.PhysicalDeviceName, ClusterIndex, i));
				VolDevExt->DE_FILTER.DE_DISKVOL.FsSetting.LastScanIndex = i+1;
				//���ôؼ�1
				VolDevExt->DE_FILTER.DE_DISKVOL.FsSetting.AvailableAllocationUnits--;
				//�����ض���֮���ƫ����
				return TableMap.mapIndex;
			} else {
				KdPrint(("SYS:VGetRealCluster:%wZ is full.\n", &VolDevExt->DE_FILTER.PhysicalDeviceName));
				//�д�����޷�����Ӱ�Ӵ�������ļ�����������
				VolDevExt->DE_FILTER.DE_DISKVOL.IsRWError = TRUE;
				VolDevExt->DE_FILTER.DE_DISKVOL.FsSetting.LastScanIndex = i;
				//���û���ҵ����еģ�������Ѿ�û�пռ���
				return GET_CLUSTER_RESULT_NO_SPACE;
			}
		}
		
	}
}


//////////////////////////////////////////////////////////////////////////
//LCXL
//��д����IRP�������
//////////////////////////////////////////////////////////////////////////
NTSTATUS VReadWriteSectorsCompletion( 
	IN PDEVICE_OBJECT DeviceObject, 
	IN PIRP Irp, 
	IN PVOID Context 
	) 
	/*++ 
	Routine Description: 
	A completion routine for use when calling the lower device objects to 
	which our filter deviceobject is attached. 

	Arguments: 

	DeviceObject - Pointer to deviceobject 
	Irp        - Pointer to a PnP Irp. 
	Context    - NULL or PKEVENT 
	Return Value: 

	NT Status is returned. 

	--*/ 
{ 
    PMDL    mdl; 
	
    UNREFERENCED_PARAMETER(DeviceObject); 
	
    // 
    // Free resources 
    // 
	//LCXL�����SystemBuffer�ڴ���ڣ����ͷ�
    if (Irp->AssociatedIrp.SystemBuffer && (Irp->Flags & IRP_DEALLOCATE_BUFFER)) { 
        ExFreePool(Irp->AssociatedIrp.SystemBuffer); 
    } 
	//LCXL���������ֵ����
	if (Irp->IoStatus.Status)
	{
		DbgPrint("!!!!!!!!!!Read Or Write HD Error Code====0x%x\n", Irp->IoStatus.Status);
	}
	/*
	��Ϊ��� IRP �����������ν����ģ��ϲ㱾�Ͳ�֪������ôһ�� IRP��
	��ô�������Ҿ�Ҫ�� CompleteRoutine ��ʹ�� IoFreeIrp()���ͷŵ���� IRP��
	������STATUS_MORE_PROCESSING_REQUIRED�������������ݡ�����һ��Ҫע�⣬
	�� CompleteRoutine�������غ���� IRP �Ѿ��ͷ��ˣ�
	������ʱ�������κι������ IRP �Ĳ�����ô����������Եģ��ض����� BSOD ����
	*/
    while (Irp->MdlAddress) { 
        mdl = Irp->MdlAddress; 
        Irp->MdlAddress = mdl->Next; 
        MmUnlockPages(mdl); 
        IoFreeMdl(mdl); 
    } 
	
    if (Irp->PendingReturned && (Context != NULL)) { 
        *Irp->UserIosb = Irp->IoStatus; 
		//LCXL�������¼�
        KeSetEvent((PKEVENT) Context, IO_DISK_INCREMENT, FALSE); 
    } 
	//LCXL���ͷ�IRP
    IoFreeIrp(Irp); 
	
    // 
    // Don't touch irp any more 
    // 
	//LCXL��������STATUS_MORE_PROCESSING_REQUIRED��Ϊ����ֵ
    return STATUS_MORE_PROCESSING_REQUIRED; 
} 



NTSTATUS VDirectReadWriteDiskVolume(
	IN PDEVICE_OBJECT DiskVolumeDO,//���̾��豸���� 
	IN ULONG MajorFunction,//�����ܺ�
	IN ULONGLONG ByteOffset,//ƫ����
	IN OUT PVOID Buffer,//Ҫ��ȡ/д��Ļ�����
	IN ULONG Length,//����������
	OUT PIO_STATUS_BLOCK iostatus,
	IN BOOLEAN Wait//�Ƿ�ȴ�
	)
{
	PIRP Irp; 
	NTSTATUS status;

	PAGED_CODE();

	ASSERT(MajorFunction==IRP_MJ_WRITE||MajorFunction==IRP_MJ_READ);
	Irp = IoBuildAsynchronousFsdRequest(MajorFunction, DiskVolumeDO, 
		Buffer, Length, (PLARGE_INTEGER) &ByteOffset, iostatus); 
	if (!Irp) { 
		return STATUS_INSUFFICIENT_RESOURCES; 
	} 
	//LCXL�� vista��Win7�����ϵĲ���ϵͳ ��ֱ�Ӵ���д������˱���, ����������Ҫ��IRP��FLAGS����SL_FORCE_DIRECT_WRITE��־
	/*
	If the SL_FORCE_DIRECT_WRITE flag is set, kernel-mode drivers can write to volume areas that they 
	normally cannot write to because of direct write blocking. Direct write blocking was implemented for 
	security reasons in Windows Vista and later operating systems. This flag is checked both at the file 
	system layer and storage stack layer. For more 
	information about direct write blocking, see Blocking Direct Write Operations to Volumes and Disks. 
	The SL_FORCE_DIRECT_WRITE flag is available in Windows Vista and later versions of Windows. 
	http://msdn.microsoft.com/en-us/library/ms795960.aspx
	*/
	if (IRP_MJ_WRITE == MajorFunction) {
		IoGetNextIrpStackLocation(Irp)->Flags |= SL_FORCE_DIRECT_WRITE;
	}
	if (Wait) { 
		KEVENT event; 
		KeInitializeEvent(&event, NotificationEvent, FALSE); 
		IoSetCompletionRoutine(Irp, VReadWriteSectorsCompletion, 
			&event, TRUE, TRUE, TRUE); 

		status = IoCallDriver(DiskVolumeDO, Irp); 
		if (STATUS_PENDING == status) { 
			KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL); 
			status = iostatus->Status; 
		} 
	} else { 
		IoSetCompletionRoutine(Irp, VReadWriteSectorsCompletion, 
			NULL, TRUE, TRUE, TRUE); 
		Irp->UserIosb = NULL; 
		status = IoCallDriver(DiskVolumeDO, Irp); 
	}
	return status;
}

RTL_GENERIC_COMPARE_RESULTS NTAPI VCompareRoutine(
	PRTL_GENERIC_TABLE Table,
	PVOID FirstStruct,
	PVOID SecondStruct
	)
{
	PLCXL_TABLE_MAP first = (PLCXL_TABLE_MAP) FirstStruct;
	PLCXL_TABLE_MAP second = (PLCXL_TABLE_MAP) SecondStruct;

	UNREFERENCED_PARAMETER(Table);

	if (first->orgIndex < second->orgIndex) {
		return GenericLessThan;
	} else if (first->orgIndex > second->orgIndex) {
		return GenericGreaterThan;
	} else {
		return GenericEqual;
	}
}


PVOID NTAPI VAllocateRoutine (
	PRTL_GENERIC_TABLE Table,
	CLONG ByteSize
	)
{
	/*
	��ȥ�������ҵ�ԭ���ˣ�ԭ������additional memory�����������ڴ治�������µ�����С
	For each new element, the AllocateRoutine is called to allocate memory for caller-supplied data 
	plus some additional memory for use by the Rtl...GenericTable routines. 
	
	Note that because of this "additional memory," 
	caller-supplied routines must not access the first (sizeof(RTL_SPLAY_LINKS ) + sizeof(LIST_ENTRY )) 
	bytes of any element in the generic table. 

	*/
	UNREFERENCED_PARAMETER(Table);
	UNREFERENCED_PARAMETER(ByteSize);

	return ExAllocateFromPagedLookasideList(&g_TableMapList);
}

VOID NTAPI VFreeRoutine (
	PRTL_GENERIC_TABLE Table,
	PVOID Buffer
	)
{
	UNREFERENCED_PARAMETER(Table);
	ExFreeToPagedLookasideList(&g_TableMapList, Buffer);
}

NTSTATUS VDeviceControl(
	IN PDEVICE_OBJECT DeviceObject, 
	IN PIRP Irp
	)
{
	PEP_DEVICE_EXTENSION devext = (PEP_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	PIO_STACK_LOCATION isp;

	PAGED_CODE();
	isp = IoGetCurrentIrpStackLocation( Irp );
	switch (isp->Parameters.DeviceIoControl.IoControlCode)
	{
	case IOCTL_VOLUME_ONLINE:
		KdPrint(("SYS:VDeviceControl:IOCTL_VOLUME_ONLINE(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
	case IOCTL_VOLUME_OFFLINE:
		KdPrint(("SYS:VDeviceControl:IOCTL_VOLUME_OFFLINE(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
#if (NTDDI_VERSION >= NTDDI_VISTA)
	case IOCTL_VOLUME_QUERY_MINIMUM_SHRINK_SIZE:
		//��ѯ����ѹ���Ĵ�С
		KdPrint(("SYS:VDeviceControl:IOCTL_VOLUME_QUERY_MINIMUM_SHRINK_SIZE(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
	case IOCTL_VOLUME_PREPARE_FOR_SHRINK:
		//��ʼ׼��ѹ����
		KdPrint(("SYS:VDeviceControl:IOCTL_VOLUME_PREPARE_FOR_SHRINK(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		if (devext->DE_FILTER.DE_DISKVOL.IsProtect) {
			//�ܱ���������ѹ����
			KdPrint(("SYS:VDeviceControl:IOCTL_VOLUME_PREPARE_FOR_SHRINK(%wZ) is protected, deny to shrink\n", &devext->DE_FILTER.PhysicalDeviceName));
			return LLCompleteRequest(STATUS_ACCESS_DENIED, Irp, 0);
		}
		break;
#endif
	default:
		//KdPrint(("SYS:VDeviceControl:0x%08x(%wZ)\n", isp->Parameters.DeviceIoControl.IoControlCode, &devext->DE_FILTER.PhysicalDeviceName));
		break;
	}
	return LLSendToNextDriver(devext->DE_FILTER.LowerDeviceObject, Irp);
}
