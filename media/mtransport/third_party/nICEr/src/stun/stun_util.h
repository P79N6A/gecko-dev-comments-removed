

































#ifndef _stun_util_h
#define _stun_util_h

#include "stun.h"
#include "local_addr.h"

extern int NR_LOG_STUN;

int nr_stun_startup(void);

int nr_stun_xor_mapped_address(UINT4 magicCookie, UINT12 transactionId, nr_transport_addr *from, nr_transport_addr *to);

int nr_stun_find_local_addresses(nr_local_addr addrs[], int maxaddrs, int *count);

int nr_stun_different_transaction(UCHAR *msg, int len, nr_stun_message *req);

char* nr_stun_msg_type(int type);

int nr_random_alphanum(char *alphanum, int size);

#endif

