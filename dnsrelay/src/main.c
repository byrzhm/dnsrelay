#include "dnsrelay.h"
#include "log.h"

int main(int argc, const char* argv[])
{
	log_hello_info();

	dnsrelay_init(argc, argv);

	// ������ѭ��
	main_loop();

	return 0;
}