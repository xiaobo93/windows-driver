#pragma  once
#include<ntifs.h>
#include<ntddk.h>
#include<ntstrsafe.h>

#define MAX_DEVNAME_LENGTH 260
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
	PDEVICE_OBJECT StorageStackDeviceObject;//磁盘设备对象

	//
	//  Name for this device.  If attached to a Volume Device Object it is the
	//  name of the physical disk drive.  If attached to a Control Device
	//  Object it is the name of the Control Device Object.
	//
	UNICODE_STRING DevieName;

	WCHAR DeviceNameBuffer[MAX_DEVNAME_LENGTH]; //存放文件系统设备名称或者磁盘设备对象名称

	// 
	// The extension used by other user.
	//

	UCHAR UserExtension[1];

} SFILTER_DEVICE_EXTENSION, *PSFILTER_DEVICE_EXTENSION;

typedef
NTSTATUS
(*PSF_REGISTER_FILE_SYSTEM_FILTER_CALLBACKS) (
	IN PDRIVER_OBJECT DriverObject,
	IN PFS_FILTER_CALLBACKS Callbacks
	);

typedef
NTSTATUS
(*PSF_ENUMERATE_DEVICE_OBJECT_LIST) (
	IN  PDRIVER_OBJECT DriverObject,
	IN  PDEVICE_OBJECT *DeviceObjectList,
	IN  ULONG DeviceObjectListSize,
	OUT PULONG ActualNumberDeviceObjects
	);

typedef
NTSTATUS
(*PSF_ATTACH_DEVICE_TO_DEVICE_STACK_SAFE) (
	IN PDEVICE_OBJECT SourceDevice,
	IN PDEVICE_OBJECT TargetDevice,
	OUT PDEVICE_OBJECT *AttachedToDeviceObject
	);

typedef
PDEVICE_OBJECT
(*PSF_GET_LOWER_DEVICE_OBJECT) (
	IN  PDEVICE_OBJECT  DeviceObject
	);

typedef
PDEVICE_OBJECT
(*PSF_GET_DEVICE_ATTACHMENT_BASE_REF) (
	IN PDEVICE_OBJECT DeviceObject
	);

typedef
NTSTATUS
(*PSF_GET_DISK_DEVICE_OBJECT) (
	IN  PDEVICE_OBJECT  FileSystemDeviceObject,
	OUT PDEVICE_OBJECT  *DiskDeviceObject
	);

typedef
PDEVICE_OBJECT
(*PSF_GET_ATTACHED_DEVICE_REFERENCE) (
	IN PDEVICE_OBJECT DeviceObject
	);

typedef
NTSTATUS
(*PSF_GET_VERSION) (
	IN OUT PRTL_OSVERSIONINFOW VersionInformation
	);
typedef struct _SF_DYNAMIC_FUNCTION_POINTERS {

	//
	//  The following routines should all be available on Windows XP (5.1) and
	//  later.
	//

	PSF_REGISTER_FILE_SYSTEM_FILTER_CALLBACKS RegisterFileSystemFilterCallbacks;
	PSF_ATTACH_DEVICE_TO_DEVICE_STACK_SAFE AttachDeviceToDeviceStackSafe;
	PSF_ENUMERATE_DEVICE_OBJECT_LIST EnumerateDeviceObjectList;
	PSF_GET_LOWER_DEVICE_OBJECT GetLowerDeviceObject;
	PSF_GET_DEVICE_ATTACHMENT_BASE_REF GetDeviceAttachmentBaseRef;
	PSF_GET_DISK_DEVICE_OBJECT GetDiskDeviceObject;
	PSF_GET_ATTACHED_DEVICE_REFERENCE GetAttachedDeviceReference;
	PSF_GET_VERSION GetVersion;

} SF_DYNAMIC_FUNCTION_POINTERS, *PSF_DYNAMIC_FUNCTION_POINTERS;