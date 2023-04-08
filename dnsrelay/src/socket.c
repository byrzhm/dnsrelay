#ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#endif

#include "log.h"
#include "socket.h"
#include "thread.h"
#include "cache.h"
#include "protocol.h"
#include <Windows.h>
#include <winsock2.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>


static SOCKET _relay_sock;
static SOCKADDR_IN _serv_addr;
#ifdef _WIN32
static UINT32 _serv_ip_addr;
#else
static uint32_t _serv_ip_addr;
#endif
//  "10.3.9.45"
//  "10.3.9.44"

/**
 * @brief 由DNS报文获得要查找的域名信息
 * @param packet       DNS报文
 * @param domain_name  域名信息
*/
static void get_domain_name(const char* packet, char *domain_name);

SOCKET get_relay_sock() {
    return _relay_sock;
}

void set_serv_addr(const char* serv_ip){
    _serv_ip_addr = inet_addr(serv_ip);
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
        // 53号端口 为 DNS 端口
        relay_addr.sin_port = htons(53);
        relay_addr.sin_addr.s_addr = INADDR_ANY;

        if (bind(_relay_sock, (SOCKADDR*)&relay_addr, sizeof(relay_addr))
                 == INVALID_SOCKET)
            log_error_message("sock_init(): bind()");
    }

    _serv_addr.sin_family = AF_INET;
    _serv_addr.sin_port = htons(53);
    _serv_addr.sin_addr.s_addr = _serv_ip_addr;
}



void recv_packet(void* handle_ptr, void* io_ptr, SOCKET recv_sock) 
{
	int recv_bytes;
	int addr_size = sizeof(SOCKADDR_IN);
    LPPER_HANDLE_DATA handle_info = (LPPER_HANDLE_DATA)handle_ptr;
    LPPER_IO_DATA io_info = (LPPER_IO_DATA)io_ptr;

	memset(&io_info->overlapped, 0, sizeof(OVERLAPPED));


    if (recv_sock == _relay_sock) {
        // WaitForSingleObject(get_relay_sock_mutex(), INFINITE);
        recv_bytes = recvfrom(recv_sock, io_info->buffer,
            BUF_SIZE, 0, (SOCKADDR*)&handle_info->sock_addr, &addr_size);
#ifdef _DEBUG
        // assert(recv_bytes > 0);
#endif

        // ReleaseMutex(get_relay_sock_mutex);
    }
    else {
        recv_bytes = recv(recv_sock, io_info->buffer, BUF_SIZE, 0);
        handle_info->sock_addr = _serv_addr;
#ifdef _DEBUG
        assert(recv_bytes > 0);
#endif
    }

    if (recv_bytes == SOCKET_ERROR) {
		//log_error_message("recv_packet(): recvfrom()");
        ;
    }
	else {
		PostQueuedCompletionStatus(
			get_com_port(), recv_bytes,
			(ULONG_PTR)handle_info, (LPOVERLAPPED)&io_info->overlapped
		);
	}
}



/**
 * @brief 转发DNS报文
 * @param packet      指向要发送的数据包的指针
 * @param send_len    要发送的字节数
 * @param ptr_to_addr 要发送的ip地址
*/
void relay_packet(const char* packet, int send_len, SOCKADDR_IN* ptr_to_addr)
{
    LPPER_HANDLE_DATA handle_info;
    LPPER_IO_DATA io_info;
    SOCKET temp_sock;

    if (ptr_to_addr == &_serv_addr) {

        temp_sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (temp_sock == INVALID_SOCKET)
            log_error_message("relay_packet(): socket()");
    
        if (connect(temp_sock, (SOCKADDR*)ptr_to_addr, sizeof(SOCKADDR)) == SOCKET_ERROR)
            log_error_message("relay_packet(): connect()");

        send(temp_sock, packet, send_len, 0);
         
        handle_info = (LPPER_HANDLE_DATA)malloc(sizeof(PER_HANDLE_DATA));
        io_info = (LPPER_IO_DATA)malloc(sizeof(PER_IO_DATA));

        // 检查内存分配情况
        if (!handle_info || !io_info)
            log_error_message("relay_packet(): malloc()");
        else {
            // 接受DNS报文
            recv_packet((void*)handle_info, (void*)io_info, temp_sock);
            closesocket(temp_sock);
        }
    }
    else {
        
#ifdef _DEBUG
        log_packet_raw(packet, send_len);
#endif
        WaitForSingleObject(get_relay_sock_mutex(), INFINITE);
        sendto(_relay_sock, packet, send_len, 0,
            (SOCKADDR*)ptr_to_addr, sizeof(SOCKADDR_IN));
        ReleaseMutex(get_relay_sock_mutex());
    }
}



/**
 * @brief 分析发来的DNS报文, 获取DNS报文头部信息
 * @param  packet            接受的DNS报文
 * @param  packet_len        接受的DNS报文字节数
 * @param  ptr_packet_info   指向报文信息的指针    [指针返回]
 * @param  ptr_dns_header    指向DNS头部信息的指针 [指针返回]
 */
