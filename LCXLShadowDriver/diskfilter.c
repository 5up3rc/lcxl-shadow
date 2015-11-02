#include "winkernel.h"
#include "driver.h"
#include "diskfilter.h"

//���̹����豸Դ�ļ������Ŵ��̹����豸�йصĺ���

//���̹����豸

//���������߳�
NTSTATUS DCreateDiskThread(
	IN PDEVICE_OBJECT DiskFilterDO
	)
{
	PEP_DEVICE_EXTENSION devext;
	NTSTATUS status;
	PAGED_CODE();

	devext = (PEP_DEVICE_EXTENSION)DiskFilterDO->DeviceExtension;
	//�ر��̱߳�ʾΪFALSE
	devext->DE_FILTER.DE_DISK.ToTerminateThread = FALSE;
	
	//��ʼ�����������������
	InitializeListHead(&devext->DE_FILTER.DE_DISK.RWListHead);
	//��ʼ���ڴ��ض����б�
	InitializeListHead(&devext->DE_FILTER.DE_DISK.RedirectListHead.ListEntry);
	//��ʼ������������
	KeInitializeSpinLock(&devext->DE_FILTER.DE_DISK.RequestListLock);
	//��ʼ����д�����¼���ͬ���¼���
	KeInitializeEvent(&devext->DE_FILTER.DE_DISK.RequestEvent, SynchronizationEvent, FALSE);
	//����ϵͳ�̣߳������Ĵ�����̾�����豸���豸����
	devext->DE_FILTER.DE_DISK.ThreadHandle = NULL;
	status = PsCreateSystemThread(&devext->DE_FILTER.DE_DISK.ThreadHandle, 
		(ACCESS_MASK)0L,
		NULL,
		NULL,
		NULL,
		DReadWriteThread, 
		DiskFilterDO);
	if (NT_SUCCESS(status)) {
		//��ȡ�߳̾��
		status = ObReferenceObjectByHandle(devext->DE_FILTER.DE_DISK.ThreadHandle,
			0,
			*PsThreadType,
			KernelMode,
			(PVOID *)&devext->DE_FILTER.DE_DISK.ThreadObject,
			NULL
			);
		if (!NT_SUCCESS(status)) {
			//���ɹ���
			devext->DE_FILTER.DE_DISK.ToTerminateThread = TRUE;
			//�����߳�
			KeSetEvent(&devext->DE_FILTER.DE_DISK.RequestEvent, 0, TRUE);
			ASSERT(0);
		}
		KdPrint(("SYS:DCreateDiskThread:PsCreateSystemThread succeed.(handle = 0x%p)\n", devext->DE_FILTER.DE_DISK.ThreadHandle));
	} else {
		KdPrint(("SYS:DCreateDiskThread:PsCreateSystemThread failed.(0x%08x)\n", status));
	}
	return status;
}

//�رմ����߳�
NTSTATUS DCloseDiskThread(
	IN PDEVICE_OBJECT DiskFilterDO
	)
{
	NTSTATUS status;
	PEP_DEVICE_EXTENSION devext;

	devext = (PEP_DEVICE_EXTENSION)DiskFilterDO->DeviceExtension;
	if (devext->DE_FILTER.DE_DISK.ThreadHandle!=NULL) {
		//������־��ΪTRUE
		devext->DE_FILTER.DE_DISK.ToTerminateThread = TRUE;
		//�����߳�
		KeSetEvent(&devext->DE_FILTER.DE_DISK.RequestEvent, 0, TRUE);

		//�ȴ��߳��˳�
		status = KeWaitForSingleObject(devext->DE_FILTER.DE_DISK.ThreadObject, UserRequest, KernelMode, FALSE, NULL);
		if (!NT_SUCCESS(status)) {
			KdPrint(("SYS:VPostDetachDiskVolDevice:KeWaitForSingleObject failed(0x%08x).\n", status));
		}
		//��������
		ObDereferenceObject(devext->DE_FILTER.DE_DISK.ThreadObject);
		//�رվ��
		ZwClose(devext->DE_FILTER.DE_DISK.ThreadHandle);
		devext->DE_FILTER.DE_DISK.ThreadHandle = NULL;
	}
	return STATUS_SUCCESS;
}

