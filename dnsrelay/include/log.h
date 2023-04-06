#pragma once

/**
 * todo: 输出debug信息到命令行
*/


#ifdef __cplusplus
extern "C" {
#endif

typedef enum _debug_level
{
    DEBUG_LEVEL_0, // * 无调试信息输出
    DEBUG_LEVEL_1, // ? 仅输出时间坐标, 序号, 客户端IP地址, 查询的域名
    DEBUG_LEVEL_2  // ! 输出冗长的调试信息
} debug_level;

void log_set_level(debug_level level);

void log_error_message(const char* message);

#ifdef __cplusplus
}
#endif