/**
 * todo: 管理套接字
*/

#include "socket.h"
#include "log.h"
#include "thread.h"
#include <winsock2.h>
#include <string.h>


#define MAX_IP_LEN  16

static SOCKET _relay_sock;
static char _serv_ip[MAX_IP_LEN] = "127.0.0.1";
//  "10.3.9.45"

SOCKET get_relay_sock() {
    return _relay_sock;
}

void set_serv_addr(const char* serv_ip){
    strncpy_s(_serv_ip, MAX_IP_LEN, serv_ip, MAX_IP_LEN - 1);
}

void sock_init()
{
    WSADATA wsaData;
    SOCKADDR_IN relay_addr;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        log_error_message("sock_init(): WSAStartup()");

    _relay_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (_relay_sock == INVALID_SOCKET)
        log_error_message("sock_init(): socket()");
    else {
        memset(&relay_addr, 0, sizeof(relay_addr));
        relay_addr.sin_family = AF_INET;
        // 9190 用来测试, 53 为 DNS 端口
        relay_addr.sin_port = htons(9190);
        relay_addr.sin_addr.s_addr = INADDR_ANY;

        if (bind(_relay_sock, (SOCKADDR*)&relay_addr, sizeof(relay_addr))
                 == INVALID_SOCKET)
            log_error_message("sock_init(): bind()");
    }    

}



void recv_packet(void* handle_ptr, void* io_ptr) 
{
	int recv_bytes;
	int addr_size = sizeof(SOCKADDR_IN);
    LPPER_HANDLE_DATA handle_info = (LPPER_HANDLE_DATA)handle_ptr;
    LPPER_IO_DATA io_info = (LPPER_IO_DATA)io_ptr;

	memset(&io_info->overlapped, 0, sizeof(OVERLAPPED));
	recv_bytes = recvfrom(_relay_sock, io_info->buffer,
		 BUF_SIZE, 0, (SOCKADDR*)&handle_info->sock_addr, &addr_size);
	if (recv_bytes == SOCKET_ERROR)
		log_error_message("recv_packet(): recvfrom()");
	else {
		PostQueuedCompletionStatus(
			get_com_port(), recv_bytes,
			(ULONG_PTR)handle_info, (LPOVERLAPPED)&io_info->overlapped
		);
	}
}



void send_packet()
{

}



#ifdef _DEBUG
void send_packet_test(const char* buf, int send_len, SOCKADDR_IN* ptr_to_addr)
{
    sendto(_relay_sock, buf, send_len, 0,
        (SOCKADDR *)ptr_to_addr, sizeof(SOCKADDR_IN));
}
#endif