#pragma once
#include<ntddk.h>
#include<usbspec.h>
#include<usb.h>
#include<usbioctl.h>
#include<usbdlib.h>


NTSTATUS
ObOpenObjectByName(
	__in POBJECT_ATTRIBUTES ObjectAttributes,
	__in_opt POBJECT_TYPE ObjectType,
	__in KPROCESSOR_MODE AccessMode,
	__inout_opt PACCESS_STATE AccessState,
	__in_opt ACCESS_MASK DesiredAccess,
	__inout_opt PVOID ParseContext,
	__out PHANDLE Handle
);
extern POBJECT_TYPE* IoDriverObjectType;
NTSTATUS
IoEnumerateDeviceObjectList(
	IN  PDRIVER_OBJECT  DriverObject,
	IN  PDEVICE_OBJECT  *DeviceObjectList,
	IN  ULONG           DeviceObjectListSize,
	OUT PULONG          ActualNumberDeviceObjects
);

typedef struct _DeviceExtension_
{
	PDEVICE_OBJECT nextDevice;
}DEVICE_EXTENSION,*PDEVICE_EXTENSION;
void DriverUnload(PDRIVER_OBJECT DriverObject)
{
	KdPrint(("[%s]卸载例程\n", __FUNCTION__));
}
NTSTATUS syPassThrough(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
	KdPrint(("[%s] %d例程执行\n",__FUNCTION__,stack->MajorFunction));
	IoSkipCurrentIrpStackLocation(Irp);
	ntStatus = IoCallDriver(pDevExt->nextDevice, Irp);
	return ntStatus;
}
NTSTATUS CallUSBD(
	IN PDEVICE_OBJECT DeviceObject,
	IN PURB Urb
)
/*--

函数描述:
	拼接IRP，将IRP发送到指定物理设备中

DeviceObject -  目标设备对象
Urb - URB信息
++*/
{
	PIRP	irp = NULL;
	KEVENT	 event;
	NTSTATUS ntStatus = STATUS_SUCCESS;
	IO_STATUS_BLOCK ioStatus;
	PIO_STACK_LOCATION nextStack;
	KeInitializeEvent(&event, NotificationEvent, FALSE);
	irp = IoBuildDeviceIoControlRequest(IOCTL_INTERNAL_USB_SUBMIT_URB,
		DeviceObject,
		NULL,
		0,
		NULL,
		0,
		TRUE,
		&event,
		&ioStatus
		);
	if (!irp)
	{
		KdPrint(("[%s]irp申请失败\n", __FUNCTION__));
		return STATUS_UNSUCCESSFUL;
	}
	nextStack = IoGetNextIrpStackLocation(irp);
	nextStack->Parameters.Others.Argument1 = Urb;
	ntStatus = IoCallDriver(DeviceObject, irp);
	if (ntStatus == STATUS_PENDING)
	{
		KeWaitForSingleObject(&event,
			Executive,
			KernelMode,
			FALSE,
			NULL);
		ntStatus = STATUS_SUCCESS;
	}
	return ntStatus;
}
NTSTATUS
SyGetDriverObject(
	IN PWCHAR DriverName,
	OUT PDRIVER_OBJECT *OutDrvObj
)
/*--

函数描述:
	通过DriverName 获取设备对象，并通过OutDrvObj 输出。
++*/
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	UNICODE_STRING szDriverName = { 0 };
	OBJECT_ATTRIBUTES sInitializedAttributes = { 0 };
	HANDLE hHandle;
	PDRIVER_OBJECT pDriverObject;

	//	打开驱动设备
	RtlInitUnicodeString(&szDriverName, DriverName);
	InitializeObjectAttributes(
		&sInitializedAttributes,
		&szDriverName,
		OBJ_KERNEL_HANDLE,
		NULL,
		NULL
	);
	ntStatus = ObOpenObjectByName(
		&sInitializedAttributes,
		*IoDriverObjectType,
		KernelMode,
		NULL,
		FILE_READ_ATTRIBUTES,
		NULL,
		&hHandle
	);
	if (!NT_SUCCESS(ntStatus)) {

		KdPrint(("[PortDriver]ObOpenObjectByName(%wZ) failed! NTSTATUS=0x%X\n", &szDriverName, ntStatus));
		return ntStatus;
	}

	ntStatus = ObReferenceObjectByHandle(
		hHandle,
		FILE_READ_ATTRIBUTES,
		*IoDriverObjectType,
		KernelMode,
		&pDriverObject,
		NULL
	);
	if (!NT_SUCCESS(ntStatus)) {

		KdPrint(("[PortDriver]ObReferenceObjectByHandle(%wZ) failed! NTSTATUS=0x%X\n", &szDriverName, ntStatus));
		ZwClose(hHandle);
		return ntStatus;
	}

	if (OutDrvObj)
	{
		*OutDrvObj = pDriverObject;
	}
	ZwClose(hHandle);

	return ntStatus;
}
NTSTATUS
SyGetDeviceObjectInDriver(
	IN PDRIVER_OBJECT DriverObject,
	ULONG_PTR * DeviceObjectList,
	PULONG DeviceObjectCount
)
/*--

函数描述:
	遍历指定DriverObject驱动对象，所有的设备对象。
++*/
{
	NTSTATUS ntStatus;
	PULONG_PTR pDeviceObjectList;
	ULONG ulDeviceObjectListSize;
	ULONG ulActualNumberDeviceObject;
	ULONG ulDeviceObjectCount;

	//	先获取设备数量，再申请内存，然后获取设备。
	ntStatus = IoEnumerateDeviceObjectList(DriverObject, NULL, 0, &ulActualNumberDeviceObject);
	if (ntStatus != STATUS_BUFFER_TOO_SMALL)
		return ntStatus;

	ulDeviceObjectListSize = ulActualNumberDeviceObject * sizeof(PDEVICE_OBJECT);
	pDeviceObjectList = (PULONG_PTR)ExAllocatePoolWithTag(NonPagedPool, ulDeviceObjectListSize, 'mDVL');
	if (pDeviceObjectList == NULL)
		return STATUS_INSUFFICIENT_RESOURCES;

	RtlZeroMemory(pDeviceObjectList, ulDeviceObjectListSize);
	//获取设备对象
	ntStatus = IoEnumerateDeviceObjectList(DriverObject, (PDEVICE_OBJECT *)pDeviceObjectList, ulDeviceObjectListSize, &ulActualNumberDeviceObject);
	if (!NT_SUCCESS(ntStatus)) {

		ExFreePool(pDeviceObjectList);
		return ntStatus;
	}

	*DeviceObjectList = (ULONG_PTR)pDeviceObjectList;
	*DeviceObjectCount = ulActualNumberDeviceObject;

	return ntStatus;
}
PDEVICE_OBJECT findDeviceByHardwareID(
	IN PWCHAR drvName,
	IN PWCHAR HardwareID
)
/*--

函数描述:
	通过驱动名称和物理设备ID，获取到物理设备对象

drvName  - 驱动名称
HardwareID - 物理设备对象

++*/
{
	PDEVICE_OBJECT outDevObj = NULL;
	NTSTATUS status;
	PDRIVER_OBJECT drvObj;
	PDEVICE_OBJECT *devObjList = NULL;
	ULONG devObjCnt = 0;

	status = SyGetDriverObject(drvName, &drvObj);
	if (!NT_SUCCESS(status))
		return NULL;

	status = SyGetDeviceObjectInDriver(drvObj, (ULONG_PTR *)(&devObjList), &devObjCnt);
	if (!NT_SUCCESS(status))
	{
		ObDereferenceObject(drvObj);
		return NULL;
	}

	{
		int i = 0;
		BOOLEAN bFound = FALSE;
		for (; i < devObjCnt; ++i)
		{
			WCHAR usbId[256] = { 0 };
			ULONG retlen = sizeof(usbId);
			//获取设备对象的物理地址
			status = IoGetDeviceProperty(devObjList[i], DevicePropertyHardwareID,
				retlen, (PVOID)usbId, &retlen);
			if (NT_SUCCESS(status))
			{
				//DbgPrint("[%s] usbid %ws.\n", __FUNCTION__,usbId);
				//校验设备ID
				if (!bFound && _wcsicmp(usbId, HardwareID) == 0)
				{
					bFound = TRUE;
					outDevObj = devObjList[i];
				}else{
					ObDereferenceObject(devObjList[i]);
				}
			}
			else {
				ObDereferenceObject(devObjList[i]);
			}
		}
	}
	ObDereferenceObject(drvObj);
	return outDevObj;
}
NTSTATUS ClosePipHanle(PDEVICE_OBJECT DeviceObject, USBD_PIPE_HANDLE pipHandle)
/*--

函数描述:

	关闭指定的管道句柄pipHanle        

参数信息：
	
	DeviceObject:设备对象
	pipHandle：管道句柄对象。

++*/
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	PURB pUrb = NULL;
	pUrb = (PURB)ExAllocatePool(NonPagedPool, sizeof(URB));
	if (pUrb)
	{
		pUrb->UrbHeader.Length = sizeof(struct _URB_PIPE_REQUEST);
		pUrb->UrbHeader.Function = URB_FUNCTION_ABORT_PIPE;
		pUrb->UrbPipeRequest.PipeHandle = pipHandle;
		ntStatus = CallUSBD(DeviceObject, pUrb);
		ExFreePool(pUrb);
	}
	return ntStatus;
}
NTSTATUS GetConfigureDevice(
	IN PDEVICE_OBJECT DeviceObject,
	OUT PUSB_CONFIGURATION_DESCRIPTOR *ConfigurationDescriptor)
