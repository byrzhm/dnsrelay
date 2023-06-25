#ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#endif

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "log.h"
#include "socket.h"
#include "thread.h"
#include "cache.h"
#include "protocol.h"
#include "common.h"

#include <Windows.h>
#include <winsock2.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>


static SOCKET _relay_sock;
static struct sockaddr_in _serv_addr;
static uint32_t _serv_ip_addr;
//  "10.3.9.45"
//  "10.3.9.44"

static inline void get_header_info(
    const char* packet,
    PACKET_INFO* ptr_packet_info,
    DNS_HEADER* ptr_dns_header
);
static inline void parse_query_info(const char* packet, char *domain_name, unsigned short* type, unsigned short* class);
static inline void get_query_name(char** dptr, char* domain_name);
static inline void get_query_type(const char* ptr, unsigned short* type, unsigned short* class);
static inline void parse_response_info(const char* packet, int packet_len);
static inline void generate_packet(
    unsigned short id,
    IPListNode* ip_list,
    const char* domain_name,
    char* packet,
    int* packet_size
);
static inline void get_ans_cnt(IPListNode* ip_list, unsigned short* ptr_ans_cnt);
static inline void generate_dns_header(char** dptr, unsigned short id, unsigned short ans_cnt);
static inline void generate_answer_section(char** dptr, IPListNode* ip_list, unsigned short ans_cnt);
static inline void generate_question_section(char** dptr, const char* domain_name);
static inline void parse_query(
    const char* packet,
    int packet_len,
    struct sockaddr_in* sock_addr,
    PACKET_INFO* ptr_packet_info,
    DNS_HEADER* ptr_dns_header
);
static inline void parse_response(
    const char* packet,
    int packet_len,
    struct sockaddr_in* serv_addr,
    PACKET_INFO* ptr_packet_info,
    DNS_HEADER* ptr_dns_header
);
static inline void get_ip_in_answer(const char* ptr, int remain_len, const char* domain_name);
static inline void relay_packet(const char* packet, int send_len, SOCKADDR_IN* ptr_to_addr);




SOCKET get_relay_sock() {
    return _relay_sock;
}

void sock_set_serv_ip(const char* serv_ip){
    _serv_ip_addr = inet_addr(serv_ip);
}

void sock_init()
{
    struct sockaddr_in relay_addr;

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        log_error_message("sock_init(): WSAStartup()");
#endif

    _relay_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (_relay_sock == INVALID_SOCKET)
        log_error_message("sock_init(): socket()");
    else
    {
        memset(&relay_addr, 0, sizeof(relay_addr));
        relay_addr.sin_family = AF_INET;
        relay_addr.sin_port = htons(53); // 53号端口 为 DNS 端口
        relay_addr.sin_addr.s_addr = INADDR_ANY;

        if (bind(_relay_sock, (struct sockaddr*)&relay_addr, sizeof(relay_addr))
                 == INVALID_SOCKET)
            log_error_message("sock_init(): bind()");

        printf("Bind UDP port 53 ...OK\n");
    }

    _serv_addr.sin_family = AF_INET;
    _serv_addr.sin_port = htons(53);
    _serv_addr.sin_addr.s_addr = _serv_ip_addr;
}


/**
 * @brief 调用recvfrom接受发来的DNS报文,
 *        调用PostQueuedCompletionStatus()唤醒线程处理DNS报文
 * @param handle_ptr 已经分配好空间的指向PER_HANDLE_DATA的指针
 * @param io_ptr     已经分配好空间的指向PER_IO_DATA的指针
 * ! [注意] io_ptr->overlapped 在传入前必须置零
 * @param recv_sock  接受报文的套接字信息
*/
void recv_packet(void* handle_ptr, void* io_ptr, SOCKET recv_sock) 
{
	int recv_bytes;
	int addr_size = sizeof(SOCKADDR_IN);
    LPPER_HANDLE_DATA handle_info = (LPPER_HANDLE_DATA)handle_ptr;
    LPPER_IO_DATA io_info = (LPPER_IO_DATA)io_ptr;

	memset(&io_info->overlapped, 0, sizeof(OVERLAPPED));

    if (recv_sock == _relay_sock) {
        // 接受来自DNS客户端的请求报文
        recv_bytes = recvfrom(recv_sock, io_info->buffer,
            BUF_SIZE, 0, (SOCKADDR*)&handle_info->sock_addr, &addr_size);
    }
    else {
        recv_bytes = recv(recv_sock, io_info->buffer, BUF_SIZE, 0);
        handle_info->sock_addr = _serv_addr;
    }

    if (recv_bytes != SOCKET_ERROR) {
		PostQueuedCompletionStatus(
			get_com_port(), recv_bytes,
			(ULONG_PTR)handle_info, (LPOVERLAPPED)&io_info->overlapped
		);
	}
}


