#include "winkernel.h"
#include "driver.h"
#include "fatlbr.h"
#include "filesysfilter.h"
#include "diskvolfilter.h"



//��ȡ��һ����������ƫ����
NTSTATUS FGetFirstDataSectorOffset(
	IN HANDLE FSDeviceHandle, 
	OUT PULONGLONG FirstDataSector
	)
{
	IO_STATUS_BLOCK ioBlock;
	NTSTATUS status;
	FAT_LBR		fatLBR = { 0 };
	LARGE_INTEGER	pos;

	PAGED_CODE();

	ASSERT(FirstDataSector != NULL);
	*FirstDataSector = 0;
	pos.QuadPart = 0;
	//LCXL���������ָ��Ϊ0���򷵻ز�����(STATUS_NOT_FOUND)
	//LCXL����ȡ�ļ���ȡ��Ϣ
	status = ZwReadFile(FSDeviceHandle, NULL, NULL, NULL, &ioBlock, &fatLBR, sizeof(fatLBR), &pos, NULL);
	//LCXL����ȡ�ļ���ȡ��Ϣ
	if (NT_SUCCESS(status) && sizeof(FAT_LBR) == ioBlock.Information) {
		DWORD dwRootDirSectors	= 0;
		DWORD dwSectorNumPreCluster	= 0;

		// Validate jump instruction to boot code. This field has two
		// allowed forms: 
		// jmpBoot[0] = 0xEB, jmpBoot[1] = 0x??, jmpBoot[2] = 0x90 
		// and
		// jmpBoot[0] = 0xE9, jmpBoot[1] = 0x??, jmpBoot[2] = 0x??
		// 0x?? indicates that any 8-bit value is allowed in that byte.
		// JmpBoot[0] = 0xEB is the more frequently used format.
		if (fatLBR.wTrailSig == 0xAA55 && (fatLBR.Jump[0] == 0xEB && fatLBR.Jump[2] == 0x90 || fatLBR.Jump[0] == 0xE9|| fatLBR.Jump[0] == 0x49)) {
			// Compute first sector offset for the FAT volumes:		
			// First, we determine the count of sectors occupied by the
			// root directory. Note that on a FAT32 volume the BPB_RootEntCnt
			// value is always 0, so on a FAT32 volume dwRootDirSectors is
			// always 0. The 32 in the above is the size of one FAT directory
			// entry in bytes. Note also that this computation rounds up.
			//LCXL���������Ҷ�ԭ��ע�͵ķ���
			//����FAT��ĵ�һ������
			//���ȣ�����ͨ����Ŀ¼ȷ����ռ�õ�����������
			//ע�⣺FAT32���BPB_RootEntCnt����0
			dwRootDirSectors = ((( fatLBR.bpb.RootEntries * 32 ) + ( fatLBR.bpb.BytesPerSector - 1 ))/fatLBR.bpb.BytesPerSector );
			// The start of the data region, the first sector of cluster 2,
			// is computed as follows:
			//�������Ŀ�ʼ��
			//�������£�
			dwSectorNumPreCluster = fatLBR.bpb.SectorsPerFat;		
			if( !dwSectorNumPreCluster ) {
				//��FAT32
				dwSectorNumPreCluster = fatLBR.ebpb32.LargeSectorsPerFat;
			}
			if( dwSectorNumPreCluster ) {
				// �õ���������ʼ�����ǵ�һ�ص�λ��
				//����������ʼ����=FAT��������+FAT���ݿ�*һ���ص�������+��Ŀ¼������
				*FirstDataSector = ( fatLBR.bpb.ReservedSectors + ( fatLBR.bpb.Fats * dwSectorNumPreCluster ) + dwRootDirSectors );

			} else {
				//��NTFS
			}
		} else {
			//status = STATUS_NOT_FOUND;
		}
	}

	return status;
}

