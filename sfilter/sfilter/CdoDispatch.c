/*--

描述:
	处理驱动与应用层交互的处理流程
++*/
#include "CdoDispatch.h"
NTSTATUS CdoPassThrough(IN PDEVICE_OBJECT DeviceObject,IN PIRP Irp)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	Irp->IoStatus.Status = ntStatus;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return ntStatus;
}
NTSTATUS CdoDeviceDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
	switch (irpStack->MajorFunction)
	{
	case IRP_MJ_DEVICE_CONTROL: 
		//设备通信位置
		break;
	default:
		ntStatus = CdoPassThrough(DeviceObject, Irp);
		break;
	}
	return ntStatus;
}