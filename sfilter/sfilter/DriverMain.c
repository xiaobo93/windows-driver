#pragma once
#include "FsHeader.h"
#include "FastIo.h"
#include "FsCallBack.h"
#include "cdoDispatch.h"
#include"FsDispatch.h"

PDEVICE_OBJECT gSFilterControlDeivceObject = NULL;
PDRIVER_OBJECT gSFilterDriverObject = NULL;

VOID DriverUnload(PDRIVER_OBJECT pDriverObject)
{
	
}
NTSTATUS SfGetObjectName(IN PVOID Object, OUT PWCHAR * name)
/*--

��������:
	���ظ�����������ƣ��������û�У�����һ�����ַ���
++*/
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	UNICODE_STRING buf;
	ULONG retLength;
	ntStatus = ObQueryNameString(Object,NULL,NULL,&retLength);
	if (ntStatus == STATUS_INFO_LENGTH_MISMATCH)
	{
		*name = ExAllocatePool(NonPagedPool, retLength);
		if (*name == NULL)
		{
			goto SfGetObjectName_Exit;
		}
		RtlInitUnicodeString(&buf, *name);
		ntStatus = ObQueryNameString(Object, NULL, NULL, &retLength);
	}
	else {
		*name = NULL;
	}
SfGetObjectName_Exit:
	return ntStatus;
}
VOID SfFsNotification(IN PDEVICE_OBJECT DeviceObject,IN BOOLEAN FsActive)
{//����´������豸��Ϣ��
	PWCHAR name = NULL;

}
NTSTATUS DeviceDispatch(IN PDEVICE_OBJECT DeviceObject,IN PIRP Irp)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	if (DeviceObject == gSFilterControlDeivceObject)
	{//�����豸����
		ntStatus = CdoDeviceDispatch(DeviceObject, Irp);
	}
	else {
		//�����豸��ǲ����
		ntStatus = FsDeviceDispatch(DeviceObject, Irp);
	}
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
	gSFilterDriverObject = DriverObject;
	RtlInitUnicodeString(&nameString, L"\\FileSystem\\Filters\\SFilterCDO");
	DriverObject->DriverUnload = DriverUnload;
	PFAST_IO_DISPATCH fastIoDispatch = NULL;
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
	{
		int i = 0;
		for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
		{
			DriverObject->MajorFunction[i] = DeviceDispatch;
		}
	}

	//������ٷַ�����
	do {
		fastIoDispatch = ExAllocatePool(NonPagedPool, sizeof(FAST_IO_DISPATCH));
		if (!fastIoDispatch)
		{
			status = STATUS_INSUFFICIENT_RESOURCES;
			goto DriverEntry_Exit;
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
			KdPrint(("�ļ��ص�������ע��ʧ��\n"));
			goto DriverEntry_Exit;
		}
	}
	//ע���ļ�ϵͳ���䶯����
	status = IoRegisterFsRegistrationChange(DriverObject, SfFsNotification);
	if (!NT_SUCCESS(status))
	{
		goto DriverEntry_Exit;
	}
	ClearFlag(gSFilterControlDeivceObject->Flags, DO_DEVICE_INITIALIZING);
goto DriverEntry_End;

DriverEntry_Exit:
	if (fastIoDispatch != NULL)
	{
		DriverObject->FastIoDispatch = NULL;
		ExFreePool(fastIoDispatch);
	}
	if (gSFilterControlDeivceObject != NULL)
	{
		IoDeleteDevice(gSFilterControlDeivceObject);
	}
DriverEntry_End:
	return status;
}
