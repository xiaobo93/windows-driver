#pragma  once
#include <ntifs.h>
#include<ntstrsafe.h>

#pragma warning(disable : 4995)
#pragma warning(disable : 4996)

#define  MAX_PATH 260
ULONG g_LogCount = 0; //日志个数
LIST_ENTRY g_LogList = {0}; //存放日志内容
KSPIN_LOCK g_LogLock = 0;  //链表锁
int g_ExitThread = 0;

//日志路径： g_LogPath\\日期\\g_logName_时间
WCHAR g_LogPath[MAX_PATH] ;
WCHAR g_logName[MAX_PATH] ;

//动态申请空间的标记 
#define GAGBASE_TAG 'GBA'

WORK_QUEUE_ITEM IopErrorLogWorkItem;

typedef struct _GAG_LOG_CONTEXT
{
    KDPC ErrorLogDpc;
    KTIMER ErrorLogTimer;
} GAG_LOG_CONTEXT, *PGAG_LOG_CONTEXT;


typedef struct _GAG_LOGINFO
{
    TIME_FIELDS Time;
    WCHAR ModuleName[30+2];
    WCHAR Log[600+2];

} GAG_LOGINFO, *PGAG_LOGINFO;

typedef struct _GAG_LOGLIST
{
    LIST_ENTRY	LogEntry;
    GAG_LOGINFO	LogtInfo;
}GAG_LOGLIST, *PGAG_LOGLIST;

//函数声明
VOID Log_SystemThread(IN PVOID StartContext);

NTSTATUS SysHelp_GetLocalTime(PTIME_FIELDS Time)
{
    NTSTATUS bRet = STATUS_UNSUCCESSFUL;
    LARGE_INTEGER CurrentTime;
    LARGE_INTEGER LocalTime;
    TIME_FIELDS TimeFiled;

    __try
    {
        if (NULL == Time)
        {
            goto exitlab;
        }

        KeQuerySystemTime(&CurrentTime);

        ExSystemTimeToLocalTime(&CurrentTime, &LocalTime);

        RtlTimeToTimeFields(&LocalTime, Time);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        DbgPrint("[GagSysBase.sys] <SysHelp_GetLocalTime> EXCEPTION_EXECUTE_HANDLER\n");
        goto exitlab;
    }

    bRet = STATUS_SUCCESS;

exitlab:
    return bRet;
}
NTSTATUS SysHelp_UnInit()
{

    g_ExitThread = 1;
    while( g_ExitThread == 1)
    {
        LARGE_INTEGER timeout;
        timeout.QuadPart = -10 * 1000 * 1000;
        KeDelayExecutionThread(KernelMode, FALSE, &timeout);
        DbgPrint("等待 队列工作线程 结束");
    }

    DbgPrint("SysHelp_UnInit\n");
    return STATUS_SUCCESS;
}

