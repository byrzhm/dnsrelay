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

/// @brief ��̬�����洢debug�ȼ�
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
 * @brief ����debug�ȼ�
*/
void log_set_db_level(debug_level db_level) {
    _db_level = db_level;
}


/**
 * @brief �������������ʱ����
*/
void log_error_message(const char *message) {
    fprintf(stderr, "[ERROR %u]: %s\n", GetLastError(), message);
    exit(1);
}


/**
 * @brief ����debug�ȼ����������Ϣ
 * @param packet           DNS����
 * @param packet_len       DNS�����ֽ���
 * @param sock_addr        DNS���ķ��ͷ����׽��ֵ�ַ
 * @param ptr_packet_info  ������Ϣ
 * @param ptr_dns_header   DNSͷ����Ϣ
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
        // ��send���֮��ſ���release
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
 * @brief ����debug�ȼ�������ܵ���DNSӦ������Ϣ
 * @param packet           DNS����
 * @param packet_len       DNS�����ֽ���
 * @param client_addr      DNS�ͻ��˵��׽��ֵ�ַ
 * @param serv_addr        DNS�ⲿ���������׽��ֵ�ַ
 * @param ptr_dns_header   DNSͷ����Ϣ
 * @param origin_id        ת��ǰ��ID
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
 * @brief ����debug�ȼ�������͵�DNS������Ϣ
 * @param to_addr           Ŀ�ĵص��׽��ֵ�ַ
 * @param send_bytes        ���͵��ֽ���
 * @param old_id            �ɵ�ID
 * @param new_id            ת�������ID
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
* @brief ���������Ϣ, �޻�����(����Ҫ)
* @param ip_str IP�ַ���
* @param name   ����
*/
void log_config_info(int line_cnt, const char* ip_str, const char* name)
{
    if (_db_level >= DEBUG_LEVEL_2) {
        printf("%10d: %s\t%s\n", line_cnt, ip_str, name);
    }
}


/**
 * @brief ������ʱ������, ���, �ͻ���IP��ַ, ��ѯ���������
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
 * @brief ���������ϸ��Ϣ
 * @param ptr_packet_info  ������Ϣ
 * @param ptr_dns_header   DNSͷ����Ϣ
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
 * @brief �����κν���, ��ʮ���������ַ���ʽ��ʾ���Ķ�������Ϣ
 * @param packet     ָ������ַ�ָ��
 * @param packet_len �����ֽ���
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