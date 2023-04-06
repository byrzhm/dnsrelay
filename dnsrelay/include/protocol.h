#pragma once

typedef struct _DNS_HEADER {
    unsigned short id;          // Transaction ID
    unsigned short flags;       // Flags
    unsigned short questions;   // Questions
    unsigned short answer;      // Answer RRs
    unsigned short authority;   // Authority RRs
    unsigned short additional;  // Additional RRs
} DNS_HEADER;