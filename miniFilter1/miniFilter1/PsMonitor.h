#pragma once
#include"PublicHeader.h"
/*
��ؽ��̴���:
ObRegisterCallbacks :win7 x86 ��Ҫʹ���ض���ǩ�������ڲ�֪��ʹ����һ�֣�
	windows xp ��֧�֣����x86��ϵͳ ʹ��ssdt���н��̱�����
*/

//��ʼ��� ������Ϣ
NTSTATUS StartMonitor();
//ֹͣ��� ������Ϣ
NTSTATUS StopMonitor();