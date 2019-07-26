













































#ifndef _DNS_UTIL_H_
#define _DNS_UTIL_H_

#include "cc_constants.h"
#include "cpr_types.h"




#define CC_DNS_OK                 0
#define CC_DNS_ERR_NOBUF          1
#define CC_DNS_ERR_INUSE          2
#define CC_DNS_ERR_TIMEOUT        3
#define CC_DNS_ERR_NOHOST         4
#define CC_DNS_ERR_HOST_UNAVAIL   5

typedef void *srv_handle_t;











cc_int32_t dnsGetHostByName(const char *hname,
                          cpr_ip_addr_t *ipaddr_ptr,
                          cc_int32_t timeout,
                          cc_int32_t retries);




















cc_int32_t dnsGetHostBySRV(cc_int8_t *service,
                         cc_int8_t *protocol,
                         cc_int8_t *domain,
                         cpr_ip_addr_t *ipaddr_ptr,
                         cc_uint16_t *port,
                         cc_int32_t timeout,
                         cc_int32_t retries,
                         srv_handle_t *psrv_handle);








void dnsFreeSrvHandle(srv_handle_t srv_handle);

#endif 