/**
 * @brief 分析发来的DNS报文
 * @param  packet            接受的DNS报文
 * @param  packet_len        接受的DNS报文字节数
 * @param  sock_addr         报文发送方的套接字地址
 * @param  ptr_packet_info   指向报文信息的指针    [指针返回]
 * @param  ptr_dns_header    指向DNS头部信息的指针 [指针返回]
 */
void parse_packet(
    const char* packet,
    int packet_len,
    struct sockaddr_in* sock_addr,
    PACKET_INFO* ptr_packet_info,
    DNS_HEADER* ptr_dns_header
)
{
    int QR;

    ptr_packet_info->ip_addr = sock_addr->sin_addr.s_addr;
    get_header_info(packet, ptr_packet_info, ptr_dns_header);

    QR = (ptr_dns_header->flags & 0x8000) >> 15;
    if (QR == 0) {
        // query from DNS client
        parse_query(packet, packet_len, sock_addr, ptr_packet_info, ptr_dns_header);
    }
    else {
        // response from foreign DNS server
        parse_response(packet, packet_len, sock_addr, ptr_packet_info, ptr_dns_header);
    }

}

/**
 * @brief 转发DNS报文
 * @param packet      指向要发送的数据包的指针
 * @param send_len    要发送的字节数
 * @param ptr_to_addr 要发送的ip地址
*/
static inline void relay_packet(const char* packet, int send_len, SOCKADDR_IN* ptr_to_addr)
{
    LPPER_HANDLE_DATA handle_info;
    LPPER_IO_DATA io_info;
    SOCKET temp_sock;

    // 向外部DNS服务器发送DNS查询请求
    if (ptr_to_addr == &_serv_addr) {
        temp_sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (temp_sock == INVALID_SOCKET)
            log_error_message("relay_packet(): socket()");

        if (connect(temp_sock, (SOCKADDR*)ptr_to_addr, sizeof(SOCKADDR)) == SOCKET_ERROR)
            log_error_message("relay_packet(): connect()");

        send(temp_sock, packet, send_len, 0);

        handle_info = (LPPER_HANDLE_DATA)Malloc(sizeof(PER_HANDLE_DATA));
        io_info = (LPPER_IO_DATA)Malloc(sizeof(PER_IO_DATA));

        // 接受DNS报文
        recv_packet((void*)handle_info, (void*)io_info, temp_sock);
        closesocket(temp_sock);
    }
    else { // 将外部DNS服务器的回复转发给内部的DNS客户端
        sendto(_relay_sock, packet, send_len, 0,
            (SOCKADDR*)ptr_to_addr, sizeof(SOCKADDR_IN));
    }
}

/**
 * @brief 分析请求报文
 * @param  packet            接受的DNS报文
 * @param  packet_len        接受的DNS报文字节数
 * @param  sock_addr         报文发送方的信息
 * @param  ptr_packet_info   指向报文信息的指针    [指针返回]
 * @param  ptr_dns_header    指向DNS头部信息的指针
 */
