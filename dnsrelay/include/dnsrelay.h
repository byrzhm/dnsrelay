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
 * @brief 程序主循环
*/
void main_loop();


#ifdef __cplucplus
}
#endif