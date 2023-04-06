#pragma once

/**
 * todo: 管理 windows 线程池
*/

#ifdef __cplusplus
extern "C" {
#endif

#define BUF_SIZE 1024

#include <WinSock2.h>

typedef struct _PER_HANDLE_DATA {
    SOCKADDR_IN sock_addr;
} PER_HANDLE_DATA, * LPPER_HANDLE_DATA;

typedef struct _PER_IO_DATA {
    OVERLAPPED overlapped;
    char buffer[BUF_SIZE];
} PER_IO_DATA, * LPPER_IO_DATA;

HANDLE get_com_port();

HANDLE get_mutex();

void thread_init();

unsigned WINAPI thread_main(LPVOID lpComPort);

#ifdef __cplusplus
}
#endif