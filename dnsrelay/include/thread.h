#pragma once

/**
 * todo: 管理 windows 线程池
*/

#ifdef __cplusplus
extern "C" {
#endif

#define BUF_SIZE 1024

#define ID_ARRAY_SIZE 8

#include <WinSock2.h>


typedef struct _PER_HANDLE_DATA {
    struct sockaddr_in sock_addr;
} PER_HANDLE_DATA, * LPPER_HANDLE_DATA;

typedef struct _PER_IO_DATA {
    OVERLAPPED overlapped;
    char buffer[BUF_SIZE];
} PER_IO_DATA, * LPPER_IO_DATA;



typedef struct _IDListNode {
    struct _IDListNode *next;
    /// @brief ids 的高16位为 与外部DNS服务器之间的DNS事务
    /// @brief ids 的低16位为 与DNS客户端之间的DNS事务ID
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
 * @brief 提供接口
 * @return _com_port
*/
HANDLE get_com_port();

/**
 * @brief 提供接口
 * @return _stdout_mutex
*/
HANDLE get_stdout_mutex();

/**
 * @brief 提供接口
 * @return _relay_sock_mutex
*/
HANDLE get_relay_sock_mutex();

/**
 * @brief 提供接口
 * @return _cache_mutex
*/
HANDLE get_cache_mutex();

/**
 * @brief 创建线程, 创建mutex, 维护ID转换表
*/
void thread_init();


/**
 * @brief 创建线程的初始运行位置
 * @param lpComPort IO完成端口
 * @return 线程返回值: 0 为正常, 其他为异常
*/
unsigned WINAPI thread_main(LPVOID lpComPort);


#define ID_NOT_FOUND     -1
#define ID_ALREADY_EXIST -2

/**
 * @brief   传入QR, flag信息, 查询目标id
 * @param   target_id 要查找的id
 * @param   QR        QR=0, 查找低位; QR=1, 查找高位
 * @param   flag      flag=0, 为查重而查找, 不删除; flag=1, 找到id后, 返回其对应id, 并删除
 * @param   ptr_addr  若传入NULL不处理, 其他情况返回id对应的DNS客户套接字地址
 * @return  ALREADY_EXIST ID已经存在, ID_NOT_FOUND未找到, 成功则返回对应的id >=0 
 */
int id_search(unsigned short target_id, int QR, int flag, SOCKADDR_IN *ptr_addr);

/**
 * @brief 插入id_pair
 * @param id_pair  要插入的id_pair
 * @param ptr_addr 要插入的套接字地址信息
*/
void id_insert(unsigned id_pair, struct sockaddr_in* ptr_addr);

#define MAKE_ID_PAIR(x, y) (((x) << 16) ^ (y))

#ifdef __cplusplus
}
#endif