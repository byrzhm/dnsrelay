#pragma once

/**
 * todo: �����׽���
*/

#ifdef __cplusplus
extern "C" {
#endif

#include "protocol.h"
#include <WinSock2.h>
#include <Windows.h>

/**
 * @brief  ���ⲿ�ṩ��ȡ��̬����_relay_sock�Ľӿ�
 * @return _relay_sock
*/
SOCKET get_relay_sock();

/// @brief ��ʼ���׽�����Ϣ
void sock_init();

/**
 * @brief �����ⲿDNS��������ַ
 * @param serv_ip �ⲿDNS������ip��ַ�ַ���
*/
void sock_set_serv_ip(const char *serv_ip);

/**
 * @brief ����recvfrom���ܷ�����DNS����, 
 *        ����PostQueuedCompletionStatus()�����̴߳���DNS����
 * @param handle_ptr �Ѿ�����ÿռ��ָ��PER_HANDLE_DATA��ָ��
 * @param io_ptr     �Ѿ�����ÿռ��ָ��PER_IO_DATA��ָ��
 * ! [ע��] io_ptr->overlapped �ڴ���ǰ��������
 * @param recv_sock  ���ܱ��ĵ��׽�����Ϣ
*/ 
void recv_packet(void *handle_ptr, void *io_ptr, SOCKET recv_sock);


/**
 * @brief ����������DNS����
 * @param  packet            ���ܵ�DNS����
 * @param  packet_len        ���ܵ�DNS�����ֽ���
 * @param  sock_addr         ���ķ��ͷ�����Ϣ
 * @param  ptr_packet_info   ָ������Ϣ��ָ��    [ָ�뷵��]
 * @param  ptr_dns_header    ָ��DNSͷ����Ϣ��ָ�� [ָ�뷵��]
 */
void parse_packet(
    const char *packet,
    int packet_len,
    struct sockaddr_in *sock_addr,
    PACKET_INFO *ptr_packet_info,
    DNS_HEADER *ptr_dns_header
);

#ifdef __cplusplus
}
#endif