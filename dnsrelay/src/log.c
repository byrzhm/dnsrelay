/**
 * todo: 输出debug信息到命令行
*/
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <WinSock2.h>

static debug_level _level;

void log_set_level(debug_level level) {
    _level = level;
}

void log_error_message(const char *message) {
    fprintf(stderr, "[ERROR]: %s %u\n", message, GetLastError());
    exit(1);
}
