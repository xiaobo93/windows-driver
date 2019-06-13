#pragma once 
//#include<ntddk.h>
#include"PublicHeader.h"
#include"PsMonitor.h"

VOID DriverUnload(PDRIVER_OBJECT DriverObject)
{
	KdPrint(("[%s]  \n",  __FUNCTION__));
	StopMonitor();
}


NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath)
{

	NTSTATUS status = STATUS_SUCCESS;
	KdPrint(("[%s] \n",__FUNCTION__));
	DriverObject->DriverUnload = DriverUnload;
	//
	StartMonitor();
	return status;
}