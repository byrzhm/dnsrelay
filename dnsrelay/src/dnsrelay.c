/**
 * todo: dnsrelay 接口, 程序主体部分
*/

#include "../include/dnsrelay.h"
#include "../include/log.h"
#include "../include/socket.h"
#include "../include/config.h"
#include <string.h>


void dnsrelay_init(int argc, const char *argv[])
{
	parse_args(argc, argv);
	sock_init();
}



/**
 * ! ./dnsrelay [-d/-dd] [dns-server-ipaddr] [filename]
 * @param:
 * 		argc: 命令行参数个数
 * 		argv: 命令行参数数组
 * @return:
 * 		如果有对应的命令行参数信息
 * 		调用 log_set_level() 设置debug等级
 * 		调用 sock_set_servaddr() 设置外部DNS服务器的sockaddr信息
 * 		调用 config_set_filepath() 设置配置文件路径
*/
void parse_args(int argc, const char* argv[])
{
	if (argc == 1)
		return;

	if (strcmp(argv[1], "-dd") == 0)
		log_set_level(DEBUG_LEVEL_2);
	else if (strcmp(argv[1], "-d") == 0)
		log_set_level(DEBUG_LEVEL_1);
	else
		log_set_level(DEBUG_LEVEL_0);

	if (argc > 2)
		sock_set_servaddr(argv[2]);
	
	if (argc > 3)
		config_set_filepath(argv[3]);
}