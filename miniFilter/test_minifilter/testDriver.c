/*
创建一个，minifilter框架。
*/
#pragma once
#include "CDO.h"
#include<fltKernel.h>
#include "callBack.h"

NTSTATUS DriverEntry(
					 IN PDRIVER_OBJECT pDriverObject,
					 IN PUNICODE_STRING pRegistryPath)
{
	NTSTATUS status = STATUS_SUCCESS;
	PSECURITY_DESCRIPTOR sd;
	OBJECT_ATTRIBUTES oa;
	UNICODE_STRING uniString;
	
	KdPrint(("Entry DriverEntry\n"));
	//注册minifilter框架 FilterRegistration存放回掉函数
	status = FltRegisterFilter(pDriverObject,&FilterRegistration,&g_FilterHandle);
	if(NT_SUCCESS(status))
	{
		status = FltStartFiltering(g_FilterHandle);
		if(!NT_SUCCESS(status))
		{
			FltUnregisterFilter(g_FilterHandle);
		}
	}

	//创建安全描述符
	status  = FltBuildDefaultSecurityDescriptor( &sd, FLT_PORT_ALL_ACCESS );
	if (!NT_SUCCESS( status )) {
		goto final;
	}
	RtlInitUnicodeString( &uniString, L"\\NPMiniPort" );
	InitializeObjectAttributes( &oa,
							   &uniString,
							   OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
							   NULL,
							   sd );
	/*
		FltCreateCommunicationPort creates a communication server port 
		on which a minifilter driver can receive connection requests 
		from user-mode applications.
	*/
	status = FltCreateCommunicationPort( g_FilterHandle,
										&g_ServerPort,
										&oa,
										NULL,
										NPMiniConnect,		//服务连接回调函数
										NPMiniDisconnect,   //服务断开回调函数
										NPMiniMessage,      //通信函数
										1 );
	FltFreeSecurityDescriptor( sd );
	if (!NT_SUCCESS( status ))
	{
		goto final;
	}  
final:
	if (!NT_SUCCESS( status ) ) 
	{

		if (NULL != g_ServerPort) {
			FltCloseCommunicationPort( g_ServerPort );
		}

		if (NULL != g_FilterHandle) {
			FltUnregisterFilter( g_FilterHandle );
		}       
	} 
	return status;
}