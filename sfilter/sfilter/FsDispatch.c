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
NTSTATUS SfFsControl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
	PSFILTER_DEVICE_EXTENSION devExt = (PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	switch (irpSp->MinorFunction)
	{
	case IRP_MN_MOUNT_VOLUME://���ؾ�
	{

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
NTSTATUS FsDeviceDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
	irpStack->MajorFunction;
	switch (irpStack->MajorFunction)
	{
	case IRP_MJ_CREATE:
		break;
	case IRP_MJ_CREATE_NAMED_PIPE:
		break;
	case IRP_MJ_CREATE_MAILSLOT:
		break;
	case IRP_MJ_FILE_SYSTEM_CONTROL:
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