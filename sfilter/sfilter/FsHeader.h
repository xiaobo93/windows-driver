#pragma  once
#include<ntifs.h>
#include<ntddk.h>
#include<ntstrsafe.h>

typedef struct _SFILTER_DEVICE_EXTENSION {

	ULONG TypeFlag;
	//
	//  Pointer to the file system device object we are attached to
	//
	PDEVICE_OBJECT AttachedToDeviceObject; //下一个设备对象
	//
	//  Pointer to the real (disk) device object that is associated with
	//  the file system device object we are attached to
	//
	PDEVICE_OBJECT StorageStackDeviceObject;//真实的设备对象

	//
	//  Name for this device.  If attached to a Volume Device Object it is the
	//  name of the physical disk drive.  If attached to a Control Device
	//  Object it is the name of the Control Device Object.
	//

	UNICODE_STRING DeviceName;

	//
	//  Buffer used to hold the above unicode strings
	//

	WCHAR DeviceNameBuffer[64];

	// 
	// The extension used by other user.
	//

	UCHAR UserExtension[1];

} SFILTER_DEVICE_EXTENSION, *PSFILTER_DEVICE_EXTENSION;