#include "protocol.h"
#include "thread.h"
#include "socket.h"
#include "log.h"
#include <stdio.h>
#include <time.h>
#include <WinSock2.h>
#include <windows.h>
#include <process.h>

static HANDLE _com_port;         // IO��ɶ˿�
static HANDLE _stdout_mutex;     // ���������
static HANDLE _cache_mutex;      // cache������
static HANDLE _id_list_mutex;    // IDת��������
static HANDLE _relay_sock_mutex; // relay_sock������ ?

static IDList _id_list;          // IDת����



static void build_id_list();
static unsigned WINAPI thread_main(LPVOID lpComPort);

/**
 * @brief �ṩ�ӿ�
 * @return _com_port
*/
HANDLE get_com_port()
{
    return _com_port;
}

/**
 * @brief �ṩ�ӿ�
 * @return _stdout_mutex
*/
HANDLE get_stdout_mutex()
{
    return _stdout_mutex;
}


/**
 * @brief �ṩ�ӿ�
 * @return _relay_sock_mutex
*/
HANDLE get_relay_sock_mutex()
{
    return _relay_sock_mutex;
}


/**
 * @brief �ṩ�ӿ�
 * @return _cache_mutex
*/
HANDLE get_cache_mutex()
{
    return _cache_mutex;
}


/**
 * @brief ����IOCP, �����߳�, ����mutex, ����IDת����
*/
void thread_init()
{
    unsigned i;
    SYSTEM_INFO sys_info;

    // ���� IO ��ɶ˿�
    _com_port = CreateIoCompletionPort(
        INVALID_HANDLE_VALUE, NULL, 0, 0
    );

    if (_com_port == NULL)
        log_error_message("thread_init(): CreateIoCompletionPort()");
    else {
        // �����߳�
        GetSystemInfo(&sys_info);
        for (i = 0;
#ifdef _DEBUG
            i < 2;
#else
            i < 2 * sys_info.dwNumberOfProcessors;
#endif
            i++)
            _beginthreadex(NULL, 0, thread_main,
                (LPVOID)_com_port, 0, NULL);
    }

    // ���� mutex
    _stdout_mutex = CreateMutex(NULL, FALSE, NULL);
    _cache_mutex = CreateMutex(NULL, FALSE, NULL);
    _id_list_mutex = CreateMutex(NULL, FALSE, NULL);
    _relay_sock_mutex = CreateMutex(NULL, FALSE, NULL);

    if (_stdout_mutex == NULL
        || _cache_mutex == NULL
        || _id_list_mutex == NULL
        || _relay_sock_mutex == NULL)
        log_error_message("thread_init(): CreateMutex()");

    // ����IDת����
    build_id_list();
}


/**
 * @brief �����̵߳ĳ�ʼ����λ��
 * @param lpComPort IO��ɶ˿�
 * @return �̷߳���ֵ: 0 Ϊ����, ����Ϊ�쳣
*/
static unsigned WINAPI thread_main(LPVOID lpComPort)
{
    HANDLE hComPort = (HANDLE)lpComPort;
    DWORD recv_bytes;
    LPPER_HANDLE_DATA handle_info;
    LPPER_IO_DATA io_info;
    BOOL bOk;
    PACKET_INFO packet_info;
    DNS_HEADER dns_header;


    while (1) {
        // [WARNING] 'GetQueuedCompletionStatus': different types for formal and actual parameter 4
        // ��� warning ���Ժ���, ������ &PER_IO_DATA == &(PER_IO_DATA.OVERLAPPED), �� PER_IO_DATA �Ľṹ������
        bOk = GetQueuedCompletionStatus(
            hComPort, &recv_bytes, (PULONG_PTR)&handle_info,
            (LPOVERLAPPED*)&io_info, INFINITE
        );

        packet_info.current_time = time(NULL);

        if (!bOk) {
                log_error_message("thread_main(): GetQueuedCompletionStatus()");
        }

        parse_packet(io_info->buffer, recv_bytes, &handle_info->sock_addr, &packet_info, &dns_header);


        free(handle_info);
        free(io_info);
    }

    return 0;
}