/*++
	DeviceObejct : 物理设备对象或着附加设备对象
函数作用：
	读取对应物理设备配置信息。

DeviceObject - 物理设备对象
返回值 ConfigurationDescriptor - 物理设备配置信息
--*/
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	PUSB_CONFIGURATION_DESCRIPTOR configurationDescriptor = NULL;
	PURB urb = NULL;
	ULONG siz = sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST);
	urb = ExAllocatePool(NonPagedPool, siz);
	siz = sizeof(URB);

	if (urb)
	{
		configurationDescriptor = ExAllocatePool(NonPagedPool, sizeof(USB_CONFIGURATION_DESCRIPTOR));
		if (configurationDescriptor)
		{
			UsbBuildGetDescriptorRequest(
				urb,
				(USHORT) sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
				USB_CONFIGURATION_DESCRIPTOR_TYPE,
				0,
				0,
				configurationDescriptor,
				NULL,
				sizeof(USB_CONFIGURATION_DESCRIPTOR),
				NULL);
			ntStatus = CallUSBD(DeviceObject, urb);
			if (!NT_SUCCESS(ntStatus)) {

				KdPrint(("[%s]读取配置失败\n", __FUNCTION__));
				goto GetConfigureDevice_Exit;
			}
		}
	}
	siz = configurationDescriptor->wTotalLength;
	ExFreePool(configurationDescriptor);
	configurationDescriptor = ExAllocatePool(NonPagedPool, siz);
	if (configurationDescriptor)
	{
		UsbBuildGetDescriptorRequest(
			urb,
			(USHORT)sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
			USB_CONFIGURATION_DESCRIPTOR_TYPE,
			0,
			0,
			configurationDescriptor,
			NULL,
			siz,
			NULL);
		ntStatus = CallUSBD(DeviceObject, urb);
		if (!NT_SUCCESS(ntStatus)) {

			KdPrint(("[%s]读取配置失败\n", __FUNCTION__));
			goto GetConfigureDevice_Exit;
		}
	}
