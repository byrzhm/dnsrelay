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
 * @brief malloc ��װ����
 * @param size ����Ŀռ��С
 * @return ���뵽���ڴ�ռ�ĵ�ַ
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
* @brief �ж��ַ��Ƿ�������
* @return �����ַ���true, ���򷵻�false
*/
static inline bool is_digit(char chr) { return chr >= '0' && chr <= '9'; }

/**
* @brief �ж��ַ��Ƿ���Ӣ����ĸ
* @return ��Ӣ����ĸ����true, ���򷵻�false
*/
static inline bool is_letter(char chr) { 
    return (chr >= 'A' && chr <= 'Z') || (chr >= 'a' && chr <= 'z');
}