static inline void parse_query(
    const char* packet,
    int packet_len,
    struct sockaddr_in* sock_addr,
    PACKET_INFO* ptr_packet_info,
    DNS_HEADER* ptr_dns_header
)
{
    char* packet_to_send;
    unsigned rand_id;
    unsigned short_max;
    unsigned short* ptr_id;
    IPListNode* ip_list = NULL;

    parse_query_info(packet, ptr_packet_info->domain_name, &ptr_packet_info->type, &ptr_packet_info->class);
    log_received_query_packet(packet, packet_len, sock_addr, ptr_packet_info, ptr_dns_header);

    // 在cache中搜索, 对不良网站进行拦截
    cache_search(ptr_packet_info->domain_name, &ip_list);

    
    if (ip_list != NULL 
        && ( ip_list->ip_addr == 0x0 // 查询结果为不良网站
        ||   (ptr_packet_info->type == 0x01 && ptr_packet_info->class == 0x01) // 查询IPv4
            )
        )
    {
        // 直接根据 cache 中信息生成相应的响应发送给DNS客户端
        int generate_size;
        packet_to_send = (char*)Malloc(sizeof(char) * MAX_DNS_PKT_SIZE);

        generate_packet(ptr_dns_header->id, ip_list, ptr_packet_info->domain_name,
            packet_to_send, &generate_size);
        sendto(_relay_sock, packet_to_send, generate_size, 0,
            (struct sockaddr*)sock_addr, sizeof(struct sockaddr_in));
        free(packet_to_send);
        log_packet_sent(sock_addr, generate_size,
            ptr_dns_header->id, ptr_dns_header->id);
        return;
    }

    // cache中没有域名信息进行中继转发
    short_max = 0xffff + 0x1;
    if (id_search(ptr_dns_header->id, 0, 0, NULL) != ID_ALREADY_EXIST)
    {
        while (1) {
            rand_id = rand() % short_max;
            if (id_search(rand_id, 1, 0, NULL) != ID_ALREADY_EXIST)
                break;
        }

        id_insert(MAKE_ID_PAIR(rand_id, ptr_dns_header->id), sock_addr);
        packet_to_send = (char*)Malloc(sizeof(char) * packet_len);
        ptr_id = (unsigned short*)packet_to_send;
        memcpy(packet_to_send, packet, packet_len);
        *ptr_id = htons(rand_id);
        log_packet_sent(&_serv_addr, packet_len, ptr_dns_header->id, rand_id);
        relay_packet(packet_to_send, packet_len, &_serv_addr);
        free(packet_to_send);
    }
}

/**
 * @brief 分析回应报文
 * @param  packet            接受的DNS报文
 * @param  packet_len        接受的DNS报文字节数
 * @param  serv_addr         外部DNS服务器的套接字地址
 * @param  ptr_packet_info   指向报文信息的指针
 * @param  ptr_dns_header    指向DNS头部信息的指针
 */
static inline void parse_response(
    const char* packet,
    int packet_len,
    struct sockaddr_in* serv_addr,
    PACKET_INFO* ptr_packet_info,
    DNS_HEADER* ptr_dns_header
)
{
    char* packet_to_send;
    unsigned short* ptr_id;
    struct sockaddr_in client_addr;
    int id;

    id = id_search(ptr_dns_header->id, 1, 1, &client_addr);
    if (id != ID_NOT_FOUND)
    {
        packet_to_send = (char*)Malloc(sizeof(char) * packet_len);
        ptr_id = (unsigned short*)packet_to_send;
        memcpy(packet_to_send, packet, packet_len);
        *ptr_id = htons(id & 0xffff);

        // 下面的 relay_packet() 直接发送 packet_to_send 到DNS客户端
        relay_packet(packet_to_send, packet_len, &client_addr);
        free(packet_to_send);

        log_received_response_packet(packet, packet_len, &client_addr, serv_addr, ptr_dns_header, id);
        parse_response_info(packet, packet_len);
    }
}

/**
* @brief 生成DNS报文
* @param id          报文id
* @param ip_list     ip链表
* @param domain_name 查找的域名
* @param packet      报文字符串 [指针返回]
* @param packet_size 报文的长度 [指针返回]
*/
static inline void generate_packet(
    unsigned short id,
    IPListNode* ip_list,
    const char* domain_name,
    char* packet,
    int* packet_size
)
{
    unsigned short ans_cnt;
    char* ptr = packet;
    
    get_ans_cnt(ip_list, &ans_cnt);

    generate_dns_header(&ptr, id, ans_cnt);

    generate_question_section(&ptr, domain_name);

    generate_answer_section(&ptr, ip_list, ans_cnt);

    *packet_size = (int)(ptr - packet);
}

/**
* @brief 一个域名可能对应多个IP，函数返回域名对应的IP数目
* @param ip_list      IP链表
* @param ptr_ans_cnt  answer的数量，即有效的IP的数目 [指针返回]
*/
static inline void get_ans_cnt(IPListNode* ip_list, unsigned short* ptr_ans_cnt)
{
    IPListNode* pnode = ip_list;
    *ptr_ans_cnt = 0;

    if (pnode->ip_addr != 0x0)
    {
        while (pnode != NULL) {
            (*ptr_ans_cnt)++;
            pnode = pnode->next;
        }
    }
}

