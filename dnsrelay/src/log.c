#ifndef  _CRT_SECURE_NO_WARNINGS
#define  _CRT_SECURE_NO_WARNINGS
#endif

#ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#endif

#include "log.h"
#include "thread.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <WinSock2.h>
#include <Windows.h>

/// @brief 静态变量存储debug等级
static debug_level _db_level;
static int _packet_cnt;

static inline void log_packet_raw(const char* packet, int packet_len);
static inline void log_packet_brief_info(
    struct sockaddr_in* sock_addr,
    PACKET_INFO* ptr_packet_info,
    DNS_HEADER* ptr_dns_header
);
static inline void log_packet_detailed_info(
    PACKET_INFO* ptr_packet_info,
    DNS_HEADER* ptr_dns_header
);
static inline void print_space();
static inline void print_header_info(DNS_HEADER* ptr_dns_header);

/**
 * @brief 设置debug等级
*/
void log_set_db_level(debug_level db_level) {
    _db_level = db_level;
}


/**
 * @brief 输出并处理运行时错误
*/
void log_error_message(const char *message) {
    fprintf(stderr, "[ERROR %u]: %s\n", GetLastError(), message);
    exit(1);
}


/**
 * @brief 根据debug等级输出包的信息
 * @param packet           DNS报文
 * @param packet_len       DNS报文字节数
 * @param sock_addr        DNS报文发送方的套接字地址
 * @param ptr_packet_info  报文信息
 * @param ptr_dns_header   DNS头部信息
*/
void log_received_query_packet(
    const char* packet,
    int packet_len,
    struct sockaddr_in* sock_addr,
    PACKET_INFO* ptr_packet_info,
    DNS_HEADER* ptr_dns_header
)
{
    if (_db_level == DEBUG_LEVEL_1) {
        WaitForSingleObject(get_stdout_mutex(), INFINITE);
        log_packet_brief_info(sock_addr, ptr_packet_info, ptr_dns_header);
        ReleaseMutex(get_stdout_mutex());
    }
    else if (_db_level == DEBUG_LEVEL_2) {
        // 在send完成之后才可以release
        WaitForSingleObject(get_stdout_mutex(), INFINITE);
        printf("RECV from %s:%u (%d bytes)\n", 
            inet_ntoa((*sock_addr).sin_addr), 
            ntohs((*sock_addr).sin_port), 
            packet_len
        );
        log_packet_raw(packet, packet_len);
        log_packet_detailed_info(ptr_packet_info, ptr_dns_header);
    }
}

/**
 * @brief 根据debug等级输出接受到的DNS应答报文信息
 * @param packet           DNS报文
 * @param packet_len       DNS报文字节数
 * @param client_addr      DNS客户端的套接字地址
 * @param serv_addr        DNS外部服务器的套接字地址
 * @param ptr_dns_header   DNS头部信息
 * @param origin_id        转换前的ID
*/
void log_received_response_packet(
    const char* packet,
    int packet_len, 
    struct sockaddr_in * client_addr,
    struct sockaddr_in * serv_addr,
    DNS_HEADER* ptr_dns_header, 
    unsigned short origin_id
)
{
    if (_db_level == DEBUG_LEVEL_2) {
        WaitForSingleObject(get_stdout_mutex(), INFINITE);
        printf("RECV from %s:%u (%d bytes)\n",
            inet_ntoa((*serv_addr).sin_addr),
            ntohs((*serv_addr).sin_port),
            packet_len
        );
        log_packet_raw(packet, packet_len);
        print_header_info(ptr_dns_header);
        printf("SEND to %s:%u (%d bytes) [ID %x -> %x]\n",
            inet_ntoa((*client_addr).sin_addr),
            ntohs((*client_addr).sin_port),
            packet_len,
            ptr_dns_header->id,
            origin_id
        );
        ReleaseMutex(get_stdout_mutex());
    }
}

/**
 * @brief 根据debug等级输出发送的DNS报文信息
 * @param to_addr           目的地的套接字地址
 * @param send_bytes        发送的字节数
 * @param old_id            旧的ID
 * @param new_id            转换后的新ID
*/
void log_packet_sent(struct sockaddr_in* to_addr, int send_bytes, unsigned short old_id, unsigned short new_id)
{
    if (_db_level >= DEBUG_LEVEL_2) {
        printf("SEND to %s:%d (%d bytes) [ID %x->%x]\n",
            inet_ntoa((*to_addr).sin_addr),
            ntohs((*to_addr).sin_port),
            send_bytes, old_id, new_id
        );
        ReleaseMutex(get_stdout_mutex());
    }
}


/**
* @brief 输出配置信息, 无互斥锁(不需要)
* @param ip_str IP字符串
* @param name   域名
*/
void log_config_info(int line_cnt, const char* ip_str, const char* name)
{
    if (_db_level >= DEBUG_LEVEL_2) {
        printf("%10d: %s\t%s\n", line_cnt, ip_str, name);
    }
}


