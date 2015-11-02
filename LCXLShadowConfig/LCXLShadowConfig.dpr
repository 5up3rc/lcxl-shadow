program LCXLShadowConfig;

uses
  SysUtils,
  Windows,
  untFunc in 'untFunc.pas',
  untDllInterface in '..\untDllInterface.pas',
  untSerInterface in '..\untSerInterface.pas',
  untCommDataType in '..\untCommDataType.pas',
  untCommFunc in '..\untCommFunc.pas';

{$R *.res}

var
  tmpstr: string;
begin
  //�˳�
  if ParamCount > 0 then
  begin
    tmpstr := UpperCase(GetCmdLineSwitch(1));
    if tmpstr = UpperCase('Install') then
    begin
      OutputDebugStr('Config:��ʼ��װ');
      if not DI_IsDriverRunning then
      begin
        if not InstallLCXLShadowDriver or not InstallLCXLShadowSer then
        begin
          OutputDebugStr(Format('Config:��װ���������ʧ�ܣ���װ�����޷�����(%d)��',[GetLastError()]));
          ExitCode := PROC_EXIT_CODE_INSTALL_FAULT;
        end;
      end
      else
      begin
        OutputDebugStr('Config:�����������У���װ�����޷����С�');
        ExitCode := PROC_EXIT_CODE_DRIVER_RUNNING;
      end;
    end
    else
    if tmpstr = UpperCase('Uninstall') then
    begin
      OutputDebugStr('Config:��ʼж��');
      if not DI_InShadowMode then
      begin
        if not UninstallLCXLShadowSer or not UninstallLCXLShadowDriver then
        begin
          OutputDebugStr('Config:ж�����������ʧ�ܣ���ж��ʧ��');
          ExitCode := PROC_EXIT_CODE_UNINSTALL_FAULT;
        end;
      end
      else
      begin
        OutputDebugStr('Config:����Ӱ��ģʽ��ж��ʧ��');
        ExitCode := PROC_EXIT_CODE_IN_SHADOW_MODE;
      end;
    end
    else
    if (tmpstr = UpperCase('Help')) or (tmpstr = UpperCase('?')) then
    begin
      MessageBox(0, PChar(RS_HELP_TEXT), '', MB_ICONINFORMATION);
    end
    else
    if (tmpstr = UpperCase('InShadowMode')) then
    begin
      //�Ƿ���Ӱ��ģʽ��
      if DI_InShadowMode then
      begin
        ExitCode := PROC_EXIT_CODE_IN_SHADOW_MODE;
      end;
    end;

  end
  else
  begin
    ExitCode := PROC_EXIT_CODE_NO_PARAM;
  end;

end.