/**
 * @brief ����IDת����
*/
static void build_id_list()
{
    IDListNode* ptr = (IDListNode*)malloc(sizeof(IDListNode));
    if (ptr == NULL) {
        log_error_message("build_id_list(): malloc()");
    }
    else
    {
        ptr->next = NULL;
        ptr->size = 0;
        _id_list.head = ptr;
        _id_list.size = 0;
    }
}


/**
 * @brief ɾ��id_pair
 * @param pnode ��ɾ��id_pair�Ľڵ��ַ
 * @param index ��ɾ��id_pair���ڵ���������
*/
static void id_remove(IDListNode* pnode, int index)
{
    int i;
    for (i = index; i < pnode->size - 1; i++) {
        pnode->id_pair_array[i] = pnode->id_pair_array[i + 1];
        pnode->client_addr[i] = pnode->client_addr[i + 1];
    }

    pnode->size--;
}

/**
 * @brief   ����QR, flag��Ϣ, ��ѯĿ��id
 * @param   target_id Ҫ���ҵ�id
 * @param   QR        QR=0, ���ҵ�λ; QR=1, ���Ҹ�λ
 * @param   flag      flag=0, Ϊ���ض�����, ��ɾ��; flag=1, �ҵ�id��, �������Ӧid, ��ɾ��
 * @param   ptr_addr  ������NULL������, �����������id��Ӧ��DNS�ͻ��׽��ֵ�ַ
 * @return  ALREADY_EXIST ID�Ѿ�����, ID_NOT_FOUNDδ�ҵ�, �ɹ��򷵻ض�Ӧ��id >=0
 */
int id_search(unsigned short target_id, int QR, int flag, SOCKADDR_IN* ptr_addr)
{
    int ret = ID_NOT_FOUND;
    int i;
    int found = 0;
    IDListNode* ptr = _id_list.head;

    WaitForSingleObject(_id_list_mutex, INFINITE);

    while (ptr != NULL && !found) {
        for (i = 0; i < ptr->size && !found;) {
            if (QR == 0) {
                if (low_id(ptr->id_pair_array[i]) == target_id) {
                    found = 1;
                }
                else {
                    i++;
                }
            }
            else {
                if (high_id(ptr->id_pair_array[i]) == target_id) {
                    found = 1;
                }
                else {
                    i++;
                }
            }
        }

        if (!found)
            ptr = ptr->next;
    }

    if (found) {
        if (flag == 0) {
            ret = ID_ALREADY_EXIST;
        }
        else if (QR == 1 && flag == 1) {
            if (ptr_addr != NULL)
                memcpy(ptr_addr, &ptr->client_addr[i], sizeof(struct sockaddr_in));

            ret = low_id(ptr->id_pair_array[i]);
            id_remove(ptr, i);
        }
    }
    else // not found
    {
        ret = ID_NOT_FOUND;
    }

    ReleaseMutex(_id_list_mutex);

    return ret;
}



/**
 * @brief ����id_pair
 * @param id_pair Ҫ�����id_pair
 * @param ptr_addr Ҫ������׽��ֵ�ַ��Ϣ
*/
void id_insert(unsigned id_pair, struct sockaddr_in* ptr_addr)
{
    IDListNode* prev = NULL;
    IDListNode* ptr = _id_list.head;

    WaitForSingleObject(_id_list_mutex, INFINITE);

    while (ptr != NULL) {
        if (ptr->size < ID_ARRAY_SIZE) {
            ptr->id_pair_array[ptr->size] = id_pair;
            ptr->client_addr[ptr->size] = *ptr_addr;
            ptr->size++;
            break;
        }
        else {
            prev = ptr;
            ptr = ptr->next;
        }
    }

    // ����Ŀռ��Ѿ�����, �ٴ�����ռ�
    if (ptr == NULL) {
        IDListNode* temp = (IDListNode*)malloc(sizeof(IDListNode));
        if (temp == NULL)
            log_error_message("id_insert(): malloc()");
        else {
            temp->next = NULL;
            temp->size = 0;
            temp->id_pair_array[temp->size] = id_pair;
            temp->client_addr[temp->size] = *ptr_addr;
            temp->size++;
            prev->next = temp;
        }
    }

    _id_list.size++;

    ReleaseMutex(_id_list_mutex);
}