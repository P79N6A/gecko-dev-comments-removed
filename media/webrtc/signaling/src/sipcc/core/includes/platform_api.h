



#ifndef _PLATFORM_API_H_
#define _PLATFORM_API_H_

#include "cpr_types.h"
#include "cpr_socket.h"
#include "ccsip_pmh.h"
#include "plat_api.h"
#include "sessionTypes.h"

void     platform_get_wired_mac_address(uint8_t *addr);
void     platform_get_active_mac_address(uint8_t *addr);
void platform_get_ip_address(cpr_ip_addr_t *ip_addr);
cpr_ip_mode_e platform_get_ip_address_mode(void);
void platform_apply_config (char * configVersionStamp, char * dialplanVersionStamp, char * fcpVersionStamp, char * cucmResult, char * loadId, char * inactiveLoadId, char * loadServer, char * logServer, boolean ppid);





cpr_ip_mode_e platGetIpAddressMode();












void *
cprGetSysHeader (void *buffer);










void
cprReleaseSysHeader (void *syshdr);

#endif
