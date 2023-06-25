#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"
#include "protocol.h"

#include <time.h>
#include <WinSock2.h>

#define ALPHABET_SIZE 66

// ����ʱ��ĺ�
#define SEC_PER_DAY   60 * 60 * 24
#define SEC_PER_MIN   60
#define TTL 		  2 * SEC_PER_MIN
#define INFINITE_TTL  3 * 365 * SEC_PER_DAY

/**
 * @brief �ڵ�����
 * @param INNER  �ڲ��ڵ�
 * @param CONFIG �������ļ��ж�ȡ��Ҷ�ӽڵ�, ����ɾ��
 * @param NORMAL �����������й����в�����������Ҷ�ӽ��, ����ɾ��
*/
typedef enum
{
	INNER, CONFIG, NORMAL
} node_type;

/**
 * @brief �ڵ���Ϣ
 * @param start_time �ڵ㱻���ʱ��ʱ��
 * @param time_to_live �ڵ������ʱ��
 * ! ʹ�� cache_search ��ѯ���ڵ�ʱ, ����ýڵ�Ľڵ�����ΪNORMAL,
 * ! ��ô�ͼ��㵱ǰʱ����start_time�Ĳ�ֵ, �����ֵ����time_to_live��ɾ���ýڵ�
 * @param ip_addr ip����ֵ��ʾ, ����ͨ������ inet_ntoa(ip_addr) ���ip�ַ��� 
 * ! inet_nota�����ڲ�ʹ����һ����̬�����洢ת�������
 * ! �����ķ���ֵָ��þ�̬�ڴ棬����inet_nota�ǲ��������, 
 * ! �����Ҫ������ֵ����һ��Ҫʹ��strcpy���ơ�
*/
typedef struct _IPListNode {
	time_t start_time;
	double time_to_live;
	struct _IPListNode* next;	// һ���������ܶ�Ӧ���IP
	uint32_t ip_addr;
} IPListNode;

/**
 * @brief �����ֵ���
*/
void cache_init();

/**
 * @brief ����һ��Ҷ�ӽڵ�
 * @param domain_name ����			   [key]
 * @param ip_addr	  ip��ַ����ֵ��ʽ [value]
*/
void cache_insert(const char* domain_name, uint32_t ip_addr);


/**
 * @brief ����һ��Ҷ�ӽڵ�
 * @param domain_name ����  [key]
 * @param addr_list   ���ذ���IP��Ϣ������
*/
void cache_search(const char* domain_name, IPListNode** addr_list);


/**
 * @brief  ���ػ�����ռ���ڴ��С
 * @return ������ռ���ڴ��С
*/
int cache_memory_size();

#ifdef __cplusplus
}
#endif