/**
* @brief 生成DNS报头
* @param dptr     指向报文的二级指针
* @param id       事务ID
* @param ans_cnt  answer_count
*/
static inline void generate_dns_header(char** dptr, unsigned short id, unsigned short ans_cnt)
{
    // id
    *(unsigned short*)(*dptr) = htons(id);    *dptr += 2;

    // flags
    if (!ans_cnt)
        *(unsigned short*)(*dptr) = htons(0x8183); // No such name.
    else
        *(unsigned short*)(*dptr) = htons(0x8180);
    *dptr += 2;

    // question_count
    *(unsigned short*)(*dptr) = htons(0x1);    *dptr += 2;

    // answer_count
    *(unsigned short*)(*dptr) = htons(ans_cnt);    *dptr += 2;

    // authority count
    *(unsigned short*)(*dptr) = htons(0x0);    *dptr += 2;
    // additional count
    *(unsigned short*)(*dptr) = htons(0x0);    *dptr += 2;
}

/**
* @brief 生成DNS报文的question部分
* @param dptr     指向报文的二级指针
* @domain_name    域名
*/
static inline void generate_question_section(char **dptr, const char* domain_name)
{
    char* str;
    char temp_name[MAX_DOMAIN_SIZE];
    strcpy(temp_name, domain_name);

    // domain name
    str = strtok(temp_name, ".");
    while (str) {
        unsigned char str_len = strlen(str) & 0xff;
        *(unsigned char*)(*dptr)++ = str_len;

        strcpy(*dptr, str);       *dptr += str_len;

        str = strtok(NULL, ".");
    }
    *(unsigned char*)(*dptr)++ = 0x00;

    // type
    *(unsigned short*)(*dptr) = htons(0x1);     *dptr += 2;
    // class
    *(unsigned short*)(*dptr) = htons(0x1);     *dptr += 2;
}

/**
* @brief 生成DNS报文的answer部分
* @param dptr         指向报文的二级指针
* @param ip_list      IP链表
* @param ans_cnt      有效IP数目
*/
static inline void generate_answer_section(char **dptr, IPListNode* ip_list, unsigned short ans_cnt)
{
    int i;
    IPListNode* pnode = ip_list;
    for (i = 0; i < ans_cnt; i++) {
        // name : 使用Compression Label
        *(unsigned short*)(*dptr) = htons(0xc00c);  *dptr += 2;
        // type : A = 1
        *(unsigned short*)(*dptr) = htons(0x1);     *dptr += 2;
        // class: IN = 1
        *(unsigned short*)(*dptr) = htons(0x1);     *dptr += 2;
        // time to live (4 octets): 生存期的秒数, 这里设置成 192 秒
        *(unsigned int*)(*dptr) = htons(0xc0);      *dptr += 4;
        // data length (2 octets): 4 为 IPv4, 16 为 IPv6
        *(unsigned short*)(*dptr) = htons(0x4);     *dptr += 2;
        // IPv4 Address
        *(unsigned int*)(*dptr) = pnode->ip_addr;  *dptr += 4; // ? htons
        pnode = pnode->next;
    }
}

/**
 * @brief 由DNS报文获得要查找的域名信息
 * @param packet       DNS报文
 * @param domain_name  域名信息             [指针返回]
 * @param type         type of the query    [指针返回]
 * @param class        class of the query   [指针返回]
*/
static inline void parse_query_info(const char* packet, char* domain_name, unsigned short* type, unsigned short* class)
{
    const char* ptr = packet + sizeof(DNS_HEADER);

    get_query_name((char **) & ptr, domain_name);
    get_query_type(ptr, type, class);
}

/**
* @brief 由DNS报文获得要查找的信息
* @param dptr        指向 packet 的二重指针
* @param domain_name 域名                 [指针返回]
*/
static inline void get_query_name(char** dptr, char* domain_name)
{
    // Question(Query) Section Format :
    // |-----------------------|
    // | Query Name (variable) | <==
    // |-----------------------|
    // | Query Type (16 bits)  | 
    // |-----------------------|
    // | Query Class (16 bits) |
    // |-----------------------|

    int i;
    int cursor;
    int size;
    cursor = 0;
    size = *((*dptr)++);
    while (size != 0) {
        for (i = 0; i < size; i++)
            domain_name[cursor++] = *((*dptr)++);
        domain_name[cursor++] = '.';
        size = *((*dptr)++);
    }
    domain_name[cursor - 1] = '\0';
}

