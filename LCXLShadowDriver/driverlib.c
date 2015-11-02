#include "winkernel.h"
#include "driverlib.h"
#include "driverlibinterface.h"
//���þ��棺�������ʽ�ǳ���
#pragma warning(disable: 4127)
//////////////////////////////////////////////////////////////////////////
//Դ�ļ���LCXLShadowLib.c
//���ߣ��޳���
//˵���������뼯��3��������3�����������󽫸������ֱ��������̹���������
//�ļ������������ͷ����豸

RTL_OSVERSIONINFOEXW g_OSVerInfo;

/////////////////////////////////////////////////////////////////////////////
//
//                      Global variables
//
/////////////////////////////////////////////////////////////////////////////

//
//  ��������

PDRIVER_OBJECT g_LCXLDriverObject = NULL;

FAST_MUTEX gSfilterAttachLock;

//
//  Given a device type, return a valid name
//

#define GET_DEVICE_TYPE_NAME( _type ) \
	((((_type) > 0) && ((_type) < (sizeof(DeviceTypeNames) / sizeof(PCHAR)))) ? \
	DeviceTypeNames[ (_type) ] : \
	"[Unknown]")

//
//  Known device type names
//

static const PCHAR DeviceTypeNames[] = {
	"",
	"BEEP",
	"CD_ROM",
	"CD_ROM_FILE_SYSTEM",
	"CONTROLLER",
	"DATALINK",
	"DFS",
	"DISK",
	"DISK_FILE_SYSTEM",
	"FILE_SYSTEM",
	"INPORT_PORT",
	"KEYBOARD",
	"MAILSLOT",
	"MIDI_IN",
	"MIDI_OUT",
	"MOUSE",
	"MULTI_UNC_PROVIDER",
	"NAMED_PIPE",
	"NETWORK",
	"NETWORK_BROWSER",
	"NETWORK_FILE_SYSTEM",
	"NULL",
	"PARALLEL_PORT",
	"PHYSICAL_NETCARD",
	"PRINTER",
	"SCANNER",
	"SERIAL_MOUSE_PORT",
	"SERIAL_PORT",
	"SCREEN",
	"SOUND",
	"STREAMS",
	"TAPE",
	"TAPE_FILE_SYSTEM",
	"TRANSPORT",
	"UNKNOWN",
	"VIDEO",
	"VIRTUAL_DISK",
	"WAVE_IN",
	"WAVE_OUT",
	"8042_PORT",
	"NETWORK_REDIRECTOR",
	"BATTERY",
	"BUS_EXTENDER",
	"MODEM",
	"VDM",
	"MASS_STORAGE",
	"SMB",
	"KS",
	"CHANGER",
	"SMARTCARD",
	"ACPI",
	"DVD",
	"FULLSCREEN_VIDEO",
	"DFS_FILE_SYSTEM",
	"DFS_VOLUME",
	"SERENUM",
	"TERMSRV",
	"KSEC"
};

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, DriverUnload)
#pragma alloc_text(PAGE, FAttachToFSDevice)
#pragma alloc_text(PAGE, FAttachToVolumeDevice)
#pragma alloc_text(PAGE, FDriverFSNotification)
#pragma alloc_text(PAGE, FEnumerateFileSystemVolumes)
#pragma alloc_text(PAGE, FGetBaseDeviceObjectName)
#pragma alloc_text(PAGE, FIsAttachedDevice)
#pragma alloc_text(PAGE, FIsShadowCopyVolume)
#pragma alloc_text(PAGE, FSLoadFileSystem)
#pragma alloc_text(PAGE, FSMountVolume)
#pragma alloc_text(PAGE, FTtyAttachVolumeDevice)
#pragma alloc_text(PAGE, GetObjectName)
#pragma alloc_text(PAGE, LCXLAddDevice)
#pragma alloc_text(PAGE, LCXLCleanup)
#pragma alloc_text(PAGE, LCXLClose)
#pragma alloc_text(PAGE, LCXLCreate)
#pragma alloc_text(PAGE, LCXLDeviceControl)
#pragma alloc_text(PAGE, LCXLFileSystemControl)
#pragma alloc_text(PAGE, LCXLPassThrough)
#pragma alloc_text(PAGE, LCXLPnp)
#pragma alloc_text(PAGE, LCXLPower)
//#pragma alloc_text(PAGE, LCXLRead)
#pragma alloc_text(PAGE, LCXLShutdown)
//#pragma alloc_text(PAGE, LCXLWrite)

#pragma alloc_text(PAGE, SfFastIoCheckIfPossible)
#pragma alloc_text(PAGE, SfFastIoDetachDevice)
#pragma alloc_text(PAGE, SfFastIoDeviceControl)
#pragma alloc_text(PAGE, SfFastIoLock)
#pragma alloc_text(PAGE, SfFastIoMdlRead)
#pragma alloc_text(PAGE, SfFastIoMdlReadComplete)
#pragma alloc_text(PAGE, SfFastIoMdlReadCompleteCompressed)
#pragma alloc_text(PAGE, SfFastIoMdlWriteComplete)
#pragma alloc_text(PAGE, SfFastIoMdlWriteCompleteCompressed)
#pragma alloc_text(PAGE, SfFastIoPrepareMdlWrite)
#pragma alloc_text(PAGE, SfFastIoQueryBasicInfo)
#pragma alloc_text(PAGE, SfFastIoQueryNetworkOpenInfo)
#pragma alloc_text(PAGE, SfFastIoQueryOpen)
#pragma alloc_text(PAGE, SfFastIoQueryStandardInfo)
#pragma alloc_text(PAGE, SfFastIoRead)
#pragma alloc_text(PAGE, SfFastIoReadCompressed)
#pragma alloc_text(PAGE, SfFastIoUnlockAll)
#pragma alloc_text(PAGE, SfFastIoUnlockAllByKey)
#pragma alloc_text(PAGE, SfFastIoUnlockSingle)
#pragma alloc_text(PAGE, SfFastIoWrite)
#pragma alloc_text(PAGE, SfFastIoWriteCompressed)
#pragma alloc_text(PAGE, SfPostFsFilterPassThrough)
#pragma alloc_text(PAGE, SfPreFsFilterPassThrough)
#endif

#define SFLT_POOL_TAG 'SFLT'

NTSTATUS DriverEntry (
	IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING RegistryPath
	)

	/*++

	Routine Description:

	This is the initialization routine for the SFILTER file system filter
	driver.  This routine creates the device object that represents this
	driver in the system and registers it for watching all file systems that
	register or unregister themselves as active file systems.

	Arguments:

	DriverObject - Pointer to driver object created by the system.

	Return Value:

	The function value is the final status from the initialization operation.

	--*/

