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
