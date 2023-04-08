#include "config.h"
#include <stdio.h>

static FILE *_fp;
static char *_filepath;


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

}