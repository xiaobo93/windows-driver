#pragma once
#include "FsHeader.h"
#include "FastIo.h"
#include "FsCallBack.h"
#include "cdoDispatch.h"
#include"FsDispatch.h"

PDEVICE_OBJECT gSFilterControlDeivceObject = NULL;
PDRIVER_OBJECT gSFilterDriverObject = NULL;
SF_DYNAMIC_FUNCTION_POINTERS gSfDynamicFunctions = { 0 }; //��ŵ���������λ��


VOID DriverUnload(PDRIVER_OBJECT pDriverObject)
{
	
}
NTSTATUS SfGetObjectName(IN PVOID Object, OUT POBJECT_NAME_INFORMATION * name)
/*--

��������:
	���ظ�����������ƣ��������û�У�����һ�����ַ���
++*/
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	ULONG retLength;
	POBJECT_NAME_INFORMATION tmp;
	ntStatus = ObQueryNameString(Object,NULL,0,&retLength);
	if (ntStatus == STATUS_INFO_LENGTH_MISMATCH)
	{
		tmp = ExAllocatePool(NonPagedPool, retLength);
		if (tmp == NULL)
		{
			goto SfGetObjectName_Exit;
		}
		RtlZeroMemory(tmp, retLength);
		ntStatus = ObQueryNameString(Object, (POBJECT_NAME_INFORMATION)tmp, retLength, &retLength);
		*name = tmp;
	}
	else {
		*name = NULL;
	}
