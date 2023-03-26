/**
 * todo: 管理 windows 线程池
*/

#include "../include/tpool.h"
#include "../include/socket.h"
#include <windows.h>

static PTP_POOL _pool = NULL;   // * 线程池
static HANDLE _com_port;        // * IO完成端口

/**
 * 
*/
void tpool_init()
{
    SYSTEM_INFO sys_info;

    _com_port = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

    _pool = CreateThreadpoolIo(sock_getsock(), OverlappedCompletionRoutine, NULL, NULL);
    if (_pool == NULL) {
        printf("CreateThreadpoolIo() failed. LastError: %u\n",
               GetLastError());
        exit(1);
    }

    // ! 设置线程池的最大线程数和最小线程数
    GetSystemInfo(&sys_info);
    SetThreadpoolThreadMaximum(_pool, sys_info.dwNumberOfProcessors * 2);
    SetThreadpoolThreadMinimum(_pool, sys_info.dwNumberOfProcessors);
    
}