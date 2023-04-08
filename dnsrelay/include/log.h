#pragma once

/**
 * todo: 输出debug信息到命令行
*/


#ifdef __cplusplus
extern "C" {
#endif

#include "protocol.h"

typedef enum _debug_level
{
    DEBUG_LEVEL_0, // * 无调试信息输出
    DEBUG_LEVEL_1, // ? 仅输出时间坐标, 序号, 客户端IP地址, 查询的域名
    DEBUG_LEVEL_2  // ! 输出冗长的调试信息
} debug_level;

/**
 * @brief 设置debug等级
*/
void log_set_level(debug_level level);

/**
 * @brief 输出并处理运行时错误
*/
void log_error_message(const char* message);

/**
 * @brief 将包的时间坐标, 序号, 客户端IP地址, 查询的域名输出
*/
void log_packet_brief_info();

/**
 * @brief 输出包的详细信息
 * @param packet           DNS报文
 * @param packet_len       DNS报文字节数
 * @param ptr_packet_info  报文信息
 * @param ptr_dns_header   DNS头部信息
*/
void log_packet_detailed_info(
    const char *packet,
    int packet_len,
    PACKET_INFO* ptr_packet_info,
    DNS_HEADER* ptr_dns_header
);

/**
 * @brief 不加任何解析, 以十六进制与字符形式显示包的二进制信息
 * @param packet     指向报文的字符指针
 * @param packet_len 报文总共的字节数
*/
void log_packet_raw(const char *packet, int packet_len);

#ifdef __cplusplus
}
#endif