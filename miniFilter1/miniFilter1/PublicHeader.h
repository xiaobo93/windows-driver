#pragma once 
/*
ntifs.h ���� ntddk.h
ntddk.h ���� wdm.h
*/
#include<ntifs.h>
//#include<wdm.h>
//#include<ntddk.h>

#define logPrint(_x_) DbgPrint _x_