VOID Log_SystemThread(IN PVOID StartContext)
{
    WCHAR LogPath[MAX_PATH] = { 0 };
    WCHAR LogDirectory[MAX_PATH] = { 0 };
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    HANDLE fileHandle = NULL;
    UNICODE_STRING filePath;
    OBJECT_ATTRIBUTES oa = { 0 };
    KIRQL oldIRQL = 0;
    PLIST_ENTRY myEntry = NULL;
    PGAG_LOGLIST pLogNode = NULL;

    while (TRUE)
    {//日志处理循环
        IO_STATUS_BLOCK isb = { 0 };
        TIME_FIELDS Time = { 0 };
        PCHAR WriteBuffer = NULL;

        //while( g_LogCount <= 20 )
        while(g_LogCount <= 0)
        {
            LARGE_INTEGER timeout;
            timeout.QuadPart = -10 * 1000 * 1000;
            KeDelayExecutionThread(KernelMode, FALSE, &timeout);
            if(g_ExitThread)
            {//退出循环
               g_ExitThread = 0;
               DbgPrint("sdfasdfasdfasdfasfd\n");
               return;
            }
        }
        //创建日志全路径
        if (!NT_SUCCESS(SysHelp_GetLocalTime(&Time)))
        {//创建一个DPC，重新启动处理流程。
            //Log_QueueRequest();
            DbgPrint("[GagSysBase.sys] <Log_SystemThread> SysHelp_GetLocalTime Error.\n");
            continue;
        }
        RtlStringCchPrintfW(LogPath, MAX_PATH, L"\\??\\%ws\\%04d-%02d-%02d\\%ws_%04d-%02d-%02d.log", 
            g_LogPath, Time.Year,Time.Month,Time.Day,g_logName ,Time.Year,Time.Month,Time.Day);
        RtlInitUnicodeString(&filePath, LogPath);
        InitializeObjectAttributes(&oa, &filePath, OBJ_CASE_INSENSITIVE|OBJ_KERNEL_HANDLE, NULL, NULL);
        //打开文件
        status = ZwCreateFile(
            &fileHandle,
            FILE_APPEND_DATA,
            &oa,
            &isb,
            NULL,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ,
            FILE_OPEN_IF,
            FILE_SYNCHRONOUS_IO_NONALERT,
            NULL,
            0
            );
        if (!NT_SUCCESS(status))
        {                
            if(status ==  STATUS_OBJECT_PATH_NOT_FOUND) //0xC000003A
            {
                //创建路径
                OBJECT_ATTRIBUTES dirOa = { 0 };
                UNICODE_STRING dirFilePath;
                WCHAR dirLogPath[MAX_PATH] = { 0 };
                RtlStringCchPrintfW(dirLogPath, MAX_PATH, L"\\??\\%ws\\%04d-%02d-%02d", 
                    g_LogPath, Time.Year,Time.Month,Time.Day);
                RtlInitUnicodeString(&dirFilePath, dirLogPath);
                InitializeObjectAttributes(&dirOa, &dirFilePath, OBJ_CASE_INSENSITIVE|OBJ_KERNEL_HANDLE, NULL, NULL);
                status = ZwCreateFile(
                    &fileHandle,
                    FILE_APPEND_DATA,
                    &dirOa,
                    &isb,
                    NULL,
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ,
                    FILE_OPEN_IF,
                    FILE_SYNCHRONOUS_IO_NONALERT|FILE_DIRECTORY_FILE,
                    NULL,
                    0
                    );
                if(fileHandle != NULL)
                {
                    ZwClose(fileHandle);
                    fileHandle = NULL;
                }
                if(NT_SUCCESS(status))
                {
                    //Log_QueueRequest();
                    continue;
                }
                DbgPrint("[GagSysBase.sys] <Log_SystemThread> Call ZwCreateFile %ws Failed, status=0x%X\n", filePath.Buffer, status);
            }
            continue;
        }

        WriteBuffer = (PCHAR)ExAllocatePoolWithTag(NonPagedPool, 700, GAGBASE_TAG);
        if (NULL == WriteBuffer)
        {
            continue;
        }
        while(!IsListEmpty(&g_LogList))
        {
            //提取日志信息
            KeAcquireSpinLock(&g_LogLock, &oldIRQL);
            myEntry = RemoveHeadList(&g_LogList);
            --g_LogCount;
            KeReleaseSpinLock(&g_LogLock, oldIRQL);
            pLogNode = CONTAINING_RECORD(myEntry, GAG_LOGLIST, LogEntry);

            //申请临时空间
            //拼接日志字符串，临时存储 大小为700

            RtlZeroMemory(WriteBuffer, 700);
            status = RtlStringCchPrintfA(WriteBuffer, 700, "[%02d-%02d-%02d] [%ws] %ws\r\n", 
                pLogNode->LogtInfo.Time.Hour,
                pLogNode->LogtInfo.Time.Minute,
                pLogNode->LogtInfo.Time.Second,
                pLogNode->LogtInfo.ModuleName,
                pLogNode->LogtInfo.Log);

            if(NT_SUCCESS(status))
            {
                //写入日志信息
                status = ZwWriteFile(
                    fileHandle,
                    NULL,
                    NULL,
                    NULL,
                    &isb,
                    WriteBuffer,
                    strlen(WriteBuffer),
                    NULL,
                    NULL);
                if (!NT_SUCCESS(status))
                {
                    DbgPrint("[GagSysBase.sys] <Log_SystemThread> Call ZwWriteFile Failed, status=0X%X\n", status);
                }
            }
            ExFreePoolWithTag(pLogNode, GAGBASE_TAG);
        } // end while(!IsListEmpty(&g_LogList))
        ExFreePoolWithTag(WriteBuffer,GAGBASE_TAG);
        if (fileHandle != NULL)
        {
            ZwClose(fileHandle);
            fileHandle = NULL;
        }
    } // end while (TRUE)
}

