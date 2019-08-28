#include "FsHeader.h"
NTSTATUS FsPassThrough(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{//Í¨ÓÃÅÉÇ²º¯Êý
	NTSTATUS ntStatus = STATUS_SUCCESS;
	PSFILTER_DEVICE_EXTENSION devExt = (PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

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