static void get_header_info(
    const char* packet,
    PACKET_INFO* ptr_packet_info,
    DNS_HEADER* ptr_dns_header
)
{
    const unsigned short* cursor = packet;

    // Transaction ID: 2 octets
    ptr_packet_info->id = ntohs(*cursor);
    ptr_dns_header->id = ntohs(*cursor);
    cursor++;

    // Flags: 2 octets
    ptr_dns_header->flags = ntohs(*cursor);
    cursor++;

    // Query Count/Zone Count: 2 octets
    ptr_dns_header->question_count = ntohs(*cursor);
    cursor++;

    // Answer Count/Prerequisite Count
    ptr_dns_header->answer_count = ntohs(*cursor);
    cursor++;

    // Authority Record Count/Update Count
    ptr_dns_header->authority_count = ntohs(*cursor);
    cursor++;

    // Additional Infomation Count
    ptr_dns_header->additional_count = ntohs(*cursor);
    cursor++;

}



/**
 * @brief 分析发来的DNS报文
 * @param  packet            接受的DNS报文
 * @param  packet_len        接受的DNS报文字节数
 * @param  sock_addr         报文发送方的信息
 * @param  ptr_packet_info   指向报文信息的指针    [指针返回]
 * @param  ptr_dns_header    指向DNS头部信息的指针 [指针返回]
 */
void parse_packet(
    const char* packet,
    int packet_len,
    struct sockaddr_in* sock_addr,
    PACKET_INFO* ptr_packet_info,
    DNS_HEADER* ptr_dns_header
)
{
    int QR;

    ptr_packet_info->ip_addr = sock_addr->sin_addr.s_addr;
    get_header_info(packet, ptr_packet_info, ptr_dns_header);

    QR = (ptr_dns_header->flags & 0x8000) >> 15;
    if (QR == 0) {
        // query from DNS client
        parse_query(packet, packet_len, sock_addr, ptr_packet_info, ptr_dns_header);
    }
    else {
        // response from foreign DNS server
        parse_response(packet, packet_len, ptr_packet_info, ptr_dns_header);
    }

}




/**
 * @brief 分析请求报文
 * @param  packet            接受的DNS报文
 * @param  packet_len        接受的DNS报文字节数
 * @param  sock_addr         报文发送方的信息
 * @param  ptr_packet_info   指向报文信息的指针    [指针返回]
 * @param  ptr_dns_header    指向DNS头部信息的指针 
 */
void parse_query(
    const char* packet,
    int packet_len,
    struct sockaddr_in* sock_addr,
    PACKET_INFO* ptr_packet_info,
    DNS_HEADER* ptr_dns_header
)
{
    char *packet_to_send;
    unsigned rand_id;
    unsigned short_max;
    unsigned short *ptr_id;

    get_domain_name(packet, ptr_packet_info->domain_name);
    // cache_search(ptr_packet_info->domain_name/*, */);

    short_max = 0xffff + 0x1;
    if (id_search(ptr_dns_header->id, 0, 0, NULL) != ID_ALREADY_EXIST)
    {
        while (1) {
            srand(time(NULL));      // 线程安全 ?
            rand_id = rand() % short_max;
            if (id_search(rand_id, 1, 0, NULL) != ID_ALREADY_EXIST)
                break;
        }

        id_insert(MAKE_ID_PAIR(rand_id, ptr_dns_header->id), sock_addr);
        packet_to_send = (char *)malloc(sizeof(char) * packet_len);
        if (packet_to_send == NULL)
            log_error_message("parse_query(): malloc()");
        else {
            ptr_id = packet_to_send;
            memcpy(packet_to_send, packet, packet_len);
            *ptr_id = htons(rand_id);
            relay_packet(packet_to_send, packet_len, &_serv_addr);
            free(packet_to_send);
        }
    }
}



/**
 * @brief 分析回应报文
 * @param  packet            接受的DNS报文
 * @param  packet_len        接受的DNS报文字节数
 * @param  ptr_packet_info   指向报文信息的指针    [指针返回]
 * @param  ptr_dns_header    指向DNS头部信息的指针 
 */
void parse_response(
    const char* packet,
    int packet_len,
    PACKET_INFO* ptr_packet_info,
    DNS_HEADER* ptr_dns_header
)
{
    char *packet_to_send;
    unsigned short *ptr_id;
    struct sockaddr_in client_addr;
    int id;

    id = id_search(ptr_dns_header->id, 1, 1, &client_addr);
    if (id != ID_NOT_FOUND)
    {
        packet_to_send = (char *)malloc(sizeof(char) * packet_len);
        if (packet_to_send == NULL)
            log_error_message("parse_response(): malloc()");
        else {
            ptr_id = packet_to_send;
            memcpy(packet_to_send, packet, packet_len);
            *ptr_id = htons(id & 0xffff);
            relay_packet(packet_to_send, packet_len, &client_addr);
            free(packet_to_send);
        }
    }
}



/**
 * @brief 由DNS报文获得要查找的域名信息
 * @param packet       DNS报文
 * @param domain_name  域名信息
*/
static void get_domain_name(const char* packet, char *domain_name)
{
    int i;
    int cursor;
    int size;

    cursor = 0;
    packet += sizeof(DNS_HEADER);
    size = *packet++;
    while (size != 0) {
        for (i = 0; i < size; i++)
            domain_name[cursor++] = *packet++;
        domain_name[cursor++] = '.';
        size = *packet++;
    }
    domain_name[cursor - 1] = '\0';
}


#ifdef _DEBUG
/**
 * @brief echo 测试
*/
void send_packet_test(const char* buf, int send_len, SOCKADDR_IN* ptr_to_addr)
{
    sendto(_relay_sock, buf, send_len, 0,
        (SOCKADDR *)ptr_to_addr, sizeof(SOCKADDR_IN));
}
#endif