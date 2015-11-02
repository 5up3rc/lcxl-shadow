#ifndef _DRIVER_LIB_H_
#define _DRIVER_LIB_H_
//////////////////////////////////////////////////////////////////////////
//Դ�ļ���LCXLShadowLib.h
//���ߣ��޳���
//˵���������뼯��3��������3�����������󽫸������ֱ��������̹���������
//�ļ������������ͷ����豸

#define DELAY_ONE_MICROSECOND   (-10)
#define DELAY_ONE_MILLISECOND   (DELAY_ONE_MICROSECOND*1000)
#define DELAY_ONE_SECOND        (DELAY_ONE_MILLISECOND*1000)

#ifndef Add2Ptr
#define Add2Ptr(P,I) ((PVOID)((PUCHAR)(P) + (I)))
#endif

//�Ƿ����ҵ��豸
#define IS_MY_DEVICE(_devObj)\
	(((_devObj)->DriverObject == g_LCXLDriverObject) && \
	((_devObj)->DeviceExtension != NULL))

//�ж��豸����
#define GET_MY_DEVICE_TYPE(_devObj) (((PEP_DEVICE_EXTENSION)(_devObj)->DeviceExtension)->DeviceType)

//
//  Macro to test if this is my device object
//
//
//  �ж��豸���ͣ��Ƿ��������豸
//

#define IS_THIS_DEVICE_TYPE(_devObj, _devType) \
	(((_devObj)->DriverObject == g_LCXLDriverObject) && \
	((_devObj)->DeviceExtension != NULL) && \
	(((PEP_DEVICE_EXTENSION)(_devObj)->DeviceExtension)->DeviceType == _devType))

//
//  �Ƿ���Ҫ���˵��豸
//

#define IS_DESIRED_DEVICE_TYPE(_type) \
	(((_type) == FILE_DEVICE_DISK_FILE_SYSTEM) || \
	((_type) == FILE_DEVICE_CD_ROM_FILE_SYSTEM) || \
	((_type) == FILE_DEVICE_NETWORK_FILE_SYSTEM))

//
//  Macro to test if FAST_IO_DISPATCH handling routine is valid
//

#define VALID_FAST_IO_DISPATCH_HANDLER(_FastIoDispatchPtr, _FieldName) \
	(((_FastIoDispatchPtr) != NULL) && \
	(((_FastIoDispatchPtr)->SizeOfFastIoDispatch) >= \
	(FIELD_OFFSET(FAST_IO_DISPATCH, _FieldName) + sizeof(void *))) && \
	((_FastIoDispatchPtr)->_FieldName != NULL))


//�������
NTSTATUS DriverEntry(
	IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING RegistryPath
	);

//����ж������
VOID DriverUnload(
	IN PDRIVER_OBJECT DriverObject
	);


NTSTATUS LCXLPassThrough(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
	);

NTSTATUS LCXLCreate(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
	);

NTSTATUS LCXLRead(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
	);

NTSTATUS LCXLWrite(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
	);

NTSTATUS LCXLDeviceControl(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
	);

NTSTATUS LCXLFileSystemControl(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
	);

NTSTATUS LCXLCleanup(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
	);

NTSTATUS LCXLClose(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
	);

NTSTATUS LCXLPower(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
	);

NTSTATUS LCXLPnp(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
	);

NTSTATUS LCXLShutdown(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
	);

//�ļ���������

//���ļ�ϵͳ�豸�����仯ʱ������ô˺���
VOID FDriverFSNotification (
	__in struct _DEVICE_OBJECT *DeviceObject,
	__in BOOLEAN FsActive
	);

//�ļ�ϵͳ�����ؾ�
NTSTATUS FSMountVolume (
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
	);
//IRP_MN_LOAD_FILE_SYSTEM
NTSTATUS FSLoadFileSystem(
	PDEVICE_OBJECT DeviceObject, 
	PIRP Irp
	);

