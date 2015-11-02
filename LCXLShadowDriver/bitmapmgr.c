// [1/26/2012 Administrator]
// ���ߣ��޳���
#include <ntddk.h>
#include "bitmapmgr.h"

#define INFO_BITMAP_TAG 'BMPT'

//��ʼ��λͼ�ṹ
NTSTATUS LCXLBitmapInit(
	IN OUT PLCXL_BITMAP bitmap,	 // λͼ���ָ��
	IN ULONGLONG BitmapSize,	 // λͼ��С����λbitΪ��λ
	IN ULONG regionBytes	     // λͼ���ȣ��ֳ�N�飬һ��ռ����byte
	)
{
	ULONGLONG BitmapUlongPtrSize;//λͼ��С����UlongPtrΪ��λ
	NTSTATUS status;

	PAGED_CODE();

	ASSERT(bitmap!=NULL);
	ASSERT(regionBytes>0);
	//λͼ��С����λbitΪ��λ
	bitmap->BitmapSize = BitmapSize;
	//λͼ���ȣ��ֳ�N�飬һ��ռ����ULONG_PTR
	bitmap->regionUlongPtr = regionBytes/sizeof(ULONG_PTR)+(regionBytes%sizeof(ULONG_PTR)>0);
	
	//����λͼ��ULONG_PTR������С��һ��ULONG_PTR�ܴ�32bit��32λϵͳ��/64bit��64λϵͳ��������
	BitmapUlongPtrSize = BitmapSize/(sizeof(ULONG_PTR)<<3)+(BitmapSize%(sizeof(ULONG_PTR)<<3)>0);
	//����Buffer�б�ĳ���N(��ULONGLONG����ǿ��ת��ΪULONG)
	bitmap->bufferListLen = (ULONG)(BitmapUlongPtrSize/bitmap->regionUlongPtr+(BitmapUlongPtrSize%bitmap->regionUlongPtr>0));
	//����Buffer�б�ĳ����������ڴ����洢λͼ�б�
	bitmap->bufferList = (PULONG_PTR*)ExAllocatePoolWithTag( PagedPool, bitmap->bufferListLen*sizeof(PULONG_PTR), INFO_BITMAP_TAG);
	//������벻���ڴ�
	if (bitmap->bufferList != NULL) {
		//��ָ��ȫ�����㣬��ʾΪ��
		RtlZeroMemory(bitmap->bufferList, bitmap->bufferListLen*sizeof(PULONG_PTR));
		status = STATUS_SUCCESS;
	} else {
		//�ڴ治��
		status = STATUS_INSUFFICIENT_RESOURCES;
	}
	return  status;
}

NTSTATUS LCXLBitmapSet(
	IN OUT PLCXL_BITMAP bitmap,	 // λͼ���ָ��
	IN ULONGLONG FirstBitmapIndex,// ��λͼ��ſ�ʼ����λΪ��λ
	IN ULONGLONG BitmapNum,		//Ҫд��BitmapNumλ������
	IN BOOLEAN IsSetTo1			//�Ƿ�����Ϊ1
	)
{
	ULONGLONG BitmapUlongPtrIndex;//λͼ���ݵ���ţ���ULONG_PTRΪ��λ
	BYTE BitmapBitIndex;//���ֽ��е����

	ULONG BitmapBufferIndex;//��ȡλͼ�������ڵ��б����
	ULONG BitmapBufferUlongPtrIndex;//��ȡλͼ�������ڵ��б�����е��ĸ�ULONG_PTR��
	//ULONGLONG i;
	//���λͼ��ų�����λͼ��С�����ش���
	if (FirstBitmapIndex+BitmapNum>bitmap->BitmapSize) {
		//����������ֵ������������
		return STATUS_FWP_OUT_OF_BOUNDS;
	}
	for (; BitmapNum>0; FirstBitmapIndex++, BitmapNum--) {
		BitmapUlongPtrIndex = FirstBitmapIndex/(sizeof(ULONG_PTR)<<3);//��ȡ��λͼ������ĸ��ֽ���
		BitmapBitIndex = FirstBitmapIndex%(sizeof(ULONG_PTR)<<3);//��ȡ��λͼ������ֽ��ϵ�λ��
		BitmapBufferIndex = (ULONG)(BitmapUlongPtrIndex/bitmap->regionUlongPtr);//��ȡ��λͼ������ĸ��б������
		BitmapBufferUlongPtrIndex = BitmapUlongPtrIndex%bitmap->regionUlongPtr;//��ȡ��λͼ������б��е��ĸ�ULONG_PTRλ����
		//�����������λͼ��û��
		if (bitmap->bufferList[BitmapBufferIndex] == NULL) {
			//�����0������Ҫ����Ĳ�����
			//Ĭ������λͼ��Ϊ0
			if (!IsSetTo1) {
				continue;
			} else {
				//�����ڴ棬׼��������
				bitmap->bufferList[BitmapBufferIndex] = (PULONG_PTR)ExAllocatePoolWithTag(PagedPool, bitmap->regionUlongPtr*sizeof(ULONG_PTR), INFO_BITMAP_TAG);
				if (bitmap->bufferList[BitmapBufferIndex] != NULL) {
					//ȫ������
					RtlZeroMemory(bitmap->bufferList[BitmapBufferIndex], bitmap->regionUlongPtr*sizeof(ULONG_PTR));
				} else {
					//�ڴ治��
					return STATUS_INSUFFICIENT_RESOURCES;
				}
			}
		}

		//�����Ϊ1
		if (IsSetTo1) {
			//����λͼ���ݣ�����Ӧ��������Ϊ1
			bitmap->bufferList[BitmapBufferIndex][BitmapBufferUlongPtrIndex] |= (ULONG_PTR)1<<BitmapBitIndex;
		} else {
			//����
			bitmap->bufferList[BitmapBufferIndex][BitmapBufferUlongPtrIndex] &= ~((ULONG_PTR)1<<BitmapBitIndex);
		}
	}
	
	return STATUS_SUCCESS;
}