{
	PFAST_IO_DISPATCH fastIoDispatch;
	//UNICODE_STRING nameString;

	NTSTATUS status;
	ULONG i;

	//
	//  Now get the current OS version that we will use to determine what logic
	//  paths to take when this driver is built to run on various OS version.
	//
	g_OSVerInfo.dwOSVersionInfoSize = sizeof(g_OSVerInfo);
	status = RtlGetVersion((PRTL_OSVERSIONINFOW)&g_OSVerInfo);
	if (!NT_SUCCESS(status)) {
		return status;
	}
	//
	//  Save our Driver Object, set our UNLOAD routine
	//

	g_LCXLDriverObject = DriverObject;


	//
	//  MULTIVERSION NOTE:
	//
	//  We can only support unload for testing environments if we can enumerate
	//  the outstanding device objects that our driver has.
	//

	//
	//  Unload is useful for development purposes. It is not recommended for
	//  production versions
	//
	//LCXL��ȥ��ж������ѡ��
	//DriverObject->DriverUnload = DriverUnload;

	//
	//  Setup other global variables
	//

	ExInitializeFastMutex( &gSfilterAttachLock );

	//
	//  Initialize the driver object with this device driver's entry points.
	//
	for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {

		DriverObject->MajorFunction[i] = LCXLPassThrough;

	}

	//
	//  We will use SfCreate for all the create operations
	//

	DriverObject->MajorFunction[IRP_MJ_CREATE] = LCXLCreate;
	DriverObject->MajorFunction[IRP_MJ_CREATE_NAMED_PIPE] = LCXLCreate;
	DriverObject->MajorFunction[IRP_MJ_CREATE_MAILSLOT] = LCXLCreate;

	DriverObject->MajorFunction[IRP_MJ_READ] = LCXLRead;
	DriverObject->MajorFunction[IRP_MJ_WRITE] = LCXLWrite;

	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = LCXLDeviceControl;

	DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] = LCXLFileSystemControl;
	DriverObject->MajorFunction[IRP_MJ_CLEANUP] = LCXLCleanup;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = LCXLClose;

	DriverObject->MajorFunction[IRP_MJ_POWER] = LCXLPower;
	DriverObject->MajorFunction[IRP_MJ_PNP] = LCXLPnp;
	DriverObject->MajorFunction[IRP_MJ_SHUTDOWN] = LCXLShutdown;
	//���̾�����

	//�����������AddDevice������ʼ��ΪDpAddDevice����
	DriverObject->DriverExtension->AddDevice = LCXLAddDevice;

	//�ļ���������
	//
	//  Allocate fast I/O data structure and fill it in.
	//
	//  NOTE:  The following FastIo Routines are not supported:
	//      AcquireFileForNtCreateSection
	//      ReleaseFileForNtCreateSection
	//      AcquireForModWrite
	//      ReleaseForModWrite
	//      AcquireForCcFlush
	//      ReleaseForCcFlush
	//
	//  For historical reasons these FastIO's have never been sent to filters
	//  by the NT I/O system.  Instead, they are sent directly to the base 
	//  file system.  On Windows XP and later OS releases, you can use the new 
	//  system routine "FsRtlRegisterFileSystemFilterCallbacks" if you need to 
	//  intercept these callbacks (see below).
	//

	fastIoDispatch = (PFAST_IO_DISPATCH)ExAllocatePoolWithTag( NonPagedPool, sizeof( FAST_IO_DISPATCH ), SFLT_POOL_TAG );
	if (!fastIoDispatch) {
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	RtlZeroMemory( fastIoDispatch, sizeof( FAST_IO_DISPATCH ) );

	fastIoDispatch->SizeOfFastIoDispatch = sizeof( FAST_IO_DISPATCH );
	fastIoDispatch->FastIoCheckIfPossible = SfFastIoCheckIfPossible;
	fastIoDispatch->FastIoRead = SfFastIoRead;
	fastIoDispatch->FastIoWrite = SfFastIoWrite;
	fastIoDispatch->FastIoQueryBasicInfo = SfFastIoQueryBasicInfo;
	fastIoDispatch->FastIoQueryStandardInfo = SfFastIoQueryStandardInfo;
	fastIoDispatch->FastIoLock = SfFastIoLock;
	fastIoDispatch->FastIoUnlockSingle = SfFastIoUnlockSingle;
	fastIoDispatch->FastIoUnlockAll = SfFastIoUnlockAll;
	fastIoDispatch->FastIoUnlockAllByKey = SfFastIoUnlockAllByKey;
	fastIoDispatch->FastIoDeviceControl = SfFastIoDeviceControl;
	fastIoDispatch->FastIoDetachDevice = SfFastIoDetachDevice;
	fastIoDispatch->FastIoQueryNetworkOpenInfo = SfFastIoQueryNetworkOpenInfo;
	fastIoDispatch->MdlRead = SfFastIoMdlRead;
	fastIoDispatch->MdlReadComplete = SfFastIoMdlReadComplete;
	fastIoDispatch->PrepareMdlWrite = SfFastIoPrepareMdlWrite;
	fastIoDispatch->MdlWriteComplete = SfFastIoMdlWriteComplete;
	fastIoDispatch->FastIoReadCompressed = SfFastIoReadCompressed;
	fastIoDispatch->FastIoWriteCompressed = SfFastIoWriteCompressed;
	fastIoDispatch->MdlReadCompleteCompressed = SfFastIoMdlReadCompleteCompressed;
	fastIoDispatch->MdlWriteCompleteCompressed = SfFastIoMdlWriteCompleteCompressed;
	fastIoDispatch->FastIoQueryOpen = SfFastIoQueryOpen;

	DriverObject->FastIoDispatch = fastIoDispatch;

	//
	//  VERSION NOTE:
	//
	//  There are 6 FastIO routines for which file system filters are bypassed as
	//  the requests are passed directly to the base file system.  These 6 routines
	//  are AcquireFileForNtCreateSection, ReleaseFileForNtCreateSection,
	//  AcquireForModWrite, ReleaseForModWrite, AcquireForCcFlush, and 
	//  ReleaseForCcFlush.
	//
	//  In Windows XP and later, the FsFilter callbacks were introduced to allow
	//  filters to safely hook these operations.  See the IFS Kit documentation for
	//  more details on how these new interfaces work.
	//
	//  MULTIVERSION NOTE:
	//  
	//  If built for Windows XP or later, this driver is built to run on 
	//  multiple versions.  When this is the case, we will test
	//  for the presence of FsFilter callbacks registration API.  If we have it,
	//  then we will register for those callbacks, otherwise, we will not.
	//


	{
		FS_FILTER_CALLBACKS fsFilterCallbacks;

		//
		//  Setup the callbacks for the operations we receive through
		//  the FsFilter interface.
		//
		//  NOTE:  You only need to register for those routines you really need
		//         to handle.  SFilter is registering for all routines simply to
		//         give an example of how it is done.
		//

		fsFilterCallbacks.SizeOfFsFilterCallbacks = sizeof( FS_FILTER_CALLBACKS );
		fsFilterCallbacks.PreAcquireForSectionSynchronization = SfPreFsFilterPassThrough;
		fsFilterCallbacks.PostAcquireForSectionSynchronization = SfPostFsFilterPassThrough;
		fsFilterCallbacks.PreReleaseForSectionSynchronization = SfPreFsFilterPassThrough;
		fsFilterCallbacks.PostReleaseForSectionSynchronization = SfPostFsFilterPassThrough;
		fsFilterCallbacks.PreAcquireForCcFlush = SfPreFsFilterPassThrough;
		fsFilterCallbacks.PostAcquireForCcFlush = SfPostFsFilterPassThrough;
		fsFilterCallbacks.PreReleaseForCcFlush = SfPreFsFilterPassThrough;
		fsFilterCallbacks.PostReleaseForCcFlush = SfPostFsFilterPassThrough;
		fsFilterCallbacks.PreAcquireForModifiedPageWriter = SfPreFsFilterPassThrough;
		fsFilterCallbacks.PostAcquireForModifiedPageWriter = SfPostFsFilterPassThrough;
		fsFilterCallbacks.PreReleaseForModifiedPageWriter = SfPreFsFilterPassThrough;
		fsFilterCallbacks.PostReleaseForModifiedPageWriter = SfPostFsFilterPassThrough;
		status = FsRtlRegisterFileSystemFilterCallbacks(DriverObject, &fsFilterCallbacks);
		if (!NT_SUCCESS( status )) {

			DriverObject->FastIoDispatch = NULL;
			ExFreePool( fastIoDispatch );
			return status;
		}
	}

	//���ö���ʼ�����ˣ���ʼ

	//�����Զ����ʼ������
	status = LCXLDriverEntry(DriverObject, RegistryPath);
	if (!NT_SUCCESS(status)) {
		//���ʧ���ˣ���
		IoUnregisterFsRegistrationChange(DriverObject, FDriverFSNotification);

		DriverObject->FastIoDispatch = NULL;
		ExFreePool( fastIoDispatch );
		return status;
	}
	//
	//  ע��ص����̡�FDriverFSNotification������ȡ�ļ�ϵͳ�������ж��
	//
	//  VERSION NOTE:
	//
	//  On Windows XP and later this will also enumerate all existing file
	//  systems (except the RAW file systems).  On Windows 2000 this does not
	//  enumerate the file systems that were loaded before this filter was
	//  loaded.
	//
	status = IoRegisterFsRegistrationChange( DriverObject, FDriverFSNotification);
	if (!NT_SUCCESS( status )) {

		KdPrint(( "SYS:DriverEntry: Error registering FS change notification, status=%08x\n", status ));
		LCXLDriverUnload(DriverObject);
		DriverObject->FastIoDispatch = NULL;
		ExFreePool( fastIoDispatch );
		return status;
	}

	//
	//  Attempt to attach to the appropriate RAW file system device objects
	//  since they are not enumerated by IoRegisterFsRegistrationChange.
	// ���Թ��ص�RAW�ļ�ϵͳ�豸�����У���Ϊ���ǲ��ᱻIoRegisterFsRegistrationChange����
	/*
	{
		PDEVICE_OBJECT rawDeviceObject;
		PFILE_OBJECT fileObject;

		//
		//  Attach to RawDisk device
		// ����RawDisk�豸

		RtlInitUnicodeString( &nameString, L"\\Device\\RawDisk" );

		status = IoGetDeviceObjectPointer(
			&nameString, 
			FILE_READ_ATTRIBUTES,
			&fileObject,
			&rawDeviceObject );

		if (NT_SUCCESS( status )) {

			FDriverFSNotification( rawDeviceObject, TRUE );
			ObDereferenceObject( fileObject );
		}

		//
		//  Attach to the RawCdRom device
		//  ����RawCDRom�豸

		RtlInitUnicodeString( &nameString, L"\\Device\\RawCdRom" );

		status = IoGetDeviceObjectPointer(
			&nameString,
			FILE_READ_ATTRIBUTES,
			&fileObject,
			&rawDeviceObject );

		if (NT_SUCCESS( status )) {

			FDriverFSNotification( rawDeviceObject, TRUE );
			ObDereferenceObject( fileObject );
		}
	}
	*/
	return status;
}

//����ж������
VOID DriverUnload(
	IN PDRIVER_OBJECT DriverObject
	)
{
	NTSTATUS status;
#define DEVOBJ_LIST_SIZE 64
	PDEVICE_OBJECT devList[DEVOBJ_LIST_SIZE];//���Ǽٶ����������64���豸
	ULONG numDevices;//�豸����
	PEP_DEVICE_EXTENSION devExt;
	LARGE_INTEGER interval;

	PAGED_CODE();
	//
	//  ���ٽ����ļ�ϵͳ�ĸ���֪ͨ
	//
	IoUnregisterFsRegistrationChange( DriverObject, FDriverFSNotification);

	//�����Զ���ж�غ���
	LCXLDriverUnload(DriverObject);

	//��ʼѭ���������ǵ��豸�ǲ���ȫ���ر��ˡ�
	while(TRUE) {
		ULONG i;
		//
		//  Get what device objects we can for this driver.  Quit if there
		//  are not any more.  Note that this routine should always be
		//  defined since this routine is only compiled for Windows XP and
		//  later.
		//
		status = IoEnumerateDeviceObjectList(
			DriverObject,
			devList,
			sizeof(devList),
			&numDevices);

		if (numDevices == 0)  {

			break;
		}
		//��ʼ�����ǵĶ����豸���²��豸�����뿪���������豸�Ļ���Ӧ������LCXLDriverUnload��ж�ص���
		for (i=0; i < numDevices; i++) {

			devExt = (PEP_DEVICE_EXTENSION)devList[i]->DeviceExtension;
			switch (devExt->DeviceType) {
			case DO_FSVOL:
			case DO_FSCTRL:
				IoDetachDevice( devExt->DE_FILTER.LowerDeviceObject );
				break;
			case DO_DISKVOL:case DO_DISK://���̿�Դ���룬û��������һ������
				/*
				IoDetachDevice( devExt->DE_DISKVOL.VLowerDeviceObject);
				*/
				break;

			case DO_CONTROL:
				//�������Ѿ�û�п����豸�ˣ������豸Ӧ����LCXLDriverUnload����ж��
				ASSERT(0);
				break;
			default:
				//û���������͵��豸��
				ASSERT(0);
				break;
			}
		}
		// �����������ֻ��˵΢������������
		//  The IO Manager does not currently add a reference count to a device
		//  object for each outstanding IRP.  This means there is no way to
		//  know if there are any outstanding IRPs on the given device.
		//  We are going to wait for a reasonable amount of time for pending
		//  irps to complete.  
		//
		//  WARNING: This does not work 100% of the time and the driver may be
		//           unloaded before all IRPs are completed.  This can easily
		//           occur under stress situations and if a long lived IRP is
		//           pending (like oplocks and directory change notifications).
		//           The system will fault when this Irp actually completes.
		//           This is a sample of how to do this during testing.  This
		//           is not recommended for production code.
		//

		interval.QuadPart = (5 * DELAY_ONE_SECOND);      //delay 5 seconds
		KeDelayExecutionThread( KernelMode, FALSE, &interval );

		//
		//  Now go back through the list and delete the device objects.
		//

		for (i=0; i < numDevices; i++) {

			//
			//  See if this is our control device object.  If not then cleanup
			//  the device extension.  If so then clear the global pointer
			//  that references it.
			//
			devExt = (PEP_DEVICE_EXTENSION)devList[i]->DeviceExtension;
			if (devExt->DeviceType == DO_FSVOL) {
				FPostDetachVolumeDevice(devList[i], devExt->DE_FILTER.PhysicalDeviceObject);
			}

			//
			//  Delete the device object, remove reference counts added by
			//  IoEnumerateDeviceObjectList.  Note that the delete does
			//  not actually occur until the reference count goes to zero.
			//

			IoDeleteDevice( devList[i] );
			ObDereferenceObject( devList[i] );
		}
	}

	ExFreePool( DriverObject->FastIoDispatch );
	DriverObject->FastIoDispatch = NULL;
}


