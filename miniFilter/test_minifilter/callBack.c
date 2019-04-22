#pragma once 
#include "callBack.h"

NTSTATUS
FsFilter1Unload ( IN FLT_FILTER_UNLOAD_FLAGS Flags )				 
{
	KdPrint(("Entry FsFilterlUnload\n"));
	FltUnregisterFilter( g_FilterHandle );

	return STATUS_SUCCESS;
}
NTSTATUS
FsFilter1InstanceSetup (
						IN PCFLT_RELATED_OBJECTS FltObjects,
						IN FLT_INSTANCE_SETUP_FLAGS Flags,
						IN DEVICE_TYPE VolumeDeviceType,
						IN FLT_FILESYSTEM_TYPE VolumeFilesystemType
						)
{
	KdPrint(("Entry FsFilterInstanceSetup \n"));
	return STATUS_SUCCESS;
}
NTSTATUS
FsFilter1InstanceQueryTeardown (
								IN PCFLT_RELATED_OBJECTS FltObjects,
								IN FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
								)
{
	KdPrint(("Entry FsFilter1InstanceQueryTeardown \n"));
	return STATUS_SUCCESS;
}
VOID
FsFilter1InstanceTeardownStart (
								IN PCFLT_RELATED_OBJECTS FltObjects,
								IN FLT_INSTANCE_TEARDOWN_FLAGS Flags
								)
{
	KdPrint(("Entry FsFilter1InstanceTeardownStart \n"));
}
VOID
FsFilter1InstanceTeardownComplete (
								   IN PCFLT_RELATED_OBJECTS FltObjects,
								   IN FLT_INSTANCE_TEARDOWN_FLAGS Flags
								   )
{
	KdPrint(("Entry FsFilter1InstanceTeardownComplete \n"));
}

CONST FLT_OPERATION_REGISTRATION Callbacks[] = {
	{ IRP_MJ_OPERATION_END }
	};

FLT_REGISTRATION FilterRegistration = {

	sizeof( FLT_REGISTRATION ),         //  Size
	FLT_REGISTRATION_VERSION,           //  Version
	0,                                  //  Flags

	NULL,                               //  Context
	Callbacks,                          //  Operation callbacks

	FsFilter1Unload,                           //  MiniFilterUnload

	FsFilter1InstanceSetup,                    //  InstanceSetup
	FsFilter1InstanceQueryTeardown,            //  InstanceQueryTeardown
	FsFilter1InstanceTeardownStart,            //  InstanceTeardownStart
	FsFilter1InstanceTeardownComplete,         //  InstanceTeardownComplete

	NULL,                               //  GenerateFileName
	NULL,                               //  GenerateDestinationFileName
	NULL                                //  NormalizeNameComponent

	};