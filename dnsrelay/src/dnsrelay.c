/**
 * dnsrelay �ӿ�, �������岿��
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
 * @brief �м̷������ĳ�ʼ��
 * @param argc �����в�������
 * @param argv �����в�������
*/
void dnsrelay_init(int argc, const char *argv[])
{
	srand((unsigned)time(NULL));
	parse_args(argc, argv); // ���������в���
	thread_init();          // ��ʼ���߳���Դ
	sock_init();            // ��ʼ���׽�����Ϣ
	cache_init();           // ��ʼ��cache
	config_init();          // ��ʼ��������Ϣ
}


/**
 * @brief ������ѭ��
*/
void main_loop()
{
	LPPER_HANDLE_DATA handle_info;
	LPPER_IO_DATA io_info;

	for (;;) {
		handle_info = (LPPER_HANDLE_DATA)Malloc(sizeof(PER_HANDLE_DATA));
		io_info = (LPPER_IO_DATA)Malloc(sizeof(PER_IO_DATA));

		// ����DNS����
		recv_packet((void*)handle_info, (void*)io_info, get_relay_sock());
	}
}



/**
 * @brief ���������в���
 * ! ./dnsrelay [-d/-dd] [dns-server-ipaddr] [filename]
 *
 * @param argc �����в�������
 * @param argv �����в�������
 *
 * ����ж�Ӧ�������в�����Ϣ
 * ���� log_set_level() ����debug�ȼ�
 * ���� sock_set_serv_ip() �����ⲿDNS��������ip��Ϣ
 * ���� config_set_filepath() ���������ļ�·��
*/
static inline void parse_args(int argc, const char* argv[])
{
	set_serv_ip(argc, argv);
	set_db_level(argc, argv);
	set_filepath(argc, argv);
}


/**
* @brief ����debug�ȼ�
* @param argc �����в�������
* @param argv �����в�������
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
* @brief �����ⲿDNS��������ַ
* @param argc �����в�������
* @param argv �����в�������
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
		// û�����������������ⲿDNS��������ַ, ʹ��Ĭ���ⲿDNS��������ַ
		// Ĭ���ⲿDNS��������ַ: "10.3.9.45", "10.3.9.44"
		sock_set_serv_ip(default_serv_ip);
		printf("Name server %s:53.\n", default_serv_ip);
	}
}





/**
* @brief ���������ļ�·��
*/
static inline void set_filepath(int argc, const char* argv[])
{
	if (argc > 3)
		config_set_filepath(argv[3]);
	else
#ifdef _DEBUG
		// debugģʽ��Ĭ�������ļ�·�� "./config/dnsrelay.txt"
		config_set_filepath("./config/dnsrelay.txt");
#else
		// releaseģʽ��Ĭ�������ļ�·�� "./config/dnsrelay.txt"
		config_set_filepath("./dnsrelay.txt");
#endif
}
