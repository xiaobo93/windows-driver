/*
�ӿڵ��ù淶�� 
//��ʼ��·�����ļ���
Log_Init(L"c:",L"hello");
//д����־
KernelWriteLog(L"EmptyDriver",L"DriverEntry");
//�˳�log������
SysHelp_UnInit();
//////////////////////////////////////////////////////////////////////////
��־�ļ�����Ϊ�� c:\\����\\hello_����
��־���ݣ�[ʱ��][EmptyDriver]DriverEntry
*/

#pragma  once;
/*
����:
    
����:
    logPath : �ļ���ŵ�·��
    name: �ļ���

��ע:
    
    ��־����ļ���ȫ·����ʽ���£�
    logPath\\����\\logName.txt

����:

    ��ʼ����־����;����һ���̣߳�ѭ�����������д洢����������
*/
NTSTATUS Log_Init(PWCHAR logPath , PWCHAR logName);

NTSTATUS SysHelp_UnInit();

/*

����:
    �������в���һ���ַ�����
��־��ʽ:
    [ʱ��] [ģ��] [����] [irql : %d��pid: %d]  ��ULONG��KeGetCurrentIrql(),(ULONG)PsGetCurrentProcessId()
    errorFunction : %s  errorcode : 0x%x
*/
NTSTATUS KernelWriteLog(PWCHAR pModuleName, PWCHAR pLogContent);