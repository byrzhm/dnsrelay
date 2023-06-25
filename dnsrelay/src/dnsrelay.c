/**
 * dnsrelay 接口, 程序主体部分
*/

#include "dnsrelay.h"
#include "log.h"
#include "socket.h"
#include "config.h"
#include "thread.h"
#include "protocol.h"
#include "cache.h"
#include "common.h"

#include <time.h>
#include <WinSock2.h>
#include <Windows.h>

static inline void parse_args(int argc, const char* argv[]);
static inline void set_serv_ip(int argc, const char* argv[]);
static inline void set_db_level(int argc, const char* argv[]);
static inline void set_filepath(int argc, const char* argv[]);
static inline bool is_digit(char chr);


/**
 * @brief 中继服务器的初始化
 * @param argc 命令行参数个数
 * @param argv 命令行参数数组
*/
void dnsrelay_init(int argc, const char *argv[])
{
	srand((unsigned)time(NULL));
	parse_args(argc, argv); // 处理命令行参数
	thread_init();          // 初始化线程资源
	sock_init();            // 初始化套接字信息
	cache_init();           // 初始化cache
	config_init();          // 初始化配置信息
}


/**
 * @brief 程序主循环
*/
void main_loop()
{
	LPPER_HANDLE_DATA handle_info;
	LPPER_IO_DATA io_info;

	for (;;) {
		handle_info = (LPPER_HANDLE_DATA)Malloc(sizeof(PER_HANDLE_DATA));
		io_info = (LPPER_IO_DATA)Malloc(sizeof(PER_IO_DATA));

		// 接受DNS报文
		recv_packet((void*)handle_info, (void*)io_info, get_relay_sock());
	}
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
 * 调用 sock_set_serv_ip() 设置外部DNS服务器的ip信息
 * 调用 config_set_filepath() 设置配置文件路径
*/
static inline void parse_args(int argc, const char* argv[])
{
	set_serv_ip(argc, argv);
	set_db_level(argc, argv);
	set_filepath(argc, argv);
}


/**
* @brief 设置debug等级
* @param argc 命令行参数个数
* @param argv 命令行参数数组
*/
static inline void set_db_level(int argc, const char* argv[])
{
	debug_level level = DEBUG_LEVEL_0;

	if (argc > 1) {
		if (!strcmp(argv[1], "-d"))
			level = DEBUG_LEVEL_1;
		else if (!strcmp(argv[1], "-dd"))
			level = DEBUG_LEVEL_2;
		else {
			printf("Usage error!\n");
			exit(1);
		}
	}

	log_set_db_level(level);
	printf("Debug level %d.\n", (int)level);
}


/**
* @brief 设置外部DNS服务器地址
* @param argc 命令行参数个数
* @param argv 命令行参数数组
*/
static void inline set_serv_ip(int argc, const char* argv[])
{
	char chr;
	char default_serv_ip[] = "10.3.9.44";
	int dot_cnt = 0;
	bool no_digit = false;

	if (argc > 2) {
		for (int i = 0; argv[2][i] != '\0'; i++) {
			chr = argv[2][i];
			if (chr == '.')
				dot_cnt++;
			else if (!is_digit(chr)) {
				no_digit = true;
				break;
			}
		}

		if (dot_cnt == 3 && !no_digit) {
			sock_set_serv_ip(argv[2]);
			printf("Name server %s:53.\n", argv[2]);
		}
		else {
			printf("Usage error!\n");
			exit(1);
		}
	}
	else {
		// 没有在命令行中设置外部DNS服务器地址, 使用默认外部DNS服务器地址
		// 默认外部DNS服务器地址: "10.3.9.45", "10.3.9.44"
		sock_set_serv_ip(default_serv_ip);
		printf("Name server %s:53.\n", default_serv_ip);
	}
}





/**
* @brief 设置配置文件路径
*/
static inline void set_filepath(int argc, const char* argv[])
{
	if (argc > 3)
		config_set_filepath(argv[3]);
	else
#ifdef _DEBUG
		// debug模式下默认配置文件路径 "./config/dnsrelay.txt"
		config_set_filepath("./config/dnsrelay.txt");
#else
		// release模式下默认配置文件路径 "./config/dnsrelay.txt"
		config_set_filepath("./dnsrelay.txt");
#endif
}
