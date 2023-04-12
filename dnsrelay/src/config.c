#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#endif 

#include "log.h"
#include "cache.h"
#include "config.h"
#include "protocol.h"
#include <string.h>
#include <stdio.h>
#include <windows.h>
#include <winsock2.h>

#define CONFIG_BUF_SIZE 1024

static FILE *_fp;
static char *_filepath;

static void* Malloc(unsigned long long size);


/**
 * @brief 设置配置文件的路径
*/
void config_set_filepath(const char* filepath) {
    _filepath = filepath;
}



/**
 * @brief 加载配置文件信息到cache
*/
void config_init()
{
    int str_len;
    char* buf    = (char *)Malloc(sizeof(char) * CONFIG_BUF_SIZE);
    char* name   = (char *)Malloc(sizeof(char) * MAX_DOMAIN_SIZE);
    char* ip_str = (char *)Malloc(sizeof(char) * MAX_IP_STR_SIZE);
    _fp = fopen(_filepath, "r");
    if (!_fp) {
        log_error_message("config_init()");
    }
    else {
        while (fgets(buf, CONFIG_BUF_SIZE, _fp) != NULL && !feof(_fp)) {
            str_len = strlen(buf);
            if (str_len != 1) {     // in case "\n"
                buf[str_len - 1] = 0;
                str_len--;
                sscanf(buf, "%s %s", ip_str, name);
#ifdef _DEBUG
                printf("%s\n%s\n~~~~~~~~\n", ip_str, name);
#endif
                cache_insert(name, inet_addr(ip_str));
            }
        }

        fclose(_fp);
        free(buf);
        free(name);
        free(ip_str);
    }
}




/**
 * @brief malloc 辅助函数
 * @param size 申请的空间大小
 * @return 申请到的内存空间的地址
*/
static void* Malloc(unsigned long long size)
{
    void* p = malloc(size);
    if (p == NULL) {
        log_error_message("cache.c: Malloc()");
    }
    return p;
}