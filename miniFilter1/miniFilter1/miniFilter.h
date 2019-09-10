#pragma once
#include <ntifs.h>
NTSTATUS miniFilterEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath);