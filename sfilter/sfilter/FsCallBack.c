#include "FsCallBack.h"

NTSTATUS
SfPreFsFilterPassThrough(
	IN PFS_FILTER_CALLBACK_DATA Data,
	OUT PVOID *CompletionContext
)
{
	return STATUS_SUCCESS;
}
VOID
SfPostFsFilterPassThrough(
	IN PFS_FILTER_CALLBACK_DATA Data,
	IN NTSTATUS OperationStatus,
	IN PVOID CompletionContext
)
{

}