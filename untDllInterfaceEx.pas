unit untDllInterfaceEx;

interface
uses
  Windows, SysUtils, untDllInterface, untCommFunc, Classes, ElAES, untCommDataType;



//���������˺���������������֤��һϵ�в���
function DI_OpenEx: THandle;

//��ȡ�����������б�
function DI_GetDiskDosNameList(hDevice: THandle): string;

var
  ID_INFO: IDINFO;

implementation

function DI_OpenEx: THandle;
var
  Sour, Dest: TMemoryStream;
  AESKey: TAESKey128;
  AESKeyLen: DWORD;
  IsSuc: Boolean;
begin
  Result := DI_Open;
  if Result = INVALID_HANDLE_VALUE then
  begin
    Exit;
  end;
  IsSuc := True;
  AESKeyLen := SizeOf(AESKey);
  if DI_GetRandomId(Result, @AESKey, AESKeyLen) then
  begin
    Sour := TMemoryStream.Create;
    Dest := TMemoryStream.Create;
    //д��������к�
    Sour.Write(ID_INFO, SizeOf(ID_INFO));
    Sour.Position := 0;
    try
      //��ʼAES����
      EncryptAESStreamECB(Sour, SizeOf(ID_INFO), AESKey, Dest);
    except
      IsSuc := False;
    end;
    if IsSuc then
    begin
      //��֤Ӳ��KEY
      IsSuc := DI_ValidateCode(Result, Dest.Memory, Dest.Position);
    end;
    Sour.Free;
    Dest.Free;
  end;
  //ʧ���ˣ�
  if not IsSuc then
  begin
    DI_Close(Result);
    Result := INVALID_HANDLE_VALUE;
  end;
end;

//��ȡ�����������б�
function DI_GetDiskDosNameList(hDevice: THandle): string;
var
  DiskListLen: DWORD;
  DriverDiskList: array of TVolumeDiskInfo;
  I: Integer;
begin
  DiskListLen := 100;
  SetLength(DriverDiskList, DiskListLen);
  if DI_GetDiskList(hDevice, @DriverDiskList[0], DiskListLen) then
  begin
    for I := 0 to DiskListLen-1 do
    begin
      Result := Result+DriverDiskList[i].DosName;
    end;
  end;
end;

initialization

//��ȡ�������к�
DiskIdentifyDevice(0, ID_INFO);

end.
