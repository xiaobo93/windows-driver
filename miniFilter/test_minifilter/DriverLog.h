/*
接口调用规范： 
//初始化路径及文件名
Log_Init(L"c:",L"hello");
//写入日志
KernelWriteLog(L"EmptyDriver",L"DriverEntry");
//退出log处理函数
SysHelp_UnInit();
//////////////////////////////////////////////////////////////////////////
日志文件名称为： c:\\日期\\hello_日期
日志内容：[时间][EmptyDriver]DriverEntry
*/

#pragma  once;
/*
功能:
    
参数:
    logPath : 文件存放的路径
    name: 文件名

备注:
    
    日志存放文件的全路径格式如下：
    logPath\\日期\\logName.txt

功能:

    初始化日志功能;创建一个线程，循环处理链表中存储的数据内容
*/
NTSTATUS Log_Init(PWCHAR logPath , PWCHAR logName);

NTSTATUS SysHelp_UnInit();

/*

功能:
    向链表中插入一个字符串。
日志格式:
    [时间] [模块] [函数] [irql : %d，pid: %d]  （ULONG）KeGetCurrentIrql(),(ULONG)PsGetCurrentProcessId()
    errorFunction : %s  errorcode : 0x%x
*/
NTSTATUS KernelWriteLog(PWCHAR pModuleName, PWCHAR pLogContent);