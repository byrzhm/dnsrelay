#include "protocol.h"
#include "thread.h"
#include "socket.h"
#include "log.h"
#include <stdio.h>
#include <time.h>
#include <WinSock2.h>
#include <windows.h>
#include <process.h>

static HANDLE _com_port;         // IO完成端口
static HANDLE _stdout_mutex;     // 输出互斥锁
static HANDLE _cache_mutex;      // cache互斥锁
static HANDLE _id_list_mutex;    // ID转换表互斥锁
static HANDLE _relay_sock_mutex; // relay_sock互斥锁 ?

static IDList _id_list;          // ID转换表


/**
 * @brief 创建ID转换表
*/
static void build_id_list();


/**
 * @brief 提供接口
 * @return _com_port
*/
HANDLE get_com_port() 
{
    return _com_port;
}

/**
 * @brief 提供接口
 * @return _stdout_mutex
*/
HANDLE get_stdout_mutex()
{
    return _stdout_mutex;
}


/**
 * @brief 提供接口
 * @return _relay_sock_mutex
*/
HANDLE get_relay_sock_mutex()
{
    return _relay_sock_mutex;
}


/**
 * @brief 提供接口
 * @return _cache_mutex
*/
HANDLE get_cache_mutex()
{
    return _cache_mutex;
}


/**
 * @brief 创建IOCP, 创建线程, 创建mutex, 创建ID转换表
*/
void thread_init()
{
    int i;
    SYSTEM_INFO sys_info;

    // 创建 IO 完成端口
    _com_port = CreateIoCompletionPort(
        INVALID_HANDLE_VALUE, NULL, 0, 0
    );

    if (_com_port == NULL) 
        log_error_message("thread_init(): CreateIoCompletionPort()");
    else {
        // 创建线程
        GetSystemInfo(&sys_info);
        for (i = 0;
#ifdef _DEBUG
            i < sys_info.dwNumberOfProcessors;
#else
            i < 2 * sys_info.dwNumberOfProcessors;
#endif
            i++)
            _beginthreadex(NULL, 0, thread_main,
                         (LPVOID)_com_port, 0, NULL);
    }

    // 创建 mutex
    _stdout_mutex     = CreateMutex(NULL, FALSE, NULL);
    _cache_mutex      = CreateMutex(NULL, FALSE, NULL);
    _id_list_mutex    = CreateMutex(NULL, FALSE, NULL);
    _relay_sock_mutex = CreateMutex(NULL, FALSE, NULL);

    if (_stdout_mutex == NULL 
        || _cache_mutex == NULL
        || _id_list_mutex == NULL 
        || _relay_sock_mutex == NULL
        )
        log_error_message("thread_init(): CreateMutex()");

    // 创建ID转换表
    build_id_list();
}


/**
 * @brief 创建线程的初始运行位置
 * @param lpComPort IO完成端口
 * @return 线程返回值: 0 为正常, 其他为异常
*/
unsigned WINAPI thread_main(LPVOID lpComPort)
{
    HANDLE hComPort = (HANDLE)lpComPort;
    DWORD recv_bytes;
    LPPER_HANDLE_DATA handle_info;
    LPPER_IO_DATA io_info;
    BOOL bOk;
    // DWORD dwError;
    time_t current_time;
    PACKET_INFO packet_info;
    DNS_HEADER dns_header;


    while (1) {
        // [WARNING] 'GetQueuedCompletionStatus': different types for formal and actual parameter 4
        // 这个 warning 不用管, 利用了 &PER_IO_DATA == &(PER_IO_DATA.OVERLAPPED), 可以看 PER_IO_DATA 的结构体声明
        bOk = GetQueuedCompletionStatus(
            hComPort, &recv_bytes, (PULONG_PTR)&handle_info,
            (LPOVERLAPPED*)&io_info, INFINITE
        );
        // dwError = GetLastError();

        time(&current_time);
        packet_info.current_time = current_time;

        if (!bOk) {
            if (io_info != NULL) {
                // Process a failed completed I/O request
                // dwError contains the reason for failure
                log_error_message("thread_main(): GetQueuedCompletionStatus()");
            }
            else {
                log_error_message("thread_main(): GetQueuedCompletionStatus()");
            }
        }

        parse_packet(io_info->buffer, recv_bytes, &handle_info->sock_addr, &packet_info, &dns_header);

#ifdef _DEBUG
        // log_packet_detailed_info(io_info->buffer, recv_bytes, &packet_info, &dns_header);
#endif

        free(handle_info);
        free(io_info);
    }

    return 0;
}

/**
 * @brief 创建ID转换表
*/
static void build_id_list()
{
    IDListNode* ptr = (IDListNode *)malloc(sizeof(IDListNode));
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
 * @brief 删除id_pair
 * @param pnode 待删除id_pair的节点地址
 * @param index 待删除id_pair所在的数组索引
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
 * @brief   传入QR, flag信息, 查询目标id
 * @param   target_id 要查找的id
 * @param   QR        QR=0, 查找低位; QR=1, 查找高位
 * @param   flag      flag=0, 为查重而查找, 不删除; flag=1, 找到id后, 返回其对应id, 并删除
 * @param   ptr_addr  若传入NULL不处理, 其他情况返回id对应的DNS客户套接字地址
 * @return  ALREADY_EXIST ID已经存在, ID_NOT_FOUND未找到, 成功则返回对应的id >=0 
 */
int id_search(unsigned short target_id, int QR, int flag, SOCKADDR_IN* ptr_addr)
{
    int ret = ID_NOT_FOUND;
    int i;
    int found = 0;
    IDListNode *ptr = _id_list.head;

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
 * @brief 插入id_pair
 * @param id_pair 要插入的id_pair
 * @param ptr_addr 要插入的套接字地址信息
*/
void id_insert(unsigned id_pair, struct sockaddr_in* ptr_addr)
{
    IDListNode *prev = NULL;
    IDListNode *ptr = _id_list.head;

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

    // 申请的空间已经用完, 再次申请空间
    if (ptr == NULL) {
        IDListNode *temp = (IDListNode *)malloc(sizeof(IDListNode));
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