SfGetObjectName_Exit:
	return ntStatus;
}
BOOLEAN SfIsAttachedToDevice(
	PDEVICE_OBJECT DeviceObject
)
{
	PDEVICE_OBJECT currentDevObj;
	PDEVICE_OBJECT nextDevObj;

	if (gSfDynamicFunctions.GetAttachedDeviceReference != NULL)
	{
		do {

			if (currentDevObj->DriverObject == gSFilterDriverObject &&
				currentDevObj->DeviceExtension != NULL)
			{
				ObDereferenceObject(currentDevObj);
				return TRUE;
			}
			if (gSfDynamicFunctions.GetLowerDeviceObject == NULL)
			{
				ObDereferenceObject(currentDevObj);
				return FALSE;
			}
			nextDevObj = (gSfDynamicFunctions.GetLowerDeviceObject)(currentDevObj);
			ObDereferenceObject(currentDevObj);
			currentDevObj = nextDevObj;

		} while (currentDevObj != NULL);
	}
	return FALSE;
}
NTSTATUS SfEnumerateFileSystemVolumes(IN PDEVICE_OBJECT FSDeviceObject, IN PUNICODE_STRING Name)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	ULONG numDevices;
	PDEVICE_OBJECT newDeviceObject;
	PSFILTER_DEVICE_EXTENSION newDevExt;
	PDEVICE_OBJECT storageStackDeviceObject;
	ULONG i;
	BOOLEAN isShadowCopyVolume;
	PDEVICE_OBJECT *devList;

	if (gSfDynamicFunctions.EnumerateDeviceObjectList != NULL)
	{
		ntStatus = (gSfDynamicFunctions.EnumerateDeviceObjectList)(
			FSDeviceObject->DriverObject,
			NULL,
			0,
			&numDevices
			);
		if (ntStatus == STATUS_BUFFER_TOO_SMALL)
		{
			numDevices += 8;
			devList = ExAllocatePoolWithTag(NonPagedPool, (numDevices * sizeof(PDEVICE_OBJECT)),
				'xb');
			if (NULL == devList)
			{
				return STATUS_INSUFFICIENT_RESOURCES;
			}
			ntStatus = (gSfDynamicFunctions.EnumerateDeviceObjectList)(
				FSDeviceObject->DriverObject,
				devList,
				(numDevices * sizeof(PDEVICE_OBJECT)),
				&numDevices
				);
			if (!NT_SUCCESS(ntStatus))
			{
				ExFreePool(devList);
				return ntStatus;
			}
			for (i = 0; i < numDevices; i++)
			{
				storageStackDeviceObject = NULL;
				if (devList[i] == FSDeviceObject ||
					(devList[i]->DeviceType != FSDeviceObject->DeviceType) ||
					SfIsAttachedToDevice(devList[i]->DeviceType))
				{//�豸�Ѿ����󶨣������ٽ��а�
					ObReferenceObject(devList[i]);
					continue;
				}
			}
		}
	}
	return ntStatus;
}
NTSTATUS sfAttachToFileSystemDevices(
	IN PDEVICE_OBJECT DeviceObject,
	IN PUNICODE_STRING DeviceName
)
{
	PSFILTER_DEVICE_EXTENSION devExt = NULL;
	PDEVICE_OBJECT newDeviceObject = NULL;
	NTSTATUS status = STATUS_SUCCESS;
	if (DeviceObject->DeviceType != FILE_DEVICE_DISK_FILE_SYSTEM ||
		DeviceObject->DeviceType != FILE_DEVICE_CD_ROM ||
		DeviceObject->DeviceType != FILE_DEVICE_NETWORK_FILE_SYSTEM)
	{
		return STATUS_SUCCESS;
	}
	if (wcsicmp(L"\\FileSystem\\Fs_Rec", DeviceObject->DriverObject->DriverName.Buffer) == 0)
	{
		return STATUS_SUCCESS;
	}
	status = IoCreateDevice(gSFilterDriverObject,
		sizeof(SFILTER_DEVICE_EXTENSION),
		NULL,
		DeviceObject->DeviceType,
		0,
		FALSE,
		&newDeviceObject);
	if (!NT_SUCCESS(status))
	{
		return status;
	}
	if (FlagOn(DeviceObject->Flags, DO_BUFFERED_IO))
	{
		SetFlag(newDeviceObject->Flags, DO_BUFFERED_IO);
	}
	if (FlagOn(DeviceObject->Flags, DO_DIRECT_IO))
	{
		SetFlag(newDeviceObject->Flags, DO_DIRECT_IO);
	}
	if (FlagOn(DeviceObject->Characteristics, FILE_DEVICE_SECURE_OPEN))
	{
		SetFlag(newDeviceObject->Characteristics, FILE_DEVICE_SECURE_OPEN);
	}
	devExt = newDeviceObject->DeviceExtension;
	devExt->AttachedToDeviceObject =  IoAttachDeviceToDeviceStack(newDeviceObject, DeviceObject);
	if (devExt->AttachedToDeviceObject == NULL)
	{
		KdPrint(("%s �ļ�ϵͳ(%ws)��ʧ��\n", __FUNCTION__, DeviceObject->DriverObject->DriverName.Buffer));
		goto sfAttachToFileSystemDevices_exit;
	}
	else {
		KdPrint(("%s �ļ�ϵͳ(%ws)�󶨳ɹ�\n", __FUNCTION__, DeviceObject->DriverObject->DriverName.Buffer));
	}
	if (wcscpy_s(devExt->DeviceNameBuffer, sizeof(devExt->DeviceNameBuffer),
		DeviceObject->DriverObject->DriverName.Buffer) != 0)
	{
		KdPrint(("%s�豸���ƿ���ʧ��\n",__FUNCTION__));
	}
	ClearFlag(newDeviceObject->Flags, DO_DEVICE_INITIALIZING);
	//�����󶨣��ļ�ϵͳ�ϵľ���Ϣ��

	return STATUS_SUCCESS;

sfAttachToFileSystemDevices_exit:
	IoDeleteDevice(newDeviceObject);
	return STATUS_SUCCESS;
}
VOID SfFsNotification(IN PDEVICE_OBJECT DeviceObject,IN BOOLEAN FsActive)
{//����´������豸��Ϣ��
	NTSTATUS ntStatus;
	POBJECT_NAME_INFORMATION name = NULL;
	ntStatus = SfGetObjectName(DeviceObject, &name);
	KdPrint(("%s��������Ϊ %ws type : 0x%x\n",__FUNCTION__, name->Name.Buffer, DeviceObject->DeviceType));
	_asm int 3;
	if (FsActive)
	{//���ļ�ϵͳ
		sfAttachToFileSystemDevices(DeviceObject,&name->Name);
	}
	else
	{//ж���ļ�ϵͳ

	}
	if (name != NULL)
	{
		ExFreePool(name);
		name = NULL;
	}
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
VOID SfLoadDynamicFunctions()
{
	UNICODE_STRING functionName;
	RtlZeroMemory(&gSfDynamicFunctions, sizeof(gSfDynamicFunctions));
	RtlInitUnicodeString(&functionName, L"FsRtlRegisterFileSystemFilterCallbacks");
	gSfDynamicFunctions.RegisterFileSystemFilterCallbacks = MmGetSystemRoutineAddress(&functionName);

	RtlInitUnicodeString(&functionName, L"IoAttachDeviceToDeviceStackSafe");
	gSfDynamicFunctions.AttachDeviceToDeviceStackSafe = MmGetSystemRoutineAddress(&functionName);

	RtlInitUnicodeString(&functionName, L"IoEnumerateDeviceObjectList");
	gSfDynamicFunctions.EnumerateDeviceObjectList = MmGetSystemRoutineAddress(&functionName);

	RtlInitUnicodeString(&functionName, L"IoGetLowerDeviceObject");
	gSfDynamicFunctions.GetLowerDeviceObject = MmGetSystemRoutineAddress(&functionName);

	RtlInitUnicodeString(&functionName, L"IoGetDeviceAttachmentBaseRef");
	gSfDynamicFunctions.GetDeviceAttachmentBaseRef = MmGetSystemRoutineAddress(&functionName);

	RtlInitUnicodeString(&functionName, L"IoGetDiskDeviceObject");
	gSfDynamicFunctions.GetDiskDeviceObject = MmGetSystemRoutineAddress(&functionName);

	RtlInitUnicodeString(&functionName, L"IoGetAttachedDeviceReference");
	gSfDynamicFunctions.GetAttachedDeviceReference = MmGetSystemRoutineAddress(&functionName);

	RtlInitUnicodeString(&functionName, L"RtlGetVersion");
	gSfDynamicFunctions.GetVersion = MmGetSystemRoutineAddress(&functionName);
}

NTSTATUS
DriverEntry(
	IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING RegistryPath
)
{
	NTSTATUS status = STATUS_SUCCESS;
	UNICODE_STRING nameString;
	PFAST_IO_DISPATCH fastIoDispatch = NULL;

	gSFilterDriverObject = DriverObject;
	RtlInitUnicodeString(&nameString, L"\\FileSystem\\Filters\\SFilterCDO");
	DriverObject->DriverUnload = DriverUnload;

	//����û�е����ĺ���
	SfLoadDynamicFunctions();

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
