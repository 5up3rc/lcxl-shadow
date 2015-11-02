#include "winkernel.h"
#include "diskiocode.h"

// ����������IDENTIFY DEVICE���������豸��Ϣ
// hDevice: �豸���
// pIdInfo:  �豸��Ϣ�ṹָ��
NTSTATUS DiskIdentifyDevice(
	IN PDEVICE_OBJECT DiskPDO, 
	IN PIDINFO pIdInfo
	)
{
	PSENDCMDINPARAMS pSCIP;		// �������ݽṹָ��
	PSENDCMDOUTPARAMS pSCOP;	// ������ݽṹָ��
	KEVENT event;
	IO_STATUS_BLOCK iostatus;
	PIRP irp;
	NTSTATUS status;				// IOCTL����ֵ

	PAGED_CODE();

	// ��������/������ݽṹ�ռ�
	pSCIP = (PSENDCMDINPARAMS)ExAllocatePoolWithTag(PagedPool, sizeof(SENDCMDINPARAMS), 'DPDO');
	if (pSCIP==NULL) {
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	pSCOP = (PSENDCMDOUTPARAMS)ExAllocatePoolWithTag(PagedPool, sizeof(SENDCMDOUTPARAMS)+sizeof(IDINFO), 'DPDO');
	if (pSCOP==NULL) {
		ExFreePool(pSCOP);
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	RtlZeroMemory(pSCIP, sizeof(SENDCMDINPARAMS));
	RtlZeroMemory(pSCOP, sizeof(SENDCMDOUTPARAMS)+sizeof(IDINFO));
	
	// ָ��ATA/ATAPI����ļĴ���ֵ
	pSCIP->irDriveRegs.bCommandReg = IDE_ATA_IDENTIFY;

	// ָ������/������ݻ�������С
	pSCOP->cBufferSize = sizeof(IDINFO);

	KeInitializeEvent(&event, NotificationEvent, FALSE);
	// IDENTIFY DEVICE
	irp = IoBuildDeviceIoControlRequest(
		DFP_RECEIVE_DRIVE_DATA,
		DiskPDO,
		pSCIP,
		sizeof(SENDCMDINPARAMS),
		pSCOP,
		sizeof(SENDCMDOUTPARAMS) + sizeof(IDINFO),
		FALSE,
		&event,
		&iostatus);
	if (irp != NULL) {
		status = IoCallDriver(DiskPDO, irp);
		if (status == STATUS_PENDING) {
			KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
			status = iostatus.Status;
		}
		if (NT_SUCCESS(status)) {
			RtlCopyMemory(pIdInfo, pSCOP->bBuffer, sizeof(IDINFO));
		} else {
			KdPrint(("SYS:DDevicePnp DFP_RECEIVE_DRIVE_DATA failed(0x%08x)\n", status));
		}
	} else {
		//��Դ����
		status = STATUS_INSUFFICIENT_RESOURCES;
	}
	// �ͷ�����/������ݿռ�
	ExFreePool(pSCOP);
	ExFreePool(pSCIP);

	return status;
}