//��ȡλͼ����
BOOLEAN LCXLBitmapGet(
	IN PLCXL_BITMAP bitmap,	 // λͼ���ָ��
	IN ULONGLONG BitmapIndex // λͼ��ţ���λΪ��λ
	)
{
	ULONGLONG BitmapUlongPtrIndex;//λͼ���ݵ���ţ���ULONG_PTRΪ��λ
	BYTE BitmapBitIndex;//���ֽ��е����

	ULONG BitmapBufferIndex;//��ȡλͼ�������ڵ��б����
	ULONG BitmapBufferUlongPtrIndex;//��ȡλͼ�������ڵ��б�����е��ĸ�ULONG_PTR��

	//���λͼ��ų�����λͼ��С�����ش���
	if (BitmapIndex>=bitmap->BitmapSize) {
		KdPrint(("SYS:!!!LCXLBitmapGet:BitmapIndex(%I64d)>=bitmap->BitmapSize(%I64d)\n", BitmapIndex, bitmap->BitmapSize));
		return FALSE;
	}

	BitmapUlongPtrIndex = BitmapIndex/(sizeof(ULONG_PTR)<<3);//��ȡ��λͼ������ĸ��ֽ���
	BitmapBitIndex = BitmapIndex%(sizeof(ULONG_PTR)<<3);//��ȡ��λͼ������ֽ��ϵ�λ��
	BitmapBufferIndex = (ULONG)(BitmapUlongPtrIndex/bitmap->regionUlongPtr);//��ȡ��λͼ������ĸ��б������
	BitmapBufferUlongPtrIndex = BitmapUlongPtrIndex%bitmap->regionUlongPtr;//��ȡ��λͼ������б��е��ĸ�ULONG_PTRλ����
	//�����������λͼ��û��
	if (bitmap->bufferList[BitmapBufferIndex] == NULL) {
		//����0
		return FALSE;
	}
	return ((bitmap->bufferList[BitmapBufferIndex][BitmapBufferUlongPtrIndex]&((ULONG_PTR)1<<BitmapBitIndex))!=0);
}

//�ͷ�λͼ�ṹ
VOID LCXLBitmapFina(
	IN OUT PLCXL_BITMAP bitmap	 // λͼ���ָ��
	)
{
	PAGED_CODE();

	ASSERT(bitmap!=NULL);
	if (bitmap->bufferList != NULL) {
		ULONG i;
		for (i = 0; i < bitmap->bufferListLen; i++) {
			if (bitmap->bufferList[i] != NULL) {
				ExFreePoolWithTag(bitmap->bufferList[i], INFO_BITMAP_TAG);
			}
		}
		ExFreePoolWithTag(bitmap->bufferList, INFO_BITMAP_TAG);
	}
	//�������ṹ����
	RtlZeroMemory(bitmap, sizeof(LCXL_BITMAP));
}

