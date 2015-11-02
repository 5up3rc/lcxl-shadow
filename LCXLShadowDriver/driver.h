#ifndef _DRIVER_H_
#define _DRIVER_H_
//////////////////////////////////////////////////////////////////////////
//�ļ������豸�����̾�����豸�����̹����豸���õ�ͷ�ļ�����Ź��õĺ���
//��Դ�ļ���ʵ���˿����豸�Ĺ���
#include "driverlibinterface.h"
#include "diskiocode.h"
#include "aesex.h"

#define CONTROL_DEVICE_NAME L"\\FileSystem\\LCXLShadow"//�����豸
#define CONTROL_SYMBOL_LINK L"\\??\\LCXLShadow"//��������

#define MEM_TAG_RWBUFFER 'RWBF'//��д�߳��еĻ�����TAG
__inline PREDIRECT_RW_MEM AllocateRedirectIrpListEntry(ULONG length, ULONGLONG offset, ULONG Tag) {
	PREDIRECT_RW_MEM NewListEntry;

	NewListEntry = (PREDIRECT_RW_MEM)ExAllocatePoolWithTag(PagedPool, sizeof(REDIRECT_RW_MEM)+length, Tag);
	NewListEntry->buffer = (PBYTE)NewListEntry+sizeof(REDIRECT_RW_MEM);
	NewListEntry->length = length;
	NewListEntry->offset = offset;
	return NewListEntry;
};

__inline VOID FreeRedirectIrpListEntry(PREDIRECT_RW_MEM ListEntry) {
	ExFreePool(ListEntry);
};

typedef enum _REDIRECT_MODE {
	RM_DIRECT,//ֱ�Ӷ�д
	RM_IN_MEMORY,//�ڴ��ж�д
	RM_IN_DISK,//���̶�д
} REDIRECT_MODE, *PREDIRECT_MODE;//��д�ض���ģʽ



//���������SCSI��SATA��ATA��RAID��SSA������Ϊ�Ǳ��ش���
#define DIS_LOCAL_DISK(_BusType) \
	((_BusType) == BusTypeSas||(_BusType) == BusTypeScsi||(_BusType) == BusTypeiScsi||\
	(_BusType) == BusTypeSata||(_BusType) == BusTypeFibre||\
	(_BusType) == BusTypeAta||(_BusType) == BusTypeAtapi||\
	(_BusType) == BusTypeRAID||\
	(_BusType) == BusTypeSsa)


//��д������Ϊ1M
#define DefaultReadWriteBufferSize 0x10000
//��д�������ṹ
typedef struct _RW_BUFFER_REC {
	PVOID Buffer;
	ULONG BufferSize;
} RW_BUFFER_REC, *PRW_BUFFER_REC;

//��ʼ��
#define RW_INIT_MEM(_RWBuffer) \
	RtlZeroMemory((_RWBuffer), sizeof(RW_BUFFER_REC))

//������д�������ṹ
#define RW_ADJUST_MEM(_RWBuffer, _RWBufferSize)				\
	if ((_RWBuffer)->BufferSize < _RWBufferSize) {			\
	if ((_RWBuffer)->BufferSize > 0) {						\
		ExFreePool((_RWBuffer)->Buffer);					\
	}														\
	(_RWBuffer)->Buffer = NULL;								\
	(_RWBuffer)->BufferSize = _RWBufferSize;				\
	(_RWBuffer)->Buffer = ExAllocatePoolWithTag(PagedPool, (_RWBuffer)->BufferSize, MEM_TAG_RWBUFFER);\
	}

//�ͷŶ�д������
#define RW_FREE_MEM(_RWBuffer) \
	if ((_RWBuffer)->Buffer != NULL) {\
		ExFreePoolWithTag((_RWBuffer)->Buffer, MEM_TAG_RWBUFFER);\
	}


//��������ͼ��
NTSTATUS LCXLChangeDriverIcon(
	PDEVICE_OBJECT VolDiskPDO
	);



