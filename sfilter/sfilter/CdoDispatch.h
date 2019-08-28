#pragma once
#include<ntifs.h>
#include<ntddk.h>
#include<ntstrsafe.h>

NTSTATUS CdoDeviceDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);