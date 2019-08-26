#pragma once
#include<ntifs.h>
#include<ntddk.h>
#include<ntstrsafe.h>
#include "FastIo.h"
#include "FileCallBack.h"

PDEVICE_OBJECT gSFilterControlDeivceObject = NULL;
PDRIVER_OBJECT gSFilterDriverObject = NULL;

typedef struct _SFILTER_DEVICE_EXTENSION {

	ULONG TypeFlag;
	//
	//  Pointer to the file system device object we are attached to
	//
	PDEVICE_OBJECT AttachedToDeviceObject; //下一个设备对象
	//
	//  Pointer to the real (disk) device object that is associated with
	//  the file system device object we are attached to
	//
	PDEVICE_OBJECT StorageStackDeviceObject;//真实的设备对象

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
{//通用派遣函数
	NTSTATUS ntStatus = STATUS_SUCCESS;
	PSFILTER_DEVICE_EXTENSION devExt = (PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	if (DeviceObject == NULL ||   //无效操作
		DeviceObject == gSFilterControlDeivceObject //控制设备操作
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
VOID
SfFsNotification(
	IN PDEVICE_OBJECT DeviceObject,
	IN BOOLEAN FsActive
)
{//监测新创建的设备信息。

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
	RtlInitUnicodeString(&nameString, L"\\FileSystem\\Filters\\SFilterCDO");
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
			KdPrint(("[%s]控制设备对象(%ws)，生成失败\n", nameString.Buffer));
			return status;
		}
	}
	else
	{
		KdPrint(("[%s]控制设备对象(%ws)，生成失败\n",nameString.Buffer));
	}
	for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
	{
		DriverObject->MajorFunction[i] = SfPassThrough;
	}
	DriverObject->MajorFunction[IRP_MJ_CREATE] ;
	DriverObject->MajorFunction[IRP_MJ_CREATE_NAMED_PIPE];
	DriverObject->MajorFunction[IRP_MJ_CREATE_MAILSLOT];
	DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL];
	DriverObject->MajorFunction[IRP_MJ_CLEANUP];
	DriverObject->MajorFunction[IRP_MJ_CLOSE];
	//处理快速分发函数
	do {
		PFAST_IO_DISPATCH fastIoDispatch = NULL;
		fastIoDispatch = ExAllocatePool(NonPagedPool, sizeof(FAST_IO_DISPATCH));
		if (!fastIoDispatch)
		{
			status = STATUS_INSUFFICIENT_RESOURCES;
		}
		RtlZeroMemory(fastIoDispatch, sizeof(FAST_IO_DISPATCH));
		fastIoDispatch->SizeOfFastIoDispatch = sizeof(FAST_IO_DISPATCH);
		fastIoDispatch->FastIoCheckIfPossible = SfFastIoCheckIfPossible;
		fastIoDispatch->FastIoRead = SfFastIoRead;
		fastIoDispatch->FastIoWrite = SfFastIoWrite;
		fastIoDispatch->FastIoQueryBasicInfo = SfFastIoQueryBasicInfo;
		fastIoDispatch->FastIoQueryStandardInfo = SfFastIoQueryStandardInfo;
		fastIoDispatch->FastIoLock = SfFastIoLock;
		fastIoDispatch->FastIoUnlockSingle = SfFastIoUnlockSingle;
		fastIoDispatch->FastIoUnlockAll = SfFastIoUnlockAll;
		fastIoDispatch->FastIoUnlockAllByKey = SfFastIoUnlockAllByKey;
		fastIoDispatch->FastIoDeviceControl = SfFastIoDeviceControl;
		fastIoDispatch->FastIoDetachDevice = SfFastIoDetachDevice;
		fastIoDispatch->FastIoQueryNetworkOpenInfo = SfFastIoQueryNetworkOpenInfo;
		fastIoDispatch->MdlRead = SfFastIoMdlRead;
		fastIoDispatch->MdlReadComplete = SfFastIoMdlReadComplete;
		fastIoDispatch->PrepareMdlWrite = SfFastIoPrepareMdlWrite;
		fastIoDispatch->MdlWriteComplete = SfFastIoMdlWriteComplete;
		fastIoDispatch->FastIoReadCompressed = SfFastIoReadCompressed;
		fastIoDispatch->FastIoWriteCompressed = SfFastIoWriteCompressed;
		fastIoDispatch->MdlReadCompleteCompressed = SfFastIoMdlReadCompleteCompressed;
		fastIoDispatch->MdlWriteCompleteCompressed = SfFastIoMdlWriteCompleteCompressed;
		fastIoDispatch->FastIoQueryOpen = SfFastIoQueryOpen;

		DriverObject->FastIoDispatch = fastIoDispatch;

	} while (FALSE);
	{
		FS_FILTER_CALLBACKS fsFilterCallbacks;
		fsFilterCallbacks.SizeOfFsFilterCallbacks = sizeof(FS_FILTER_CALLBACKS);
		fsFilterCallbacks.PreAcquireForSectionSynchronization = SfPreFsFilterPassThrough;
		fsFilterCallbacks.PostAcquireForSectionSynchronization = SfPostFsFilterPassThrough;
		fsFilterCallbacks.PreReleaseForSectionSynchronization = SfPreFsFilterPassThrough;
		fsFilterCallbacks.PostReleaseForSectionSynchronization = SfPostFsFilterPassThrough;
		fsFilterCallbacks.PreAcquireForCcFlush = SfPreFsFilterPassThrough;
		fsFilterCallbacks.PostAcquireForCcFlush = SfPostFsFilterPassThrough;
		fsFilterCallbacks.PreReleaseForCcFlush = SfPreFsFilterPassThrough;
		fsFilterCallbacks.PostReleaseForCcFlush = SfPostFsFilterPassThrough;
		fsFilterCallbacks.PreAcquireForModifiedPageWriter = SfPreFsFilterPassThrough;
		fsFilterCallbacks.PostAcquireForModifiedPageWriter = SfPostFsFilterPassThrough;
		fsFilterCallbacks.PreReleaseForModifiedPageWriter = SfPreFsFilterPassThrough;
		fsFilterCallbacks.PostReleaseForModifiedPageWriter = SfPostFsFilterPassThrough;
		status = FsRtlRegisterFileSystemFilterCallbacks(DriverObject, &fsFilterCallbacks);
		if (!NT_SUCCESS(status))
		{
			KdPrint(("文件回调函数，注册失败\n"));
			goto DriverEntry_Exit;
		}
	}

goto DriverEntry_End;

DriverEntry_Exit:
	if (gSFilterControlDeivceObject != NULL)
	{
		IoDeleteDevice(gSFilterControlDeivceObject);
	}
DriverEntry_End:
	return status;
}
