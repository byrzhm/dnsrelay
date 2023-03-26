/**
 * todo: 输出debug信息到命令行
*/
#include "../include/log.h"
#include <stdio.h>
#include <Windows.h>

static debug_level _level;

void log_set_level(debug_level level) {
    _level = level;
}

void log_error_message(const char *message) {
    fprintf(stderr, "[ERROR]: %s\n", message);
}

void log_error_code(DWORD error_code)
{
    fprintf(stderr, "\t\t%u\n", error_code);
}

void log_error_code_wsa(int error_code)
{
    fprintf(stderr, "\t\t%d\n", error_code);
}