NTSTATUS Log_Init(PWCHAR logPath,PWCHAR logName)
{

    //初始化日志链表
	InitializeListHead(&g_LogList);
	KeInitializeSpinLock(&g_LogLock);

    //初始化日志路径和名称
    RtlZeroMemory(g_LogPath,MAX_PATH);
    RtlCopyMemory(g_LogPath,logPath,wcslen(logPath)*2);
    RtlZeroMemory(g_logName,MAX_PATH);
    RtlCopyMemory(g_logName,logName,wcslen(logName)*2);

    //添加任务到队列中，系统负责调用    
    ExInitializeWorkItem( &IopErrorLogWorkItem, Log_SystemThread, NULL );
    ExQueueWorkItem( &IopErrorLogWorkItem, DelayedWorkQueue );

	return STATUS_SUCCESS;
}
NTSTATUS SysHelp_Log_Add2(PWCHAR pModuleName, PWCHAR pLogContent)
{
    NTSTATUS bRet = STATUS_UNSUCCESSFUL;

    if (g_LogCount > 50000)
    {
        DbgPrint("[GagSysBase.sys] <SysHelp_Log_Add2> g_LogCount > 50000\n");
        return FALSE;
    }

    __try
    {
        do 
        {
            PGAG_LOGLIST pLogNode = NULL;
            KIRQL OldIRQL = 0;
            TIME_FIELDS Time = { 0 };

            if (!MmIsAddressValid(pModuleName) || !MmIsAddressValid(pLogContent))
            {
                DbgPrint("[GagSysBase.sys] <SysHelp_Log_Add2> MmIsAddressValid == FALSE\n");
                break;
            }

            pLogNode = (PGAG_LOGLIST)ExAllocatePoolWithTag(NonPagedPool, sizeof(GAG_LOGLIST), GAGBASE_TAG);
            if(NULL == pLogNode)
            {
                DbgPrint("[GagSysBase.sys] <SysHelp_Log_Add2> ExAllocatePoolWithTag  pLogNode Failed\n");
                break;
            }
            memset(pLogNode, 0, sizeof(GAG_LOGLIST));

            SysHelp_GetLocalTime(&Time);
            RtlCopyMemory(&pLogNode->LogtInfo.Time, &Time, sizeof(TIME_FIELDS));
            RtlStringCchCopyW(pLogNode->LogtInfo.ModuleName, 30+2, pModuleName);
            RtlStringCchCopyW(pLogNode->LogtInfo.Log, 600+2, pLogContent);

            KeAcquireSpinLock(&g_LogLock, &OldIRQL);
            InsertTailList(&g_LogList, &pLogNode->LogEntry);
            ++g_LogCount;
            KeReleaseSpinLock(&g_LogLock, OldIRQL);

            bRet = STATUS_SUCCESS;

        } while (FALSE);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        DbgPrint("[GagSysBase.sys] <SysHelp_Log_Add2> EXCEPTION_EXECUTE_HANDLER\n");
    }

    return bRet;
}   

NTSTATUS KernelWriteLog(PWCHAR pModuleName, PWCHAR pLogContent)
{
    if (pModuleName && pLogContent)
    {
        return SysHelp_Log_Add2(pModuleName, pLogContent);
    }
    return STATUS_UNSUCCESSFUL;
}