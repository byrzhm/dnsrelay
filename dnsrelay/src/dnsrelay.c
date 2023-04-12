/**
 * todo: dnsrelay 接口, 程序主体部分
*/

#include "dnsrelay.h"
#include "log.h"
#include "socket.h"
#include "config.h"
#include "thread.h"
#include "protocol.h"
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <WinSock2.h>
#include <Windows.h>


/**
 * @brief 中继服务器的初始化
 * @param argc 命令行参数个数
 * @param argv 命令行参数数组
*/
void dnsrelay_init(int argc, const char *argv[])
{
	// 顺序固定
	parse_args(argc, argv);
	thread_init();
	sock_init();
	cache_init(); 
	config_init();
}



/**
 * @brief 分析命令行参数
 * ! ./dnsrelay [-d/-dd] [dns-server-ipaddr] [filename]
 * 
 * @param argc 命令行参数个数
 * @param argv 命令行参数数组
 * 
 * 如果有对应的命令行参数信息
 * 调用 log_set_level() 设置debug等级
 * 调用 sock_set_servaddr() 设置外部DNS服务器的sockaddr信息
 * 调用 config_set_filepath() 设置配置文件路径
*/
void parse_args(int argc, const char* argv[])
{
	if (argc > 1) {
		// 设置debug等级
		if (strcmp(argv[1], "-dd") == 0)
			log_set_level(DEBUG_LEVEL_2);
		else if (strcmp(argv[1], "-d") == 0)
			log_set_level(DEBUG_LEVEL_1);
		else
			log_set_level(DEBUG_LEVEL_0);
	}

	// 设置外部DNS服务器地址
	if (argc > 2)
		set_serv_addr(argv[2]);
	else
		// 默认外部DNS服务器地址 "10.3.9.45", "10.3.9.44"
		set_serv_addr("10.3.9.44");	
	
	// 设置配置文件路径
	if (argc > 3)
		config_set_filepath(argv[3]);
	else
		// 默认配置文件路径 "./dnsrelay.txt"
		// config_set_filepath("./dnsrelay.txt");
		config_set_filepath("./config/dnsrelay.txt");
}




/**
 * @brief 程序主循环
*/
void main_loop()
{
	LPPER_HANDLE_DATA handle_info;
	LPPER_IO_DATA io_info;

	while(1) {
		handle_info = (LPPER_HANDLE_DATA)malloc(sizeof(PER_HANDLE_DATA));
		io_info = (LPPER_IO_DATA)malloc(sizeof(PER_IO_DATA));

		// 检查内存分配情况
		if (!handle_info || !io_info)	
			log_error_message("main_loop(): malloc()");
		else {
			// 接受DNS报文
			recv_packet((void*)handle_info, (void*)io_info, get_relay_sock());
		}
	}
}