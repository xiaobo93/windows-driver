// ConsoleApplication5.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include <iostream>
#include<windows.h>
#define  testCode  CTL_CODE(FILE_DEVICE_UNKNOWN,0x800,METHOD_BUFFERED,FILE_ALL_ACCESS)
int main()
{
	HANDLE handle = CreateFileA(
		"\\\\.\\test",
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	if (handle == INVALID_HANDLE_VALUE)
	{
		printf("this is a err %d \n",GetLastError());
		system("pause");
		return 0;
	}
	char inBuffer[100] = { 0 };
	char outBuffer[50] = { 0 };
	DWORD dwSize;
	strcpy(inBuffer, "hello world\n");
	BOOL RE = DeviceIoControl(handle, testCode, inBuffer, sizeof(inBuffer), outBuffer, sizeof(outBuffer), &dwSize, NULL);
	printf("%x %d \n", RE, GetLastError());
	printf("driver re: %s\n", outBuffer);
	CloseHandle(handle);
	system("pause");
	return 0;
}
