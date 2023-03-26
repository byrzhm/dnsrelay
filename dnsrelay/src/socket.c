/**
 * todo: 管理套接字
*/

#include "../include/socket.h"
#include "../include/log.h"
#include <winsock2.h>
#include <string.h>


static WSADATA _wsa_data;
static SOCKET _sock;
static char _serv_ip[16] = "10.3.9.45";



HANDLE sock_getsock() {
    return (HANDLE)_sock;
}

void sock_init() {
    if (WSAStartup(MAKEWORD(2, 2), &_wsa_data) != 0)
    {
        log_error_message("sock_init(): WSAStartup()");
        log_error_code_wsa(WSAGetLastError());
        exit(1);
    }


    _sock = WSASocket(AF_INET, SOCK_DGRAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (_sock == INVALID_SOCKET)
    {
        log_error_message("sock_init(): socket()", WSAGetLastError());
        log_error_code_wsa(WSAGetLastError());
        exit(1);
    }
}

void sock_set_servaddr(const char* serv_ip){
    strcpy(_serv_ip, serv_ip);
}

VOID OverlappedCompletionRoutine(
    PTP_CALLBACK_INSTANCE pInstance, 
    PVOID                 pvContext, 
    PVOID                 pOverlapped, 
    ULONG                 IoResult, 
    ULONG_PTR             NumberOfBytesTransferred, 
    PTP_IO                pIo)
{

}
