#pragma once

/**
 * todo: 管理套接字
*/

#ifdef __cplusplus
extern "C" {
#endif

#include "protocol.h"
#include <WinSock2.h>
#include <Windows.h>

/**
 * @brief  向外部提供获取静态变量_relay_sock的接口
 * @return _relay_sock
*/
SOCKET get_relay_sock();

/// @brief 初始化套接字信息
void sock_init();

/**
 * @brief 设置外部DNS服务器地址
 * @param serv_ip 外部DNS服务器ip地址字符串
*/
void sock_set_serv_ip(const char *serv_ip);

/**
 * @brief 调用recvfrom接受发来的DNS报文, 
 *        调用PostQueuedCompletionStatus()唤醒线程处理DNS报文
 * @param handle_ptr 已经分配好空间的指向PER_HANDLE_DATA的指针
 * @param io_ptr     已经分配好空间的指向PER_IO_DATA的指针
 * ! [注意] io_ptr->overlapped 在传入前必须置零
 * @param recv_sock  接受报文的套接字信息
*/ 
void recv_packet(void *handle_ptr, void *io_ptr, SOCKET recv_sock);


/**
 * @brief 分析发来的DNS报文
 * @param  packet            接受的DNS报文
 * @param  packet_len        接受的DNS报文字节数
 * @param  sock_addr         报文发送方的信息
 * @param  ptr_packet_info   指向报文信息的指针    [指针返回]
 * @param  ptr_dns_header    指向DNS头部信息的指针 [指针返回]
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