//�ļ������豸
NTSTATUS FDeviceCreate(
	IN PDEVICE_OBJECT DeviceObject, 
	IN PIRP Irp
	)
{
	PEP_DEVICE_EXTENSION devext;
	//PIO_STACK_LOCATION isp;
	//PFILE_OBJECT FileObject;
	PAGED_CODE();

	devext = (PEP_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

	return LLSendToNextDriver(devext->DE_FILTER.LowerDeviceObject, Irp);
}

NTSTATUS FDeviceCleanup(
	IN PDEVICE_OBJECT DeviceObject, 
	IN PIRP Irp
	)
{
	PEP_DEVICE_EXTENSION devext = (PEP_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

	PAGED_CODE();

	return LLSendToNextDriver(devext->DE_FILTER.LowerDeviceObject, Irp);
}

NTSTATUS FDeviceClose(
	IN PDEVICE_OBJECT DeviceObject, 
	IN PIRP Irp
	)
{
	PEP_DEVICE_EXTENSION devext = (PEP_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

	PAGED_CODE();

	return LLSendToNextDriver(devext->DE_FILTER.LowerDeviceObject, Irp);
}

//�Ƿ���Ҫ����������豸
//�������False���򲻹��ش��豸
//ע�⣺�п��ܻ�����ѹ��ص��豸����������أ���Ҫ���б��
BOOL FPreAttachVolumeDevice(
	IN PDEVICE_OBJECT VolumeDeviceObject,//���豸����
	IN PDEVICE_OBJECT DiskDeviceObject,//�����豸����
	IN PUNICODE_STRING DiskDeviceName///�����豸����
	)
{
	PAGED_CODE();

	UNREFERENCED_PARAMETER(DiskDeviceName);
	UNREFERENCED_PARAMETER(DiskDeviceObject);
	UNREFERENCED_PARAMETER(VolumeDeviceObject);

	KdPrint(("SYS:FPreAttachVolumeDevice:DiskDeviceName=%wZ\n", DiskDeviceName));	
	return TRUE;
}

//���سɹ�ʱ�����ô˺���
VOID FPostAttachVolumeDevice(
	IN PDEVICE_OBJECT FilterDeviceObject//�ļ�ϵͳ�����豸
	)
{
	NTSTATUS status;
	PEP_DEVICE_EXTENSION fsdevext;
	PDEVICE_OBJECT devobj;
	PEP_DEVICE_EXTENSION voldevext;

	PAGED_CODE();

	fsdevext = (PEP_DEVICE_EXTENSION)FilterDeviceObject->DeviceExtension;
	KdPrint(("SYS(%d):FPostAttachVolumeDevice:DiskDeviceName=%wZ\n", PsGetCurrentProcessId(), &fsdevext->DE_FILTER.DE_FSVOL.DiskVolDeviceName));

	//Ѱ������Ӧ���²���̾�����豸

	//��ȡ�����е�һ���豸���󣬿�ʼ����
	devobj = FilterDeviceObject->DriverObject->DeviceObject;
	while (devobj != NULL) {
		voldevext = (PEP_DEVICE_EXTENSION)devobj->DeviceExtension;
		//����ҵ����豸�Ǵ��̾�����豸
		if (voldevext->DeviceType==DO_DISKVOL) {
			//����ҵ����²�Ĵ��̾�����豸
			if (voldevext->DE_FILTER.PhysicalDeviceObject == fsdevext->DE_FILTER.DE_FSVOL.DiskVolDeviceObject) {
				break;
			}
		}
		//û���ҵ�������һ���豸
		devobj = devobj->NextDevice;
	}
	//�ҵ��²��豸��
	if (devobj != NULL) {
		HANDLE hThread;

		voldevext = (PEP_DEVICE_EXTENSION)devobj->DeviceExtension;
		//�����ļ�ϵͳ�����豸�ʹ��̾�����豸�Ĺ�ϵ
		fsdevext->DE_FILTER.DE_FSVOL.DiskVolFilter = devobj;
		voldevext->DE_FILTER.DE_DISKVOL.FSVolFilter = FilterDeviceObject;

		if (g_DriverSetting.InitState == DS_NONE) {
			//��ʼ��ʼ��
			g_DriverSetting.InitState = DS_INITIALIZING;
			KdPrint(("SYS:FPostAttachVolumeDevice:The volume %wZ is(maybe) system volume\n", &fsdevext->DE_FILTER.DE_FSVOL.DiskVolDeviceName));
			status = PsCreateSystemThread(&hThread, 
				(ACCESS_MASK)0L,
				NULL,
				NULL,
				NULL,
				VGetDriverSettingThread, 
				FilterDeviceObject);
			if (NT_SUCCESS(status)) {
				ZwClose(hThread);
			}
		}

		if (voldevext->DE_FILTER.DE_DISKVOL.InitializeStatus==DVIS_NONE||!voldevext->DE_FILTER.DE_DISKVOL.IsProtect) {
			//���ó�ʼ��״̬
			voldevext->DE_FILTER.DE_DISKVOL.InitializeStatus = DVIS_NONE;
			status = PsCreateSystemThread(&hThread, 
				(ACCESS_MASK)0L,
				NULL,
				NULL,
				NULL,
				VGetFSInfoThread, 
				FilterDeviceObject);
			if (NT_SUCCESS(status)) {
				ZwClose(hThread);
			}
		}
		status = STATUS_SUCCESS;
	} else {
		//����Ҳ�����˵�����������߰�����豸��
		KdPrint(("SYS(%d):FPostAttachVolumeDevice (%wZ) is not find is below disk volume device.\n", PsGetCurrentProcessId(), &fsdevext->DE_FILTER.DE_FSVOL.DiskVolDeviceName));
		status = STATUS_NOT_FOUND;
	}
	if (!NT_SUCCESS(status)){
		KdPrint(("SYS(%d):FPostAttachVolumeDevice (%wZ) is Fail!0x%08x,Delete filter deivce\n", PsGetCurrentProcessId(), &fsdevext->DE_FILTER.DE_FSVOL.DiskVolDeviceName, status));
		IoDetachDevice(fsdevext->DE_FILTER.LowerDeviceObject);
		FPostDetachVolumeDevice(FilterDeviceObject, fsdevext->DE_FILTER.LowerDeviceObject);
		IoDeleteDevice(FilterDeviceObject);
	}
}

//ж�ؾ��豸
VOID FPostDetachVolumeDevice(
	IN PDEVICE_OBJECT DeviceObject,//�ҵ��豸
	IN PDEVICE_OBJECT VolumeDeviceObject//�豸��ȷ�е����²��豸
	)
{
	PEP_DEVICE_EXTENSION fsdevExt;
	PEP_DEVICE_EXTENSION voldevExt;
	PAGED_CODE();

	UNREFERENCED_PARAMETER( VolumeDeviceObject );
	fsdevExt = (PEP_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

	ASSERT(fsdevExt->DeviceType == DO_FSVOL);

	//�����Phy
	KdPrint(("SYS:FPostDetachVolumeDevice:DiskDeviceName=%wZ\n", &fsdevExt->DE_FILTER.DE_FSVOL.DiskVolDeviceName));
	//����ļ�ϵͳ�����豸�ʹ��̾�����豸�Ĺ�ϵ
	if (fsdevExt->DE_FILTER.DE_FSVOL.DiskVolFilter != NULL) {
		voldevExt = (PEP_DEVICE_EXTENSION)fsdevExt->DE_FILTER.DE_FSVOL.DiskVolFilter->DeviceExtension;
		ASSERT(voldevExt->DeviceType == DO_DISKVOL);
		voldevExt->DE_FILTER.DE_DISKVOL.FSVolFilter = NULL;
		fsdevExt->DE_FILTER.DE_FSVOL.DiskVolFilter = NULL;
	}
}

NTSTATUS FSetDirectRWFile(
	IN PEP_DEVICE_EXTENSION fsdevext, 
	IN PUNICODE_STRING FilePath,
	IN ULONGLONG DataClusterOffset,//��������ƫ����
	OUT PLCXL_BITMAP BitmapDirect
	)
{
	NTSTATUS status;
	IO_STATUS_BLOCK IoStatusBlock;
	UNICODE_STRING FullFilePath;
	WCHAR FullFilePathBuf[MAX_PATH+1];
	HANDLE hFile;
	OBJECT_ATTRIBUTES oa;

	PAGED_CODE();
	ASSERT(fsdevext != NULL);
	ASSERT(FilePath != NULL);
	ASSERT(BitmapDirect!=NULL);

	//�ϲ�·��	
	RtlInitEmptyUnicodeString(&FullFilePath, FullFilePathBuf, sizeof(FullFilePathBuf));
	RtlCopyUnicodeString(&FullFilePath, &((PEP_DEVICE_EXTENSION)fsdevext->DE_FILTER.DE_FSVOL.DiskVolFilter->DeviceExtension)->DE_FILTER.PhysicalDeviceName);
	RtlAppendUnicodeStringToString(&FullFilePath, FilePath);
	InitializeObjectAttributes(&oa, &FullFilePath, OBJ_CASE_INSENSITIVE|OBJ_KERNEL_HANDLE, NULL, NULL);
	//��Ҫ��ȡ���ļ�
	status = IoCreateFileSpecifyDeviceObjectHint(
		&hFile, 
		GENERIC_READ | SYNCHRONIZE,
		&oa, 
		&IoStatusBlock, 
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
		PRETRIEVAL_POINTERS_BUFFER FileClusters;
		//��ô��б�
		status = GetFileClusterList(hFile, &FileClusters);
		if (NT_SUCCESS(status)) {
			LARGE_INTEGER PrevVCN;
			//�Լ���������ݽṹ���������ٶ�д
			PLCXL_VCN_EXTENT pExtent;
			DWORD i;

			//���뵽ֱ�Ӷ�дλͼ��
			PrevVCN = FileClusters->StartingVcn;
			for (i = 0, pExtent = (PLCXL_VCN_EXTENT)&FileClusters->Extents[i]; i < FileClusters->ExtentCount; i++, pExtent++) {
				KdPrint(("LCXL Setting:GetFileClusterList(%016I64x->%016I64x)\n", (ULONGLONG)pExtent->Lcn.QuadPart, (ULONGLONG)(pExtent->Lcn.QuadPart+pExtent->NextVcn.QuadPart-PrevVCN.QuadPart-1)));
				//д�뵽����λͼ��
				LCXLBitmapSet(BitmapDirect, (ULONGLONG)pExtent->Lcn.QuadPart+DataClusterOffset, (ULONGLONG)(pExtent->NextVcn.QuadPart-PrevVCN.QuadPart), TRUE);
				//��ȡ��һ�����б�Ŀ�ʼ
				PrevVCN = pExtent->NextVcn;
			}
			//�ͷ��ڴ�
			ExFreePool(FileClusters);
		} else {
			KdPrint(("SYS:FSetDirectRWFile:GetFileClusterList(%wZ) failed(0x%08x)\n", FilePath, status));
		}
		ZwClose(hFile);
	} else {
		KdPrint(("SYS:FSetDirectRWFile:ObOpenObjectByPointer(0x%08x) failed\n", status));
	}
	return status;
}
