









#ifndef _PKIX_PL_LDAPCERTSTORE_H
#define _PKIX_PL_LDAPCERTSTORE_H

#include "pkix_pl_ldapt.h"
#include "pkix_pl_common.h"

#ifdef __cplusplus
extern "C" {
#endif



















typedef enum {
        LDAP_CONNECT_PENDING,
        LDAP_CONNECTED,
        LDAP_BIND_PENDING,
        LDAP_BIND_RESPONSE,
        LDAP_BIND_RESPONSE_PENDING,
        LDAP_BOUND,
        LDAP_SEND_PENDING,
        LDAP_RECV,
        LDAP_RECV_PENDING,
        LDAP_RECV_INITIAL,
        LDAP_RECV_NONINITIAL,
        LDAP_ABANDON_PENDING
} LDAPConnectStatus;

#define LDAP_CACHEBUCKETS       128
#define RCVBUFSIZE              512

struct PKIX_PL_LdapCertStoreContext {
        PKIX_PL_LdapClient *client;
};



PKIX_Error *pkix_pl_LdapCertStoreContext_RegisterSelf(void *plContext);

PKIX_Error *
pkix_pl_LdapCertStore_BuildCertList(
        PKIX_List *responseList,
        PKIX_List **pCerts,
        void *plContext);

#ifdef __cplusplus
}
#endif

#endif
