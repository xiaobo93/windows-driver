#include "FsHeader.h"
NTSTATUS FsPassThrough(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{//ͨ����ǲ����
	NTSTATUS ntStatus = STATUS_SUCCESS;
	PSFILTER_DEVICE_EXTENSION devExt = (PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

	IoSkipCurrentIrpStackLocation(Irp);
	ntStatus = IoCallDriver(devExt->AttachedToDeviceObject, Irp);

	return ntStatus;
}
NTSTATUS
SfFsControlCompletion(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp,
	IN PVOID Context
)
{
	KeSetEvent((PKEVENT)Context, IO_NO_INCREMENT, FALSE);
	return STATUS_MORE_PROCESSING_REQUIRED;
}
/*++
������
	�󶨾��豸��
--*/
NTSTATUS SfFsControlMountVolume(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	PSFILTER_DEVICE_EXTENSION devExt = DeviceObject->DeviceExtension;
	PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
	PDEVICE_OBJECT newDeviceObject;
	PDEVICE_OBJECT storageStackDeviceObject;
	PSFILTER_DEVICE_EXTENSION newDevExt = NULL;
	NTSTATUS ntStatus = STATUS_SUCCESS;
	KEVENT waitEvent;

	storageStackDeviceObject = irpSp->Parameters.MountVolume.Vpb->RealDevice;
	
	ntStatus = IoCreateDevice(gSFilterDriverObject,
		sizeof(SFILTER_DEVICE_EXTENSION),
		NULL,
		DeviceObject->DeviceType,
		0,
		FALSE,
		&newDeviceObject
		);
	if (!NT_SUCCESS(ntStatus))
	{
		IoSkipCurrentIrpStackLocation(Irp);
		ntStatus = IoCallDriver(DeviceObject, Irp);
		return ntStatus;
	}
	newDevExt = newDeviceObject->DeviceExtension;
	newDevExt->StorageStackDeviceObject = storageStackDeviceObject;
	RtlInitEmptyUnicodeString(&newDevExt->DevieName, newDevExt->DeviceNameBuffer, sizeof(newDevExt->DeviceNameBuffer));
	SfGetObjectName(storageStackDeviceObject, &newDevExt->DevieName);
	KeInitializeEvent(&waitEvent, NotificationEvent, FALSE);
	IoCopyCurrentIrpStackLocationToNext(Irp);
	IoSetCompletionRoutine(Irp,
		SfFsControlCompletion,
		&waitEvent,
		TRUE,
		TRUE,
		TRUE
		);
	ntStatus = IoCallDriver(devExt->AttachedToDeviceObject, Irp);
	if (STATUS_PENDING == ntStatus)
	{
		ntStatus = KeWaitForSingleObject(
			&waitEvent,
			Executive,
			KernelMode,
			FALSE,
			NULL);
	}
	if (NT_SUCCESS(Irp->IoStatus.Status))
	{
		PVPB vpb;
		vpb = storageStackDeviceObject->Vpb;

		ExAcquireFastMutex(&gSfilterAttachLock);
		if (!SfIsAttachedToDevice(vpb->DeviceObject))
		{//�豸��û�б���
			ntStatus = STATUS_UNSUCCESSFUL;
			if (FlagOn(vpb->DeviceObject->Flags, DO_BUFFERED_IO))
			{
				SetFlag(newDeviceObject->Flags, DO_BUFFERED_IO);
			}
			if (FlagOn(vpb->DeviceObject->Flags, DO_DIRECT_IO))
			{
				SetFlag(newDeviceObject->Flags, DO_DIRECT_IO);
			}

			{
				int tt = 0;
				for (tt = 0; tt < 8; tt++)
				{//ѭ�����豸����
					LARGE_INTEGER interval;
					newDevExt->AttachedToDeviceObject = IoAttachDeviceToDeviceStack(newDeviceObject, vpb->DeviceObject);
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
			{//У���Ƿ��ǰ󶨳ɹ���
				IoDeleteDevice(newDeviceObject);
				newDeviceObject = NULL;
			}

		}
		ExReleaseFastMutex(&gSfilterAttachLock);
	}
	return ntStatus;
}
NTSTATUS SfFsControl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
	PSFILTER_DEVICE_EXTENSION devExt = (PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	switch (irpSp->MinorFunction)
	{
	case IRP_MN_MOUNT_VOLUME://���ؾ�
	{
		ntStatus = SfFsControlMountVolume(DeviceObject, Irp);
		return ntStatus;
	}
		break;
	case IRP_MN_LOAD_FILE_SYSTEM: //�����ļ�ϵͳ
	{//��Ҫ�����ļ�ʶ����ժ������ Irp���֮��ժ��ɾ�����豸����
		IoDetachDevice(devExt->AttachedToDeviceObject);
		IoDeleteDevice(DeviceObject);
	}
		break;
	case IRP_MN_USER_FS_REQUEST: //����ؾ�
		break;//����Ӱ�쵽ϵͳ�������в�����
	default:
		break;
	}
	IoSkipCurrentIrpStackLocation(Irp);
	ntStatus = IoCallDriver(devExt->AttachedToDeviceObject, Irp);
	return ntStatus;
}
NTSTATUS SfRead(PDEVICE_OBJECT DeviceObject,PIRP Irp)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
	PFILE_OBJECT file_object = irpSp->FileObject;
	PSFILTER_DEVICE_EXTENSION devExt = DeviceObject->DeviceExtension;

	IoSkipCurrentIrpStackLocation(Irp);
	IoCallDriver(devExt->AttachedToDeviceObject, Irp);
	return ntStatus;
}
NTSTATUS SfCreate(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	NTSTATUS ntStauts = STATUS_SUCCESS;
	PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
	PFILE_OBJECT file_object = irpSp->FileObject;
	PSFILTER_DEVICE_EXTENSION devExt = DeviceObject->DeviceExtension;
	KdPrint(("���������·��%wZ\n", file_object->FileName));
	IoSkipCurrentIrpStackLocation(Irp);
	IoCallDriver(devExt->AttachedToDeviceObject, Irp);
	return ntStauts;
}
NTSTATUS FsDeviceDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
	switch (irpStack->MajorFunction)
	{
	case IRP_MJ_FILE_SYSTEM_CONTROL:
		ntStatus = SfFsControl(DeviceObject, Irp);
		break;
	case IRP_MJ_CREATE:
		ntStatus = SfCreate(DeviceObject,Irp);
		break;
	case  IRP_MJ_WRITE:
		break;
	case  IRP_MJ_READ:
		break;
	case IRP_MJ_CREATE_NAMED_PIPE:
		break;
	case IRP_MJ_CREATE_MAILSLOT:
		break;

	case IRP_MJ_CLEANUP:
		break;
	case IRP_MJ_CLOSE:
		break;
	default:
		ntStatus = FsPassThrough(DeviceObject, Irp);
		break;
	}
	return ntStatus;
}