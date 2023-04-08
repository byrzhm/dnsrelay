#ifndef  _CRT_SECURE_NO_WARNINGS
#define  _CRT_SECURE_NO_WARNINGS
#endif

#ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#endif

#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <WinSock2.h>
#include <Windows.h>

/// @brief 静态变量存储debug等级
static debug_level _level;


/**
 * @brief 设置debug等级
*/
void log_set_level(debug_level level) {
    _level = level;
}



/**
 * @brief 输出并处理运行时错误
*/
void log_error_message(const char *message) {
    fprintf(stderr, "[ERROR]: %s %u\n", message, GetLastError());
    exit(1);
}



/**
 * @brief 将包的时间坐标, 序号, 客户端IP地址, 查询的域名输出
*/
void log_packet_brief_info()
{

}



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
)
{
    int i;
    unsigned char* ptr;
    int count;
    struct sockaddr_in temp_addr;

    memset(&temp_addr, 0, sizeof(temp_addr));
    temp_addr.sin_addr.s_addr = ptr_packet_info->ip_addr;

    ptr = packet;
    count = 0;
    WaitForSingleObject(get_stdout_mutex(), INFINITE);
    
    printf("\n\n\n");

    printf("%s", asctime(localtime(&ptr_packet_info->current_time)));
    printf("Transaction ID: %#x\n", ptr_packet_info->id);
    printf("Src: %s\n", inet_ntoa(temp_addr.sin_addr));
    printf("\t%s\n\n", ptr_packet_info->domain_name);

    for (int i = 0; i < packet_len; i++) {
        printf("%.2x ", ptr[i]);
        ++count;
        if (count % 16 == 0)
            putc('\n', stdout);
        else if (count % 8 == 0)
            printf("   ");
    }
    putc('\n', stdout);
    
    count = 0;
    for (int i = 0; i < packet_len; i++) {
        if ((ptr[i] >= 'a' && ptr[i] <= 'z') || (ptr[i] >= 'A' && ptr[i] <= 'Z'))
            printf("%c  ", ptr[i]);
        else 
            printf(".  ");
        ++count;
        if (count % 16 == 0)
            putc('\n', stdout);
        else if (count % 8 == 0)
            printf("   ");
    }
    
    printf("\n\n\n");
    ReleaseMutex(get_stdout_mutex());
}



/**
 * @brief 不加任何解析, 以十六进制与字符形式显示包的二进制信息
 * @param packet     指向包的字符指针
 * @param packet_len 包的字节数
*/
void log_packet_raw(const char *packet, int packet_len)
{
    int i;
    unsigned char* ptr;
    int count;


    ptr = packet;
    count = 0;
    
    WaitForSingleObject(get_stdout_mutex(), INFINITE);
    
    printf("\n\n\n");
    
    for (int i = 0; i < packet_len; i++) {
        printf("%.2x ", ptr[i]);
        ++count;
        if (count % 16 == 0)
            putc('\n', stdout);
        else if (count % 8 == 0)
            printf("   ");
    }
    putc('\n', stdout);
    
    count = 0;
    for (int i = 0; i < packet_len; i++) {
        if ((ptr[i] >= 'a' && ptr[i] <= 'z') || (ptr[i] >= 'A' && ptr[i] <= 'Z'))
            printf("%c  ", ptr[i]);
        else
            printf(".  ");
        ++count;
        if (count % 16 == 0)
            putc('\n', stdout);
        else if (count % 8 == 0)
            printf("   ");
    }
    
    printf("\n\n\n");
    ReleaseMutex(get_stdout_mutex());
}