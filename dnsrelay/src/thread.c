/**
 * todo: 管理 windows 线程
*/

#include "thread.h"
#include "socket.h"
#include <WinSock2.h>
#include <windows.h>

static HANDLE _com_port;        // * IO完成端口

/**
 * @return: _com_port
*/
HANDLE get_com_port() 
{
    return _com_port;
}

/**
 * @fn: 创建线程
*/
void thread_init()
{
    int i;
    SYSTEM_INFO sys_info;

    _com_port = CreateIoCompletionPort(
        INVALID_HANDLE_VALUE, NULL, 0, 0
    );

    if (_com_port == NULL) 
        log_error_message("thread_init(): CreateIoCompletionPort()");
    else {
        GetSystemInfo(&sys_info);
        for (i = 0; i < 2 * sys_info.dwNumberOfProcessors; i++)
            _beginthreadex(NULL, 0, thread_main,
                         (LPVOID)_com_port, 0, NULL);
    }
}


/**
 * @fn: 创建线程的初始运行位置
*/
unsigned WINAPI thread_main(LPVOID lpComPort)
{
    HANDLE hComPort = (HANDLE)lpComPort;
    DWORD recv_bytes;
    LPPER_HANDLE_DATA handle_info;
    LPPER_IO_DATA io_info;
    BOOL bOk;
    // DWORD dwError;

    while (1) {
        bOk = GetQueuedCompletionStatus(
            hComPort, &recv_bytes, (PULONG_PTR)&handle_info,
            (LPOVERLAPPED)&io_info, INFINITE
        );
        // dwError = GetLastError();

        if (!bOk) {
            if (io_info != NULL) {
                // Process a failed completed I/O request
                // dwError contains the reason for failure
                log_error_message("thread_main(): GetQueuedCompletionStatus()");
            }
            else {
                // if (dwError == WAIT_TIMEOUT) {
                //     printf("timeout\n");
                //     continue;
                // }
                // else
                    log_error_message("thread_main(): GetQueuedCompletionStatus()");
            }
        }

        /*
            todo: 测试多线程是可用的
        */

#ifdef _DEBUG
        send_packet_test(io_info->buffer, recv_bytes, &handle_info->sock_addr);
#endif

        free(handle_info);
        free(io_info);
    }

    return 0;
}