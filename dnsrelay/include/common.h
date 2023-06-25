#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#ifdef _WIN32

#include <WinSock2.h>

typedef UINT32 uint32_t;

#else
// Linux
#include <arpa/inet.h>
#include <sys/socket.h>

typedef int SOCKET;

#endif

#define BUF_SIZE 1024

/**
 * @brief malloc 封装函数
 * @param size 申请的空间大小
 * @return 申请到的内存空间的地址
*/
static inline void* Malloc(size_t size)
{
    void* p = malloc(size);
    if (p == NULL) {
        fprintf(stderr, "[ERROR]: malloc() failed. %u\n", GetLastError());
        exit(1);
    }
    return p;
}

/**
* @brief 判断字符是否是数字
* @return 是数字返回true, 否则返回false
*/
static inline bool is_digit(char chr) { return chr >= '0' && chr <= '9'; }

/**
* @brief 判断字符是否是英文字母
* @return 是英文字母返回true, 否则返回false
*/
static inline bool is_letter(char chr) { 
    return (chr >= 'A' && chr <= 'Z') || (chr >= 'a' && chr <= 'z');
}