//�ж��ǲ���ShadowCopyVolume��
NTSTATUS FIsShadowCopyVolume (
	IN PDEVICE_OBJECT StorageStackDeviceObject,
	OUT PBOOLEAN IsShadowCopy
	);
//�жϴ��豸�Ƿ��Ѿ������ǵ��豸������
BOOL FIsAttachedDevice (
	IN PDEVICE_OBJECT VolumeObject
	);

/*
 *���Թ��ؾ��豸
 *ע�⣺����˾��豸�Ѿ������ǵ��豸�����أ���ô���ص���TRUE
 */
BOOL FTtyAttachVolumeDevice (
	IN PDEVICE_OBJECT VolumeDeviceObject,//���豸
	IN PDEVICE_OBJECT DiskDeviceObject//�����豸
	);

//�����ļ�ϵͳ�豸����
NTSTATUS FAttachToVolumeDevice (
	IN PDEVICE_OBJECT VolumeDeviceObject,
	IN PDEVICE_OBJECT DiskDeviceObject,
	IN PUNICODE_STRING DiskDeviceName,
	OUT PDEVICE_OBJECT *FilterDeviceObject//���˵Ĵ����豸
	);

//�����ļ�ϵͳ�豸����
NTSTATUS FAttachToFSDevice (
	IN PDEVICE_OBJECT FSDeviceObject,
	IN BOOL IsNoAttachFSRecognizer//�Ƿ�����ļ�ϵͳʶ����
	);

//ö���ļ�ϵͳ���Ѿ����صľ�
NTSTATUS FEnumerateFileSystemVolumes(
	IN PDEVICE_OBJECT FSDeviceObject
	);

VOID FGetBaseDeviceObjectName (
	IN PDEVICE_OBJECT DeviceObject,
	IN OUT PUNICODE_STRING Name
	);
//Fast IOϵ�к���()

BOOLEAN SfFastIoCheckIfPossible(
	IN PFILE_OBJECT FileObject,
	IN PLARGE_INTEGER FileOffset,
	IN ULONG Length,
	IN BOOLEAN Wait,
	IN ULONG LockKey,
	IN BOOLEAN CheckForReadOperation,
	OUT PIO_STATUS_BLOCK IoStatus,
	IN PDEVICE_OBJECT DeviceObject
	);

BOOLEAN SfFastIoRead(
	IN PFILE_OBJECT FileObject,
	IN PLARGE_INTEGER FileOffset,
	IN ULONG Length,
	IN BOOLEAN Wait,
	IN ULONG LockKey,
	OUT PVOID Buffer,
	OUT PIO_STATUS_BLOCK IoStatus,
	IN PDEVICE_OBJECT DeviceObject
	);

BOOLEAN SfFastIoWrite(
	IN PFILE_OBJECT FileObject,
	IN PLARGE_INTEGER FileOffset,
	IN ULONG Length,
	IN BOOLEAN Wait,
	IN ULONG LockKey,
	IN PVOID Buffer,
	OUT PIO_STATUS_BLOCK IoStatus,
	IN PDEVICE_OBJECT DeviceObject
	);

BOOLEAN SfFastIoQueryBasicInfo(
	IN PFILE_OBJECT FileObject,
	IN BOOLEAN Wait,
	OUT PFILE_BASIC_INFORMATION Buffer,
	OUT PIO_STATUS_BLOCK IoStatus,
	IN PDEVICE_OBJECT DeviceObject
	);

BOOLEAN SfFastIoQueryStandardInfo(
	IN PFILE_OBJECT FileObject,
	IN BOOLEAN Wait,
	OUT PFILE_STANDARD_INFORMATION Buffer,
	OUT PIO_STATUS_BLOCK IoStatus,
	IN PDEVICE_OBJECT DeviceObject
	);

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
	);

BOOLEAN SfFastIoUnlockSingle(
	IN PFILE_OBJECT FileObject,
	IN PLARGE_INTEGER FileOffset,
	IN PLARGE_INTEGER Length,
	PEPROCESS ProcessId,
	ULONG Key,
	OUT PIO_STATUS_BLOCK IoStatus,
	IN PDEVICE_OBJECT DeviceObject
	);

