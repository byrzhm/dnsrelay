/**
 * todo: 管理套接字
*/

#include "../include/socket.h"
#include "../include/log.h"
#include <winsock2.h>
#include <string.h>

static SOCKET _sock;
static char _serv_ip[16] = "10.3.9.45";

void sock_init() {
    _sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (_sock == SOCKET_ERROR)
        log_error_handling("socket()");
}

void sock_set_servaddr(const char* serv_ip){
    strcpy(_serv_ip, serv_ip);
}