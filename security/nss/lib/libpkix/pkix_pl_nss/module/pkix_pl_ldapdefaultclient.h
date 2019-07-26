









#ifndef _PKIX_PL_LDAPDEFAULTCLIENT_H
#define _PKIX_PL_LDAPDEFAULTCLIENT_H

#include "pkix_pl_ldapt.h"
#include "pkix_pl_common.h"

#ifdef __cplusplus
extern "C" {
#endif















typedef enum {
        CONNECT_PENDING,
        CONNECTED,
        BIND_PENDING,
        BIND_RESPONSE,
        BIND_RESPONSE_PENDING,
        BOUND,
        SEND_PENDING,
        RECV,
        RECV_PENDING,
        RECV_INITIAL,
        RECV_NONINITIAL,
        ABANDON_PENDING
} LdapClientConnectStatus;

struct PKIX_PL_LdapDefaultClientStruct {
        PKIX_PL_LdapClient vtable;
        LdapClientConnectStatus connectStatus;
        PKIX_UInt32 messageID;
        PKIX_PL_HashTable *cachePtr;
        PKIX_PL_Socket *clientSocket;
        PRPollDesc pollDesc;
        void *callbackList; 
        LDAPBindAPI *bindAPI;
        PRArenaPool *arena;
        PRTime lastIO;
        void *sendBuf;
        PKIX_UInt32 bytesToWrite;
        void *rcvBuf;
        PKIX_UInt32 capacity;
        void *currentInPtr;
        PKIX_UInt32 currentBytesAvailable;
        void *bindMsg;
        PKIX_UInt32 bindMsgLen;
        PKIX_List *entriesFound;
        PKIX_PL_LdapRequest *currentRequest;
        PKIX_PL_LdapResponse *currentResponse;
};



PKIX_Error *pkix_pl_LdapDefaultClient_RegisterSelf(void *plContext);

#ifdef __cplusplus
}
#endif

#endif
