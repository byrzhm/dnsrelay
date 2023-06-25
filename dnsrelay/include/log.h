#pragma once

/**
 * 输出debug信息到命令行
*/


#ifdef __cplusplus
extern "C" {
#endif

#include "protocol.h"
#include <stdio.h>

typedef enum _debug_level
{
    DEBUG_LEVEL_0, // 无调试信息输出
    DEBUG_LEVEL_1, // 仅输出时间坐标, 序号, 客户端IP地址, 查询的域名
    DEBUG_LEVEL_2  // 输出冗长的调试信息
} debug_level;



/**
 * @brief 设置debug等级
*/
void log_set_db_level(debug_level level);

/**
 * @brief 输出并处理运行时错误
*/
void log_error_message(const char* message);

/**
 * @brief 根据debug等级输出接受到的DNS请求报文信息
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
);

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
);

/**
 * @brief 根据debug等级输出发送的DNS报文信息
 * @param to_addr           目的地的套接字地址
 * @param send_bytes        发送的字节数
 * @param old_id            旧的ID
 * @param new_id            转换后的新ID
*/
void log_packet_sent(struct sockaddr_in* to_addr, int send_bytes, unsigned short old_id, unsigned short new_id);

/**
* @brief 输出配置信息
* @param ip_str IP字符串
* @param name   域名
*/
void log_config_info(int line_cnt, const char* ip_str, const char* name);


/**
* @brief 输出程序版本信息与用法
*/
static void inline log_hello_info()
{
    printf("DNSRELAY, Version 0.1, Build: " __DATE__ " " __TIME__ "\n");
    printf("Usage: dnsrelay [-d | -dd] [<dns-server>] [<db-file>]\n\n");
}


#ifdef __cplusplus
}
#endif