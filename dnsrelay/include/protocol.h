#pragma once

#ifdef _WIN32
typedef unsigned __int32 uint32_t;
#endif

#ifdef _DEBUG
#include <assert.h>
#endif

#include <WinSock2.h>
#include <Windows.h>
#include <time.h>

// 域名的最大长度 255 + 1 '\0'
#define MAX_DOMAIN_SIZE 256

typedef struct _PACKET_INFO {
    char domain_name[MAX_DOMAIN_SIZE];
    time_t current_time;
#ifdef _WIN32
    UINT32 ip_addr;
#else
    uint32_t ip_addr;
#endif
    unsigned short id;
} PACKET_INFO;


typedef struct _DNS_HEADER {
    unsigned short id;                // Transaction ID
    unsigned short flags;             // Flags
    unsigned short question_count;    // Questions
    unsigned short answer_count;      // Answer RRs
    unsigned short authority_count;   // Authority RRs
    unsigned short additional_count;  // Additional RRs
} DNS_HEADER;

// sizeof(DNS_HEADER) == 12