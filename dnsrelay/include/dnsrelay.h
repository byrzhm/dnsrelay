#pragma once

/**
 * todo: dnsrelay 接口, 程序主体部分
*/


#ifdef __cplucplus
extern "C" {
#endif

/**
 * @fn 创建线程, 套接字, 读取配置文件
 * @brief 中继服务器的初始化
 * @param argc 命令行参数个数
 * @param argv 命令行参数数组
*/
void dnsrelay_init(int argc, const char *argv[]);


/**
 * @brief 分析命令行参数
 * ! ./dnsrelay [-d/-dd] [dns-server-ipaddr] [filename]
 * @param argc 命令行参数个数
 * @param argv 命令行参数数组
*
 * 如果有对应的命令行参数信息
 * 调用 log_set_level() 设置debug等级
 * 调用 sock_set_servaddr() 设置外部DNS服务器的sockaddr信息
 * 调用 config_set_filepath() 设置配置文件路径
*/
void parse_args(int argc, const char* argv[]);


/**
 * @brief 程序主循环
*/
void main_loop();


#ifdef __cplucplus
}
#endif