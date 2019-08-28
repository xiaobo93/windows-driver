#pragma  once
#include "FsHeader.h"

NTSTATUS FsDeviceDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);