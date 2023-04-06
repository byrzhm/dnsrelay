/**
 * ! main
*/
#include "dnsrelay.h"


int main(int argc, const char* argv[])
{
	dnsrelay_init(argc, argv);

	// 程序主循环
	main_loop();

	return 0;
}