/**
 * todo: 输出debug信息到命令行
*/
#include "../include/log.h"
#include <stdio.h>

static debug_level _level;

void log_set_level(debug_level level) {
    _level = level;
}

void log_error_handling(const char *message) {
    fprintf("[ERROR]: %s\n", )
}