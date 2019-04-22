#pragma once
#include<fltKernel.h>


extern PFLT_FILTER  g_FilterHandle;
extern PFLT_PORT 	g_ServerPort;
extern PFLT_PORT 	g_ClientPort;

NTSTATUS
NPMiniConnect(
			  __in PFLT_PORT ClientPort,
			  __in PVOID ServerPortCookie,
			  __in_bcount(SizeOfContext) PVOID ConnectionContext,
			  __in ULONG SizeOfContext,
			  __deref_out_opt PVOID *ConnectionCookie
			  );

VOID
NPMiniDisconnect(
				 __in_opt PVOID ConnectionCookie
				 );

NTSTATUS
NPMiniMessage (
			   __in PVOID ConnectionCookie, 
			   __in_bcount_opt(InputBufferSize) PVOID InputBuffer,
			   __in ULONG InputBufferSize,
			   __out_bcount_part_opt(OutputBufferSize,*ReturnOutputBufferLength) PVOID OutputBuffer,
			   __in ULONG OutputBufferSize,
			   __out PULONG ReturnOutputBufferLength
			   );