GetConfigureDevice_Exit:
	if (urb)
	{
		ExFreePool(urb);
		urb = NULL;
	}
	if (NT_SUCCESS(ntStatus))
	{
		*ConfigurationDescriptor = configurationDescriptor;
	}
	else {
		if (configurationDescriptor)
		{
			ExFreePool(configurationDescriptor);
			configurationDescriptor = NULL;
		}
	}
	return ntStatus;
}
#define OUTPIP  1  //从设备对象发送数据到硬件设备中
#define INPIP  2   //从硬件设备中读取数据到设备对象
NTSTATUS GetPipHanle(
	PDEVICE_OBJECT DeviceObject,
	PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
	_Out_ USBD_PIPE_HANDLE *pipHandle,
	_In_ ULONG sign
)
/*--

函数描述:
	根据设备对象和设备配置信息，获取指定sign的管道句柄pipHandle
参数：
	DeviceObject:设备对象
	ConfigurationDescriptor：设备配置信息
	pipHandle：管道句柄
	sign：标识
++*/
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	PUSBD_INTERFACE_LIST_ENTRY interfaceList, tmp;
	LONG numberOfInterfaces = ConfigurationDescriptor->bNumInterfaces;
	LONG interfaceNumber, interfaceindex;
	PUSB_INTERFACE_DESCRIPTOR   interfaceDescriptor;
	PURB                        urb = NULL;
	tmp = interfaceList = ExAllocatePool(NonPagedPool, sizeof(USBD_INTERFACE_LIST_ENTRY)*(numberOfInterfaces + 1));
	if (!tmp)
	{
		KdPrint(("[%s]内存申请失败\n", __FUNCTION__));
		ntStatus = STATUS_UNSUCCESSFUL;
		goto SelectInterface_Exit;
	}
	interfaceNumber = interfaceindex = 0;

	while (interfaceNumber < numberOfInterfaces) {

		interfaceDescriptor = USBD_ParseConfigurationDescriptorEx(
			ConfigurationDescriptor,
			ConfigurationDescriptor,
			interfaceindex,
			0, -1, -1, -1);

		if (interfaceDescriptor) {

			interfaceList->InterfaceDescriptor = interfaceDescriptor;
			interfaceList->Interface = NULL;
			interfaceList++;
			interfaceNumber++;
		}
		interfaceindex++;
	}
	interfaceList->InterfaceDescriptor = NULL;
	interfaceList->Interface = NULL;
	urb = USBD_CreateConfigurationRequestEx(ConfigurationDescriptor, tmp);
	//获取端点信息
	if (urb)
	{
		PUSBD_INTERFACE_INFORMATION Interface = &urb->UrbSelectConfiguration.Interface;
		PUSBD_PIPE_INFORMATION pipInfo = NULL;
		int i = 0;
		for (i = 0; i < Interface->NumberOfPipes; i++)
		{
			pipInfo = &Interface->Pipes[i];
			pipInfo->MaximumPacketSize = USBD_DEFAULT_MAXIMUM_TRANSFER_SIZE;
		}
	}
	ntStatus = CallUSBD(DeviceObject, urb);
	//获取端点信息
	if (urb)
	{
		PUSBD_INTERFACE_INFORMATION Interface = &urb->UrbSelectConfiguration.Interface;
		PUSBD_PIPE_INFORMATION pipInfo = NULL;
		ULONG tmpSign = 0;
		int i = 0;
		for (i = 0; i < Interface->NumberOfPipes; i++)
		{
			pipInfo = &Interface->Pipes[i];
			pipInfo->MaximumPacketSize = USBD_DEFAULT_MAXIMUM_TRANSFER_SIZE;
			if (USB_ENDPOINT_DIRECTION_IN(pipInfo->EndpointAddress) )
			{
				if (sign == INPIP && tmpSign == 0)
				{
					*pipHandle = pipInfo->PipeHandle;
					tmpSign = 1;
					continue;
				}
			}
			else {
				if (sign == OUTPIP && tmpSign == 0)
				{
					*pipHandle = pipInfo->PipeHandle;
					tmpSign = 1;
					continue;
				}
			}
			ntStatus = ClosePipHanle(DeviceObject, pipInfo->PipeHandle);
			if (!NT_SUCCESS(ntStatus))
			{
				KdPrint(("[%s]关闭管道句柄失败\n",__FUNCTION__));
			}
		}
	}