//��ȡ����������
//DeviceObject:�ļ�ϵͳ���˶���
NTSTATUS 
	LCXLGetDriverSetting(
	PDEVICE_OBJECT DeviceObject
	);

//������������
NTSTATUS LCXLSaveShadowSetting();

//�������³�ʼ��
VOID
	LCXLDriverReinitialize (
	__in struct _DRIVER_OBJECT *DriverObject,
	__in_opt PVOID Context,
	__in ULONG Count
	);
//���������ļ�
#define LCXL_SETTING_PATH L"\\LLShadowS.sys"

typedef enum _LCXL_DRIVER_SETTING_STATE {DS_NONE, DS_INITIALIZING, DS_INITIALIZED} LCXL_DRIVER_SETTING_STATE;

//#define 
//�洢���ļ����������ýṹ
typedef struct _LCXL_FILE_DRIVER_SETTING {
	LCXL_SHADOW_MODE ShadowMode;//���̱���ģʽ
	BOOLEAN NeedPass;//�Ƿ���Ҫ����
	WCHAR Passmd5[32];//�����MD5ֵ
	ULONG CustomProtectVolumeNum;//�Զ���Ҫ�����ľ������
	WCHAR CustomProtectVolume[26][MAX_DEVNAME_LENGTH];//�Զ���Ҫ�����ľ���豸���ƣ��Զ�������������ܱ���26�����̾�
} LCXL_FILE_DRIVER_SETTING, *PLCXL_FILE_DRIVER_SETTING;

typedef struct _LCXL_DISK_INFO {
	IDINFO DiskIDInfo;//����ID��Ϣ
	PDRIVER_OBJECT DiskDriverObject;//������������
	//PDRIVER_DISPATCH MFRead;//ԭʼ���������������DISK�����Ƿ񱻸���
	PDRIVER_DISPATCH OrgMFWrite;//ԭʼд�������������DISK�����Ƿ񱻸���
} LCXL_DISK_INFO, *PLCXL_DISK_INFO;//������Ϣ

//�������ýṹ
typedef struct _LCXL_DRIVER_SETTING {
	LCXL_DRIVER_SETTING_STATE InitState;//�Ƿ��ʼ����
	KEVENT SettingInitEvent;//��ʼ���ɹ����¼�
	ULONG DriverErrorType;//���������й������Ƿ��д���
	LCXL_SHADOW_MODE CurShadowMode;//��ǰ���̱���ģʽ
	PDEVICE_OBJECT SettingDO;//���������ļ����ڵ���������
	PRETRIEVAL_POINTERS_BUFFER FileClusters;//�����ļ��Ĵ��б�
	ULONGLONG DataClusterOffset;//���ݴ��б�ƫ����������FAT�ļ�ϵͳ�У�NTFS�ļ�ϵͳ��ֵΪ0
	ULONG BytesPerAllocationUnit;//�����ļ����ڴ��̾�Ĵش�С
	
	BOOLEAN IsFirstGetDiskInfo;//�Ƿ��Ѿ���ȡ������Ϣ
	LCXL_DISK_INFO DiskInfo;//������Ϣ
	//������Ҫд�뵽�ļ�������
	LCXL_FILE_DRIVER_SETTING SettingFile;
} LCXL_DRIVER_SETTING, *PLCXL_DRIVER_SETTING;

#define MEM_SETTING_TAG 'DSET'
//�����ļ��Ĵ�С��Ĭ��Ϊ4K
#define SETTING_FILE_SIZE 0x1000
//************************************
// Method:    LCXLWriteSettingToBuffer
// FullName:  LCXLWriteSettingToBuffer
// Access:    public 
// Returns:   NTSTATUS
// Qualifier: ������д�뻺������
// Parameter: IN PLCXL_DRIVER_SETTING DriverSetting�������ݽṹ
// Parameter: OUT PVOID * Buffer���ݻ���������������ʹ�����ʱ��ʹ��LCXLFreeSettingBuffer�ͷŵ�
// Parameter: OUT PULONG BufferSize���ݻ�������С
//************************************
NTSTATUS LCXLWriteSettingToBuffer(
	IN PLCXL_DRIVER_SETTING DriverSetting, 
	OUT PVOID *Buffer,
	OUT PULONG BufferSize
);

