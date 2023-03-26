/**
 * todo: 管理套接字
*/

#pragma once

#include <Windows.h>

HANDLE sock_getsock();

void sock_init();

void sock_set_servaddr(const char* serv_ip);

VOID CALLBACK OverlappedCompletionRoutine(
	PTP_CALLBACK_INSTANCE pInstance,
	PVOID				  pvContext,
	PVOID				  pOverlapped,
	ULONG				  IoResult,
	ULONG_PTR			  NumberOfBytesTransferred,
	PTP_IO                pIo
);