NTSTATUS LCXLPassThrough(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
	)
{
	//����ȷ��������������豸
	NTSTATUS status;
	//�Ƿ����ҵ��豸
	if (IS_MY_DEVICE(DeviceObject)) {
		switch(GET_MY_DEVICE_TYPE(DeviceObject)) {
		case DO_CONTROL:
			status = LLCompleteRequest(STATUS_INVALID_DEVICE_REQUEST, Irp, 0);
			break;
		//�����豸��ֱ�ӷ����²�
		case DO_FSCTRL:
		case DO_FSVOL:
		case DO_DISK:
		case DO_DISKVOL:
			status = LLSendToNextDriver(((PEP_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->DE_FILTER.LowerDeviceObject, Irp);
			break;
		default:
			ASSERT(0);
			status = LLCompleteRequest(STATUS_INVALID_DEVICE_REQUEST, Irp, 0);
			break;
		}
	} else {
		ASSERT(0);
		status = LLCompleteRequest(STATUS_INVALID_DEVICE_REQUEST, Irp, 0);
	}
	return status;
}

NTSTATUS LCXLCreate(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
	)
{
	//����ȷ��������������豸
	NTSTATUS status;
	//�Ƿ����ҵ��豸
	if (IS_MY_DEVICE(DeviceObject)) {
		switch(GET_MY_DEVICE_TYPE(DeviceObject)) {
		case DO_CONTROL:
			status = CDeviceCreate(DeviceObject, Irp);
			break;
		case DO_FSCTRL:
		case DO_DISKVOL:
		case DO_DISK:
			status = LLSendToNextDriver(((PEP_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->DE_FILTER.LowerDeviceObject, Irp);
			break;
		case DO_FSVOL:
			status = FDeviceCreate(DeviceObject, Irp);
			break;
		default:
			ASSERT(0);
			status = LLCompleteRequest(STATUS_INVALID_DEVICE_REQUEST, Irp, 0);
			break;
		}
	} else {
		ASSERT(0);
		status = LLCompleteRequest(STATUS_INVALID_DEVICE_REQUEST, Irp, 0);
	}
	return status;
}

NTSTATUS LCXLRead(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
	)
{
	//����ȷ��������������豸
	NTSTATUS status;
	//�Ƿ����ҵ��豸
	if (IS_MY_DEVICE(DeviceObject)) {
		switch(GET_MY_DEVICE_TYPE(DeviceObject)) {
		case DO_CONTROL:
			status = LLCompleteRequest(STATUS_INVALID_DEVICE_REQUEST, Irp, 0);
			break;
		case DO_FSCTRL:
		case DO_FSVOL:
			status = LLSendToNextDriver(((PEP_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->DE_FILTER.LowerDeviceObject, Irp);
			break;
		case DO_DISKVOL:
			status = VDeviceRead(DeviceObject, Irp);
			break;
		case DO_DISK:
			status = DDeviceRead(DeviceObject, Irp);
			break;
		default:
			ASSERT(0);
			status = LLCompleteRequest(STATUS_INVALID_DEVICE_REQUEST, Irp, 0);
			break;
		}
	} else {
		ASSERT(0);
		status = LLCompleteRequest(STATUS_INVALID_DEVICE_REQUEST, Irp, 0);
	}
	return status;
}

NTSTATUS LCXLWrite(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
	)
{
	//����ȷ��������������豸
	NTSTATUS status;
	//�Ƿ����ҵ��豸
	if (IS_MY_DEVICE(DeviceObject)) {
		switch(GET_MY_DEVICE_TYPE(DeviceObject)) {
		case DO_CONTROL:
			status = LLCompleteRequest(STATUS_INVALID_DEVICE_REQUEST, Irp, 0);
			break;
		case DO_FSCTRL:
		case DO_FSVOL:
			status = LLSendToNextDriver(((PEP_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->DE_FILTER.LowerDeviceObject, Irp);
			break;
		case DO_DISKVOL:
			status = VDeviceWrite(DeviceObject, Irp);
			break;
		case DO_DISK:
			status = DDeviceWrite(DeviceObject, Irp);
			break;
		default:
			ASSERT(0);
			status = LLCompleteRequest(STATUS_INVALID_DEVICE_REQUEST, Irp, 0);
			break;
		}
	} else {
		ASSERT(0);
		status = LLCompleteRequest(STATUS_INVALID_DEVICE_REQUEST, Irp, 0);
	}
	return status;
}

NTSTATUS LCXLDeviceControl(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
	)
{
	//����ȷ��������������豸
	NTSTATUS status;

	//�Ƿ����ҵ��豸
	if (IS_MY_DEVICE(DeviceObject)) {
		//PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation( Irp );
		switch(GET_MY_DEVICE_TYPE(DeviceObject)) {
		case DO_CONTROL:
			status = CDeviceControl(DeviceObject, Irp);
			break;
		case DO_FSCTRL:
		case DO_FSVOL:
			status = LLSendToNextDriver(((PEP_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->DE_FILTER.LowerDeviceObject, Irp);
			break;
		case DO_DISKVOL:
			status = VDeviceControl(DeviceObject, Irp);
			break;
		case DO_DISK:
			status = DDeviceControl(DeviceObject, Irp);
			break;
		default:
			ASSERT(0);
			status = LLCompleteRequest(STATUS_INVALID_DEVICE_REQUEST, Irp, 0);
			break;
		}
	} else {
		ASSERT(0);
		status = LLCompleteRequest(STATUS_INVALID_DEVICE_REQUEST, Irp, 0);
	}
	return status;
}

NTSTATUS LCXLFileSystemControl(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
	)
{
	//����ȷ��������������豸
	NTSTATUS status;
	//�Ƿ����ҵ��豸
	if (IS_MY_DEVICE(DeviceObject)) {
		PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation( Irp );

		switch(GET_MY_DEVICE_TYPE(DeviceObject)) {
		case DO_CONTROL:
			status = LLCompleteRequest(STATUS_INVALID_DEVICE_REQUEST, Irp, 0);
			break;
		case DO_FSCTRL:
			///��Ҫ����
			//
			//  Process the minor function code.
			//

			switch (irpSp->MinorFunction) {

			case IRP_MN_MOUNT_VOLUME:

				return FSMountVolume( DeviceObject, Irp );

			case IRP_MN_LOAD_FILE_SYSTEM:

				return FSLoadFileSystem( DeviceObject, Irp );

			case IRP_MN_USER_FS_REQUEST:
				{
					PEP_DEVICE_EXTENSION devExt = (PEP_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
					switch (irpSp->Parameters.FileSystemControl.FsControlCode) {

					case FSCTL_DISMOUNT_VOLUME:
						
						FPostDetachVolumeDevice(DeviceObject, devExt->DE_FILTER.PhysicalDeviceObject);
						KdPrint(("SYS:FSCTL_DISMOUNT_VOLUME:Dismounting volume %p\n", devExt->DE_FILTER.PhysicalDeviceObject));
						
						break;
					case FSCTL_LOCK_VOLUME:
						
						KdPrint(("SYS:FSCTL_DISMOUNT_VOLUME:Dismounting volume %p\n", devExt->DE_FILTER.PhysicalDeviceObject));
						break;
					case FSCTL_UNLOCK_VOLUME:

						KdPrint(("SYS:FSCTL_DISMOUNT_VOLUME:Dismounting volume %p\n", devExt->DE_FILTER.PhysicalDeviceObject));
						break;
					}

					break;
				}
			}
			status = LLSendToNextDriver(((PEP_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->DE_FILTER.LowerDeviceObject, Irp);
			break;
		case DO_FSVOL:
		case DO_DISKVOL:
		case DO_DISK:
			status = LLSendToNextDriver(((PEP_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->DE_FILTER.LowerDeviceObject, Irp);
			break;
		default:
			ASSERT(0);
			status = LLCompleteRequest(STATUS_INVALID_DEVICE_REQUEST, Irp, 0);
			break;
		}
	} else {
		ASSERT(0);
		status = LLCompleteRequest(STATUS_INVALID_DEVICE_REQUEST, Irp, 0);
	}
	return status;
}

NTSTATUS LCXLCleanup(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
	)
{
	//����ȷ��������������豸
	NTSTATUS status;
	//�Ƿ����ҵ��豸
	if (IS_MY_DEVICE(DeviceObject)) {
		switch(GET_MY_DEVICE_TYPE(DeviceObject)) {
		case DO_CONTROL:
			status = LLCompleteRequest(STATUS_SUCCESS, Irp, 0);
			break;
		case DO_FSVOL:
			status = FDeviceCleanup(DeviceObject, Irp);
			break;
		case DO_FSCTRL:
		case DO_DISK:
		case DO_DISKVOL:
			status = LLSendToNextDriver(((PEP_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->DE_FILTER.LowerDeviceObject, Irp);
			break;
		default:
			ASSERT(0);
			status = LLCompleteRequest(STATUS_INVALID_DEVICE_REQUEST, Irp, 0);
			break;
		}
	} else {
		ASSERT(0);
		status = LLCompleteRequest(STATUS_INVALID_DEVICE_REQUEST, Irp, 0);
	}
	return status;
}

NTSTATUS LCXLClose(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
	)
{
	//����ȷ��������������豸
	NTSTATUS status;
	//�Ƿ����ҵ��豸
	if (IS_MY_DEVICE(DeviceObject)) {
		switch(GET_MY_DEVICE_TYPE(DeviceObject)) {
		case DO_CONTROL:
			status = LLCompleteRequest(STATUS_SUCCESS, Irp, 0);
			break;
		case DO_FSVOL:
			status = FDeviceClose(DeviceObject, Irp);
			break;
		case DO_FSCTRL:
		case DO_DISK:
		case DO_DISKVOL:
			status = LLSendToNextDriver(((PEP_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->DE_FILTER.LowerDeviceObject, Irp);
			break;
		default:
			ASSERT(0);
			status = LLCompleteRequest(STATUS_INVALID_DEVICE_REQUEST, Irp, 0);
			break;
		}
	} else {
		ASSERT(0);
		status = LLCompleteRequest(STATUS_INVALID_DEVICE_REQUEST, Irp, 0);
	}
	return status;
}

NTSTATUS LCXLPower(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
	)
{
	//����ȷ��������������豸
	NTSTATUS status;
	//�Ƿ����ҵ��豸
	if (IS_MY_DEVICE(DeviceObject)) {
		switch(GET_MY_DEVICE_TYPE(DeviceObject)) {
		case DO_CONTROL:
			//�����豸û�е�Դ����
			status = LLCompleteRequest(STATUS_INVALID_DEVICE_REQUEST, Irp, 0);
			break;
		case DO_FSCTRL:
		case DO_FSVOL:
		case DO_DISK:
			//��̬�жϲ���ϵͳ�汾
			PoStartNextPowerIrp(Irp);
			IoSkipCurrentIrpStackLocation(Irp);
			status = PoCallDriver(((PEP_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->DE_FILTER.LowerDeviceObject, Irp);
			break;
		case DO_DISKVOL:
			status = VDevicePower(DeviceObject, Irp);
			break;
		default:
			ASSERT(0);
			status = LLCompleteRequest(STATUS_INVALID_DEVICE_REQUEST, Irp, 0);
			break;
		}
	} else {
		ASSERT(0);
		status = LLCompleteRequest(STATUS_INVALID_DEVICE_REQUEST, Irp, 0);
	}

	return status;
}

NTSTATUS LCXLPnp(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
	)
{
	//����ȷ��������������豸
	NTSTATUS status;
	//�Ƿ����ҵ��豸
	if (IS_MY_DEVICE(DeviceObject)) {
		switch(GET_MY_DEVICE_TYPE(DeviceObject)) {
		case DO_CONTROL:
			status = LLCompleteRequest(STATUS_INVALID_DEVICE_REQUEST, Irp, 0);
			break;
		case DO_FSCTRL:
		case DO_FSVOL:
			status = LLSendToNextDriver(((PEP_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->DE_FILTER.LowerDeviceObject, Irp);
			break;
		case DO_DISKVOL:
			status = VDevicePnp(DeviceObject, Irp);
			break;
		case DO_DISK:
			status = DDevicePnp(DeviceObject, Irp);
			break;
		default:
			ASSERT(0);
			status = LLCompleteRequest(STATUS_INVALID_DEVICE_REQUEST, Irp, 0);
			break;
		}
	} else {
		ASSERT(0);
		status = LLCompleteRequest(STATUS_INVALID_DEVICE_REQUEST, Irp, 0);
	}
	return status;
}

NTSTATUS LCXLShutdown(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
	)
{
	//����ȷ��������������豸
	NTSTATUS status;
	PEP_DEVICE_EXTENSION devext;

	devext = (PEP_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	PAGED_CODE();
	//�Ƿ����ҵ��豸
	if (IS_MY_DEVICE(DeviceObject)) {
		//�ػ��¼������͵�IRPһ���Ǹ������豸��
		switch(GET_MY_DEVICE_TYPE(DeviceObject)) {
		case DO_CONTROL:
			status = CShutdown(DeviceObject, Irp);
			break;
		default:
			//�����ģ�������һ��ȥ
			status = LLSendToNextDriver(devext->DE_FILTER.LowerDeviceObject, Irp);
			break;
		}
	} else {
		ASSERT(0);
		status = LLCompleteRequest(STATUS_INVALID_DEVICE_REQUEST, Irp, 0);
	}
	return status;
}

//�ļ�ϵͳ�����¼�
VOID FDriverFSNotification (
	__in struct _DEVICE_OBJECT *DeviceObject,
	__in BOOLEAN FsActive
	)
{
#ifdef DBG
	UNICODE_STRING devname;
	WCHAR devnamebuf[MAX_DEVNAME_LENGTH];

	RtlInitEmptyUnicodeString(&devname, devnamebuf, sizeof(devnamebuf));
	GetObjectName(DeviceObject, &devname);
	KdPrint(("SYS:FDriverFSNotification:DeviceType=%s, DeviceName=%wZ, IsActive=%d\n", DeviceTypeNames[DeviceObject->DeviceType], &devname, FsActive));
#endif

	//������豸�������ͣ���������Ҫ���ص��豸
	if (DeviceObject->DeviceType==FILE_DEVICE_DISK_FILE_SYSTEM) {
		if (FsActive) {
			NTSTATUS status;
			//FEnumerateFileSystemVolumes
			//��Ҫ�����ļ�ϵͳʶ�������Ӧ�ð�
			status = FAttachToFSDevice(DeviceObject, FALSE);
			if (NT_SUCCESS(status)) {
				KdPrint(("SYS:FDriverFSNotification:FAttachToFSDevice Succ!\n"));
				//�ɹ��������ļ�ϵͳ�����صľ�
				status = FEnumerateFileSystemVolumes(DeviceObject);
				if (!NT_SUCCESS(status)) {
					KdPrint(("SYS:FDriverFSNotification:FEnumerateFileSystemVolumes Fail!0x%08x\n", status));
				}
			} else {
				if (status == STATUS_UNSUCCESSFUL) {
					KdPrint(("SYS:FDriverFSNotification:FAttachToFSDevice Is FSRec Device\n"));
				} else {
					KdPrint(("SYS:FDriverFSNotification:FAttachToFSDevice Fail!\n", status));
				}
			}
		} else {
			//�ҵĹ����豸
			PDEVICE_OBJECT ourAttachedDevice;
			PDEVICE_OBJECT LowerDevice;//�ײ��豸

			LowerDevice = DeviceObject;

			ourAttachedDevice = LowerDevice->AttachedDevice;
			//��ʼ�������ص��豸
			while (NULL != ourAttachedDevice) {
				//�ǲ����Լ����豸
				if (IS_MY_DEVICE(ourAttachedDevice)) {
					KdPrint(("SYS:FDriverFSNotification:found my attached device, delete it.\n"));
					//�ӵײ��豸������
					IoDetachDevice( LowerDevice );
					//ɾ�����ǵ��豸
					IoDeleteDevice( ourAttachedDevice );

					break;
				}

				//
				//  ������һ���豸
				//

				LowerDevice = ourAttachedDevice;
				ourAttachedDevice = LowerDevice->AttachedDevice;
			}
		}
	}

}
//�ļ�ϵͳ�����ؾ��¼�
NTSTATUS FSMountVolume (
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
	)
{
	PEP_DEVICE_EXTENSION devExt = (PEP_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation( Irp );
	PDEVICE_OBJECT storageStackDeviceObject;
	NTSTATUS status;
	BOOLEAN isShadowCopyVolume;

	ASSERT(IS_MY_DEVICE( DeviceObject));
	ASSERT(DeviceObject->DeviceType == FILE_DEVICE_DISK_FILE_SYSTEM);

	//
	//  Get the real device object (also known as the storage stack device
	//  object or the disk device object) pointed to by the vpb parameter
	//  because this vpb may be changed by the underlying file system.
	//  Both FAT and CDFS may change the VPB address if the volume being
	//  mounted is one they recognize from a previous mount.
	//
	// �����ʵ���豸����
	storageStackDeviceObject = irpSp->Parameters.MountVolume.Vpb->RealDevice;

	//
	//  Determine if this is a shadow copy volume.  If so don't attach to it.
	//  NOTE:  There is no reason sfilter shouldn't attach to these volumes,
	//         this is simply a sample of how to not attach if you don't want
	//         to
	//
	//����Ƿ���һ��ShadowCopy�豸��������򲻹���
	status = FIsShadowCopyVolume ( storageStackDeviceObject, 
		&isShadowCopyVolume );
	//�����ShadowCopy�豸
	if (NT_SUCCESS(status) && isShadowCopyVolume) {
#ifdef DBG
			UNICODE_STRING shadowDeviceName;
			WCHAR shadowNameBuffer[MAX_DEVNAME_LENGTH];

			//
			//  Get the name for the debug display
			//

			RtlInitEmptyUnicodeString( &shadowDeviceName, 
				shadowNameBuffer, 
				sizeof(shadowNameBuffer) );

			GetObjectName( storageStackDeviceObject, 
				&shadowDeviceName );
			KdPrint(("SYS:FSMountVolume:%wZ is a shadow copy volume, do not attach it.\n", &shadowDeviceName));
#endif
			return LLSendToNextDriver(devExt->DE_FILTER.LowerDeviceObject, Irp);
	}
	//
	//  This is a mount request.  Create a device object that can be
	//  attached to the file system's volume device object if this request
	//  is successful.  We allocate this memory now since we can not return
	//  an error in the completion routine.  
	//
	//  Since the device object we are going to attach to has not yet been
	//  created (it is created by the base file system) we are going to use
	//  the type of the file system control device object.  We are assuming
	//  that the file system control device object will have the same type
	//  as the volume device objects associated with it.
	//�ȴ�IRP���
	status = LLWaitIRPCompletion(devExt->DE_FILTER.LowerDeviceObject, Irp);

	//���IRP����ɹ����������ؾ�ɹ���
	if (NT_SUCCESS(status)) {
		//���Թ��ؾ��豸
		FTtyAttachVolumeDevice(storageStackDeviceObject->Vpb->DeviceObject, storageStackDeviceObject);
	}
	status = LLCompleteRequest(status, Irp, Irp->IoStatus.Information);
	//���IRP
	return status;
}

//IRP_MN_LOAD_FILE_SYSTEM
NTSTATUS FSLoadFileSystem(
	PDEVICE_OBJECT DeviceObject, 
	PIRP Irp
	)

	/*++

	Routine Description:

	This routine is invoked whenever an I/O Request Packet (IRP) w/a major
	function code of IRP_MJ_FILE_SYSTEM_CONTROL is encountered.  For most
	IRPs of this type, the packet is simply passed through.  However, for
	some requests, special processing is required.

	Arguments:

	DeviceObject - Pointer to the device object for this driver.

	Irp - Pointer to the request packet representing the I/O request.

	Return Value:

	The function value is the status of the operation.

	--*/
{
	PEP_DEVICE_EXTENSION devExt = (PEP_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	NTSTATUS status;

	ASSERT(devExt->DeviceType == DO_FSCTRL);
	status = LLWaitIRPCompletion(devExt->DE_FILTER.LowerDeviceObject, Irp);
	if (!NT_SUCCESS( Irp->IoStatus.Status ) && 
		(Irp->IoStatus.Status != STATUS_IMAGE_ALREADY_LOADED)) {
			//������¹���ʧ�ܣ�������Ҳ�ܲ�����o(�s���t)o
			IoAttachDeviceToDeviceStackSafe(DeviceObject, devExt->DE_FILTER.PhysicalDeviceObject, &devExt->DE_FILTER.LowerDeviceObject);
			ASSERT(devExt->DE_FILTER.LowerDeviceObject != NULL);
	}
	return LLCompleteRequest(status, Irp, Irp->IoStatus.Information);
}

//�ж��ǲ���ShadowCopyVolume��
NTSTATUS FIsShadowCopyVolume (
	IN PDEVICE_OBJECT StorageStackDeviceObject,
	OUT PBOOLEAN IsShadowCopy
	)
	/*++

	Routine Description:

	This routine will determine if the given volume is for a ShadowCopy volume
	or some other type of volume.

	VERSION NOTE:

	ShadowCopy volumes were introduced in Windows XP, therefore, if this
	driver is running on W2K, we know that this is not a shadow copy volume.

	Also note that in Windows XP, we need to test to see if the driver name
	of this device object is \Driver\VolSnap in addition to seeing if this
	device is read-only.  For Windows Server 2003, we can infer that
	this is a ShadowCopy by looking for a DeviceType == FILE_DEVICE_VIRTUAL_DISK
	and read-only volume.

	Arguments:

	StorageStackDeviceObject - pointer to the disk device object
	IsShadowCopy - returns TRUE if this is a shadow copy, FALSE otherwise

	Return Value:

	The status of the operation.  If this operation fails IsShadowCopy is
	always set to FALSE.

	--*/
{

	PAGED_CODE();

	//
	//  Default to NOT a shadow copy volume
	//

	*IsShadowCopy = FALSE;
	//�ж��ǲ���Windows XPϵͳ
	if (IS_WINDOWSXP()) {

		UNICODE_STRING volSnapDriverName;
		WCHAR buffer[MAX_DEVNAME_LENGTH];
		PUNICODE_STRING storageDriverName;
		ULONG returnedLength;
		NTSTATUS status;

		//
		//  In Windows XP, all ShadowCopy devices were of type FILE_DISK_DEVICE.
		//  If this does not have a device type of FILE_DISK_DEVICE, then
		//  it is not a ShadowCopy volume.  Return now.
		//

		if (FILE_DEVICE_DISK != StorageStackDeviceObject->DeviceType) {

			return STATUS_SUCCESS;
		}

		//
		//  Unfortunately, looking for the FILE_DEVICE_DISK isn't enough.  We
		//  need to find out if the name of this driver is \Driver\VolSnap as
		//  well.
		//

		storageDriverName = (PUNICODE_STRING) buffer;
		RtlInitEmptyUnicodeString( storageDriverName, 
			(PWCHAR)Add2Ptr( storageDriverName, sizeof( UNICODE_STRING )),
			sizeof( buffer ) - sizeof( UNICODE_STRING ) );

		status = ObQueryNameString( StorageStackDeviceObject,
			(POBJECT_NAME_INFORMATION)storageDriverName,
			storageDriverName->MaximumLength,
			&returnedLength );

		if (!NT_SUCCESS( status )) {

			return status;
		}

		RtlInitUnicodeString( &volSnapDriverName, L"\\Driver\\VolSnap" );

		if (RtlEqualUnicodeString( storageDriverName, &volSnapDriverName, TRUE )) {

			//
			// This is a ShadowCopy volume, so set our return parameter to true.
			//

			*IsShadowCopy = TRUE;

		} else {

			//
			//  This is not a ShadowCopy volume, but IsShadowCopy is already 
			//  set to FALSE.  Fall through to return to the caller.
			//

			NOTHING;
		}

		return STATUS_SUCCESS;

	} else {

		PIRP irp;
		KEVENT event;
		IO_STATUS_BLOCK iosb;
		NTSTATUS status;

		//
		//  For Windows Server 2003 and later, it is sufficient to test for a
		//  device type fo FILE_DEVICE_VIRTUAL_DISK and that the device
		//  is read-only to identify a ShadowCopy.
		//

		//
		//  If this does not have a device type of FILE_DEVICE_VIRTUAL_DISK, then
		//  it is not a ShadowCopy volume.  Return now.
		//

		if (FILE_DEVICE_VIRTUAL_DISK != StorageStackDeviceObject->DeviceType) {

			return STATUS_SUCCESS;
		}

		//
		//  It has the correct device type, see if it is marked as read only.
		//
		//  NOTE:  You need to be careful which device types you do this operation
		//         on.  It is accurate for this type but for other device
		//         types it may return misleading information.  For example the
		//         current microsoft cdrom driver always returns CD media as
		//         readonly, even if the media may be writable.  On other types
		//         this state may change.
		//

		KeInitializeEvent( &event, NotificationEvent, FALSE );
		//����һ��IRP
		irp = IoBuildDeviceIoControlRequest( IOCTL_DISK_IS_WRITABLE,
			StorageStackDeviceObject,
			NULL,
			0,
			NULL,
			0,
			FALSE,
			&event,
			&iosb );

		//
		//  If we could not allocate an IRP, return an error
		//

		if (irp == NULL) {

			return STATUS_INSUFFICIENT_RESOURCES;
		}

		//
		//  Call the storage stack and see if this is readonly
		//

		status = IoCallDriver( StorageStackDeviceObject, irp );

		if (status == STATUS_PENDING) {

			KeWaitForSingleObject( &event,
				Executive,
				KernelMode,
				FALSE,
				NULL );

			status = iosb.Status;
		}

		//
		//  If the media is write protected then this is a shadow copy volume
		// �����д����������Shadow Copy�豸

		if (STATUS_MEDIA_WRITE_PROTECTED == status) {

			*IsShadowCopy = TRUE;
			status = STATUS_SUCCESS;
		}

		//
		//  Return the status of the IOCTL.  IsShadowCopy is already set to FALSE
		//  which is what we want if STATUS_SUCCESS was returned or if an error
		//  was returned.
		//

		return status;
	}
}

//�жϴ��豸�Ƿ��Ѿ������ǵ��豸������
BOOL FIsAttachedDevice (
	IN PDEVICE_OBJECT VolumeObject
	)
	/*++

	Routine Description:

	VERSION: Windows XP and later

	This walks down the attachment chain looking for a device object that
	belongs to this driver.  If one is found, the attached device object
	is returned in AttachedDeviceObject.

	Arguments:

	DeviceObject - The device chain we want to look through

	AttachedDeviceObject - The Sfilter device attached to this device.

	Return Value:

	TRUE if we are attached, FALSE if not

	--*/
{
	PDEVICE_OBJECT currentDevObj;
	PDEVICE_OBJECT nextDevObj;
	BOOL IsAttached = FALSE;
	PAGED_CODE();
	//
	//  Get the device object at the TOP of the attachment chain
	//

	currentDevObj = IoGetAttachedDeviceReference( VolumeObject );
	do {

		if (IS_MY_DEVICE( currentDevObj)) {
			IsAttached = TRUE;
			ObDereferenceObject( currentDevObj );
			break;
		} 
		//
		//  Get the next attached object.  This puts a reference on 
		//  the device object.
		//

		nextDevObj = IoGetLowerDeviceObject( currentDevObj );

		//
		//  Dereference our current device object, before
		//  moving to the next one.
		//

		ObDereferenceObject( currentDevObj );

		currentDevObj = nextDevObj;
	} while (NULL != currentDevObj);
	return IsAttached;
}

/*
*���Թ��ؾ��豸
*ע�⣺����˾��豸�Ѿ������ǵ��豸�����أ���ô���ص���TRUE
*/
BOOL FTtyAttachVolumeDevice (
	IN PDEVICE_OBJECT VolumeDeviceObject,//���豸
	IN PDEVICE_OBJECT DiskDeviceObject//�����豸
	)
{
	//
	//  Acquire lock so we can atomically test if we area already attached
	//  and if not, then attach.  This prevents a double attach race
	//  condition.
	//
	UNICODE_STRING devname;
	NTSTATUS status;
	WCHAR devnamebuf[MAX_DEVNAME_LENGTH];
	BOOL IsMounted = FALSE;
	BOOL IsSuc = FALSE;
	PDEVICE_OBJECT FilterDeviceObject;

	FilterDeviceObject = NULL;
	RtlInitEmptyUnicodeString(&devname, devnamebuf, sizeof(devnamebuf));
	GetObjectName(DiskDeviceObject, &devname);
	
	//���ýӿڣ��������Ƿ�Ҫ����
	if (!FPreAttachVolumeDevice(VolumeDeviceObject, DiskDeviceObject, &devname)) {
		return FALSE;
	}
	
	//������
	ExAcquireFastMutex( &gSfilterAttachLock );
	//
	//  The mount succeeded.  If we are not already attached, attach to the
	//  device object.  Note: one reason we could already be attached is
	//  if the underlying file system revived a previous mount.
	//
	//����Ƿ��Ѿ�������
	if (!FIsAttachedDevice(VolumeDeviceObject)) {
		status = FAttachToVolumeDevice(VolumeDeviceObject, DiskDeviceObject, &devname, &FilterDeviceObject);
		IsSuc = NT_SUCCESS(status);
		if (!NT_SUCCESS(status)) {
			KdPrint(("SYS:FTtyAttachVolumeDevice:FAttachToVolumeDevice Fail!(0x%08x)\n", status));
		} else {
			IsMounted = TRUE;
		}
	} else {
		IsSuc = TRUE;
	}
	//
	//  ����
	//
	ExReleaseFastMutex( &gSfilterAttachLock );
	if (IsMounted) {
		//�������豸��ʼ����־����֤�ڼ��豸�ڳ�ʼ���ڼ䲻����������������
		ClearFlag( FilterDeviceObject->Flags, DO_DEVICE_INITIALIZING );
		FPostAttachVolumeDevice(FilterDeviceObject);
		
	}
	return IsSuc;
}

//�����ļ�ϵͳ�豸����
NTSTATUS FAttachToVolumeDevice (
	IN PDEVICE_OBJECT VolumeDeviceObject,
	IN PDEVICE_OBJECT DiskDeviceObject,
	IN PUNICODE_STRING DiskDeviceName,
	OUT PDEVICE_OBJECT *FilterDeviceObject//���˵Ĵ����豸
	)
{
	NTSTATUS status;
	PDEVICE_OBJECT newDeviceObject = NULL;

	PAGED_CODE();

	KdPrint(("SYS:FAttachToVolumeDevice:VolumeDeviceObject=%wZ\n", DiskDeviceName));

	//�ؼ������豸
	status = IoCreateDevice( g_LCXLDriverObject,
		sizeof( EP_DEVICE_EXTENSION),
		NULL,
		VolumeDeviceObject->DeviceType,
		0,
		FALSE,
		&newDeviceObject );
	if (NT_SUCCESS( status )) {
		PEP_DEVICE_EXTENSION devExt;//�豸��չ
		INT i;
		//�����豸��ʶ
		if ( FlagOn( VolumeDeviceObject->Flags, DO_BUFFERED_IO )) {
			SetFlag( newDeviceObject->Flags, DO_BUFFERED_IO );
		}

		if ( FlagOn( VolumeDeviceObject->Flags, DO_DIRECT_IO )) {
			SetFlag( newDeviceObject->Flags, DO_DIRECT_IO );
		}

		if ( FlagOn( VolumeDeviceObject->Characteristics, FILE_DEVICE_SECURE_OPEN ) ) {
			SetFlag( newDeviceObject->Characteristics, FILE_DEVICE_SECURE_OPEN );
		}
		//��ʼ���豸��չ
		devExt = (PEP_DEVICE_EXTENSION)newDeviceObject->DeviceExtension;
		RtlZeroMemory(devExt, sizeof(EP_DEVICE_EXTENSION));
		devExt->DeviceType = DO_FSVOL;//������������
		devExt->DE_FILTER.PhysicalDeviceObject = VolumeDeviceObject;
		//���ƴ�������
		//ע�⣺���ļ�ϵͳ�У��ļ�ϵͳ�����豸��û�����ֵģ�
		RtlInitEmptyUnicodeString(&devExt->DE_FILTER.PhysicalDeviceName, devExt->DE_FILTER.PhysicalDeviceNameBuffer,
			sizeof(devExt->DE_FILTER.PhysicalDeviceNameBuffer));
		//���ƴ�������
		RtlInitEmptyUnicodeString(&devExt->DE_FILTER.DE_FSVOL.DiskVolDeviceName, devExt->DE_FILTER.DE_FSVOL.DiskVolDeviceNameBuffer,
			sizeof(devExt->DE_FILTER.DE_FSVOL.DiskVolDeviceNameBuffer));
		RtlCopyUnicodeString(&devExt->DE_FILTER.DE_FSVOL.DiskVolDeviceName, DiskDeviceName);
		//���ƴ��̾����
		devExt->DE_FILTER.DE_FSVOL.DiskVolDeviceObject = DiskDeviceObject;
		//ѭ�����Թ���
		for (i=0; i < 80; i++) {
			LARGE_INTEGER interval;
			//���ص��ļ�ϵͳ�豸��
			status = IoAttachDeviceToDeviceStackSafe(newDeviceObject,VolumeDeviceObject, &devExt->DE_FILTER.LowerDeviceObject);
			if (NT_SUCCESS(status)) {

				//����ɹ������˳�ѭ��
				break;
			} else {
				//���ʧ�ܣ��п������豸û�г�ʼ����ϣ���ͣ0.05��ȴ��豸��ɳ�ʼ�����ٽ��й���
				interval.QuadPart = (50 * DELAY_ONE_MILLISECOND);
				KeDelayExecutionThread( KernelMode, FALSE, &interval );
			}
		}
		//������سɹ�
		if (!NT_SUCCESS(status)){
			KdPrint(("SYS:FAttachToVolumeDevice:IoAttachDeviceToDeviceStackSafe Fail!0x%08x\n", status));
			//����ɾ���豸
			IoDeleteDevice(newDeviceObject);
			newDeviceObject = NULL;
		} else {
			//����豸��ʼ����־
			//ClearFlag( newDeviceObject->Flags, DO_DEVICE_INITIALIZING );
		}
	} else {
		KdPrint(("SYS:FAttachToVolumeDevice:IoCreateDevice Fail!0x%08x\n", status));
	}
	*FilterDeviceObject = newDeviceObject;
	return status;
}

//�����ļ�ϵͳ�豸����
NTSTATUS FAttachToFSDevice (
	IN PDEVICE_OBJECT FSDeviceObject,
	IN BOOL IsNoAttachFSRecognizer//�Ƿ�����ļ�ϵͳʶ����
	)
{
	NTSTATUS status;
	PEP_DEVICE_EXTENSION devExt;
	PDEVICE_OBJECT NewDeviceObject;

	NewDeviceObject = NULL;
	if (FSDeviceObject->DeviceType!= FILE_DEVICE_DISK_FILE_SYSTEM) {
		return STATUS_SUCCESS;
	}

	if (IsNoAttachFSRecognizer) {
		UNICODE_STRING fsName;
		WCHAR tempNameBuffer[MAX_DEVNAME_LENGTH];
		UNICODE_STRING fsrecName;

		RtlInitEmptyUnicodeString( &fsName,
			tempNameBuffer,
			sizeof(tempNameBuffer) );

		GetObjectName( FSDeviceObject->DriverObject, &fsName );

		//
		//  See if we should attach to the standard file system recognizer device
		//  or not
		//
		RtlInitUnicodeString( &fsrecName, L"\\FileSystem\\Fs_Rec" );
		//
		//  See if this is one of the standard Microsoft file system recognizer
		//  devices (see if this device is in the FS_REC driver).  If so skip
		//  it.  We no longer attach to file system recognizer devices, we
		//  simply wait for the real file system driver to load.
		//
		if (RtlCompareUnicodeString( &fsName, &fsrecName, TRUE ) == 0) {
			//���ļ�ϵͳʶ�������򷵻�ʧ��
			return STATUS_UNSUCCESSFUL;
		}
	}
	//
	//  We want to attach to this file system.  Create a new device object we
	//  can attach with.
	//

	status = IoCreateDevice( g_LCXLDriverObject,
		sizeof( EP_DEVICE_EXTENSION),
		NULL,
		FSDeviceObject->DeviceType,
		0,
		FALSE,
		&NewDeviceObject );
	//
	//  Propagate flags from Device Object we are trying to attach to.
	//  Note that we do this before the actual attachment to make sure
	//  the flags are properly set once we are attached (since an IRP
	//  can come in immediately after attachment but before the flags would
	//  be set).
	//

	if ( FlagOn( FSDeviceObject->Flags, DO_BUFFERED_IO )) {

		SetFlag(NewDeviceObject->Flags, DO_BUFFERED_IO );
	}

	if ( FlagOn( FSDeviceObject->Flags, DO_DIRECT_IO )) {

		SetFlag( NewDeviceObject->Flags, DO_DIRECT_IO );
	}

	if ( FlagOn( FSDeviceObject->Characteristics, FILE_DEVICE_SECURE_OPEN ) ) {

		SetFlag( NewDeviceObject->Characteristics, FILE_DEVICE_SECURE_OPEN );
	}
	//
	//  Do the attachment
	//

	devExt = (PEP_DEVICE_EXTENSION)NewDeviceObject->DeviceExtension;
	RtlZeroMemory(devExt, sizeof(EP_DEVICE_EXTENSION));
	devExt->DeviceType = DO_FSCTRL;//����Ϊ�ļ�ϵͳ���ƶ�
	devExt->DE_FILTER.PhysicalDeviceObject = FSDeviceObject;
	RtlInitEmptyUnicodeString(&devExt->DE_FILTER.PhysicalDeviceName, devExt->DE_FILTER.PhysicalDeviceNameBuffer,
		sizeof(devExt->DE_FILTER.PhysicalDeviceNameBuffer));
	status = IoAttachDeviceToDeviceStackSafe(NewDeviceObject, FSDeviceObject, &devExt->DE_FILTER.LowerDeviceObject);
	if (!NT_SUCCESS(status)) {
		IoDeleteDevice(NewDeviceObject);
		return status;
	}
	GetObjectName(devExt->DE_FILTER.PhysicalDeviceObject, &devExt->DE_FILTER.PhysicalDeviceName);
	//
	//  Mark we are done initializing
	// �����ʼ����־

	ClearFlag( NewDeviceObject->Flags, DO_DEVICE_INITIALIZING );

	//������ö���ļ�ϵͳ�ľ�

	return status;
}

NTSTATUS FEnumerateFileSystemVolumes(
	IN PDEVICE_OBJECT FSDeviceObject
	)
	/*++

	Routine Description:

	Enumerate all the mounted devices that currently exist for the given file
	system and attach to them.  We do this because this filter could be loaded
	at any time and there might already be mounted volumes for this file system.

	Arguments:

	FSDeviceObject - The device object for the file system we want to enumerate

	Name - An already initialized unicode string used to retrieve names
	This is passed in to reduce the number of strings buffers on
	the stack.

	Return Value:

	The status of the operation

	--*/
{
	PDEVICE_OBJECT *devList;

	NTSTATUS status;
	ULONG numDevices;
	ULONG i;


	PAGED_CODE();

	//
	//  Find out how big of an array we need to allocate for the
	//  mounted device list.
	//

	status = IoEnumerateDeviceObjectList(
		FSDeviceObject->DriverObject,
		NULL,
		0,
		&numDevices);

	//
	//  We only need to get this list of there are devices.  If we
	//  don't get an error there are no devices so go on.
	//

	if (!NT_SUCCESS( status )) {

		ASSERT(STATUS_BUFFER_TOO_SMALL == status);

		//
		//  Allocate memory for the list of known devices
		//

		numDevices += 8;        //grab a few extra slots

		devList = (PDEVICE_OBJECT*)ExAllocatePoolWithTag( NonPagedPool, 
			(numDevices * sizeof(PDEVICE_OBJECT)), 
			SFLT_POOL_TAG );
		if (NULL == devList) {
			//�ڴ治���ˣ�
			return STATUS_INSUFFICIENT_RESOURCES;
		}

		//
		//  Now get the list of devices.  If we get an error again
		//  something is wrong, so just fail.
		//
		status = IoEnumerateDeviceObjectList(FSDeviceObject->DriverObject,
			devList,
			(numDevices * sizeof(PDEVICE_OBJECT)),
			&numDevices);

		if (!NT_SUCCESS( status ))  {

			ExFreePool( devList );
			return status;
		}

		//
		//  ��ʼ�����豸�б�
		//

		for (i=0; i < numDevices; i++) {

			//
			//  Initialize state so we can cleanup properly
			//

			//storageStackDeviceObject = NULL;
			//�����豸��Ҫ����
			//-�����豸
			//-�豸���Ͳ�һ��
			//-�Ѿ�������
			if ((devList[i] != FSDeviceObject) &&
				(devList[i]->DeviceType == FSDeviceObject->DeviceType) &&
				!FIsAttachedDevice(devList[i])) {
					UNICODE_STRING BaseDevName;
					WCHAR BaseDevNameBuf[MAX_DEVNAME_LENGTH];

					RtlInitEmptyUnicodeString(&BaseDevName, BaseDevNameBuf, sizeof(BaseDevNameBuf));

					//��ȡ�ײ��豸����
					FGetBaseDeviceObjectName(devList[i],&BaseDevName);
					//
					//  See if this device has a name.  If so, then it must
					//  be a control device so don't attach to it.  This handles
					//  drivers with more then one control device (like FastFat).
					//
					if (BaseDevName.Length == 0) {
						//
						//  Get the real (disk,storage stack) device object associated
						//  with this file system device object.  Only try to attach
						//  if we have a disk device object.
						//
						PDEVICE_OBJECT storageStackDeviceObject = NULL;
						//��ȡ�洢�豸����
						status = IoGetDiskDeviceObject(devList[i], &storageStackDeviceObject);
						if (NT_SUCCESS(status)) { 
							BOOLEAN isShadowCopyVolume;
							//  Determine if this is a shadow copy volume.  If so don't
							//  attach to it.
							//  NOTE:  There is no reason sfilter shouldn't attach to these
							//         volumes, this is simply a sample of how to not
							//         attach if you don't want to
							//
							status = FIsShadowCopyVolume (storageStackDeviceObject, &isShadowCopyVolume);
							//�������Shadow Copy�豸
							if (NT_SUCCESS(status)&&!isShadowCopyVolume) {
								//Ϊʲô������storageStackDeviceObject
								//��storageStackDeviceObject->Vbp->RealDevice
								//������storageStackDeviceObject->Vbp��mount ���豸ʱ��䣬������ʱ��Ҫԭ����RealDevice������鿴FSMountVolume����
								FTtyAttachVolumeDevice(devList[i], storageStackDeviceObject);
							}
							ObDereferenceObject(storageStackDeviceObject);
						}
					}
			}
			ObDereferenceObject( devList[i] );
		}

		////
		////  We are going to ignore any errors received while attaching.  We
		////  simply won't be attached to those volumes if we get an error
		////
		//
		//status = STATUS_SUCCESS;

		//
		//  Free the memory we allocated for the list
		//

		ExFreePool( devList );
	}

	return status;
}

VOID FGetBaseDeviceObjectName (
	IN PDEVICE_OBJECT DeviceObject,
	IN OUT PUNICODE_STRING Name
	)
	/*++

	Routine Description:

	This locates the base device object in the given attachment chain and then
	returns the name of that object.

	If no name can be found, an empty string is returned.

	Arguments:

	Object - The object whose name we want

	Name - A unicode string that is already initialized with a buffer that
	receives the name of the device object.

	Return Value:

	None

	--*/
{
	//
	//  Get the base file system device object
	//

	DeviceObject = IoGetDeviceAttachmentBaseRef( DeviceObject );

	//
	//  Get the name of that object
	//

	GetObjectName( DeviceObject, Name );

	//
	//  Remove the reference added by IoGetDeviceAttachmentBaseRef
	//

	ObDereferenceObject( DeviceObject );
}

BOOLEAN SfFastIoCheckIfPossible(
	IN PFILE_OBJECT FileObject,
	IN PLARGE_INTEGER FileOffset,
	IN ULONG Length,
	IN BOOLEAN Wait,
	IN ULONG LockKey,
	IN BOOLEAN CheckForReadOperation,
	OUT PIO_STATUS_BLOCK IoStatus,
	IN PDEVICE_OBJECT DeviceObject
	)
	/*++

	Routine Description:

	This routine is the fast I/O "pass through" routine for checking to see
	whether fast I/O is possible for this file.

	This function simply invokes the file system's corresponding routine, or
	returns FALSE if the file system does not implement the function.

	Arguments:

	FileObject - Pointer to the file object to be operated on.

	FileOffset - Byte offset in the file for the operation.

	Length - Length of the operation to be performed.

	Wait - Indicates whether or not the caller is willing to wait if the
	appropriate locks, etc. cannot be acquired

	LockKey - Provides the caller's key for file locks.

	CheckForReadOperation - Indicates whether the caller is checking for a
	read (TRUE) or a write operation.

	IoStatus - Pointer to a variable to receive the I/O status of the
	operation.

	DeviceObject - Pointer to this driver's device object, the device on
	which the operation is to occur.

	Return Value:

	The function value is TRUE or FALSE based on whether or not fast I/O
	is possible for this file.

	--*/
{
	PDEVICE_OBJECT nextDeviceObject;
	PFAST_IO_DISPATCH fastIoDispatch;

	PAGED_CODE();

	// return FALSE;	// add by tanwen.
	// ��������ҵ��豸(Ӱ���豸���ܷ����������)    
	if(!IS_THIS_DEVICE_TYPE( DeviceObject, DO_FSVOL ))
		return FALSE;


	if (DeviceObject->DeviceExtension) {

		ASSERT(IS_THIS_DEVICE_TYPE( DeviceObject, DO_FSVOL ));

		//
		//  Pass through logic for this type of Fast I/O
		//

		nextDeviceObject = ((PEP_DEVICE_EXTENSION) DeviceObject->DeviceExtension)->DE_FILTER.LowerDeviceObject;
		ASSERT(nextDeviceObject);

		fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;

		if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoCheckIfPossible )) {

			return (fastIoDispatch->FastIoCheckIfPossible)(
				FileObject,
				FileOffset,
				Length,
				Wait,
				LockKey,
				CheckForReadOperation,
				IoStatus,
				nextDeviceObject );
		}
	}
	return FALSE;

}

BOOLEAN SfFastIoRead(
	IN PFILE_OBJECT FileObject,
	IN PLARGE_INTEGER FileOffset,
	IN ULONG Length,
	IN BOOLEAN Wait,
	IN ULONG LockKey,
	OUT PVOID Buffer,
	OUT PIO_STATUS_BLOCK IoStatus,
	IN PDEVICE_OBJECT DeviceObject
	)
{

	PAGED_CODE();

	UNREFERENCED_PARAMETER( FileObject );
	UNREFERENCED_PARAMETER( FileOffset );
	UNREFERENCED_PARAMETER( Length );
	UNREFERENCED_PARAMETER( Wait );
	UNREFERENCED_PARAMETER( LockKey );
	UNREFERENCED_PARAMETER( Buffer );
	UNREFERENCED_PARAMETER( IoStatus );

	if(!IS_THIS_DEVICE_TYPE( DeviceObject, DO_FSVOL)) {
		return FALSE;
	}
	return FALSE;	// add by tanwen.

}

BOOLEAN SfFastIoWrite(
	IN PFILE_OBJECT FileObject,
	IN PLARGE_INTEGER FileOffset,
	IN ULONG Length,
	IN BOOLEAN Wait,
	IN ULONG LockKey,
	IN PVOID Buffer,
	OUT PIO_STATUS_BLOCK IoStatus,
	IN PDEVICE_OBJECT DeviceObject
	)
{
	PAGED_CODE();

	UNREFERENCED_PARAMETER( FileObject );
	UNREFERENCED_PARAMETER( FileOffset );
	UNREFERENCED_PARAMETER( Length );
	UNREFERENCED_PARAMETER( Wait );
	UNREFERENCED_PARAMETER( LockKey );
	UNREFERENCED_PARAMETER( Buffer );
	UNREFERENCED_PARAMETER( IoStatus );

	if(!IS_THIS_DEVICE_TYPE( DeviceObject, DO_FSVOL)) {
		return FALSE;
	}
	return FALSE;	// add by tanwen.

}

BOOLEAN SfFastIoQueryBasicInfo(
	IN PFILE_OBJECT FileObject,
	IN BOOLEAN Wait,
	OUT PFILE_BASIC_INFORMATION Buffer,
	OUT PIO_STATUS_BLOCK IoStatus,
	IN PDEVICE_OBJECT DeviceObject
	)
{
	PAGED_CODE();

	UNREFERENCED_PARAMETER( FileObject );
	UNREFERENCED_PARAMETER( Wait );
	UNREFERENCED_PARAMETER( Buffer );
	UNREFERENCED_PARAMETER( IoStatus );

	if(!IS_THIS_DEVICE_TYPE( DeviceObject, DO_FSVOL)) {
		return FALSE;
	}
	return FALSE;	// add by tanwen.

}

BOOLEAN SfFastIoQueryStandardInfo(
	IN PFILE_OBJECT FileObject,
	IN BOOLEAN Wait,
	OUT PFILE_STANDARD_INFORMATION Buffer,
	OUT PIO_STATUS_BLOCK IoStatus,
	IN PDEVICE_OBJECT DeviceObject
	)
{
	PAGED_CODE();

	UNREFERENCED_PARAMETER( FileObject );
	UNREFERENCED_PARAMETER( Wait );
	UNREFERENCED_PARAMETER( Buffer );
	UNREFERENCED_PARAMETER( IoStatus );

	if(!IS_THIS_DEVICE_TYPE( DeviceObject, DO_FSVOL)) {
		return FALSE;
	}
	return FALSE;	// add by tanwen.

}

BOOLEAN SfFastIoLock(
	IN PFILE_OBJECT FileObject,
	IN PLARGE_INTEGER FileOffset,
	IN PLARGE_INTEGER Length,
	PEPROCESS ProcessId,
	ULONG Key,
	BOOLEAN FailImmediately,
	BOOLEAN ExclusiveLock,
	OUT PIO_STATUS_BLOCK IoStatus,
	IN PDEVICE_OBJECT DeviceObject
	)
{
	PAGED_CODE();

	UNREFERENCED_PARAMETER( FileObject );
	UNREFERENCED_PARAMETER( FileOffset );
	UNREFERENCED_PARAMETER( Length );
	UNREFERENCED_PARAMETER( ProcessId );
	UNREFERENCED_PARAMETER( Key );
	UNREFERENCED_PARAMETER( FailImmediately );
	UNREFERENCED_PARAMETER( ExclusiveLock );
	UNREFERENCED_PARAMETER( IoStatus );

	if(!IS_THIS_DEVICE_TYPE( DeviceObject, DO_FSVOL)) {
		return FALSE;
	}
	return FALSE;	// add by tanwen.

}

BOOLEAN SfFastIoUnlockSingle(
	IN PFILE_OBJECT FileObject,
	IN PLARGE_INTEGER FileOffset,
	IN PLARGE_INTEGER Length,
	PEPROCESS ProcessId,
	ULONG Key,
	OUT PIO_STATUS_BLOCK IoStatus,
	IN PDEVICE_OBJECT DeviceObject
	)
{
	PAGED_CODE();

	UNREFERENCED_PARAMETER( FileObject );
	UNREFERENCED_PARAMETER( FileOffset );
	UNREFERENCED_PARAMETER( Length );
	UNREFERENCED_PARAMETER( ProcessId );
	UNREFERENCED_PARAMETER( Key );
	UNREFERENCED_PARAMETER( IoStatus );

	if(!IS_THIS_DEVICE_TYPE( DeviceObject, DO_FSVOL)) {
		return FALSE;
	}
	return FALSE;	// add by tanwen.

}

BOOLEAN SfFastIoUnlockAll(
	IN PFILE_OBJECT FileObject,
	PEPROCESS ProcessId,
	OUT PIO_STATUS_BLOCK IoStatus,
	IN PDEVICE_OBJECT DeviceObject
	)
{
	PAGED_CODE();

	UNREFERENCED_PARAMETER( FileObject );
	UNREFERENCED_PARAMETER( ProcessId );
	UNREFERENCED_PARAMETER( IoStatus );

	if(!IS_THIS_DEVICE_TYPE( DeviceObject, DO_FSVOL)) {
		return FALSE;
	}
	return FALSE;	// add by tanwen.

}

BOOLEAN SfFastIoUnlockAllByKey(
	IN PFILE_OBJECT FileObject,
	PVOID ProcessId,
	ULONG Key,
	OUT PIO_STATUS_BLOCK IoStatus,
	IN PDEVICE_OBJECT DeviceObject
	)
{
	PAGED_CODE();

	UNREFERENCED_PARAMETER( FileObject );
	UNREFERENCED_PARAMETER( ProcessId );
	UNREFERENCED_PARAMETER( Key );
	UNREFERENCED_PARAMETER( IoStatus );

	if(!IS_THIS_DEVICE_TYPE( DeviceObject, DO_FSVOL)) {
		return FALSE;
	}
	return FALSE;	// add by tanwen.

}

BOOLEAN SfFastIoDeviceControl(
	IN PFILE_OBJECT FileObject,
	IN BOOLEAN Wait,
	IN PVOID InputBuffer OPTIONAL,
	IN ULONG InputBufferLength,
	OUT PVOID OutputBuffer OPTIONAL,
	IN ULONG OutputBufferLength,
	IN ULONG IoControlCode,
	OUT PIO_STATUS_BLOCK IoStatus,
	IN PDEVICE_OBJECT DeviceObject
	)
{
	PAGED_CODE();

	UNREFERENCED_PARAMETER( FileObject );
	UNREFERENCED_PARAMETER( Wait );
	UNREFERENCED_PARAMETER( InputBuffer );
	UNREFERENCED_PARAMETER( InputBufferLength );
	UNREFERENCED_PARAMETER( OutputBuffer );
	UNREFERENCED_PARAMETER( OutputBufferLength );
	UNREFERENCED_PARAMETER( IoControlCode );
	UNREFERENCED_PARAMETER( IoStatus );

	if(!IS_THIS_DEVICE_TYPE( DeviceObject, DO_FSVOL)) {
		return FALSE;
	}
	return FALSE;	// add by tanwen.

}

VOID SfFastIoDetachDevice(
	IN PDEVICE_OBJECT SourceDevice,
	IN PDEVICE_OBJECT TargetDevice
	)
{
	PEP_DEVICE_EXTENSION devExt = (PEP_DEVICE_EXTENSION)SourceDevice->DeviceExtension;

	PAGED_CODE();


	ASSERT(IS_MY_DEVICE( SourceDevice));
	switch(GET_MY_DEVICE_TYPE(SourceDevice)) {
	case DO_FSVOL:
		//
		//  Detach from the file system's volume device object.
		//
		KdPrint(("SYS:SfFastIoDetachDevice:Delete the FSDO of %wZ\n", &devExt->DE_FILTER.DE_FSVOL.DiskVolDeviceName));
		IoDetachDevice( TargetDevice );
		FPostDetachVolumeDevice(SourceDevice, devExt->DE_FILTER.PhysicalDeviceObject);
		IoDeleteDevice( SourceDevice );
		break;
	case DO_DISKVOL:
		KdPrint(("SYS:SfFastIoDetachDevice:DO_DISKVOL:PDN=%wZ\n", &devExt->DE_FILTER.PhysicalDeviceName));
		IoDetachDevice( TargetDevice );
		IoDeleteDevice( SourceDevice );
		break;
	default:
		ASSERT(0);
		break;
	}
}

BOOLEAN SfFastIoQueryNetworkOpenInfo(
	IN PFILE_OBJECT FileObject,
	IN BOOLEAN Wait,
	OUT PFILE_NETWORK_OPEN_INFORMATION Buffer,
	OUT PIO_STATUS_BLOCK IoStatus,
	IN PDEVICE_OBJECT DeviceObject
	)
{
	PAGED_CODE();

	UNREFERENCED_PARAMETER( FileObject );
	UNREFERENCED_PARAMETER( Wait );
	UNREFERENCED_PARAMETER( Buffer );
	UNREFERENCED_PARAMETER( IoStatus );

	if(!IS_THIS_DEVICE_TYPE( DeviceObject, DO_FSVOL)) {
		return FALSE;
	}
	return FALSE;	// add by tanwen.

}

BOOLEAN SfFastIoMdlRead(
	IN PFILE_OBJECT FileObject,
	IN PLARGE_INTEGER FileOffset,
	IN ULONG Length,
	IN ULONG LockKey,
	OUT PMDL *MdlChain,
	OUT PIO_STATUS_BLOCK IoStatus,
	IN PDEVICE_OBJECT DeviceObject
	)
{
	PAGED_CODE();

	UNREFERENCED_PARAMETER( FileObject );
	UNREFERENCED_PARAMETER( FileOffset );
	UNREFERENCED_PARAMETER( Length );
	UNREFERENCED_PARAMETER( LockKey );
	UNREFERENCED_PARAMETER( MdlChain );
	UNREFERENCED_PARAMETER( IoStatus );

	if(!IS_THIS_DEVICE_TYPE( DeviceObject, DO_FSVOL)) {
		return FALSE;
	}
	return FALSE;	// add by tanwen.

}


BOOLEAN SfFastIoMdlReadComplete(
	IN PFILE_OBJECT FileObject,
	IN PMDL MdlChain,
	IN PDEVICE_OBJECT DeviceObject
	)
{
	PAGED_CODE();

	UNREFERENCED_PARAMETER( FileObject );
	UNREFERENCED_PARAMETER( MdlChain );

	if(!IS_THIS_DEVICE_TYPE( DeviceObject, DO_FSVOL)) {
		return FALSE;
	}
	return FALSE;	// add by tanwen.

}

BOOLEAN SfFastIoPrepareMdlWrite(
	IN PFILE_OBJECT FileObject,
	IN PLARGE_INTEGER FileOffset,
	IN ULONG Length,
	IN ULONG LockKey,
	OUT PMDL *MdlChain,
	OUT PIO_STATUS_BLOCK IoStatus,
	IN PDEVICE_OBJECT DeviceObject
	)
{
	PAGED_CODE();

	UNREFERENCED_PARAMETER( FileObject );
	UNREFERENCED_PARAMETER( FileOffset );
	UNREFERENCED_PARAMETER( Length );
	UNREFERENCED_PARAMETER( LockKey );
	UNREFERENCED_PARAMETER( MdlChain );
	UNREFERENCED_PARAMETER( IoStatus );

	if(!IS_THIS_DEVICE_TYPE( DeviceObject, DO_FSVOL)) {
		return FALSE;
	}
	return FALSE;	// add by tanwen.

}

BOOLEAN SfFastIoMdlWriteComplete(
	IN PFILE_OBJECT FileObject,
	IN PLARGE_INTEGER FileOffset,
	IN PMDL MdlChain,
	IN PDEVICE_OBJECT DeviceObject
	)
{
	PAGED_CODE();

	UNREFERENCED_PARAMETER( FileObject );
	UNREFERENCED_PARAMETER( FileOffset );
	UNREFERENCED_PARAMETER( MdlChain );

	if(!IS_THIS_DEVICE_TYPE( DeviceObject, DO_FSVOL)) {
		return FALSE;
	}
	return FALSE;	// add by tanwen.

}

BOOLEAN SfFastIoReadCompressed(
	IN PFILE_OBJECT FileObject,
	IN PLARGE_INTEGER FileOffset,
	IN ULONG Length,
	IN ULONG LockKey,
	OUT PVOID Buffer,
	OUT PMDL *MdlChain,
	OUT PIO_STATUS_BLOCK IoStatus,
	OUT struct _COMPRESSED_DATA_INFO *CompressedDataInfo,
	IN ULONG CompressedDataInfoLength,
	IN PDEVICE_OBJECT DeviceObject
	)
{
	PAGED_CODE();

	UNREFERENCED_PARAMETER( FileObject );
	UNREFERENCED_PARAMETER( FileOffset );
	UNREFERENCED_PARAMETER( Length );
	UNREFERENCED_PARAMETER( LockKey );
	UNREFERENCED_PARAMETER( Buffer );
	UNREFERENCED_PARAMETER( MdlChain );
	UNREFERENCED_PARAMETER( IoStatus );
	UNREFERENCED_PARAMETER( CompressedDataInfo );
	UNREFERENCED_PARAMETER( CompressedDataInfoLength );

	if(!IS_THIS_DEVICE_TYPE( DeviceObject, DO_FSVOL)) {
		return FALSE;
	}
	return FALSE;	// add by tanwen.

}

BOOLEAN SfFastIoWriteCompressed(
	IN PFILE_OBJECT FileObject,
	IN PLARGE_INTEGER FileOffset,
	IN ULONG Length,
	IN ULONG LockKey,
	IN PVOID Buffer,
	OUT PMDL *MdlChain,
	OUT PIO_STATUS_BLOCK IoStatus,
	IN struct _COMPRESSED_DATA_INFO *CompressedDataInfo,
	IN ULONG CompressedDataInfoLength,
	IN PDEVICE_OBJECT DeviceObject
	)
{
	PAGED_CODE();

	UNREFERENCED_PARAMETER( FileObject );
	UNREFERENCED_PARAMETER( FileOffset );
	UNREFERENCED_PARAMETER( Length );
	UNREFERENCED_PARAMETER( LockKey );
	UNREFERENCED_PARAMETER( Buffer );
	UNREFERENCED_PARAMETER( MdlChain );
	UNREFERENCED_PARAMETER( IoStatus );
	UNREFERENCED_PARAMETER( CompressedDataInfo );
	UNREFERENCED_PARAMETER( CompressedDataInfoLength );

	if(!IS_THIS_DEVICE_TYPE( DeviceObject, DO_FSVOL)) {
		return FALSE;
	}
	return FALSE;	// add by tanwen.

}

BOOLEAN SfFastIoMdlReadCompleteCompressed(
	IN PFILE_OBJECT FileObject,
	IN PMDL MdlChain,
	IN PDEVICE_OBJECT DeviceObject
	)
{
	PAGED_CODE();

	UNREFERENCED_PARAMETER( FileObject );
	UNREFERENCED_PARAMETER( MdlChain );

	if(!IS_THIS_DEVICE_TYPE( DeviceObject, DO_FSVOL)) {
		return FALSE;
	}
	return FALSE;	// add by tanwen.

}

BOOLEAN SfFastIoMdlWriteCompleteCompressed(
	IN PFILE_OBJECT FileObject,
	IN PLARGE_INTEGER FileOffset,
	IN PMDL MdlChain,
	IN PDEVICE_OBJECT DeviceObject
	)
{
	PAGED_CODE();

	UNREFERENCED_PARAMETER( FileObject );
	UNREFERENCED_PARAMETER( FileOffset );
	UNREFERENCED_PARAMETER( MdlChain );

	if(!IS_THIS_DEVICE_TYPE( DeviceObject, DO_FSVOL)) {
		return FALSE;
	}
	return FALSE;	// add by tanwen.

}

BOOLEAN SfFastIoQueryOpen(
	IN PIRP Irp,
	OUT PFILE_NETWORK_OPEN_INFORMATION NetworkInformation,
	IN PDEVICE_OBJECT DeviceObject
	)
{
	PAGED_CODE();

	UNREFERENCED_PARAMETER( Irp );
	UNREFERENCED_PARAMETER( NetworkInformation );

	if(!IS_THIS_DEVICE_TYPE( DeviceObject, DO_FSVOL)) {
		return FALSE;
	}
	return FALSE;	// add by tanwen.

}

//
NTSTATUS SfPreFsFilterPassThrough (
	IN PFS_FILTER_CALLBACK_DATA Data,
	OUT PVOID *CompletionContext
	)
	/*++

	Routine Description:

	This routine is the FS Filter pre-operation "pass through" routine.

	Arguments:

	Data - The FS_FILTER_CALLBACK_DATA structure containing the information
	about this operation.

	CompletionContext - A context set by this operation that will be passed
	to the corresponding SfPostFsFilterOperation call.

	Return Value:

	Returns STATUS_SUCCESS if the operation can continue or an appropriate
	error code if the operation should fail.

	--*/
{
	UNREFERENCED_PARAMETER( Data );
	UNREFERENCED_PARAMETER( CompletionContext );
	ASSERT(IS_THIS_DEVICE_TYPE(Data->DeviceObject, DO_FSVOL ));
	return STATUS_SUCCESS;
}

VOID
	SfPostFsFilterPassThrough (
	IN PFS_FILTER_CALLBACK_DATA Data,
	IN NTSTATUS OperationStatus,
	IN PVOID CompletionContext
	)
	/*++

	Routine Description:

	This routine is the FS Filter post-operation "pass through" routine.

	Arguments:

	Data - The FS_FILTER_CALLBACK_DATA structure containing the information
	about this operation.

	OperationStatus - The status of this operation.        

	CompletionContext - A context that was set in the pre-operation 
	callback by this driver.

	Return Value:

	None.

	--*/
{
	UNREFERENCED_PARAMETER( Data );
	UNREFERENCED_PARAMETER( OperationStatus );
	UNREFERENCED_PARAMETER( CompletionContext );

	ASSERT(IS_THIS_DEVICE_TYPE(Data->DeviceObject, DO_FSVOL ));
}

//���̾���˺���
//����豸
NTSTATUS
	LCXLAddDevice (
	__in struct _DRIVER_OBJECT *DriverObject,
	__in struct _DEVICE_OBJECT *PhysicalDeviceObject
	)
{
	NTSTATUS status;
	PDEVICE_OBJECT newdev;
	
	//UNICODE_STRING drivername;
	DO_TYPE DeviceType;
	WCHAR DeviceGuid[128];
	ULONG ReturnLength;
	PAGED_CODE();

	//��ȡ��������
	//��ȡ����GUID����
	//���̾�����Ϊ{4D36E967-E325-11CE-BFC1-08002BE10318}
	DeviceType = DO_DISKVOL;
	if (NT_SUCCESS(IoGetDeviceProperty(PhysicalDeviceObject, DevicePropertyClassGuid, sizeof(DeviceGuid), DeviceGuid, &ReturnLength))) {
		UNICODE_STRING guidname;
		UNICODE_STRING diskguid;
		RtlInitUnicodeString(&guidname, DeviceGuid);
		RtlInitUnicodeString(&diskguid, L"{4D36E967-E325-11CE-BFC1-08002BE10318}");
		if (RtlCompareUnicodeString(&guidname, &diskguid, TRUE)==0) {
			DeviceType = DO_DISK;
		}
	}
#ifdef DBG
	if (DeviceType == DO_DISK) {
		KdPrint(("SYS:%wZ is a disk driver\n", &PhysicalDeviceObject->DriverObject->DriverName));
	} else {
		KdPrint(("SYS:%wZ is a disk volume driver\n", &PhysicalDeviceObject->DriverObject->DriverName));
	}
#endif
	//����һ�������豸������豸��FILE_DEVICE_DISK���͵��豸���Ҿ���EP_DEVICE_EXTENSION���͵��豸��չ
	status = IoCreateDevice(
		DriverObject,
		sizeof(EP_DEVICE_EXTENSION),
		NULL,
		FILE_DEVICE_DISK,
		FILE_DEVICE_SECURE_OPEN,
		FALSE,
		&newdev);
	if (NT_SUCCESS(status)) {
		PEP_DEVICE_EXTENSION devext;

		devext = (PEP_DEVICE_EXTENSION)newdev->DeviceExtension;
		RtlZeroMemory(devext, sizeof(EP_DEVICE_EXTENSION));
		//�����豸
		status = IoAttachDeviceToDeviceStackSafe(newdev, PhysicalDeviceObject, &devext->DE_FILTER.LowerDeviceObject);
		if (NT_SUCCESS(status)) {
			devext->DeviceType = DeviceType;
			RtlInitEmptyUnicodeString(&devext->DE_FILTER.PhysicalDeviceName, devext->DE_FILTER.PhysicalDeviceNameBuffer, sizeof(devext->DE_FILTER.PhysicalDeviceNameBuffer));
			GetObjectName(PhysicalDeviceObject, &devext->DE_FILTER.PhysicalDeviceName);
			KdPrint(("SYS:LCXLAddDevice:%wZ-%wZ\n", &PhysicalDeviceObject->DriverObject->DriverName, &devext->DE_FILTER.PhysicalDeviceName));
			devext->DE_FILTER.PhysicalDeviceObject = PhysicalDeviceObject;
			//�Թ����豸���豸���Խ��г�ʼ���������豸���豸����Ӧ�ú������²��豸��ͬ
			newdev->Flags |= devext->DE_FILTER.LowerDeviceObject->Flags;
			
			//�������豸���豸���Լ��ϵ�Դ�ɷ�ҳ������
			newdev->Flags |= DO_POWER_PAGABLE;

			//����Ǵ��̾�����豸
			if (DeviceType == DO_DISKVOL) {
				VPostAttachDiskVolDevice(newdev);
			} else {
				DPostAttachDiskDevice(newdev);
			}
			//���ȥ�������豸�ĳ�ʼ����ʶ����ʾ����ʹ����
			newdev->Flags &= ~DO_DEVICE_INITIALIZING;
		} else {
			//ʧ�ܣ�ɾ�����̾�����豸
			IoDeleteDevice(newdev);
		}
	}
	return status;
}

//��ͬ����

#define TAG_GET_OBJ_NAME 'OBJN'
//��ȡ��������
NTSTATUS GetObjectName (
	IN PVOID Object,
	IN OUT PUNICODE_STRING Name
	)
	/*++

	Routine Description:

	This routine will return the name of the given object.
	If a name can not be found an empty string will be returned.

	Arguments:

	Object - The object whose name we want

	Name - A unicode string that is already initialized with a buffer that
	receives the name of the object.

	Return Value:

	None

	--*/
{
	NTSTATUS status;
	ULONG retLength;
	POBJECT_NAME_INFORMATION objname;

	objname = (POBJECT_NAME_INFORMATION)ExAllocatePoolWithTag( NonPagedPool, Name->MaximumLength+sizeof(OBJECT_NAME_INFORMATION), TAG_GET_OBJ_NAME);
	if (objname == NULL) {
		KdPrint(("SYS:GetObjectName ExAllocatePoolWithTag Fail!\n"));
		//������Դ����
		status = STATUS_INSUFFICIENT_RESOURCES;
		return status;
	}
	// ��ѯ����
	status = ObQueryNameString( Object, objname, Name->MaximumLength+sizeof(OBJECT_NAME_INFORMATION), &retLength);
	//����ɹ�
	if (NT_SUCCESS( status )) {
		Name->Length = 0;
		RtlCopyUnicodeString( Name, &objname->Name );
	} else {
		//�����صĳ��ȷŵ�Length��
		Name->Length = (USHORT)retLength-sizeof(OBJECT_NAME_INFORMATION);
	}
	ExFreePoolWithTag(objname,TAG_GET_OBJ_NAME);

	return status;
}