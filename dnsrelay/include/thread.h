#pragma once

/**
 * todo: ���� windows �̳߳�
*/

#ifdef __cplusplus
extern "C" {
#endif



#include "common.h"

#define ID_ARRAY_SIZE 8

#ifdef _WIN32

#include <WinSock2.h>
#include <Windows.h>

typedef struct _PER_HANDLE_DATA {
    struct sockaddr_in sock_addr;
} PER_HANDLE_DATA, * LPPER_HANDLE_DATA;

typedef struct _PER_IO_DATA {
    OVERLAPPED overlapped;
    char buffer[BUF_SIZE];
} PER_IO_DATA, * LPPER_IO_DATA;

typedef struct _IDListNode {
    struct _IDListNode *next;
    /// @brief ids �ĸ�16λΪ ���ⲿDNS������֮���DNS����
    /// @brief ids �ĵ�16λΪ ��DNS�ͻ���֮���DNS����ID
    unsigned id_pair_array[ID_ARRAY_SIZE];
    struct sockaddr_in client_addr[ID_ARRAY_SIZE];
    int size;
#define high_id(id_pair) ((id_pair) >> 16)
#define low_id(id_pair)  ((id_pair) & 0xFFFF)
} IDListNode;

typedef struct _IDList {
    IDListNode *head;
    int size;
} IDList;


/**
 * @brief �ṩ�ӿ�
 * @return _com_port
*/
HANDLE get_com_port();

/**
 * @brief �ṩ�ӿ�
 * @return _stdout_mutex
*/
HANDLE get_stdout_mutex();

/**
 * @brief �ṩ�ӿ�
 * @return _relay_sock_mutex
*/
HANDLE get_relay_sock_mutex();

/**
 * @brief �ṩ�ӿ�
 * @return _cache_mutex
*/
HANDLE get_cache_mutex();

/**
 * @brief �����߳�, ����mutex, ά��IDת����
*/
void thread_init();


#define ID_NOT_FOUND     -1
#define ID_ALREADY_EXIST -2

/**
 * @brief   ����QR, flag��Ϣ, ��ѯĿ��id
 * @param   target_id Ҫ���ҵ�id
 * @param   QR        QR=0, ���ҵ�λ; QR=1, ���Ҹ�λ
 * @param   flag      flag=0, Ϊ���ض�����, ��ɾ��; flag=1, �ҵ�id��, �������Ӧid, ��ɾ��
 * @param   ptr_addr  ������NULL������, �����������id��Ӧ��DNS�ͻ��׽��ֵ�ַ
 * @return  ALREADY_EXIST ID�Ѿ�����, ID_NOT_FOUNDδ�ҵ�, �ɹ��򷵻ض�Ӧ��id >=0 
 */
int id_search(unsigned short target_id, int QR, int flag, SOCKADDR_IN *ptr_addr);

/**
 * @brief ����id_pair
 * @param id_pair  Ҫ�����id_pair
 * @param ptr_addr Ҫ������׽��ֵ�ַ��Ϣ
*/
void id_insert(unsigned id_pair, struct sockaddr_in* ptr_addr);

#define MAKE_ID_PAIR(x, y) (((x) << 16) ^ (y))

#endif

#ifdef __cplusplus
}
#endif