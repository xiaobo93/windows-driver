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
		//Ӧ�ù��˵�����ͷ��
		KdPrint(("%d�������� %wZ ����ַ��0x%x\n",i,&tmp->ModuleName,tmp->ModuleBaseAddress));
		//ָ����һ������
		ListEntry = ListEntry->Flink;
		tmp = CONTAINING_RECORD(ListEntry, LDR_DATA_TABLE_ENTRY, LoadOrder);
		i++;
	} while (tmp != ldr);
	return STATUS_SUCCESS;
}
VOID DriverUnload(PDRIVER_OBJECT DriverObject)
{

}
NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	ScanOSsys(DriverObject);
	//https://blog.csdn.net/HXC_HUANG/article/details/60471347
	DriverObject->DriverUnload = DriverUnload;
	return ntStatus;
}