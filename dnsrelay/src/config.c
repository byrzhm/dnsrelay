#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#endif 

#include "common.h"
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
static const char *_filepath;


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
    int line_cnt = 0;

    _fp = fopen(_filepath, "r");
    if (!_fp) {
        // 读取配置文件失败
        log_error_message(__FUNCTION__ ":fopen() error!");
    }
    else {
        printf("Try to load table \"%s\" ... OK\n", _filepath);

        while (fgets(buf, CONFIG_BUF_SIZE, _fp) != NULL && !feof(_fp)) {
            str_len = (int)strlen(buf);
            if (str_len != 1) {     // 防止一行只有 "\n", 即空行
                buf[str_len - 1] = 0;
                str_len--;
                if (sscanf(buf, "%s %s", ip_str, name) != 2)
                    log_error_message(__FUNCTION__ ": sscanf() failed");
                
                log_config_info(++line_cnt, ip_str, name);
                cache_insert(name, inet_addr(ip_str));
            }
        }
        printf("%d names, occupy %d bytes memory\n", line_cnt, cache_memory_size());

        fclose(_fp);
        free(buf);
        free(name);
        free(ip_str);
    }
}