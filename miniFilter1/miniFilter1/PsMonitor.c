#pragma once
#include"PsMonitor.h"
#define _WIN64 x
PVOID g_MonitorHandle = NULL;

OB_PREOP_CALLBACK_STATUS ProcessObjectPreCallback(
	__in PVOID  RegistrationContext,
	__in POB_PRE_OPERATION_INFORMATION  OperationInformation
)
{//���̼�غ���
	HANDLE targetID ;
	HANDLE sourceID;
	ACCESS_MASK desiredAccess;
	if (OperationInformation->KernelHandle)
	{//user_modeģʽ
		return OB_PREOP_SUCCESS;
	}
	desiredAccess = OperationInformation->Parameters->CreateHandleInformation.DesiredAccess;
	sourceID = (HANDLE)PsGetCurrentProcessId();
	targetID = (HANDLE)PsGetProcessId((PEPROCESS)OperationInformation->Object);
	
	KdPrint(("%d --- %d\n", targetID, sourceID));

	return OB_PREOP_SUCCESS;
}
OB_PREOP_CALLBACK_STATUS
ThreadObjectPreCallback(
	__in PVOID  RegistrationContext,
	__in POB_PRE_OPERATION_INFORMATION  OperationInformation)
{//�̼߳�غ���
	return OB_PREOP_SUCCESS;
}
NTSTATUS StartX64Monitor()
{
	NTSTATUS status = STATUS_SUCCESS;
	OB_OPERATION_REGISTRATION ob_op[2] = { 0 };
	OB_CALLBACK_REGISTRATION ob_callback = { 0 };
	UNICODE_STRING us;
	if (KeGetCurrentIrql() > APC_LEVEL)
	{
		KdPrint(("StartX64Monitor KeGetCurrentIrql() > APC_LEVEL "));
	}
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

	status = ObRegisterCallbacks(&ob_callback, &g_MonitorHandle);
	if (!NT_SUCCESS(status))
	{
		KdPrint(("ObRegisterCallbacks = 0x%x", status));
	}
	return status;
}
NTSTATUS StopX64Monitor()
{
	NTSTATUS status = STATUS_SUCCESS;
	KdPrint(("ֹͣ���̼��\n"));
	if (NULL != g_MonitorHandle)
	{
		ObUnRegisterCallbacks(g_MonitorHandle);
	}
	return status;
}
NTSTATUS StartMonitor()
{
	NTSTATUS status = STATUS_SUCCESS;
#ifdef _WIN64
	status = StartX64Monitor();
#else
	KdPrint(("x32λϵͳ���̱�����װ"));
#endif // _WIN64
	return status;
}
NTSTATUS StopMonitor()
{//ֹͣ���̼��
	NTSTATUS status = STATUS_SUCCESS;
#ifdef  _WIN64
	status = StopX64Monitor();
#else
	KdPrint(("x32λϵͳ���̱���ж��"));
#endif //  _WIN64
	return STATUS_SUCCESS;
}