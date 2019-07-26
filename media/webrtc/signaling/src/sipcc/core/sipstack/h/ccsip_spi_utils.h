






































#ifndef _CCSIP_UTILS_H_
#define _CCSIP_UTILS_H_

#include "cpr_types.h"
#include "ccsip_pmh.h"

boolean
sipSPI_validate_hostname(char *);

boolean
sipSPI_validate_ip_addr_name(char *);

extern int
sipSPICheckDomainToken(char *token);

boolean
sipSPI_validate_hostname (char *str);
#endif
