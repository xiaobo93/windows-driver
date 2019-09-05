#pragma once
#include "FsHeader.h"
#include "FastIo.h"
#include "FsCallBack.h"
#include "cdoDispatch.h"
#include"FsDispatch.h"

PDEVICE_OBJECT gSFilterControlDeivceObject = NULL;
PDRIVER_OBJECT gSFilterDriverObject = NULL;
SF_DYNAMIC_FUNCTION_POINTERS gSfDynamicFunctions; //存放导出函数的位置
FAST_MUTEX gSfilterAttachLock;

VOID DriverUnload(PDRIVER_OBJECT pDriverObject)
{
	
}
NTSTATUS SfGetObjectName(IN PVOID Object, PUNICODE_STRING name)
/*--
函数描述:
	返回给定对象的名称，如果对象没有，返回一个空字符串
++*/
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	ULONG retLength;
	POBJECT_NAME_INFORMATION tmp ;
	WCHAR Name[MAX_DEVNAME_LENGTH] = { 0 };

	tmp = (POBJECT_NAME_INFORMATION)Name;
	ntStatus = ObQueryNameString(Object,tmp, sizeof(Name),&retLength);

	name->Length = 0;
	if (NT_SUCCESS(ntStatus))
	{
		RtlCopyUnicodeString(name,&tmp->Name);
	}
	return ntStatus;
}
BOOLEAN SfIsAttachedToDevice(
	PDEVICE_OBJECT DeviceObject
)
/*--

函数描述:
	判断当前的设备对象是否已经被绑定
返回值：
TRUE ： 已经被绑定 ；FALSE：遍历失败，没有被绑定。
++*/
{
	PDEVICE_OBJECT currentDevObj = NULL;
	PDEVICE_OBJECT nextDevObj = NULL;
	BOOLEAN ret = FALSE;
	if (gSfDynamicFunctions.GetAttachedDeviceReference == NULL ||
		gSfDynamicFunctions.GetLowerDeviceObject == NULL)
	{
		goto SfIsAttachedToDevice_Exit;
	}
	currentDevObj = (gSfDynamicFunctions.GetAttachedDeviceReference)(DeviceObject);
	while (currentDevObj != NULL) 
	{
		if (currentDevObj->DriverObject == gSFilterDriverObject &&
			currentDevObj->DeviceExtension != NULL)
		{//该设备已经被绑定
			ret = TRUE;
			goto SfIsAttachedToDevice_Exit;
		}
		ObDereferenceObject(currentDevObj);
		nextDevObj = (gSfDynamicFunctions.GetLowerDeviceObject)(currentDevObj);
		currentDevObj = nextDevObj;
	} 
SfIsAttachedToDevice_Exit:
	if (currentDevObj != NULL)
	{
		ObDereferenceObject(currentDevObj);
		currentDevObj = NULL;
	}
	return ret;
}
NTSTATUS SfEnumerateFileSystemVolumes(IN PDEVICE_OBJECT FSDeviceObject)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	ULONG numDevices;
	PDEVICE_OBJECT newDeviceObject;
	PSFILTER_DEVICE_EXTENSION newDevExt;
	PDEVICE_OBJECT storageStackDeviceObject;
	ULONG i;
	BOOLEAN isShadowCopyVolume;
	PDEVICE_OBJECT *devList;
	if (gSfDynamicFunctions.EnumerateDeviceObjectList == NULL ||
		gSfDynamicFunctions.GetDeviceAttachmentBaseRef == NULL ||
		gSfDynamicFunctions.GetDiskDeviceObject == NULL)
	{//校验导出函数
		return STATUS_UNSUCCESSFUL;
	}
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
		//获取驱动上的所有设备对象
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
		{//循环绑定设备对象
			WCHAR tmp[MAX_DEVNAME_LENGTH] = { 0 };
			UNICODE_STRING DeviceName;
			RtlInitEmptyUnicodeString(&DeviceName, tmp, sizeof(tmp));
			SfGetObjectName(devList[i], &DeviceName);
			storageStackDeviceObject = NULL;
			while(TRUE)
			{
				if (devList[i] == FSDeviceObject ||
					(devList[i]->DeviceType != FSDeviceObject->DeviceType) ||
					SfIsAttachedToDevice(devList[i]))
				{//设备已经被绑定，不会再进行绑定
					break;
				}
				//通过检查，设备链最底层的设备对象是否包含设备名。
				{
					PDEVICE_OBJECT pLowerDevice = NULL;
					WCHAR tmp[MAX_DEVNAME_LENGTH] = { 0 };
					UNICODE_STRING pLowerDeviceName;
					RtlInitEmptyUnicodeString(&pLowerDeviceName, tmp, sizeof(tmp));
					pLowerDevice = (gSfDynamicFunctions.GetDeviceAttachmentBaseRef)(devList[i]);
					//获取设备名称
					SfGetObjectName(pLowerDevice,&pLowerDeviceName);
					ObReferenceObject(pLowerDevice);
					if (pLowerDeviceName.Length > 0)
					{
						break;
					}
				}
				ntStatus = (gSfDynamicFunctions.GetDiskDeviceObject)(devList[i],
					&storageStackDeviceObject);
				if (!NT_SUCCESS(ntStatus))
				{
					break;
				}
				ntStatus = IoCreateDevice(gSFilterDriverObject,
					sizeof(SFILTER_DEVICE_EXTENSION),
					NULL,
					devList[i]->DeviceType,
					0,
					FALSE,
					&newDeviceObject);
				if (!NT_SUCCESS(ntStatus))
				{
					break;
				}
				if (FlagOn(devList[i]->Flags, DO_BUFFERED_IO))
				{
					SetFlag(newDeviceObject->Flags, DO_BUFFERED_IO);
				}
				if (FlagOn(devList[i]->Flags, DO_DIRECT_IO))
				{
					SetFlag(newDeviceObject->Flags, DO_DIRECT_IO);
				}
				newDevExt = newDeviceObject->DeviceExtension;
				newDevExt->TypeFlag = 'ss';
				newDevExt->StorageStackDeviceObject = storageStackDeviceObject;
				RtlInitEmptyUnicodeString(&newDevExt->DevieName, newDevExt->DeviceNameBuffer, MAX_DEVNAME_LENGTH);
				SfGetObjectName(storageStackDeviceObject, &newDevExt->DevieName);
				ExAcquireFastMutex(&gSfilterAttachLock);
				ntStatus = STATUS_UNSUCCESSFUL;
				{
					int tt = 0;
					for (tt = 0; tt < 8; tt++)
					{//循环绑定设备对象
						LARGE_INTEGER interval;
						newDevExt->AttachedToDeviceObject = IoAttachDeviceToDeviceStack(newDeviceObject, devList[i]);
						if (newDevExt->AttachedToDeviceObject != NULL)
						{
							ClearFlag(newDeviceObject->Flags, DO_DEVICE_INITIALIZING);
							ntStatus = STATUS_SUCCESS;
							break;
						}
						interval.QuadPart = (500 * -10 * 1000);
						KeDelayExecutionThread(KernelMode, FALSE, &interval);
					}
				}
				if (!NT_SUCCESS(ntStatus))
				{//校验是否是绑定成功。
					IoDeleteDevice(newDeviceObject);
					newDeviceObject = NULL;
				}
				ExReleaseFastMutex(&gSfilterAttachLock);
				break;
			}
			if (storageStackDeviceObject != NULL)
			{
				ObReferenceObject(storageStackDeviceObject);
				storageStackDeviceObject = NULL;
			}
			ObReferenceObject(devList[i]);
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
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	WCHAR tmp2[MAX_DEVNAME_LENGTH] = { 0 };
	if (DeviceObject->DeviceType != FILE_DEVICE_DISK_FILE_SYSTEM &&   //0x8
		DeviceObject->DeviceType != FILE_DEVICE_CD_ROM_FILE_SYSTEM &&             //0x3
		DeviceObject->DeviceType != FILE_DEVICE_NETWORK_FILE_SYSTEM)   //0x14
	{
		KdPrint(("%s文件系统，不符合绑定规则,驱动名称为：%ws\n",__FUNCTION__,
			DeviceObject->DriverObject->DriverName.Buffer));
		status = STATUS_SUCCESS;
		goto sfAttachToFileSystemDevices_exit;
	}

	if (_wcsicmp(L"\\FileSystem\\Fs_Rec", DeviceObject->DriverObject->DriverName.Buffer) == 0)
	{//\\FileSystem\\Fs_Rec 识别器。不进行绑定
		KdPrint(("%s (%s)该设备为 文件系统识别器，不进行绑定",
			__FUNCTION__, DeviceObject->DriverObject->DriverName.Buffer));
		status = STATUS_SUCCESS;
		goto sfAttachToFileSystemDevices_exit;
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
		KdPrint(("%s 文件系统(%ws)绑定失败\n", __FUNCTION__, DeviceObject->DriverObject->DriverName.Buffer));
		goto sfAttachToFileSystemDevices_exit;
	}
	else {
		KdPrint(("%s 文件系统(%ws)绑定成功\n", __FUNCTION__, DeviceObject->DriverObject->DriverName.Buffer));
		status = STATUS_SUCCESS;
	}

	if (wcscpy_s(devExt->DeviceNameBuffer,MAX_DEVNAME_LENGTH,DeviceName->Buffer) != 0)
	{
		KdPrint(("%s设备名称拷贝失败\n",__FUNCTION__));
	}
	ClearFlag(newDeviceObject->Flags, DO_DEVICE_INITIALIZING);
	
	return status;
sfAttachToFileSystemDevices_exit:
	if (newDeviceObject != NULL)
	{
		IoDeleteDevice(newDeviceObject);
		newDeviceObject = NULL;
	}
	return status;
}
VOID SfDetachFromFileSystemDevice(IN PDEVICE_OBJECT DeviceObject)
{
	PDEVICE_OBJECT ourAttachedDevice;
	PSFILTER_DEVICE_EXTENSION devExt = NULL;

	for (ourAttachedDevice = DeviceObject->AttachedDevice;ourAttachedDevice!= NULL; ourAttachedDevice = ourAttachedDevice->AttachedDevice)
	{
		if (ourAttachedDevice->DriverObject == gSFilterDriverObject &&
			ourAttachedDevice->DeviceExtension != NULL)
		{
			devExt = ourAttachedDevice->DeviceExtension;
			KdPrint(("%s卸载文件系统\n", __FUNCTION__));
			IoDetachDevice(devExt->AttachedToDeviceObject);
			IoDeleteDevice(ourAttachedDevice);
			return;
		}
	}
	return;
}
VOID SfFsNotification(IN PDEVICE_OBJECT DeviceObject,IN BOOLEAN FsActive)
{//监测新创建的设备信息。
	NTSTATUS ntStatus;
	WCHAR tmp[MAX_DEVNAME_LENGTH] = { 0 };
	UNICODE_STRING DeviceName;
	RtlInitEmptyUnicodeString(&DeviceName, tmp,sizeof(tmp));
	ntStatus = SfGetObjectName(DeviceObject,&DeviceName);
	KdPrint(("%s -- %d 文件系统设备名:%ws 设备类型 type : 0x%x\n",
		__FUNCTION__, FsActive, 
		DeviceName.Buffer, DeviceObject->DeviceType));
	if (FsActive)
	{   //绑定文件系统
		ntStatus = sfAttachToFileSystemDevices(DeviceObject,&DeviceName);
		if (NT_SUCCESS(ntStatus))
		{
			//遍历绑定，文件系统上的卷信息。
			SfEnumerateFileSystemVolumes(DeviceObject);
		}
	}
	else
	{//卸载文件系统
		SfDetachFromFileSystemDevice(DeviceObject);
	}
}
NTSTATUS DeviceDispatch(IN PDEVICE_OBJECT DeviceObject,IN PIRP Irp)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	if (DeviceObject == gSFilterControlDeivceObject)
	{//控制设备操作
		ntStatus = CdoDeviceDispatch(DeviceObject, Irp);
	}
	else {
		//常规设备派遣函数
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

	ExInitializeFastMutex(&gSfilterAttachLock);

	//加载没有导出的函数
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
			KdPrint(("[%s]控制设备对象(%ws)，生成失败\n",__FUNCTION__, nameString.Buffer));
		}
	}
	else
	{ 
		if (!NT_SUCCESS(status))
		{
			KdPrint(("[%s]控制设备对象(%ws)，生成失败\n", __FUNCTION__, nameString.Buffer));
		}
	}

	{
		int i = 0;
		for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
		{
			DriverObject->MajorFunction[i] = DeviceDispatch;
		}
	}

	//处理快速分发函数
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
			KdPrint(("文件回调函数，注册失败\n"));
			goto DriverEntry_Exit;
		}
	}
	//注册文件系统，变动函数
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
