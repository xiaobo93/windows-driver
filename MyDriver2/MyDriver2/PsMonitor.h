#pragma once
#include"PublicHeader.h"
/*
监控进程创建:
ObRegisterCallbacks :win7 x86 需要使用特定的签名（现在不知道使用哪一种）
	windows xp 不支持，针对x86的系统 使用ssdt进行进程保护。
*/

//开始监控 进程信息
NTSTATUS StartMonitor();
//停止监控 进程信息
NTSTATUS StopMonitor();