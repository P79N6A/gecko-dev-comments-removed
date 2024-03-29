
































static char *RCSSTRING __UNUSED__="$Id: stun_util.c,v 1.2 2008/04/28 18:21:30 ekr Exp $";

#include <errno.h>
#include <csi_platform.h>

#ifdef WIN32
#include <winsock2.h>
#include <stdlib.h>
#include <io.h>
#include <time.h>
#else   
#include <string.h>
#endif  
#include <assert.h>

#include "stun.h"
#include "stun_reg.h"
#include "registry.h"
#include "addrs.h"
#include "transport_addr_reg.h"
#include "nr_crypto.h"
#include "hex.h"


int NR_LOG_STUN = 0;

int
nr_stun_startup(void)
{
   int r,_status;

    if ((r=r_log_register("stun", &NR_LOG_STUN)))
      ABORT(r);

   _status=0;
 abort:
   return _status;
}

int
nr_stun_xor_mapped_address(UINT4 magicCookie, UINT12 transactionId, nr_transport_addr *from, nr_transport_addr *to)
{
    int _status;

    switch (from->ip_version) {
    case NR_IPV4:
        nr_ip4_port_to_transport_addr(
            (ntohl(from->u.addr4.sin_addr.s_addr) ^ magicCookie),
            (ntohs(from->u.addr4.sin_port) ^ (magicCookie>>16)),
            from->protocol, to);
        break;
    case NR_IPV6:
        {
          union {
            unsigned char addr[16];
            UINT4 addr32[4];
          } maskedAddr;

          maskedAddr.addr32[0] = htonl(magicCookie); 
          memcpy(&maskedAddr.addr32[1], transactionId.octet, sizeof(transactionId));

          
          
          for (int i = 0; i < sizeof(maskedAddr); ++i) {
            maskedAddr.addr[i] ^= from->u.addr6.sin6_addr.s6_addr[i];
          }

          nr_ip6_port_to_transport_addr(
              (struct in6_addr*)&maskedAddr,
              (ntohs(from->u.addr6.sin6_port) ^ (magicCookie>>16)),
              from->protocol, to);
        }
        break;
    default:
        assert(0);
        ABORT(R_INTERNAL);
        break;
    }

    _status = 0;
  abort:
    return _status;
}

int
nr_stun_find_local_addresses(nr_local_addr addrs[], int maxaddrs, int *count)
{
    int r,_status;
    NR_registry *children = 0;

    if ((r=NR_reg_get_child_count(NR_STUN_REG_PREF_ADDRESS_PRFX, (unsigned int*)count)))
        if (r == R_NOT_FOUND)
            *count = 0;
        else
            ABORT(r);

    if (*count == 0) {
        char allow_loopback;
        char allow_link_local;

        if ((r=NR_reg_get_char(NR_STUN_REG_PREF_ALLOW_LOOPBACK_ADDRS, &allow_loopback))) {
            if (r == R_NOT_FOUND)
                allow_loopback = 0;
            else
                ABORT(r);
        }

        if ((r=NR_reg_get_char(NR_STUN_REG_PREF_ALLOW_LINK_LOCAL_ADDRS, &allow_link_local))) {
            if (r == R_NOT_FOUND)
                allow_link_local = 0;
            else
                ABORT(r);
        }

        if ((r=nr_stun_get_addrs(addrs, maxaddrs, !allow_loopback, !allow_link_local, count)))
            ABORT(r);

        goto done;
    }

    if (*count >= maxaddrs) {
        r_log(NR_LOG_STUN, LOG_INFO, "Address list truncated from %d to %d", *count, maxaddrs);
       *count = maxaddrs;
    }

#if 0
    if (*count > 0) {
      



        children = RCALLOC((*count + 10) * sizeof(*children));
        if (!children)
            ABORT(R_NO_MEMORY);

        assert(sizeof(size_t) == sizeof(*count));

        if ((r=NR_reg_get_children(NR_STUN_REG_PREF_ADDRESS_PRFX, children, (size_t)(*count + 10), (size_t*)count)))
            ABORT(r);

        for (i = 0; i < *count; ++i) {
            if ((r=nr_reg_get_transport_addr(children[i], 0, &addrs[i].addr)))
                ABORT(r);
        }
    }
#endif

  done:

     _status=0;
 abort:
     RFREE(children);
     return _status;
}

