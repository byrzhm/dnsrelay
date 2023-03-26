/**
 * todo: dnsrelay 接口, 程序主体部分
*/

#pragma once

void dnsrelay_init(int argc, const char *argv[]);

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
void parse_args(int argc, const char* argv[]);