VOID DReadWriteThread(
	IN PVOID Context//�߳������ģ������Ǵ��̾������������
	)
{
	//�豸����ָ��
	PDEVICE_OBJECT DeviceObject;
	//�豸��չ
	PEP_DEVICE_EXTENSION diskdevext;

	PAGED_CODE();

	//��������̵߳����ȼ�Ϊ��ʵʱ��
	KeSetPriorityThread(KeGetCurrentThread(), LOW_REALTIME_PRIORITY);
	DeviceObject = (PDEVICE_OBJECT)Context;
	diskdevext = (PEP_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	ASSERT(diskdevext->DeviceType == DO_DISK);
	//����߳̽�����־ΪTRUE�����˳�
	while (!diskdevext->DE_FILTER.DE_DISK.ToTerminateThread) {
		//������е����
		PLIST_ENTRY	ReqEntry;

		//�ȵȴ��������ͬ���¼������������û��irp��Ҫ�������ǵ��߳̾͵ȴ�������ó�cpuʱ��������߳�
		KeWaitForSingleObject(
			&diskdevext->DE_FILTER.DE_DISK.RequestEvent,
			Executive,
			KernelMode,
			FALSE,
			NULL
			);
		//��������е��ײ��ó�һ��������׼����������ʹ�������������ƣ����Բ����г�ͻ
		while (ReqEntry = ExInterlockedRemoveHeadList(&diskdevext->DE_FILTER.DE_DISK.RWListHead, &diskdevext->DE_FILTER.DE_DISK.RequestListLock), ReqEntry != NULL) {
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
			if (IrpSp->MajorFunction == IRP_MJ_READ) {
				//�ض��������
				KdPrint((
					"SYS:DReadWriteThread:Read disk %wZ outside, offset=0x%016I64x, length=%d\n", 
					&diskdevext->DE_FILTER.PhysicalDeviceName, 
					IrpSp->Parameters.Read.ByteOffset.QuadPart, 
					IrpSp->Parameters.Read.Length
					));
				status = LLWaitIRPCompletion(diskdevext->DE_FILTER.LowerDeviceObject, Irp);
				if (NT_SUCCESS(status)) {
					LCXLReadFromRedirectRWMemList(
						&diskdevext->DE_FILTER.DE_DISK.RedirectListHead, 
						buffer,
						(ULONGLONG)IrpSp->Parameters.Read.ByteOffset.QuadPart, 
						IrpSp->Parameters.Read.Length
						);
				} else {
					KdPrint(("SYS:DReadWriteThread:LLWaitIRPCompletion Error:0x%08x\n", status));
				}
				LLCompleteRequest(status, Irp, Irp->IoStatus.Information);
			} else {
				//�ض��������
				KdPrint((
					"SYS:DReadWriteThread:Write disk %wZ outside, offset=0x%016I64x, length=%d\n", 
					&diskdevext->DE_FILTER.PhysicalDeviceName, 
					IrpSp->Parameters.Write.ByteOffset.QuadPart, 
					IrpSp->Parameters.Write.Length
					));
				//�ض���д����
				LCXLInsertToRedirectRWMemList(
					&diskdevext->DE_FILTER.DE_DISK.RedirectListHead, 
					buffer, 
					(ULONGLONG)IrpSp->Parameters.Write.ByteOffset.QuadPart, 
					IrpSp->Parameters.Write.Length
					);
#ifdef DBG
				{
					PREDIRECT_RW_MEM RedirectListEntry;
					ULONG TotalSize = 0;

					RedirectListEntry = &diskdevext->DE_FILTER.DE_DISK.RedirectListHead;
					//��д���ض��򣬲������ܵĺϲ��б�
					for (RedirectListEntry = (PREDIRECT_RW_MEM)RedirectListEntry->ListEntry.Flink; RedirectListEntry != &diskdevext->DE_FILTER.DE_DISK.RedirectListHead;RedirectListEntry = (PREDIRECT_RW_MEM)RedirectListEntry->ListEntry.Flink) {
						TotalSize += RedirectListEntry->length;
					}
					DbgPrint("SYS:DReadWriteThread:TotalSize=%x\n", TotalSize);
				}
#endif
				//������ֱ�ӳɹ���ɣ�����ƭϵͳ
				LLCompleteRequest(STATUS_SUCCESS, Irp, IrpSp->Parameters.Write.Length);
			}
			//-----------
		}
	}
	PsTerminateSystemThread(STATUS_SUCCESS);
}

//����д��hook����
NTSTATUS DHookDeviceWrite(
	IN PDEVICE_OBJECT DeviceObject, 
	IN PIRP Irp
	)
{
	PDEVICE_OBJECT diskFDO;
	//�鿴���ص��豸
	diskFDO = DeviceObject->AttachedDevice;
	//��ʼѭ������
	while (diskFDO!=NULL) {
		//�ж����ǵ������Ƿ��ڶ�ջ��
		if (diskFDO->DriverObject == g_CDO->DriverObject) {
			break;
		}
		diskFDO = diskFDO->AttachedDevice;
	}
	//�ҵ���DISK�豸
	if (diskFDO!=NULL) {
		//�ж�IRP��ģʽ�ǲ����ں�ģʽ������Ƿ�ֹring3Ӧ�ó�����ƻ�
		//IoGetRequestorProcessId��Ϊ�˷�ֹ������������Դ��̵��ƻ�
		if (Irp->RequestorMode == KernelMode&&IoGetRequestorProcessId(Irp)<=4||g_DriverSetting.CurShadowMode == SM_NONE) {
			return g_DriverSetting.DiskInfo.OrgMFWrite(DeviceObject, Irp);
		} else {
			//ʧ�ܰɣ��ף���û��Ȩ��Ŷ
			KdPrint(("SYS:VDeviceWrite(%d):Irp->RequestorMode != KernelMode\n", IoGetRequestorProcessId(Irp)));
		}
	} else {
		//���������ˣ�ʧ��
		KdPrint(("SYS:DHookDeviceWrite:LCXLShadow Filter Device not Found!!!!\n"));
	}
	return LLCompleteRequest(STATUS_ACCESS_DENIED, Irp, 0);
}

//Hook DISK ������WRITE����
__inline VOID DHookDiskMJWriteFunc()
{
	PDRIVER_DISPATCH WriteFunc;

	WriteFunc = g_DriverSetting.DiskInfo.DiskDriverObject->MajorFunction[IRP_MJ_WRITE];
	if (WriteFunc != DHookDeviceWrite) {
		//��ȡԭʼ��DISK��д������
		g_DriverSetting.DiskInfo.OrgMFWrite = WriteFunc;
		//�滻DISK��д������
		g_DriverSetting.DiskInfo.DiskDriverObject->MajorFunction[IRP_MJ_WRITE] = DHookDeviceWrite;
	}
}

VOID DPostAttachDiskDevice(
	IN PDEVICE_OBJECT FilterDeviceObject//�ļ�ϵͳ�����豸 
	)
{
	PEP_DEVICE_EXTENSION devext;
	NTSTATUS status;

	PAGED_CODE();
	devext = (PEP_DEVICE_EXTENSION)FilterDeviceObject->DeviceExtension;
	if (!g_DriverSetting.IsFirstGetDiskInfo) {
		UNICODE_STRING DiskDriverName;
		PDEVICE_OBJECT diskFDO;

		g_DriverSetting.IsFirstGetDiskInfo = TRUE;
		RtlInitUnicodeString(&DiskDriverName, L"\\Driver\\Disk");
		diskFDO = devext->DE_FILTER.PhysicalDeviceObject;
		
		while (diskFDO!=NULL) {
			//�ж��ǲ���Disk���豸
			if (RtlCompareUnicodeString(&diskFDO->DriverObject->DriverName, &DiskDriverName, TRUE)==0) {
				break;
			}
			diskFDO = diskFDO->AttachedDevice;
		}
		
		//�ҵ���DISK��ײ��豸
		if (diskFDO!=NULL) {
			//��ȡDISK��������
			g_DriverSetting.DiskInfo.DiskDriverObject = diskFDO->DriverObject;
			DHookDiskMJWriteFunc();
		}
		//��ȡ����ID
		status = DiskIdentifyDevice(devext->DE_FILTER.LowerDeviceObject, &g_DriverSetting.DiskInfo.DiskIDInfo);
		if (NT_SUCCESS(status)) {

		} else {
			//fix-it:Ŀǰ���ܻ�ȡSCSIӲ�̵����к�
			//Ŀǰ������
			RtlZeroMemory(&g_DriverSetting.DiskInfo.DiskIDInfo, sizeof(g_DriverSetting.DiskInfo.DiskIDInfo));
			KdPrint(("!!!SYS:DPostAttachDiskDevice:DiskIdentifyDevice failed: 0x%08x\n", status));
		}
	} else {
		return;
	}
}

NTSTATUS DDeviceRead(
	IN PDEVICE_OBJECT DeviceObject, 
	IN PIRP Irp
	)
{
	PEP_DEVICE_EXTENSION diskdevext;
	//Windows 8 ��,�˺�����IRQ�����APC_LEVEL,�������Ҫɾ��PAGED_CODE();
	//PAGED_CODE();

	diskdevext = (PEP_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	if (diskdevext->DE_FILTER.DE_DISK.InitializeStatus == DIS_MEMORY) {
		PIO_STACK_LOCATION irpsp;
		KIRQL OldIrql;
		BOOLEAN IsSafeOpt;//�Ƿ��ǰ�ȫ
		ULONG i;

		irpsp = IoGetCurrentIrpStackLocation(Irp);
		IsSafeOpt = FALSE;
		KeAcquireSpinLock(&diskdevext->DE_FILTER.DE_DISK.RequestListLock, &OldIrql);
		for (i = 0; i < diskdevext->DE_FILTER.DE_DISK.DiskVolFilterNum; i++) {
			//���IRP��д�����ڴ��̾�������ڣ��������
			if (irpsp->Parameters.Read.ByteOffset.QuadPart>=diskdevext->DE_FILTER.DE_DISK.DiskVolFilterDO[i].StartingOffset.QuadPart&&
				irpsp->Parameters.Read.ByteOffset.QuadPart+irpsp->Parameters.Read.Length<=diskdevext->DE_FILTER.DE_DISK.DiskVolFilterDO[i].StartingOffset.QuadPart+diskdevext->DE_FILTER.DE_DISK.DiskVolFilterDO[i].ExtentLength.QuadPart) {
					IsSafeOpt = TRUE;
					break;
			}
		}
		KeReleaseSpinLock(&diskdevext->DE_FILTER.DE_DISK.RequestListLock, OldIrql);
		if (!IsSafeOpt) {
			//��IRP����Ϊ�ȴ�״̬
			IoMarkIrpPending(Irp);
			//Ȼ�����irp�Ž���Ӧ�����������
			ExInterlockedInsertTailList(
				&diskdevext->DE_FILTER.DE_DISK.RWListHead,
				&Irp->Tail.Overlay.ListEntry,
				&diskdevext->DE_FILTER.DE_DISK.RequestListLock
				);
			//���ö��еĵȴ��¼���֪ͨ���ж����irp���д���
			KeSetEvent(
				&diskdevext->DE_FILTER.DE_DISK.RequestEvent, 
				(KPRIORITY)0, 
				FALSE);
			//����pending״̬�����irp���㴦������
			return STATUS_PENDING;
		}
	}
	return LLSendToNextDriver(diskdevext->DE_FILTER.LowerDeviceObject, Irp);
}

NTSTATUS DDeviceWrite(
	IN PDEVICE_OBJECT DeviceObject, 
	IN PIRP Irp
	)
{
	PEP_DEVICE_EXTENSION diskdevext;

	//Windows 8 ��,�˺�����IRQ�����APC_LEVEL,�������Ҫɾ��PAGED_CODE();
	//PAGED_CODE();

	diskdevext = (PEP_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	if (diskdevext->DE_FILTER.DE_DISK.InitializeStatus == DIS_MEMORY) {
		KIRQL OldIrql;
		PIO_STACK_LOCATION irpsp;
		BOOLEAN IsSafeOpt;//�Ƿ��ǰ�ȫ
		ULONG i;

		irpsp = IoGetCurrentIrpStackLocation(Irp);
		IsSafeOpt = FALSE;
		//PAGED_CODE();
		KeAcquireSpinLock(&diskdevext->DE_FILTER.DE_DISK.RequestListLock, &OldIrql);
		for (i = 0; i < diskdevext->DE_FILTER.DE_DISK.DiskVolFilterNum; i++) {
			//���IRP��д�����ڴ��̾�������ڣ��������
			if (irpsp->Parameters.Write.ByteOffset.QuadPart>=diskdevext->DE_FILTER.DE_DISK.DiskVolFilterDO[i].StartingOffset.QuadPart&&
				irpsp->Parameters.Write.ByteOffset.QuadPart+irpsp->Parameters.Write.Length<=diskdevext->DE_FILTER.DE_DISK.DiskVolFilterDO[i].StartingOffset.QuadPart+diskdevext->DE_FILTER.DE_DISK.DiskVolFilterDO[i].ExtentLength.QuadPart) {
					IsSafeOpt = TRUE;
					break;
			}
		}
		KeReleaseSpinLock(&diskdevext->DE_FILTER.DE_DISK.RequestListLock, OldIrql);
		if (!IsSafeOpt) {
			diskdevext->DE_FILTER.DE_DISK.IsVolumeChanged = TRUE;
			//��IRP����Ϊ�ȴ�״̬
			IoMarkIrpPending(Irp);
			//Ȼ�����irp�Ž���Ӧ�����������
			ExInterlockedInsertTailList(
				&diskdevext->DE_FILTER.DE_DISK.RWListHead,
				&Irp->Tail.Overlay.ListEntry,
				&diskdevext->DE_FILTER.DE_DISK.RequestListLock
				);
			//���ö��еĵȴ��¼���֪ͨ���ж����irp���д���
			KeSetEvent(
				&diskdevext->DE_FILTER.DE_DISK.RequestEvent, 
				(KPRIORITY)0, 
				FALSE);
			//����pending״̬�����irp���㴦������
			return STATUS_PENDING;
		} else {
			IsSafeOpt = TRUE;
		}
	}
	//���HOOK�Ƿ񱻻ָ���������ָ���������HOOK
	DHookDiskMJWriteFunc();
	return LLSendToNextDriver(diskdevext->DE_FILTER.LowerDeviceObject, Irp);
}

NTSTATUS DDevicePnp(
	IN PDEVICE_OBJECT DeviceObject, 
	IN PIRP Irp
	)
{
	PEP_DEVICE_EXTENSION devext = (PEP_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	PIO_STACK_LOCATION isp;
	NTSTATUS status;

	PAGED_CODE();
	isp = IoGetCurrentIrpStackLocation( Irp );
	switch(isp->MinorFunction) 
	{
	case IRP_MN_START_DEVICE:
		//�����豸
		KdPrint(("SYS:DDevicePnp:IRP_MN_START_DEVICE(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		status = LLWaitIRPCompletion(devext->DE_FILTER.LowerDeviceObject, Irp);
		if (NT_SUCCESS(status)) {
			KEVENT event;
			PIRP QueryIrp;
			NTSTATUS localstatus;
			IO_STATUS_BLOCK iostatus;
			PSTORAGE_DEVICE_DESCRIPTOR DeviceDescriptor;//����������Ϣ
			ULONG DeviceDescriptorSize;
			STORAGE_DEVICE_NUMBER DeviceNumber;

			//����Ŀ���豸���õı��
			DeviceObject->Characteristics |= devext->DE_FILTER.LowerDeviceObject->Characteristics & (FILE_REMOVABLE_MEDIA | FILE_READ_ONLY_DEVICE | FILE_FLOPPY_DISKETTE); 
			//��ʼ��
			devext->DE_FILTER.DE_DISK.DiskNumber = (ULONG)-1;
			KeInitializeEvent(&event, NotificationEvent, FALSE);
			
			//
			// ��ѯ�������
			//
			QueryIrp = IoBuildDeviceIoControlRequest(
				IOCTL_STORAGE_GET_DEVICE_NUMBER,
				devext->DE_FILTER.LowerDeviceObject,
				NULL,
				0,
				&DeviceNumber,
				sizeof(DeviceNumber),
				FALSE,
				&event,
				&iostatus);

			if (QueryIrp != NULL) {
				localstatus = IoCallDriver(devext->DE_FILTER.LowerDeviceObject, QueryIrp);
				if (localstatus == STATUS_PENDING) {
					KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
					localstatus = iostatus.Status;
				}
				if (NT_SUCCESS(localstatus)) {
					devext->DE_FILTER.DE_DISK.DiskNumber = DeviceNumber.DeviceNumber;
				} else {
					KdPrint(("SYS:DDevicePnp(%wZ) IOCTL_STORAGE_GET_DEVICE_NUMBER failed(0x%08x)\n", &devext->DE_FILTER.PhysicalDeviceName, localstatus));
				}
			} else {
				//��Դ����
				localstatus = STATUS_INSUFFICIENT_RESOURCES;
			}
			//������ɹ���ʹ����������
			if (!NT_SUCCESS(localstatus)) {
				VOLUME_NUMBER VolumeNumber;

				//����¼�
				KeClearEvent(&event);
				RtlZeroMemory(&VolumeNumber, sizeof(VOLUME_NUMBER));
				//��ѯ��
				QueryIrp = IoBuildDeviceIoControlRequest(
					IOCTL_VOLUME_QUERY_VOLUME_NUMBER,
					devext->DE_FILTER.LowerDeviceObject, 
					NULL, 
					0,
					&VolumeNumber, 
					sizeof(VolumeNumber), 
					FALSE, 
					&event, 
					&iostatus);
				if (QueryIrp!=NULL) {
					localstatus = IoCallDriver(devext->DE_FILTER.LowerDeviceObject, QueryIrp);
					if (localstatus == STATUS_PENDING) {
						KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
						localstatus = iostatus.Status;
					}
					if (NT_SUCCESS(localstatus)) {
						devext->DE_FILTER.DE_DISK.DiskNumber = VolumeNumber.VolumeNumber;
					}
				} else {
					//��Դ����
					localstatus = STATUS_INSUFFICIENT_RESOURCES;
				}
			}
			
			//����¼�
			KeClearEvent(&event);
			DeviceDescriptorSize =  sizeof(STORAGE_DEVICE_DESCRIPTOR)+1024;
			DeviceDescriptor = (PSTORAGE_DEVICE_DESCRIPTOR)ExAllocatePoolWithTag(NonPagedPool, DeviceDescriptorSize, 'DESL');
			if (DeviceDescriptor != NULL) {
				PSTORAGE_PROPERTY_QUERY Query; //   ��ѯ������� 
				ULONG QuerySize;

				RtlZeroMemory(DeviceDescriptor, DeviceDescriptorSize);
				DeviceDescriptor->Size = DeviceDescriptorSize;

				QuerySize = sizeof(STORAGE_PROPERTY_QUERY)+1024;
				Query = (PSTORAGE_PROPERTY_QUERY)ExAllocatePoolWithTag(NonPagedPool, QuerySize, 'QUEY');

				RtlZeroMemory(Query, QuerySize);
				//���ò�ѯ����
				Query->PropertyId = StorageDeviceProperty;
				Query->QueryType = PropertyStandardQuery;
				//��ѯ���ԣ�������Ҫ��������Ϣ
				QueryIrp = IoBuildDeviceIoControlRequest(
					IOCTL_STORAGE_QUERY_PROPERTY,
					devext->DE_FILTER.PhysicalDeviceObject,
					Query,
					QuerySize,
					DeviceDescriptor,
					DeviceDescriptorSize,
					FALSE,
					&event,
					&iostatus);

				if (QueryIrp != NULL) {
					localstatus = IoCallDriver(devext->DE_FILTER.PhysicalDeviceObject, QueryIrp);
					if (localstatus == STATUS_PENDING) {
						KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
						localstatus = iostatus.Status;
					}
					if (NT_SUCCESS(localstatus)) {
						devext->DE_FILTER.DE_DISK.BusType = DeviceDescriptor->BusType;
					} else {
						KdPrint(("SYS:DDevicePnp(%wZ) IOCTL_STORAGE_QUERY_PROPERTY failed(0x%08x)\n", &devext->DE_FILTER.PhysicalDeviceName, localstatus));
					}
				}
				ExFreePool(Query);
				ExFreePool(DeviceDescriptor);
			} else {
				KdPrint(("SYS:DDevicePnp(%wZ) ExAllocatePoolWithTag failed\n", &devext->DE_FILTER.PhysicalDeviceName));
			}
			//����¼�
			KeClearEvent(&event);
			//��ѯ����������С
			QueryIrp = IoBuildDeviceIoControlRequest(
				IOCTL_DISK_GET_DRIVE_GEOMETRY,
				devext->DE_FILTER.LowerDeviceObject,
				NULL,
				0,
				&devext->DE_FILTER.DE_DISK.DiskGeometry,
				sizeof(devext->DE_FILTER.DE_DISK.DiskGeometry),
				FALSE,
				&event,
				&iostatus);
			if (QueryIrp != NULL) {
				localstatus = IoCallDriver(devext->DE_FILTER.LowerDeviceObject, QueryIrp);
				if (localstatus == STATUS_PENDING) {
					KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
					localstatus = iostatus.Status;
				}
				if (NT_SUCCESS(localstatus)) {
					KdPrint(("SYS:DDevicePnp(%wZ) IOCTL_DISK_GET_DRIVE_GEOMETRY, BytesPerSector=%d\n", &devext->DE_FILTER.PhysicalDeviceName, devext->DE_FILTER.DE_DISK.DiskGeometry.BytesPerSector));
				} else {
					KdPrint(("SYS:DDevicePnp(%wZ) IOCTL_DISK_GET_DRIVE_GEOMETRY failed!(0x%08x)\n", &devext->DE_FILTER.PhysicalDeviceName, localstatus));
				}
			}
			if (DIS_LOCAL_DISK(devext->DE_FILTER.DE_DISK.BusType)) {
				DCreateDiskThread(DeviceObject);
			}
			/*else {
				KdPrint(("SYS:DDevicePnp:IRP_MN_START_DEVICE(%wZ) is not a local disk, detach it\n", &devext->DE_FILTER.PhysicalDeviceName));
				//���Ǳ��ؾ���ɾ�������豸
				IoDetachDevice(devext->DE_FILTER.LowerDeviceObject);
				//������ڹ����豸����Ҫɾ�������Ⲣ����������ɾ����ֻ�Ǽ��������ã����Է���devext�ǰ�ȫ�ġ�
				IoDeleteDevice(DeviceObject);
			}*/
		}
		return LLCompleteRequest(status, Irp, Irp->IoStatus.Information);
		break;
		/*
	case IRP_MN_QUERY_REMOVE_DEVICE:
		KdPrint(("SYS:DDevicePnp:IRP_MN_QUERY_REMOVE_DEVICE(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
		*/
	case IRP_MN_REMOVE_DEVICE:
		KdPrint(("SYS:DDevicePnp:IRP_MN_REMOVE_DEVICE(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		IoDetachDevice(devext->DE_FILTER.LowerDeviceObject);
		if (DIS_LOCAL_DISK(devext->DE_FILTER.DE_DISK.BusType)) {
			//�ر��豸
			DCloseDiskThread(DeviceObject);
		}
		//������ڹ����豸����Ҫɾ�������Ⲣ����������ɾ����ֻ�Ǽ��������ã����Է���devext�ǰ�ȫ�ġ�
		IoDeleteDevice(DeviceObject);
		break;
		/*
	case IRP_MN_CANCEL_REMOVE_DEVICE:
		KdPrint(("SYS:DDevicePnp:IRP_MN_CANCEL_REMOVE_DEVICE(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
		*/
	case IRP_MN_STOP_DEVICE:
		KdPrint(("SYS:DDevicePnp:IRP_MN_STOP_DEVICE(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		status = LLWaitIRPCompletion(devext->DE_FILTER.LowerDeviceObject, Irp);
		if (NT_SUCCESS(status)) {
		}
		return LLCompleteRequest(status, Irp, Irp->IoStatus.Information);
		break;
		/*
	case IRP_MN_QUERY_STOP_DEVICE:
		KdPrint(("SYS:DDevicePnp:IRP_MN_QUERY_STOP_DEVICE(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
	case IRP_MN_CANCEL_STOP_DEVICE:
		KdPrint(("SYS:DDevicePnp:IRP_MN_CANCEL_STOP_DEVICE(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;

	case IRP_MN_QUERY_DEVICE_RELATIONS:
		KdPrint(("SYS:DDevicePnp:IRP_MN_QUERY_DEVICE_RELATIONS-%s(%wZ)\n", qrydevtypestrlst[isp->Parameters.QueryDeviceRelations.Type], &devext->DE_FILTER.PhysicalDeviceName));
		break;
	case IRP_MN_QUERY_INTERFACE:
		KdPrint(("SYS:DDevicePnp:IRP_MN_QUERY_INTERFACE(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
	case IRP_MN_QUERY_CAPABILITIES:
		KdPrint(("SYS:DDevicePnp:IRP_MN_QUERY_CAPABILITIES(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
	case IRP_MN_QUERY_RESOURCES:
		KdPrint(("SYS:DDevicePnp:IRP_MN_QUERY_RESOURCES(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
	case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
		KdPrint(("SYS:DDevicePnp:IRP_MN_QUERY_RESOURCE_REQUIREMENTS(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
	case IRP_MN_QUERY_DEVICE_TEXT:
		KdPrint(("SYS:DDevicePnp:IRP_MN_QUERY_DEVICE_TEXT(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
	case IRP_MN_FILTER_RESOURCE_REQUIREMENTS:
		KdPrint(("SYS:DDevicePnp:IRP_MN_FILTER_RESOURCE_REQUIREMENTS(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;

	case IRP_MN_READ_CONFIG:
		KdPrint(("SYS:DDevicePnp:IRP_MN_READ_CONFIG(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
	case IRP_MN_WRITE_CONFIG:
		KdPrint(("SYS:DDevicePnp:IRP_MN_WRITE_CONFIG(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
	case IRP_MN_EJECT:
		KdPrint(("SYS:DDevicePnp:IRP_MN_EJECT(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
	case IRP_MN_SET_LOCK:
		KdPrint(("SYS:DDevicePnp:IRP_MN_SET_LOCK(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
	case IRP_MN_QUERY_ID:
		KdPrint(("SYS:DDevicePnp:IRP_MN_QUERY_ID(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
	case IRP_MN_QUERY_PNP_DEVICE_STATE:
		KdPrint(("SYS:DDevicePnp:IRP_MN_QUERY_PNP_DEVICE_STATE(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
	case IRP_MN_QUERY_BUS_INFORMATION:
		KdPrint(("SYS:DDevicePnp:IRP_MN_QUERY_BUS_INFORMATION(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
	case IRP_MN_DEVICE_USAGE_NOTIFICATION:
		KdPrint(("SYS:DDevicePnp:IRP_MN_DEVICE_USAGE_NOTIFICATION(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
	case IRP_MN_SURPRISE_REMOVAL:
		KdPrint(("SYS:DDevicePnp:IRP_MN_SURPRISE_REMOVAL(%wZ)\n", &devext->DE_FILTER.PhysicalDeviceName));
		break;
		*/
	}

	return LLSendToNextDriver(devext->DE_FILTER.LowerDeviceObject, Irp);
}

NTSTATUS DDeviceControl(
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
	case IOCTL_DISK_DELETE_DRIVE_LAYOUT://ɾ��������
	case IOCTL_DISK_SET_DRIVE_LAYOUT_EX://���÷�����Ϣ
	case IOCTL_DISK_SET_DRIVE_LAYOUT://���÷�����Ϣ
	case IOCTL_DISK_GROW_PARTITION://��չ����
	case IOCTL_DISK_SET_PARTITION_INFO_EX://���÷�����Ϣ
	case IOCTL_DISK_SET_PARTITION_INFO://���÷�����Ϣ
	case IOCTL_DISK_VERIFY://������֤��ִ��д0����
		KdPrint(("SYS:DDeviceControl:IOCTL_0x%08x(%wZ)\n", isp->Parameters.DeviceIoControl.IoControlCode, &devext->DE_FILTER.PhysicalDeviceName));
		if (DIS_LOCAL_DISK(devext->DE_FILTER.DE_DISK.BusType)) {
			BOOLEAN HasShadowVolume;//�Ƿ��б�����
			ULONG i;
			PDEVICE_OBJECT DiskVolFilterDO;
	
			HasShadowVolume = FALSE;
			for (i = 0; i < devext->DE_FILTER.DE_DISK.DiskVolFilterNum; i++) {
				PEP_DEVICE_EXTENSION voldevext;

				DiskVolFilterDO = devext->DE_FILTER.DE_DISK.DiskVolFilterDO[i].FilterDO;
				voldevext = (PEP_DEVICE_EXTENSION)DiskVolFilterDO->DeviceExtension;
				//�Ƿ��б������ڴ�����
				HasShadowVolume = HasShadowVolume||voldevext->DE_FILTER.DE_DISKVOL.IsProtect;
			}
			//����б��������ֹ�˲���
			if (HasShadowVolume) {
				return LLCompleteRequest(STATUS_ACCESS_DENIED, Irp, 0);
			}
		}
		break;
	}
	return LLSendToNextDriver(devext->DE_FILTER.LowerDeviceObject, Irp);
}