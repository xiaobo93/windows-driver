#pragma once
#include<ntddk.h>

PVOID g_MonitorHandle = NULL;

OB_PREOP_CALLBACK_STATUS
ProcessObjectPreCallback(
	__in PVOID  RegistrationContext,
	__in POB_PRE_OPERATION_INFORMATION  OperationInformation)
{//进程监控函数
	return 0;
}
OB_PREOP_CALLBACK_STATUS
ThreadObjectPreCallback(
	__in PVOID  RegistrationContext,
	__in POB_PRE_OPERATION_INFORMATION  OperationInformation)
{//线程监控函数
	return 0;
}
NTSTATUS StartMonitor()
{//开始进程监控
	if (KeGetCurrentIrql() > APC_LEVEL)
	{

	}
	NTSTATUS status = STATUS_SUCCESS;
	OB_OPERATION_REGISTRATION ob_op[2] = { 0 };
	OB_CALLBACK_REGISTRATION ob_callback = { 0 };
	UNICODE_STRING us;

	KdPrint(("开始进程监控\n"));

	ob_op[0].ObjectType = PsProcessType;
	ob_op[0].Operations = OB_OPERATION_HANDLE_CREATE;
	ob_op[0].PreOperation = ProcessObjectPreCallback;
	ob_op[0].PostOperation = NULL;

	ob_op[1].ObjectType = PsThreadType;
	ob_op[1].Operations = OB_OPERATION_HANDLE_CREATE;
	ob_op[1].PreOperation = ThreadObjectPreCallback;
	ob_op[1].PostOperation = NULL;

	RtlInitUnicodeString(&us, L"360010");
	ob_callback.Version = OB_FLT_REGISTRATION_VERSION;
	ob_callback.Altitude = us;
	ob_callback.OperationRegistrationCount = sizeof(ob_op) / sizeof(OB_OPERATION_REGISTRATION);
	ob_callback.OperationRegistration = ob_op;
	ob_callback.RegistrationContext = NULL;

	status = ObRegisterCallbacks(&ob_callback, &(g_MonitorHandle));
	return status;
}
NTSTATUS StopMonitor()
{//停止进程监控
	KdPrint(("停止进程监控\n"));
	ObUnRegisterCallbacks(g_MonitorHandle);
	return STATUS_SUCCESS;
}