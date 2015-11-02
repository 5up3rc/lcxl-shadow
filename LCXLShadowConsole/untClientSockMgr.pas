unit untClientSockMgr;

interface
uses
  Windows, System.SysUtils, LCXLIOCPBase, LCXLIOCPCmd;
type
  TClientSockLst = class(TCmdSockLst)
  protected
    procedure CreateSockObj(var SockObj: TSocketObj); override; // ����
  end;

  TClientSockObj = class(TCmdSockObj)
  private
    //�Ƿ񾭹���֤
    FIsAuthed: Boolean;
    FPassIsEmpty: Boolean;
    FPassIsVerified: Boolean;
  public
    property IsAuthed: Boolean read FIsAuthed write FIsAuthed;
    property PassIsEmpty: Boolean read FPassIsEmpty write FPassIsEmpty;
    property PassIsVerified: Boolean read FPassIsVerified write FPassIsVerified;
  end;

  TIOCPClientList = class(TIOCPCMDList)

  end;

implementation

{ TClientSockLst }

procedure TClientSockLst.CreateSockObj(var SockObj: TSocketObj);
begin
  SockObj := TClientSockObj.Create;

end;

end.