/**
 * @brief 将包的时间坐标, 序号, 客户端IP地址, 查询的域名输出
*/
static inline void log_packet_brief_info(
    struct sockaddr_in* sock_addr,
    PACKET_INFO* ptr_packet_info,
    DNS_HEADER* ptr_dns_header
)
{
    
    int time_str_len;
    char time_str[50] = "null time";
    char* temp_ptr = asctime(localtime(&ptr_packet_info->current_time));

    if (temp_ptr != NULL) {
        strcpy(time_str, temp_ptr);
        time_str_len = (int)strlen(time_str);
        if (time_str[time_str_len - 1] == '\n') {
            time_str[time_str_len - 1] = 0;
        }
    }

    if (!(ptr_dns_header->flags & 0x8000 >> 15)) {
        // query
        printf("%2d:  %s    Client %s    %s, TYPE %d, CLASS %d\n",
            _packet_cnt++,
            time_str,
            inet_ntoa((*sock_addr).sin_addr),
            ptr_packet_info->domain_name,
            ptr_packet_info->type,
            ptr_packet_info->class
        );
    }
}



/**
 * @brief 输出包的详细信息
 * @param ptr_packet_info  报文信息
 * @param ptr_dns_header   DNS头部信息
*/
static inline void log_packet_detailed_info(
    PACKET_INFO* ptr_packet_info,
    DNS_HEADER* ptr_dns_header
)
{
    int time_str_len;
    char time_str[50] = "null time";
    struct sockaddr_in temp_addr;
    char* temp_ptr = asctime(localtime(&ptr_packet_info->current_time));

    if (temp_ptr != NULL) {
        strcpy(time_str, temp_ptr);
        time_str_len = (int)strlen(time_str);
        if (time_str[time_str_len - 1] == '\n') {
            time_str[time_str_len - 1] = 0;
        }
    }

    memset(&temp_addr, 0, sizeof(temp_addr));
    temp_addr.sin_addr.s_addr = ptr_packet_info->ip_addr;

    print_header_info(ptr_dns_header);

    printf("%2d:  %s    Client %s    %s, TYPE %d, CLASS %d\n",
        _packet_cnt++,
        time_str,
        inet_ntoa(temp_addr.sin_addr),
        ptr_packet_info->domain_name,
        ptr_packet_info->type,
        ptr_packet_info->class
    );
}


/**
 * @brief 不加任何解析, 以十六进制与字符形式显示包的二进制信息
 * @param packet     指向包的字符指针
 * @param packet_len 包的字节数
*/
static inline void log_packet_raw(const char* packet, int packet_len)
{
    int i;
    const unsigned char* ptr = packet;

    for (i = 0; i < packet_len; i++) {
        if (i % 16 == 0)
            print_space();
        printf("%.2x ", ptr[i]);
        if ((i + 1) % 16 == 0)
            printf("\n");
        else if ((i + 1) % 8 == 0)
            printf("   ");
    }

    if (packet_len % 16 != 0)
        printf("\n");

    for (i = 0; i < packet_len; i++) {
        if (i % 16 == 0)
            print_space();

        if ((ptr[i] >= 'a' && ptr[i] <= 'z') || (ptr[i] >= 'A' && ptr[i] <= 'Z'))
            printf("%c  ", ptr[i]);
        else
            printf(".  ");

        if ((i + 1) % 16 == 0)
            printf("\n");
        else if ((i + 1) % 8 == 0)
            printf("   ");
    }

    if (packet_len % 16 != 0)
        printf("\n");
}

static inline void print_space() { printf("        "); }

static inline void print_header_info(DNS_HEADER* ptr_dns_header)
{
    printf("      "
        "ID %x, QR %d, OPCODE %d, AA %d, TC %d, RD %d, RA %d, Z %d, AD %d, CD %d, RCODE %d\n"
        "      "
        "QDCOUNT %d, ANCOUNT %d, NSCOUNT %d, ARCOUNT %d\n",
        (ptr_dns_header->id),                       // ID
        (ptr_dns_header->flags & 0x8000) >> 15,     // QR
        (ptr_dns_header->flags & 0x7800) >> 11,     // OPCODE
        (ptr_dns_header->flags & 0x0400) >> 10,     // AA
        (ptr_dns_header->flags & 0x0200) >> 9,      // TC
        (ptr_dns_header->flags & 0x0100) >> 8,      // RD
        (ptr_dns_header->flags & 0x0080) >> 7,      // RA
        (ptr_dns_header->flags & 0x0040) >> 6,      // Z
        (ptr_dns_header->flags & 0x0020) >> 5,      // AD
        (ptr_dns_header->flags & 0x0010) >> 4,      // CD
        (ptr_dns_header->flags & 0x000F),           // RCODE
        (ptr_dns_header->question_count),           // QDCOUNT
        (ptr_dns_header->answer_count),             // ANCOUNT
        (ptr_dns_header->authority_count),          // NSCOUNT
        (ptr_dns_header->additional_count)          // ARCOUNT
    );
}