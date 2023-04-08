#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 设置配置文件的路径
*/
void config_set_filepath(const char* filepath);

/**
 * @brief 加载配置文件信息到cache
*/
void config_init();

#ifdef __cplusplus
}
#endif