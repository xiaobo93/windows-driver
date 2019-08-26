#pragma  once
#include<ntifs.h>
#include<ntddk.h>
#include<ntstrsafe.h>

NTSTATUS
SfPreFsFilterPassThrough(
	IN PFS_FILTER_CALLBACK_DATA Data,
	OUT PVOID *CompletionContext
);

VOID
SfPostFsFilterPassThrough(
	IN PFS_FILTER_CALLBACK_DATA Data,
	IN NTSTATUS OperationStatus,
	IN PVOID CompletionContext
);