#pragma once 
#include<ntddk.h>


VOID DriverUnload(PDRIVER_OBJECT DriverObject)
{
	KdPrint(("[%s]  \n",  __FUNCTION__));
}


NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath)
{

	NTSTATUS status = STATUS_SUCCESS;
	KdPrint(("[%s] \n",__FUNCTION__));
	DriverObject->DriverUnload = DriverUnload;
	return status;
}