SelectInterface_Exit:
	if (tmp)
	{
		ExFreePool(tmp);
	}
	return ntStatus;
}
NTSTATUS GetOutPipHanle(
	_In_ PDEVICE_OBJECT DeviceObject,
	_Out_ USBD_PIPE_HANDLE *pipHandle)
/*--

函数描述:
	获取指定设备的管道句柄
参数信息：
	DeviceObject:设备对象
	pipHandle：管道句柄

++*/
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor = NULL;
	ntStatus = GetConfigureDevice(DeviceObject, &ConfigurationDescriptor);
	if (ConfigurationDescriptor)
	{
		ntStatus = GetPipHanle(DeviceObject,ConfigurationDescriptor,pipHandle,OUTPIP);
		if (!NT_SUCCESS(ntStatus))
		{
			KdPrint(("[%s]管道句柄获取失败\n",__FUNCTION__));
			goto GetOutPipHanle_Exit;
		}
	}
	else {
		KdPrint(("[%s]设备配置信息获取失败\n", __FUNCTION__));
		goto GetOutPipHanle_Exit;
	}
GetOutPipHanle_Exit:
	if (ConfigurationDescriptor)
	{
		ExFreePool(ConfigurationDescriptor);
		ConfigurationDescriptor = NULL;
	}
	return ntStatus;
}
NTSTATUS sendMessage(PDRIVER_OBJECT DriverObject)
/*--

函数描述:
	URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER: // 0x9  中断或者批量传输
向指定USB设备中，写入数据。
	效果：USB打印机，打印出数据。

DriverObject - 暂时无用

++*/
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	//绑定设备对象
	PDEVICE_OBJECT targetDevObj = NULL;
	USBD_PIPE_HANDLE pipOutHanlde;
	_asm int 3;
	targetDevObj = findDeviceByHardwareID(L"\\Driver\\usbhub", L"USB\\VID_0519&PID_000F&REV_0100");
	if (targetDevObj)
	{
		ntStatus = GetOutPipHanle(targetDevObj,&pipOutHanlde);
	}
	else {
		KdPrint(("[%s]设备对象获取失败\n", __FUNCTION__));
		goto sendMessage_Exit;
	}
	if (NT_SUCCESS(ntStatus))
	{//发送数据
		char buf[] = { "123456abcdef\x0a""123456abcdef\x0a""123456abcdef\x0a""123456abcdef\x0a""123456abcdef\x0a""123456abcdef\x0a""123456abcdef\x0a""123456abcdef\x0a""123456abcdef\x0a""123456abcdef\x0a""123456abcdef\x0a""123456abcdef\x0a""\x1d\x56\x42\x00" };
		LONG siz;
		URB urb;
		siz = sizeof(buf);
		UsbBuildInterruptOrBulkTransferRequest(
			&urb,
			sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER),
			pipOutHanlde,//(USBD_PIPE_HANDLE)1,//0x87AFAA14,    //这个值需要设置
			buf,
			NULL,
			siz,
			0,
			NULL
		);
		ntStatus = CallUSBD(targetDevObj, &urb);
		if (!NT_SUCCESS(ntStatus))
		{
			ntStatus = URB_STATUS(&urb);
			//return USBD_SUCCESS(ustatus);
			DbgPrint("[%s]ustatus 0x%x\n", __FUNCTION__, ntStatus);
		}
		ntStatus = ClosePipHanle(targetDevObj, pipOutHanlde);
		if (!NT_SUCCESS(ntStatus))
		{
			KdPrint(("[%s]关闭管道信息失败\n", __FUNCTION__));
			goto sendMessage_Exit;
		}
	}
sendMessage_Exit:
	if (targetDevObj != NULL)
	{
		ObDereferenceObject(targetDevObj);
	}
	//构造IRP发送数据
	return ntStatus;
}
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	int i = 0;
	KdPrint(("[%s]进行初始化", __FUNCTION__));
	DriverObject->DriverUnload = DriverUnload;
	for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
	{
		DriverObject->MajorFunction[i] = syPassThrough;
	}
	KdPrint(("[%s]初始化成功", __FUNCTION__));
	sendMessage(DriverObject);
	return ntStatus;
}