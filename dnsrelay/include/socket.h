#pragma once

/**
 * todo: 管理套接字
*/

#ifdef __cplusplus
extern "C" {
#endif

#include <WinSock2.h>

SOCKET get_relay_sock();

void sock_init();

void set_serv_addr(const char* serv_ip);

void recv_packet(void* handle_ptr, void* io_ptr);

void send_packet();

#ifdef _DEBUG
void send_packet_test(const char* message, int send_len, SOCKADDR_IN* ptr_to_addr);
#endif

#ifdef __cplusplus
}
#endif