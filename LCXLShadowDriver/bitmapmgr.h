//λͼ�����ļ�
//  [1/26/2012 Administrator]
//  ���ߣ��޳���
#ifndef _BITMAP_MGR_H_
#define _BITMAP_MGR_H_

#include <windef.h>

//�洢λͼ�Ľṹ
//  ���������ڴ�
typedef struct _LCXL_BITMAP {
	//λͼ�ж��ٸ���λ����bitΪ��λ
	ULONGLONG	BitmapSize;
	// ÿ����ռ����ULONG_PTR
	ULONG		regionUlongPtr;
	// ָ��bitmap�洢�ռ��ָ���б�ĳ���
	ULONG		bufferListLen;
	// ָ��bitmap�洢�ռ��ָ���б�
	PULONG_PTR		*bufferList;
} LCXL_BITMAP, * PLCXL_BITMAP;

//��ʼ��λͼ�ṹ
NTSTATUS LCXLBitmapInit(
	OUT PLCXL_BITMAP bitmap,	 // λͼ���ָ��
	IN ULONGLONG BitmapSize,	 // λͼ�ж��ٸ���λ
	IN ULONG regionBytes	     // λͼ���ȣ��ֳ�N�飬һ��ռ����byte
	);

//����λͼ����
NTSTATUS LCXLBitmapSet(
	IN OUT PLCXL_BITMAP bitmap,	 // λͼ���ָ��
	IN ULONGLONG FirstBitmapIndex,// ��λͼ��ſ�ʼ����λΪ��λ
	IN ULONGLONG BitmapNum,		//Ҫд��BitmapNumλ������
	IN BOOLEAN IsSetTo1			//�Ƿ�����Ϊ1
	);

//��ȡλͼ����
BOOLEAN LCXLBitmapGet(
	IN PLCXL_BITMAP bitmap,	 // λͼ���ָ��
	IN ULONGLONG BitmapIndex // λͼ��ţ���λΪ��λ
	);

//�ͷ�λͼ�ṹ
VOID LCXLBitmapFina(
	IN OUT PLCXL_BITMAP bitmap	 // λͼ���ָ��
	);

//��ȡλͼ����
__inline BOOLEAN BitmapGet(
	IN PBYTE bitmap,		 // λͼ����ָ��
	IN ULONGLONG BitmapIndex // λͼ��ţ���λΪ��λ
	)
{
	ULONGLONG BitmapByteIndex;//λͼ��ţ����ֽ�Ϊ��λ
	BYTE BitmapBitIndex;//λͼ����ڴ��ֽ��е����

	BitmapByteIndex = BitmapIndex/8;//��ȡ��λͼ������ĸ��ֽ���
	BitmapBitIndex = BitmapIndex%8;//��ȡ��λͼ������ֽ��ϵ�λ��
	return ((bitmap[BitmapByteIndex]&(1<<BitmapBitIndex))!=0);
}

//����λͼ����
__inline VOID BitmapSet(
	IN OUT PBYTE bitmap,		 // λͼ����ָ��
	IN ULONGLONG BitmapIndex, // λͼ��ţ���λΪ��λ
	IN BOOLEAN IsSetTo1			//�Ƿ�����Ϊ1
	)
{
	ULONGLONG BitmapByteIndex;//λͼ��ţ����ֽ�Ϊ��λ
	BYTE BitmapBitIndex;//λͼ����ڴ��ֽ��е����

	BitmapByteIndex = BitmapIndex/8;//��ȡ��λͼ������ĸ��ֽ���
	BitmapBitIndex = BitmapIndex%8;//��ȡ��λͼ������ֽ��ϵ�λ��
	//�����Ϊ1
	if (IsSetTo1) {
		//����λͼ���ݣ�����Ӧ��������Ϊ1
		bitmap[BitmapByteIndex] |= (BYTE)1<<BitmapBitIndex;
	} else {
		//����
		bitmap[BitmapByteIndex] &= ~((BYTE)1<<BitmapBitIndex);
	}
}

//Ѱ�ҿ��е�λ(��Ϊ0��λ)�����
//������ص����ΪBitmapSize�������λͼ������
ULONGLONG LCXLBitmapFindFreeBit(
	IN PLCXL_BITMAP LcxlBitmap,		 // λͼ����ָ��
	IN ULONGLONG StartOffset //��StartOffset����ʼ�ң�ʵ����������StartOffset/sizeof(ULONG_PTR)*8/(sizeof(ULONG_PTR)*8)��ʼ�ң�Ҳ���ǰ�ULONG_PTR���룬����ٶ�
	);

//ͨ��Bitmap����LCXLBitmapλͼ��LCXLλͼ�Ĵ�СΪBitmapSize+BitmapOffset��
NTSTATUS LCXLBitmapCreateFromBitmap(
	IN PBYTE bitmap,		 // λͼ����ָ��
	IN ULONGLONG BitmapSize, // λͼ��С����λΪ��λ
	IN ULONGLONG BitmapOffset,//bitmapλͼ��LCXLBitmapλͼ��ƫ����
	IN ULONG regionBytes,	     // λͼ���ȣ��ֳ�N�飬һ��ռ����byte
	OUT PLCXL_BITMAP LcxlBitmap//�µ�LCXLλͼ
	);

#endif