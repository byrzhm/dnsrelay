#pragma once

/**
 * ���debug��Ϣ��������
*/


#ifdef __cplusplus
extern "C" {
#endif

#include "protocol.h"
#include <stdio.h>

typedef enum _debug_level
{
    DEBUG_LEVEL_0, // �޵�����Ϣ���
    DEBUG_LEVEL_1, // �����ʱ������, ���, �ͻ���IP��ַ, ��ѯ������
    DEBUG_LEVEL_2  // ����߳��ĵ�����Ϣ
} debug_level;



/**
 * @brief ����debug�ȼ�
*/
void log_set_db_level(debug_level level);

/**
 * @brief �������������ʱ����
*/
void log_error_message(const char* message);

/**
 * @brief ����debug�ȼ�������ܵ���DNS��������Ϣ
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
);

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
);

/**
 * @brief ����debug�ȼ�������͵�DNS������Ϣ
 * @param to_addr           Ŀ�ĵص��׽��ֵ�ַ
 * @param send_bytes        ���͵��ֽ���
 * @param old_id            �ɵ�ID
 * @param new_id            ת�������ID
*/
void log_packet_sent(struct sockaddr_in* to_addr, int send_bytes, unsigned short old_id, unsigned short new_id);

/**
* @brief ���������Ϣ
* @param ip_str IP�ַ���
* @param name   ����
*/
void log_config_info(int line_cnt, const char* ip_str, const char* name);


/**
* @brief �������汾��Ϣ���÷�
*/
static void inline log_hello_info()
{
    printf("DNSRELAY, Version 0.1, Build: " __DATE__ " " __TIME__ "\n");
    printf("Usage: dnsrelay [-d | -dd] [<dns-server>] [<db-file>]\n\n");
}


#ifdef __cplusplus
}
#endif