int
nr_stun_different_transaction(UCHAR *msg, int len, nr_stun_message *req)
{
    int _status;
    nr_stun_message_header header;
    char reqid[44];
    char msgid[44];
    int len2;

    if (sizeof(header) > len)
        ABORT(R_FAILED);

    assert(sizeof(header.id) == sizeof(UINT12));

    memcpy(&header, msg, sizeof(header));

    if (memcmp(&req->header.id, &header.id, sizeof(header.id))) {
        nr_nbin2hex((UCHAR*)&req->header.id, sizeof(req->header.id), reqid, sizeof(reqid), &len2);
        nr_nbin2hex((UCHAR*)&header.id, sizeof(header.id), msgid, sizeof(msgid), &len2);
        r_log(NR_LOG_STUN, LOG_DEBUG, "Mismatched message IDs %s/%s", reqid, msgid);
        ABORT(R_NOT_FOUND);
    }

   _status=0;
 abort:
   return _status;
}

char*
nr_stun_msg_type(int type)
{
    char *ret = 0;

    switch (type) {
    case NR_STUN_MSG_BINDING_REQUEST:
         ret = "BINDING-REQUEST";
         break;
    case NR_STUN_MSG_BINDING_INDICATION:
         ret = "BINDING-INDICATION";
         break;
    case NR_STUN_MSG_BINDING_RESPONSE:
         ret = "BINDING-RESPONSE";
         break;
    case NR_STUN_MSG_BINDING_ERROR_RESPONSE:
         ret = "BINDING-ERROR-RESPONSE";
         break;

#ifdef USE_TURN
    case NR_STUN_MSG_ALLOCATE_REQUEST:
         ret = "ALLOCATE-REQUEST";
         break;
    case NR_STUN_MSG_ALLOCATE_RESPONSE:
         ret = "ALLOCATE-RESPONSE";
         break;
    case NR_STUN_MSG_ALLOCATE_ERROR_RESPONSE:
         ret = "ALLOCATE-ERROR-RESPONSE";
         break;
    case NR_STUN_MSG_REFRESH_REQUEST:
         ret = "REFRESH-REQUEST";
         break;
    case NR_STUN_MSG_REFRESH_RESPONSE:
         ret = "REFRESH-RESPONSE";
         break;
    case NR_STUN_MSG_REFRESH_ERROR_RESPONSE:
         ret = "REFRESH-ERROR-RESPONSE";
         break;
    case NR_STUN_MSG_SEND_INDICATION:
         ret = "SEND-INDICATION";
         break;
    case NR_STUN_MSG_DATA_INDICATION:
         ret = "DATA-INDICATION";
         break;
    case NR_STUN_MSG_PERMISSION_REQUEST:
         ret = "PERMISSION-REQUEST";
         break;
    case NR_STUN_MSG_PERMISSION_RESPONSE:
         ret = "PERMISSION-RESPONSE";
         break;
    case NR_STUN_MSG_PERMISSION_ERROR_RESPONSE:
         ret = "PERMISSION-ERROR-RESPONSE";
         break;
#endif 

    default:
         
         break;
    }

    return ret;
}

int
nr_random_alphanum(char *alphanum, int size)
{
    static char alphanums[256] = {
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
        'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
        'U', 'V', 'W', 'X', 'Y', 'Z',
        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
        'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
        'u', 'v', 'w', 'x', 'y', 'z',
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
        'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
        'U', 'V', 'W', 'X', 'Y', 'Z',
        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
        'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
        'u', 'v', 'w', 'x', 'y', 'z',
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
        'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
        'U', 'V', 'W', 'X', 'Y', 'Z',
        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
        'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
        'u', 'v', 'w', 'x', 'y', 'z',
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
        'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
        'U', 'V', 'W', 'X', 'Y', 'Z',
        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
        'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
        'u', 'v', 'w', 'x', 'y', 'z',
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H' };
    int i;

    nr_crypto_random_bytes((UCHAR*)alphanum, size);

    
    for (i = 0; i < size; ++i)
        alphanum[i] = alphanums[(UCHAR)alphanum[i]];

    return 0;
}