/**
* @brief 由DNS报文获得要查找的类型
* @param ptr          指向DNS请求报文的qtype, qclass的指针
* @param type         type of the query   [指针返回]
* @param class        class of the query  [指针返回]
*/
static inline void get_query_type(const char* ptr, unsigned short* type, unsigned short* class)
{
    // Question(Query) Section Format :
    // |-----------------------|
    // | Query Name (variable) |
    // |-----------------------|
    // | Query Type (16 bits)  | <==
    // |-----------------------|
    // | Query Class (16 bits) | <==
    // |-----------------------|

    *type = ntohs(*(unsigned short*)ptr);
    ptr += sizeof(unsigned short);
    *class = ntohs(*(unsigned short*)ptr);
    ptr += sizeof(unsigned short);
}

/** 
* @brief 解析DNS响应报文中的信息
* @param packet       DNS报文
* @param packet_len   DNS报文长度
*/
static inline void parse_response_info(const char* packet, int packet_len)
{
    const char* ptr = packet;
    char* name = (char*)Malloc(sizeof(char) * MAX_DOMAIN_SIZE);
    // DNS Fixed Header : 12 bytes
    ptr += 12;

    // Question(Query) Section Format
    get_query_name((char**)&ptr, name);
    ptr += sizeof(unsigned short) * 2;

    // Answer Section Format :
    get_ip_in_answer(ptr, packet_len - (int)(ptr - packet), name);
    free(name);
}

/**
* @brief 从answer section中获取IP信息, 并且插入到Cache中
*/
static inline void get_ip_in_answer(const char* ptr, int remain_len, const char* domain_name)
{
    // Answer Section Format :
    // |-----------------------|
    // |    Name (variable)    |
    // |-----------------------|
    // |    Type (16 bits)     |
    // |-----------------------|
    // |    Class (16 bits)    |
    // |-----------------------|
    // |         TTL           |
    // |      (32 bits)        |
    // |-----------------------|
    // |    RDLENGTH (16 bits) |
    // |-----------------------|
    // |    RDATA (variable)   |
    // |-----------------------|
    unsigned short type;
    unsigned short class;
    unsigned short rd_len;
    uint32_t ip_addr;
    int i = 0;

    while (i < remain_len) {
        // Name field
        if ((ptr[i] & 0xc0) == 0xc0) { // compression label
            i += 2;
        }
        else { // data label
            while (ptr[i] != 0) i++;
            i++;
        }

        // type & class
        type = ntohs(*((unsigned short*)(&ptr[i])));
        class = ntohs(*((unsigned short*)(&ptr[i + 2])));
        i += 4;

        // ttl not used
        i += 4;

        // rd_len
        rd_len = ntohs(*((unsigned short*)(&ptr[i])));
        i += 2;

        // rdata
        if (type == 0x0001 && class == 0x0001) {
            // rdata 为IPv4地址
            ip_addr = *((uint32_t*)(&ptr[i]));
            cache_insert(domain_name, ip_addr);
        }
        i += rd_len;
    }
    
}

/**
 * @brief 分析发来的DNS报文, 获取DNS报文头部信息
 * @param  packet            接受的DNS报文
 * @param  packet_len        接受的DNS报文字节数
 * @param  ptr_packet_info   指向报文信息的指针    [指针返回]
 * @param  ptr_dns_header    指向DNS头部信息的指针 [指针返回]
 */
static inline void get_header_info(
    const char* packet,
    PACKET_INFO* ptr_packet_info,
    DNS_HEADER* ptr_dns_header
)
{
    const unsigned short* cursor = (const unsigned short*)packet;

    // Transaction ID: 2 octets
    ptr_dns_header->id = ntohs(*cursor);
    cursor++;

    // Flags: 2 octets
    ptr_dns_header->flags = ntohs(*cursor);
    cursor++;

    // Query Count/Zone Count: 2 octets
    ptr_dns_header->question_count = ntohs(*cursor);
    cursor++;

    // Answer Count/Prerequisite Count
    ptr_dns_header->answer_count = ntohs(*cursor);
    cursor++;

    // Authority Record Count/Update Count
    ptr_dns_header->authority_count = ntohs(*cursor);
    cursor++;

    // Additional Infomation Count
    ptr_dns_header->additional_count = ntohs(*cursor);
    cursor++;
}