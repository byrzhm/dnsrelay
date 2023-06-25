#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"
#include "protocol.h"

#include <time.h>
#include <WinSock2.h>

#define ALPHABET_SIZE 66

// 关于时间的宏
#define SEC_PER_DAY   60 * 60 * 24
#define SEC_PER_MIN   60
#define TTL 		  2 * SEC_PER_MIN
#define INFINITE_TTL  3 * 365 * SEC_PER_DAY

/**
 * @brief 节点种类
 * @param INNER  内部节点
 * @param CONFIG 从配置文件中读取的叶子节点, 不可删除
 * @param NORMAL 程序正常运行过程中产生的正常的叶子结点, 可以删除
*/
typedef enum
{
	INNER, CONFIG, NORMAL
} node_type;

/**
 * @brief 节点信息
 * @param start_time 节点被添加时的时间
 * @param time_to_live 节点的生存时期
 * ! 使用 cache_search 查询到节点时, 如果该节点的节点类型为NORMAL,
 * ! 那么就计算当前时间与start_time的差值, 如果差值大于time_to_live则删除该节点
 * @param ip_addr ip的数值表示, 可以通过调用 inet_ntoa(ip_addr) 获得ip字符串 
 * ! inet_nota函数内部使用了一个静态变量存储转化结果，
 * ! 函数的返回值指向该静态内存，所以inet_nota是不可重入的, 
 * ! 如果需要将返回值保存一定要使用strcpy复制。
*/
typedef struct _IPListNode {
	time_t start_time;
	double time_to_live;
	struct _IPListNode* next;	// 一个域名可能对应多个IP
	uint32_t ip_addr;
} IPListNode;

/**
 * @brief 创建字典树
*/
void cache_init();

/**
 * @brief 插入一个叶子节点
 * @param domain_name 域名			   [key]
 * @param ip_addr	  ip地址的数值形式 [value]
*/
void cache_insert(const char* domain_name, uint32_t ip_addr);


/**
 * @brief 查找一个叶子节点
 * @param domain_name 域名  [key]
 * @param addr_list   返回包含IP信息的链表
*/
void cache_search(const char* domain_name, IPListNode** addr_list);


/**
 * @brief  返回缓存所占的内存大小
 * @return 缓存所占的内存大小
*/
int cache_memory_size();

#ifdef __cplusplus
}
#endif
