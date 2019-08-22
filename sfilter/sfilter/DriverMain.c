#pragma once
#include<ntddk.h>
#include<ntstrsafe.h>
PDEVICE_OBJECT gSFilterControlDeivceObject = NULL;
PDRIVER_OBJECT gSFilterDriverObject = NULL;

typedef struct _SFILTER_DEVICE_EXTENSION {

	ULONG TypeFlag;
	//
	//  Pointer to the file system device object we are attached to
	//
	PDEVICE_OBJECT AttachedToDeviceObject; //��һ���豸����
	//
	//  Pointer to the real (disk) device object that is associated with
	//  the file system device object we are attached to
	//
	PDEVICE_OBJECT StorageStackDeviceObject;//��ʵ���豸����

	//
	//  Name for this device.  If attached to a Volume Device Object it is the
	//  name of the physical disk drive.  If attached to a Control Device
	//  Object it is the name of the Control Device Object.
	//

	UNICODE_STRING DeviceName;

	//
	//  Buffer used to hold the above unicode strings
	//

	WCHAR DeviceNameBuffer[64];

	// 
	// The extension used by other user.
	//

	UCHAR UserExtension[1];

} SFILTER_DEVICE_EXTENSION, *PSFILTER_DEVICE_EXTENSION;

VOID DriverUnload(PDRIVER_OBJECT pDriverObject)
{
	
}
NTSTATUS SfPassThrough(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{//ͨ����ǲ����
	NTSTATUS ntStatus = STATUS_SUCCESS;
	PSFILTER_DEVICE_EXTENSION devExt = (PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	if (DeviceObject == NULL ||   //��Ч����
		DeviceObject == gSFilterControlDeivceObject //�����豸����
		|| DeviceObject->DriverObject == gSFilterDriverObject
		)
	{
		ntStatus = STATUS_UNSUCCESSFUL;
		goto SfPassThrough_exit;
	}
	IoSkipCurrentIrpStackLocation(Irp);
	ntStatus = IoCallDriver(devExt->AttachedToDeviceObject, Irp);
SfPassThrough_exit:
	return ntStatus;
}
NTSTATUS
DriverEntry(
	IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING RegistryPath
)
{
	NTSTATUS status = STATUS_SUCCESS;
	UNICODE_STRING nameString;
	int i = 0;
	gSFilterDriverObject = DriverObject;
	RtlInitUnicodeString(&nameString, L"\\FileSystem\\Filters\\SFilter");
	DriverObject->DriverUnload = DriverUnload;
	status = IoCreateDevice(DriverObject,
		0,
		&nameString,
		FILE_DEVICE_DISK_FILE_SYSTEM,
		FILE_DEVICE_SECURE_OPEN,
		FALSE,
		&gSFilterControlDeivceObject);
	if (status == STATUS_OBJECT_PATH_NOT_FOUND)
	{
		RtlInitUnicodeString(&nameString, L"\\FileSystem\\SFilterCDO");
		status = IoCreateDevice(DriverObject,
			0,
			&nameString,
			FILE_DEVICE_DISK_FILE_SYSTEM,
			FILE_DEVICE_SECURE_OPEN,
			FALSE,
			&gSFilterControlDeivceObject);
		if (!NT_SUCCESS(status))
		{
			KdPrint(("[%s]�����豸����(%ws)������ʧ��\n", nameString.Buffer));
			return status;
		}
	}
	else
	{
		KdPrint(("[%s]�����豸����(%ws)������ʧ��\n",nameString.Buffer));
	}
	for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
	{
		DriverObject->MajorFunction[i] = SfPassThrough;
	}
	DriverObject->MajorFunction[IRP_MJ_CREATE];
	DriverObject->MajorFunction[IRP_MJ_CREATE_NAMED_PIPE];
	DriverObject->MajorFunction[IRP_MJ_CREATE_MAILSLOT];
	DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL];
	DriverObject->MajorFunction[IRP_MJ_CLEANUP];
	DriverObject->MajorFunction[IRP_MJ_CLOSE];

	return status;
}