//Ѱ�ҿ��е�λ(��Ϊ0��λ)�����
//������ص����ΪBitmapSize�������λͼ������
ULONGLONG LCXLBitmapFindFreeBit(
	IN PLCXL_BITMAP LcxlBitmap,// λͼ����ָ��
	IN ULONGLONG StartOffset //��StartOffset����ʼ�ң�ʵ����������StartOffset/sizeof(ULONG_PTR)*8/(sizeof(ULONG_PTR)*8)��Ҳ���ǰ�ULONG_PTR���룬����ٶ�
	)
{
	ULONGLONG BitmapUlongPtrIndex;//λͼ���ݵ���ţ���ULONG_PTRΪ��λ
	//BYTE BitmapBitIndex;//���ֽ��е����

	ULONG BitmapBufferIndex;//��ȡλͼ�������ڵ��б����
	ULONG BitmapBufferUlongPtrIndex;//��ȡλͼ�������ڵ��б�����е��ĸ�ULONG_PTR��
	ULONGLONG i;
	BYTE j;
	ULONG_PTR tmp;

	ASSERT(LcxlBitmap);
	//ʹ��ULONG_PTR�����Ч��
	BitmapUlongPtrIndex = StartOffset/(sizeof(ULONG_PTR)<<3);//��ȡ��λͼ������ĸ��ֽ���
	//���涼��
	for (i = BitmapUlongPtrIndex; i < LcxlBitmap->BitmapSize/(sizeof(ULONG_PTR)<<3); i++) {
		BitmapBufferIndex = (ULONG)(i/LcxlBitmap->regionUlongPtr);//��ȡ��λͼ������ĸ��б������
		
		if (LcxlBitmap->bufferList[BitmapBufferIndex] != NULL) {
			BitmapBufferUlongPtrIndex = i%LcxlBitmap->regionUlongPtr;//��ȡ��λͼ������б��е��ĸ�ULONG_PTRλ����
			tmp = LcxlBitmap->bufferList[BitmapBufferIndex][BitmapBufferUlongPtrIndex];
			if (tmp!=~((ULONG_PTR)0)) {
				//�����0ֵ
				for (j = 0; j < sizeof(ULONG_PTR)<<3; j++) {
					if ((tmp&((ULONG_PTR)1<<j))==0) {
						return (i*sizeof(ULONG_PTR)<<3)+j;
					}
				}
				//��Ӧ���Ҳ�����
				ASSERT(0);
			}
		} else {
			return i*sizeof(ULONG_PTR)<<3;
		}
	}
	//�ҵ�ĩβ�ˣ������ULONG_PTR���д���
	BitmapBufferIndex = (ULONG)(i/LcxlBitmap->regionUlongPtr);//��ȡ��λͼ������ĸ��б������
	if (LcxlBitmap->bufferList[BitmapBufferIndex] != NULL) {
		BitmapBufferUlongPtrIndex = i%LcxlBitmap->regionUlongPtr;//��ȡ��λͼ������б��е��ĸ�ULONG_PTRλ����
		tmp = LcxlBitmap->bufferList[BitmapBufferIndex][BitmapBufferUlongPtrIndex];
		for (j = 0; j < LcxlBitmap->BitmapSize%(sizeof(ULONG_PTR)<<3); j++) {
			if ((tmp&((ULONG_PTR)1<<j))==0) {
				return (i*sizeof(ULONG_PTR)<<3)+j;
			}
		}
	} else {
		return i*sizeof(ULONG_PTR)<<3;
	}
	//λͼ�п���λ������
	return LcxlBitmap->BitmapSize;
}

//ͨ��Bitmap����LCXLBitmapλͼ��LCXLλͼ�Ĵ�СΪBitmapSize+BitmapOffset��
NTSTATUS LCXLBitmapCreateFromBitmap(
	IN PBYTE bitmap,		   // λͼ����ָ��
	IN ULONGLONG BitmapSize,   // λͼ��С����λΪ��λ
	IN ULONGLONG BitmapOffset, // bitmapλͼ��LCXLBitmapλͼ��ƫ����
	IN ULONG regionBytes,	   // λͼ���ȣ��ֳ�N�飬һ��ռ����byte
	OUT PLCXL_BITMAP LcxlBitmap// �µ�LCXLλͼ
	)
{
	NTSTATUS status;
	//��λͼ��С�Ļ�����������8λ
	static INT reInt = 8;
	status = LCXLBitmapInit(LcxlBitmap, BitmapSize+BitmapOffset+reInt, regionBytes);
	if (NT_SUCCESS(status)) {
		ULONGLONG i;
		ULONGLONG j;

		for (j = 0, i = BitmapOffset; i<BitmapSize; i++, j++) {
			ULONG ByteIndex;
			ULONG BitIndex;

			ByteIndex = (ULONG)(j/8);
			BitIndex = j%8;
			status = LCXLBitmapSet(LcxlBitmap, i, 1, bitmap[ByteIndex]&(1<<BitIndex));
		}
	}
	return status;
}