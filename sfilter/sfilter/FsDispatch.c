#include "FsHeader.h"
NTSTATUS FsPassThrough(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{//通用派遣函数
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
	case IRP_MN_MOUNT_VOLUME://挂载卷
	{

	}
		break;
	case IRP_MN_LOAD_FILE_SYSTEM: //加载文件系统
	{//需要将，文件识别器摘除掉。 Irp完成之后，摘除删除该设备对象
		IoDetachDevice(devExt->AttachedToDeviceObject);
		IoDeleteDevice(DeviceObject);
	}
		break;
	case IRP_MN_USER_FS_REQUEST: //解挂载卷
		break;//不会影响到系统，不进行操作。
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