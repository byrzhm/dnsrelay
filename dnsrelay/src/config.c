/**
* todo: 配置文件相关
*/

#include "../include/config.h"
#include <stdio.h>

static FILE *_fp;
static char *_filepath = "./dnsrelay.txt";

void config_set_filepath(const char* filepath) {
    _filepath = filepath;
}