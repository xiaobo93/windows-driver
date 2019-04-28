// test_console.cpp : �������̨Ӧ�ó������ڵ㡣
//
#pragma once
#include<Windows.h>
#include<stdio.h>

//
//����minifilter����ϵĺ���
//
//
//����Ŀ¼��C:\WinDDK\7600.16385.1\inc\ddk  ���ӿ�Ŀ¼��C:\WinDDK\7600.16385.1\lib\win7\i386\
//
#include <fltUser.h>
#pragma comment(lib, "fltLib.lib")
//#pragma comment(lib, "fltMgr.lib")


HANDLE g_hPort = INVALID_HANDLE_VALUE;

typedef enum _NPMINI_COMMAND {
	ENUM_PASS = 0,
	ENUM_BLOCK
} NPMINI_COMMAND;

typedef struct _COMMAND_MESSAGE { \
	NPMINI_COMMAND 	Command;  
} COMMAND_MESSAGE, *PCOMMAND_MESSAGE;


int InitialCommunicationPort(void)
{
	DWORD hResult = FilterConnectCommunicationPort( 
		L"\\NPMiniPort", 
		0, 
		NULL, 
		0, 
		NULL, 
		&g_hPort );

	if (hResult != S_OK) {
		return hResult;
	}
	return 0;
}

int NPSendMessage(PVOID InputBuffer)
{

	//
	//�������У��������ݡ�
	//
	DWORD bytesReturned = 0;
	DWORD hResult = 0;
	PCOMMAND_MESSAGE commandMessage = (PCOMMAND_MESSAGE) InputBuffer;	

	hResult = FilterSendMessage(
		g_hPort,
		commandMessage,
		sizeof(COMMAND_MESSAGE),
		NULL,
		NULL,
		&bytesReturned );

	if (hResult != S_OK) {
		return hResult;
	}
	return 0;
}
int NPGetMessage(PVOID )
{
	//
	//��ȡ�����е�����
	//
	return 0;
}
int main(int argc, char* argv[])
{
	COMMAND_MESSAGE s;
	s.Command = ENUM_BLOCK;
	int k = InitialCommunicationPort();
	if(k != 0)
	{
		printf("InitialCommuicationPort failed 0x%x \n",k);
		system("pause");
		return 0;
	}
	k = NPSendMessage(&s);
	if(k != 0)
	{
		printf("NPSendMessage failed\n");
		system("pause");
		return 0;
	}
	//��ȡ�����е�����
	system("pause");
	return 0;
}

