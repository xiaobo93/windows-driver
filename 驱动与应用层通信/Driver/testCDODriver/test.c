#pragma  once
#include <ntddk.h>
#define  DEVICE_NAME L"\\Device\\MTReadDevice"
#define  SYM_LINK_NAME L"\\DosDevices\\test"
PDEVICE_OBJECT g_cdoDevice;
UNICODE_STRING g_cdoDeivceName;
UNICODE_STRING g_cdoSymLinkName;
NTSTATUS commDispatch(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	Irp->IoStatus.Status = ntStatus;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return ntStatus;
}
NTSTATUS cdoDispatch(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
	ULONG cbin = irpStack->Parameters.DeviceIoControl.InputBufferLength;
	ULONG cbout = irpStack->Parameters.DeviceIoControl.OutputBufferLength;
	UCHAR *buffer = (UCHAR *)Irp->AssociatedIrp.SystemBuffer;
	ULONG code = irpStack->Parameters.DeviceIoControl.IoControlCode;
	ULONG info = 0;
	KdPrint(("code : 0x%x\n", code));
	KdPrint(("%s", buffer));
	strcpy(buffer, "xiaobo\n");
	Irp->IoStatus.Status = ntStatus;
	Irp->IoStatus.Information = cbout;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	KdPrint(("Enter cdoDispatch\n"));
	return ntStatus;
}
VOID Unload(IN PDRIVER_OBJECT DriverObject)
{
	IoDeleteSymbolicLink(&g_cdoSymLinkName);
	IoDeleteDevice(g_cdoDevice);
	KdPrint(("Enter ulond\n"));
}
NTSTATUS DriverEntry(
	IN PDRIVER_OBJECT  DriverObject,
	IN PUNICODE_STRING RegistryPath
)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	ULONG i = 0;
	for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
	{
		DriverObject->MajorFunction[i] = commDispatch;
	}
	KdPrint(("enter driverEntry\n"));
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = cdoDispatch;
	DriverObject->DriverUnload = Unload;
	RtlInitUnicodeString(&g_cdoDeivceName,DEVICE_NAME);
	RtlInitUnicodeString(&g_cdoSymLinkName, SYM_LINK_NAME);
	ntStatus = IoCreateDevice(
		DriverObject,
		0, 
		&g_cdoDeivceName,
		FILE_DEVICE_UNKNOWN, 
		0, FALSE,
		&g_cdoDevice
	);
	if (!NT_SUCCESS(ntStatus))
	{
		KdPrint(("create device failed \n"));
		return STATUS_UNSUCCESSFUL;
	}
	g_cdoDevice->Flags |= DO_BUFFERED_IO;
	ntStatus = IoCreateSymbolicLink(&g_cdoSymLinkName, &g_cdoDeivceName);
	if (!NT_SUCCESS(ntStatus))
	{
		KdPrint(("iocreatesymbolicLink is failed \n"));
		return STATUS_UNSUCCESSFUL;
	}
	return ntStatus;
}