#pragma once
#include "CDO.h"

PFLT_FILTER g_FilterHandle = NULL;
PFLT_PORT 	g_ServerPort = NULL;
PFLT_PORT 	g_ClientPort = NULL;



typedef enum _NPMINI_COMMAND {
	ENUM_PASS = 0,
	ENUM_BLOCK
	} NPMINI_COMMAND;

typedef struct _COMMAND_MESSAGE {
	NPMINI_COMMAND 	Command;  
	} COMMAND_MESSAGE, *PCOMMAND_MESSAGE;


NTSTATUS
NPMiniConnect(
			  __in PFLT_PORT ClientPort,
			  __in PVOID ServerPortCookie,
			  __in_bcount(SizeOfContext) PVOID ConnectionContext,
			  __in ULONG SizeOfContext,
			  __deref_out_opt PVOID *ConnectionCookie
			  )
	{
	KdPrint(("[mini-filter] NPMiniConnect\n"));
	PAGED_CODE();

	UNREFERENCED_PARAMETER( ServerPortCookie );
	UNREFERENCED_PARAMETER( ConnectionContext );
	UNREFERENCED_PARAMETER( SizeOfContext);
	UNREFERENCED_PARAMETER( ConnectionCookie );

	ASSERT( g_ClientPort == NULL );
	g_ClientPort = ClientPort;
	return STATUS_SUCCESS;
	}
VOID
NPMiniDisconnect(
				 __in_opt PVOID ConnectionCookie
				 )
	{		
	PAGED_CODE();
	UNREFERENCED_PARAMETER( ConnectionCookie );
	KdPrint(("[mini-filter] NPMiniDisconnect\n"));

	//  Close our handle
	FltCloseClientPort( g_FilterHandle, &g_ClientPort );
	}
NTSTATUS
NPMiniMessage (
			   __in PVOID ConnectionCookie, 
			   __in_bcount_opt(InputBufferSize) PVOID InputBuffer,
			   __in ULONG InputBufferSize,
			   __out_bcount_part_opt(OutputBufferSize,*ReturnOutputBufferLength) PVOID OutputBuffer,
			   __in ULONG OutputBufferSize,
			   __out PULONG ReturnOutputBufferLength
			   )
	{
	//
	//对通信数据，进行处理。
	//
	NPMINI_COMMAND command;
	NTSTATUS status;

	PAGED_CODE();

	UNREFERENCED_PARAMETER( ConnectionCookie );
	UNREFERENCED_PARAMETER( OutputBufferSize );
	UNREFERENCED_PARAMETER( OutputBuffer );

	KdPrint(("[mini-filter] NPMiniMessage"));

	//                      **** PLEASE READ ****    
	//  The INPUT and OUTPUT buffers are raw user mode addresses.  The filter
	//  manager has already done a ProbedForRead (on InputBuffer) and
	//  ProbedForWrite (on OutputBuffer) which guarentees they are valid
	//  addresses based on the access (user mode vs. kernel mode).  The
	//  minifilter does not need to do their own probe.
	//  The filter manager is NOT doing any alignment checking on the pointers.
	//  The minifilter must do this themselves if they care (see below).
	//  The minifilter MUST continue to use a try/except around any access to
	//  these buffers.    

	if ((InputBuffer != NULL) &&
		(InputBufferSize >= (FIELD_OFFSET(COMMAND_MESSAGE,Command) +
		sizeof(NPMINI_COMMAND)))) {

			try  {
				//  Probe and capture input message: the message is raw user mode
				//  buffer, so need to protect with exception handler
				command = ((PCOMMAND_MESSAGE) InputBuffer)->Command;

				} except( EXCEPTION_EXECUTE_HANDLER ) {

					return GetExceptionCode();
				}

			switch (command) {
			case ENUM_PASS:
				{            		
				DbgPrint("[mini-filter] ENUM_PASS");
				status = STATUS_SUCCESS;         
				break;
				}
			case ENUM_BLOCK:
				{            		
				DbgPrint("[mini-filter] ENUM_BLOCK");
				status = STATUS_SUCCESS;                          
				break;
				}               		

			default:
				DbgPrint("[mini-filter] default");
				status = STATUS_INVALID_PARAMETER;
				break;
				}
		} else {

			status = STATUS_INVALID_PARAMETER;
		}

	return status;
	}