BOOLEAN SfFastIoUnlockAll(
	IN PFILE_OBJECT FileObject,
	PEPROCESS ProcessId,
	OUT PIO_STATUS_BLOCK IoStatus,
	IN PDEVICE_OBJECT DeviceObject
	);

BOOLEAN SfFastIoUnlockAllByKey(
	IN PFILE_OBJECT FileObject,
	PVOID ProcessId,
	ULONG Key,
	OUT PIO_STATUS_BLOCK IoStatus,
	IN PDEVICE_OBJECT DeviceObject
	);

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
	);

VOID SfFastIoDetachDevice(
	IN PDEVICE_OBJECT SourceDevice,
	IN PDEVICE_OBJECT TargetDevice
	);

BOOLEAN SfFastIoQueryNetworkOpenInfo(
	IN PFILE_OBJECT FileObject,
	IN BOOLEAN Wait,
	OUT PFILE_NETWORK_OPEN_INFORMATION Buffer,
	OUT PIO_STATUS_BLOCK IoStatus,
	IN PDEVICE_OBJECT DeviceObject
	);

BOOLEAN SfFastIoMdlRead(
	IN PFILE_OBJECT FileObject,
	IN PLARGE_INTEGER FileOffset,
	IN ULONG Length,
	IN ULONG LockKey,
	OUT PMDL *MdlChain,
	OUT PIO_STATUS_BLOCK IoStatus,
	IN PDEVICE_OBJECT DeviceObject
	);


BOOLEAN SfFastIoMdlReadComplete(
	IN PFILE_OBJECT FileObject,
	IN PMDL MdlChain,
	IN PDEVICE_OBJECT DeviceObject
	);

BOOLEAN SfFastIoPrepareMdlWrite(
	IN PFILE_OBJECT FileObject,
	IN PLARGE_INTEGER FileOffset,
	IN ULONG Length,
	IN ULONG LockKey,
	OUT PMDL *MdlChain,
	OUT PIO_STATUS_BLOCK IoStatus,
	IN PDEVICE_OBJECT DeviceObject
	);

BOOLEAN SfFastIoMdlWriteComplete(
	IN PFILE_OBJECT FileObject,
	IN PLARGE_INTEGER FileOffset,
	IN PMDL MdlChain,
	IN PDEVICE_OBJECT DeviceObject
	);

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
	);

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
	);

BOOLEAN SfFastIoMdlReadCompleteCompressed(
	IN PFILE_OBJECT FileObject,
	IN PMDL MdlChain,
	IN PDEVICE_OBJECT DeviceObject
	);

BOOLEAN SfFastIoMdlWriteCompleteCompressed(
	IN PFILE_OBJECT FileObject,
	IN PLARGE_INTEGER FileOffset,
	IN PMDL MdlChain,
	IN PDEVICE_OBJECT DeviceObject
	);

BOOLEAN SfFastIoQueryOpen(
	IN PIRP Irp,
	OUT PFILE_NETWORK_OPEN_INFORMATION NetworkInformation,
	IN PDEVICE_OBJECT DeviceObject
	);

//
NTSTATUS
	SfPreFsFilterPassThrough (
	IN PFS_FILTER_CALLBACK_DATA Data,
	OUT PVOID *CompletionContext
	);

VOID
	SfPostFsFilterPassThrough (
	IN PFS_FILTER_CALLBACK_DATA Data,
	IN NTSTATUS OperationStatus,
	IN PVOID CompletionContext
	);

//���̾���˺���
NTSTATUS
	LCXLAddDevice (
	__in struct _DRIVER_OBJECT *DriverObject,
	__in struct _DEVICE_OBJECT *PhysicalDeviceObject
	);

//��ͬ����

//��ȡ��������
NTSTATUS GetObjectName (
	IN PVOID Object,
	IN OUT PUNICODE_STRING Name
	);

#endif