#pragma  once
#include <ntifs.h>
#include "head.h"
NTSTATUS ScanOSsys(PDRIVER_OBJECT DriverObject)
{
	PLDR_DATA_TABLE_ENTRY ldr = (PLDR_DATA_TABLE_ENTRY)DriverObject->DriverSection;
	PLDR_DATA_TABLE_ENTRY tmp = ldr;
	PLIST_ENTRY ListEntry = &ldr->LoadOrder;
	ULONG i = 0;
	do
	{
	Start:
		//应该过滤掉链表头。
		if (tmp->EntryPoint != 0x0 && tmp->ModuleSize != 0)
		{
			KdPrint(("%d驱动名称 %wZ 基地址：0x%x\n", i, &tmp->ModuleName, tmp->ModuleBaseAddress));
			i++;
			if (_wcsicmp(tmp->ModuleName.Buffer, L"miniFilter.sys") == 0)
			{
				_asm int 3;
				ListEntry->Blink->Flink = ListEntry->Flink;
				ListEntry->Flink->Blink = ListEntry->Blink;
				KdPrint(("驱动文件匹配成功\n"));
				if (ldr == tmp)
				{
					ListEntry = ListEntry->Flink;
					tmp = CONTAINING_RECORD(ListEntry, LDR_DATA_TABLE_ENTRY, LoadOrder);
					ldr = tmp;
					goto Start;
				}
			}
		}
		//指向下一个链表
		ListEntry = ListEntry->Flink;
		tmp = CONTAINING_RECORD(ListEntry, LDR_DATA_TABLE_ENTRY, LoadOrder);
	} while (tmp != ldr);

	return STATUS_SUCCESS;
}
VOID DriverUnload(PDRIVER_OBJECT DriverObject)
{

}
LARGE_INTEGER g_regCookie = { 0 };
NTSTATUS RegistryFilterCallBack(PVOID CallbackContext,PVOID Argument1,PVOID Argument2)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	REG_NOTIFY_CLASS notifyClass = (REG_NOTIFY_CLASS)(ULONG_PTR)Argument1;
	PREG_CREATE_KEY_INFORMATION info = (PREG_CREATE_KEY_INFORMATION)Argument2;
	switch (notifyClass)
	{
		//增、删、改、查
	//创建key值
	case RegNtPreCreateKey:
	case RegNtPreCreateKeyEx:
	{
		if (info->CompleteName->Length > sizeof(WCHAR) &&
			info->CompleteName->Buffer[0] == L'\\')
		{//绝对路径
			KdPrint(("=========%wZ\n",info->CompleteName));
		}
		else {
			//相对路径
			CHAR  strRootPath[256*2] = { 0 };
			ULONG uRetrurnLen = 0;
			POBJECT_NAME_INFORMATION pNameInfo =
				(POBJECT_NAME_INFORMATION)strRootPath;
			if (info->RootObject != NULL)
			{
				NTSTATUS status;
				PCUNICODE_STRING regPath;
				status = ObQueryNameString(info->RootObject,
					pNameInfo,
					sizeof(strRootPath),
					&uRetrurnLen);
				status = CmCallbackGetKeyObjectID(&g_regCookie, info->RootObject, NULL, &regPath);
				if (!NT_SUCCESS(status))
				{
					KdPrint(("Error, registry name query failed with code:%08x\n", status));
					return FALSE;
				}
				if (NT_SUCCESS(status))
				{
					//pNameInfo->Name 为路径，info->CompleteName 为新创建项目的名称。
					KdPrint(("///==%wZ== %wZ ==== %wZ\n",regPath, &(pNameInfo->Name),info->CompleteName));
				}
				else {
					KdPrint(("///===0x%x\n", status));
				}
			}
			else {
				KdPrint(("===failed\n"));
			}
		}
	}
		break;
	default:
		//KdPrint(("-------------%wZ\n", info->CompleteName));
		break;
	}
	return ntStatus;
}
NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	//ScanOSsys(DriverObject);
	//https://blog.csdn.net/HXC_HUANG/article/details/60471347
	//进程保护
	//文件保护
	//注册表保护
	{
		UNICODE_STRING altitude;
		RtlInitUnicodeString(&altitude, L"320000");
		ntStatus = CmRegisterCallbackEx(
			RegistryFilterCallBack,
			&altitude,
			DriverObject,
			NULL,
			&g_regCookie,
			NULL);
		if (!NT_SUCCESS(ntStatus))
		{
			KdPrint(("Error,registry filter registration failed with code :0x%x", ntStatus));
			return ntStatus;
		}
	}
	
	DriverObject->DriverUnload = DriverUnload;
	return ntStatus;
}