













































#include "plstr.h"
#include "cpr_types.h"
#include "cpr_stdlib.h"
#include "cpr_string.h"
#include "pmhutils.h"
#include "ccsip_spi_utils.h"
#include "util_string.h"
#include "ccsip_core.h"

extern uint32_t IPNameCk(char *name, char *addr_error);






int
sipSPICheckDomainToken (char *token)
{
    if (token == NULL) {
        return FALSE;
    }

    if (*token) {
        if (isalnum((int)token[strlen(token) - 1]) == FALSE) {
            return FALSE;
        }
    }

    if (isalnum((int)token[0]) == FALSE) {
        return FALSE;
    }

    while (*token) {
        if ((isalnum((int)*token) == FALSE) && (*token != '-')) {
            return FALSE;
        }
        token++;
    }
    return TRUE;
}





boolean
sipSPI_validate_hostname (char *str)
{
    char *tok;
    char ip_addr_out[MAX_IPADDR_STR_LEN];
    char *strtok_state;
    
    if (str == NULL) {
        return FALSE;
    }

    
    if (cpr_inet_pton(AF_INET6, str, ip_addr_out)) {
        return TRUE;
    }

    if (*str) {
        if (isalnum((int)str[strlen(str) - 1]) == FALSE) {
            return FALSE;
        }
    }

    if (isalnum((int)str[0]) == FALSE) {
        return FALSE;
    }

    tok = PL_strtok_r(str, ".", &strtok_state);
    if (tok == NULL) {
        return FALSE;
    } else {
        if (sipSPICheckDomainToken(tok) == FALSE) {
            return FALSE;
        }
    }

    while ((tok = PL_strtok_r(NULL, ".", &strtok_state)) != NULL) {
        if (sipSPICheckDomainToken(tok) == FALSE) {
            return FALSE;
        }
    }

    return TRUE;
}






boolean
sipSPI_validate_ip_addr_name (char *str)
{
    uint32_t sip_address;
    boolean  retval = TRUE;
    char    *target = NULL;
    char     addr_error;

    if (str == NULL) {
        return FALSE;
    } else {
        target = cpr_strdup(str);
        if (!target) {
            return FALSE;
        }
    }
    sip_address = IPNameCk(target, &addr_error);
    if ((!sip_address) && (addr_error)) {
        cpr_free(target);
        return retval;
    } else {
        if (sipSPI_validate_hostname(target) == FALSE) {
            retval = FALSE;
        }
    }
    cpr_free(target);
    return retval;
}