#define LCXLFreeSettingBuffer(_Buffer) ExFreePoolWithTag(_Buffer, MEM_SETTING_TAG)

//************************************
// Method:    LCXLReadSettingFromBuffer
// FullName:  LCXLReadSettingFromBuffer
// Access:    public 
// Returns:   NTSTATUS
// Qualifier: �ӻ������ж�ȡ����
// Parameter: IN PVOID Buffer
// Parameter: IN ULONG BufferSize
// Parameter: IN OUT PLCXL_DRIVER_SETTING DriverSetting
//************************************
NTSTATUS LCXLReadSettingFromBuffer(
	IN PVOID Buffer,
	IN ULONG BufferSize,
	IN OUT PLCXL_DRIVER_SETTING DriverSetting
	);

//************************************
// Method:    LCXLVolumeNeedProtect
// FullName:  LCXLVolumeNeedProtect
// Access:    public 
// Returns:   BOOLEAN
// Qualifier: ���̾��Ƿ���Ҫ����
// Parameter: PDEVICE_OBJECT FSFilterDO �ļ�ϵͳ���˶���
//************************************
BOOLEAN LCXLVolumeNeedProtect(PDEVICE_OBJECT FSFilterDO);

//���뵽�ض����д�б���
void LCXLInsertToRedirectRWMemList(IN PREDIRECT_RW_MEM RedirectListHead, IN PVOID buffer, IN ULONGLONG offset, IN ULONG length);
//���ض����д�б��ж�ȡ
void LCXLReadFromRedirectRWMemList(IN PREDIRECT_RW_MEM RedirectListHead, IN PVOID buffer, IN ULONGLONG offset, IN ULONG length);


//Ӧ�ò������ʱ���ýṹ
typedef struct _LCXL_APP_RUNTIME_SETTING {
	HANDLE PID;//����PID
} LCXL_APP_RUNTIME_SETTING, *PLCXL_APP_RUNTIME_SETTING;

//VCN_EXTENT�еĽṹ��
typedef struct _LCXL_VCN_EXTENT {
	LARGE_INTEGER NextVcn;
	LARGE_INTEGER Lcn;
} LCXL_VCN_EXTENT, *PLCXL_VCN_EXTENT;

//ͨ���������ӻ�ȡ�豸����
NTSTATUS CGetDeviceNamebySymbolicLink(
	IN OUT PUNICODE_STRING DeviceName
	);

//ͨ��DOS���ƻ�ȡ���̾�����豸
NTSTATUS CGetDiskVolFilterDOByDosName(
	IN LPWSTR DosName, 
	OUT PDEVICE_OBJECT *DiskVolFilterDO
	);
//ͨ��Volume���ƻ�ȡ���̾�����豸
NTSTATUS CGetDiskVolFilterDOByVolumeName(
	IN PUNICODE_STRING VolumeName, 
	OUT PDEVICE_OBJECT *DiskVolFilterDO
	);

//FileObject�йصĽṹ
//������֤һ�������Ƿ��ǿ��ŵ�
typedef struct _LCXL_FILE_CONTEXT {
	BYTE aeskey[16];//128bit��AES key
	BOOLEAN IsCredible;//�Ƿ��ǿ��ŵ�
	BOOLEAN PasswordRight;//�����Ƿ���ȷ
} LCXL_FILE_CONTEXT, *PLCXL_FILE_CONTEXT;

//����g_DriverSetting����
extern LCXL_DRIVER_SETTING g_DriverSetting;
//����g_CDO
extern PDEVICE_OBJECT g_CDO;
//ϵͳ��ʼ������¼�
extern KEVENT	g_SysInitEvent;
//���ڴ��ض���ĺ��б�
extern PAGED_LOOKASIDE_LIST g_TableMapList;

#endif