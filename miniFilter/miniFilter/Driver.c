#pragma  once 
#include<ntddk.h>

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	NTSTATUS status	= STATUS_SUCCESS;
	return status;
}