










#define MINIMUM_MSG_LENGTH 5

#include "pkix_pl_ldapdefaultclient.h"











































static PKIX_Error *
pkix_pl_LdapDefaultClient_MakeBind(
        PRArenaPool *arena,
        PKIX_Int32 versionData,
        LDAPBindAPI *bindAPI,
        PKIX_UInt32 msgNum,
        SECItem **pBindMsg,
        void *plContext)
{
        LDAPMessage msg;
        char version = '\0';
        SECItem *encoded = NULL;
        PKIX_UInt32 len = 0;

        PKIX_ENTER(LDAPDEFAULTCLIENT, "pkix_pl_LdapDefaultClient_MakeBind");
        PKIX_NULLCHECK_TWO(arena, pBindMsg);

        PKIX_PL_NSSCALL(LDAPDEFAULTCLIENT, PORT_Memset,
                (&msg, 0, sizeof (LDAPMessage)));

        version = (char)versionData;

        msg.messageID.type = siUnsignedInteger;
        msg.messageID.data = (void*)&msgNum;
        msg.messageID.len = sizeof (msgNum);

        msg.protocolOp.selector = LDAP_BIND_TYPE;

        msg.protocolOp.op.bindMsg.version.type = siUnsignedInteger;
        msg.protocolOp.op.bindMsg.version.data = (void *)&version;
        msg.protocolOp.op.bindMsg.version.len = sizeof (char);

        





        if (bindAPI->selector == SIMPLE_AUTH) {
                msg.protocolOp.op.bindMsg.bindName.type = siAsciiString;
                msg.protocolOp.op.bindMsg.bindName.data =
                        (void *)bindAPI->chooser.simple.bindName;
                len = PL_strlen(bindAPI->chooser.simple.bindName);
                msg.protocolOp.op.bindMsg.bindName.len = len;

                msg.protocolOp.op.bindMsg.authentication.type = siAsciiString;
                msg.protocolOp.op.bindMsg.authentication.data =
                        (void *)bindAPI->chooser.simple.authentication;
                len = PL_strlen(bindAPI->chooser.simple.authentication);
                msg.protocolOp.op.bindMsg.authentication.len = len;
        }

        PKIX_PL_NSSCALLRV(LDAPDEFAULTCLIENT, encoded, SEC_ASN1EncodeItem,
                (arena, NULL, (void *)&msg, PKIX_PL_LDAPMessageTemplate));
        if (!encoded) {
                PKIX_ERROR(PKIX_SECASN1ENCODEITEMFAILED);
        }

        *pBindMsg = encoded;
cleanup:

        PKIX_RETURN(LDAPDEFAULTCLIENT);
}


































static PKIX_Error *
pkix_pl_LdapDefaultClient_MakeUnbind(
        PRArenaPool *arena,
        PKIX_UInt32 msgNum,
        SECItem **pUnbindMsg,
        void *plContext)
{
        LDAPMessage msg;
        SECItem *encoded = NULL;

        PKIX_ENTER(LDAPDEFAULTCLIENT, "pkix_pl_LdapDefaultClient_MakeUnbind");
        PKIX_NULLCHECK_TWO(arena, pUnbindMsg);

        PKIX_PL_NSSCALL(LDAPDEFAULTCLIENT, PORT_Memset,
                (&msg, 0, sizeof (LDAPMessage)));

        msg.messageID.type = siUnsignedInteger;
        msg.messageID.data = (void*)&msgNum;
        msg.messageID.len = sizeof (msgNum);

        msg.protocolOp.selector = LDAP_UNBIND_TYPE;

        msg.protocolOp.op.unbindMsg.dummy.type = siBuffer;
        msg.protocolOp.op.unbindMsg.dummy.data = NULL;
        msg.protocolOp.op.unbindMsg.dummy.len = 0;

        PKIX_PL_NSSCALLRV(LDAPDEFAULTCLIENT, encoded, SEC_ASN1EncodeItem,
                (arena, NULL, (void *)&msg, PKIX_PL_LDAPMessageTemplate));
        if (!encoded) {
                PKIX_ERROR(PKIX_SECASN1ENCODEITEMFAILED);
        }

        *pUnbindMsg = encoded;
cleanup:

        PKIX_RETURN(LDAPDEFAULTCLIENT);
}






























static PKIX_Error *
pkix_pl_LdapDefaultClient_MakeAbandon(
        PRArenaPool *arena,
        PKIX_UInt32 msgNum,
        SECItem **pAbandonMsg,
        void *plContext)
{
        LDAPMessage msg;
        SECItem *encoded = NULL;

        PKIX_ENTER(LDAPDEFAULTCLIENT, "pkix_pl_LdapDefaultClient_MakeAbandon");
        PKIX_NULLCHECK_TWO(arena, pAbandonMsg);

        PKIX_PL_NSSCALL(LDAPDEFAULTCLIENT, PORT_Memset,
                (&msg, 0, sizeof (LDAPMessage)));

        msg.messageID.type = siUnsignedInteger;
        msg.messageID.data = (void*)&msgNum;
        msg.messageID.len = sizeof (msgNum);

        msg.protocolOp.selector = LDAP_ABANDONREQUEST_TYPE;

        msg.protocolOp.op.abandonRequestMsg.messageID.type = siBuffer;
        msg.protocolOp.op.abandonRequestMsg.messageID.data = (void*)&msgNum;
        msg.protocolOp.op.abandonRequestMsg.messageID.len = sizeof (msgNum);

        PKIX_PL_NSSCALLRV(LDAPDEFAULTCLIENT, encoded, SEC_ASN1EncodeItem,
                (arena, NULL, (void *)&msg, PKIX_PL_LDAPMessageTemplate));
        if (!encoded) {
                PKIX_ERROR(PKIX_SECASN1ENCODEITEMFAILED);
        }

        *pAbandonMsg = encoded;
cleanup:

        PKIX_RETURN(LDAPDEFAULTCLIENT);
}































static PKIX_Error *
pkix_pl_LdapDefaultClient_DecodeBindResponse(
        PRArenaPool *arena,
        SECItem *src,
        LDAPMessage *pBindResponse,
        SECStatus *pStatus,
        void *plContext)
{
        SECStatus rv = SECFailure;
        LDAPMessage response;

        PKIX_ENTER
                (LDAPDEFAULTCLIENT,
                "pkix_pl_LdapDefaultClient_DecodeBindResponse");
        PKIX_NULLCHECK_FOUR(arena, src, pBindResponse, pStatus);

        PKIX_PL_NSSCALL
                (LDAPDEFAULTCLIENT,
                PORT_Memset,
                (&response, 0, sizeof (LDAPMessage)));

        PKIX_PL_NSSCALLRV(LDAPDEFAULTCLIENT, rv, SEC_ASN1DecodeItem,
            (arena, &response, PKIX_PL_LDAPMessageTemplate, src));

        if (rv == SECSuccess) {
                *pBindResponse = response;
        }

        *pStatus = rv;

        PKIX_RETURN(LDAPDEFAULTCLIENT);
}
























static PKIX_Error *
pkix_pl_LdapDefaultClient_VerifyBindResponse(
        PKIX_PL_LdapDefaultClient *client,
        PKIX_UInt32 bufLen,
        void *plContext)
{
        SECItem decode = {siBuffer, NULL, 0};
        SECStatus rv = SECFailure;
        LDAPMessage msg;
        LDAPBindResponse *ldapBindResponse = NULL;

        PKIX_ENTER
                (LDAPDEFAULTCLIENT,
                "pkix_pl_LdapDefaultClient_VerifyBindResponse");
        PKIX_NULLCHECK_TWO(client, client->rcvBuf);

        decode.data = (void *)(client->rcvBuf);
        decode.len = bufLen;

        PKIX_CHECK(pkix_pl_LdapDefaultClient_DecodeBindResponse
                (client->arena, &decode, &msg, &rv, plContext),
                PKIX_LDAPDEFAULTCLIENTDECODEBINDRESPONSEFAILED);

        if (rv == SECSuccess) {
                ldapBindResponse = &msg.protocolOp.op.bindResponseMsg;
                if (*(ldapBindResponse->resultCode.data) == SUCCESS) {
                        client->connectStatus = BOUND;
                } else {
                        PKIX_ERROR(PKIX_BINDREJECTEDBYSERVER);
                }
        } else {
                PKIX_ERROR(PKIX_CANTDECODEBINDRESPONSEFROMSERVER);
        }

cleanup:

        PKIX_RETURN(LDAPDEFAULTCLIENT);
}

































static PKIX_Error *
pkix_pl_LdapDefaultClient_RecvCheckComplete(
        PKIX_PL_LdapDefaultClient *client,
        PKIX_UInt32 bytesProcessed,
        PKIX_Boolean *pKeepGoing,
        void *plContext)
{
        PKIX_Boolean complete = PKIX_FALSE;
        SECStatus rv = SECFailure;
        LDAPMessageType messageType = 0;
        LDAPResultCode resultCode = 0;

        PKIX_ENTER
                (LDAPDEFAULTCLIENT,
                "pkix_pl_LdapDefaultClient_RecvCheckComplete");
        PKIX_NULLCHECK_TWO(client, pKeepGoing);

        PKIX_CHECK(pkix_pl_LdapResponse_IsComplete
                (client->currentResponse, &complete, plContext),
                PKIX_LDAPRESPONSEISCOMPLETEFAILED);

        if (complete) {
                PKIX_CHECK(pkix_pl_LdapResponse_Decode
                       (client->arena, client->currentResponse, &rv, plContext),
                        PKIX_LDAPRESPONSEDECODEFAILED);

                if (rv != SECSuccess) {
                        PKIX_ERROR(PKIX_CANTDECODESEARCHRESPONSEFROMSERVER);
                }

                PKIX_CHECK(pkix_pl_LdapResponse_GetMessageType
                        (client->currentResponse, &messageType, plContext),
                        PKIX_LDAPRESPONSEGETMESSAGETYPEFAILED);

                if (messageType == LDAP_SEARCHRESPONSEENTRY_TYPE) {

                        if (client->entriesFound == NULL) {
                                PKIX_CHECK(PKIX_List_Create
                                    (&(client->entriesFound), plContext),
                                    PKIX_LISTCREATEFAILED);
                        }

                        PKIX_CHECK(PKIX_List_AppendItem
                                (client->entriesFound,
                                (PKIX_PL_Object *)client->currentResponse,
                                plContext),
                                PKIX_LISTAPPENDITEMFAILED);

                        PKIX_DECREF(client->currentResponse);

                        
                        if (client->currentBytesAvailable == 0) {
                                client->connectStatus = RECV;
                                *pKeepGoing = PKIX_TRUE;
                        } else {
                                client->connectStatus = RECV_INITIAL;
                                client->currentInPtr = &((char *)
                                    (client->currentInPtr))[bytesProcessed];
                                *pKeepGoing = PKIX_TRUE;
                        }

                } else if (messageType == LDAP_SEARCHRESPONSERESULT_TYPE) {
                        PKIX_CHECK(pkix_pl_LdapResponse_GetResultCode
                                (client->currentResponse,
                                &resultCode,
                                plContext),
                                PKIX_LDAPRESPONSEGETRESULTCODEFAILED);

                        if ((client->entriesFound == NULL) &&
                            ((resultCode == SUCCESS) ||
                            (resultCode == NOSUCHOBJECT))) {
                                PKIX_CHECK(PKIX_List_Create
                                    (&(client->entriesFound), plContext),
                                    PKIX_LISTCREATEFAILED);
                        } else if (resultCode == SUCCESS) {
                                PKIX_CHECK(PKIX_List_SetImmutable
                                    (client->entriesFound, plContext),
                                    PKIX_LISTSETIMMUTABLEFAILED);
                                PKIX_CHECK(PKIX_PL_HashTable_Add
                                    (client->cachePtr,
                                    (PKIX_PL_Object *)client->currentRequest,
                                    (PKIX_PL_Object *)client->entriesFound,
                                    plContext),
                                    PKIX_HASHTABLEADDFAILED);
                        } else {
                            PKIX_ERROR(PKIX_UNEXPECTEDRESULTCODEINRESPONSE);
                        }

                        client->connectStatus = BOUND;
                        *pKeepGoing = PKIX_FALSE;
                        PKIX_DECREF(client->currentResponse);

                } else {
                        PKIX_ERROR(PKIX_SEARCHRESPONSEPACKETOFUNKNOWNTYPE);
                }
        } else {
                client->connectStatus = RECV;
                *pKeepGoing = PKIX_TRUE;
        }

cleanup:
        PKIX_RETURN(LDAPDEFAULTCLIENT);
}



static PKIX_Error *
pkix_pl_LdapDefaultClient_InitiateRequest(
        PKIX_PL_LdapClient *client,
        LDAPRequestParams *requestParams,
        void **pPollDesc,
        PKIX_List **pResponse,
        void *plContext);

static PKIX_Error *
pkix_pl_LdapDefaultClient_ResumeRequest(
        PKIX_PL_LdapClient *client,
        void **pPollDesc,
        PKIX_List **pResponse,
        void *plContext);































PKIX_Error *
pkix_pl_LdapDefaultClient_CreateHelper(
        PKIX_PL_Socket *socket,
        LDAPBindAPI *bindAPI,
        PKIX_PL_LdapDefaultClient **pClient,
        void *plContext)
{
        PKIX_PL_HashTable *ht;
        PKIX_PL_LdapDefaultClient *ldapDefaultClient = NULL;
        PKIX_PL_Socket_Callback *callbackList;
        PRFileDesc *fileDesc = NULL;
        PRArenaPool *arena = NULL;

        PKIX_ENTER(LDAPDEFAULTCLIENT, "pkix_pl_LdapDefaultClient_CreateHelper");
        PKIX_NULLCHECK_TWO(socket, pClient);

        PKIX_CHECK(PKIX_PL_Object_Alloc
                    (PKIX_LDAPDEFAULTCLIENT_TYPE,
                    sizeof (PKIX_PL_LdapDefaultClient),
                    (PKIX_PL_Object **)&ldapDefaultClient,
                    plContext),
                    PKIX_COULDNOTCREATELDAPDEFAULTCLIENTOBJECT);

        ldapDefaultClient->vtable.initiateFcn =
                pkix_pl_LdapDefaultClient_InitiateRequest;
        ldapDefaultClient->vtable.resumeFcn =
                pkix_pl_LdapDefaultClient_ResumeRequest;

        PKIX_CHECK(pkix_pl_Socket_GetPRFileDesc
                (socket, &fileDesc, plContext),
                PKIX_SOCKETGETPRFILEDESCFAILED);

        ldapDefaultClient->pollDesc.fd = fileDesc;
        ldapDefaultClient->pollDesc.in_flags = 0;
        ldapDefaultClient->pollDesc.out_flags = 0;

        ldapDefaultClient->bindAPI = bindAPI;

        PKIX_CHECK(PKIX_PL_HashTable_Create
                (LDAP_CACHEBUCKETS, 0, &ht, plContext),
                PKIX_HASHTABLECREATEFAILED);

        ldapDefaultClient->cachePtr = ht;

        PKIX_CHECK(pkix_pl_Socket_GetCallbackList
                (socket, &callbackList, plContext),
                PKIX_SOCKETGETCALLBACKLISTFAILED);

        ldapDefaultClient->callbackList = callbackList;

        PKIX_INCREF(socket);
        ldapDefaultClient->clientSocket = socket;

        ldapDefaultClient->messageID = 0;

        ldapDefaultClient->bindAPI = bindAPI;

        arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
        if (!arena) {
            PKIX_ERROR_FATAL(PKIX_OUTOFMEMORY);
        }
        ldapDefaultClient->arena = arena;

        ldapDefaultClient->sendBuf = NULL;
        ldapDefaultClient->bytesToWrite = 0;

        PKIX_CHECK(PKIX_PL_Malloc
                (RCVBUFSIZE, &ldapDefaultClient->rcvBuf, plContext),
                PKIX_MALLOCFAILED);
        ldapDefaultClient->capacity = RCVBUFSIZE;

        ldapDefaultClient->bindMsg = NULL;
        ldapDefaultClient->bindMsgLen = 0;

        ldapDefaultClient->entriesFound = NULL;
        ldapDefaultClient->currentRequest = NULL;
        ldapDefaultClient->currentResponse = NULL;

        *pClient = ldapDefaultClient;

cleanup:

        if (PKIX_ERROR_RECEIVED) {
                PKIX_DECREF(ldapDefaultClient);
        }

        PKIX_RETURN(LDAPDEFAULTCLIENT);
}


































PKIX_Error *
PKIX_PL_LdapDefaultClient_Create(
        PRNetAddr *sockaddr,
        PRIntervalTime timeout,
        LDAPBindAPI *bindAPI,
        PKIX_PL_LdapDefaultClient **pClient,
        void *plContext)
{
        PRErrorCode status = 0;
        PKIX_PL_Socket *socket = NULL;
        PKIX_PL_LdapDefaultClient *client = NULL;

        PKIX_ENTER(LDAPDEFAULTCLIENT, "PKIX_PL_LdapDefaultClient_Create");
        PKIX_NULLCHECK_TWO(sockaddr, pClient);

        PKIX_CHECK(pkix_pl_Socket_Create
                (PKIX_FALSE, timeout, sockaddr, &status, &socket, plContext),
                PKIX_SOCKETCREATEFAILED);

        PKIX_CHECK(pkix_pl_LdapDefaultClient_CreateHelper
                (socket, bindAPI, &client, plContext),
                PKIX_LDAPDEFAULTCLIENTCREATEHELPERFAILED);

        
        if (status == 0) {
                if (client->bindAPI != NULL) {
                        client->connectStatus = CONNECTED;
                } else {
                        client->connectStatus = BOUND;
                }
        } else {
                client->connectStatus = CONNECT_PENDING;
        }

        *pClient = client;

cleanup:
        if (PKIX_ERROR_RECEIVED) {
                PKIX_DECREF(client);
        }

        PKIX_DECREF(socket);

        PKIX_RETURN(LDAPDEFAULTCLIENT);
}


































PKIX_Error *
PKIX_PL_LdapDefaultClient_CreateByName(
        char *hostname,
        PRIntervalTime timeout,
        LDAPBindAPI *bindAPI,
        PKIX_PL_LdapDefaultClient **pClient,
        void *plContext)
{
        PRErrorCode status = 0;
        PKIX_PL_Socket *socket = NULL;
        PKIX_PL_LdapDefaultClient *client = NULL;

        PKIX_ENTER(LDAPDEFAULTCLIENT, "PKIX_PL_LdapDefaultClient_CreateByName");
        PKIX_NULLCHECK_TWO(hostname, pClient);

        PKIX_CHECK(pkix_pl_Socket_CreateByName
                (PKIX_FALSE, timeout, hostname, &status, &socket, plContext),
                PKIX_SOCKETCREATEBYNAMEFAILED);

        PKIX_CHECK(pkix_pl_LdapDefaultClient_CreateHelper
                (socket, bindAPI, &client, plContext),
                PKIX_LDAPDEFAULTCLIENTCREATEHELPERFAILED);

        
        if (status == 0) {
                if (client->bindAPI != NULL) {
                        client->connectStatus = CONNECTED;
                } else {
                        client->connectStatus = BOUND;
                }
        } else {
                client->connectStatus = CONNECT_PENDING;
        }

        *pClient = client;

cleanup:
        if (PKIX_ERROR_RECEIVED) {
                PKIX_DECREF(client);
        }

        PKIX_DECREF(socket);

        PKIX_RETURN(LDAPDEFAULTCLIENT);
}





static PKIX_Error *
pkix_pl_LdapDefaultClient_Destroy(
        PKIX_PL_Object *object,
        void *plContext)
{
        PKIX_Int32 bytesWritten = 0;
        PKIX_PL_LdapDefaultClient *client = NULL;
        PKIX_PL_Socket_Callback *callbackList = NULL;
        SECItem *encoded = NULL;

        PKIX_ENTER(LDAPDEFAULTCLIENT,
                    "pkix_pl_LdapDefaultClient_Destroy");
        PKIX_NULLCHECK_ONE(object);

        PKIX_CHECK(pkix_CheckType
                    (object, PKIX_LDAPDEFAULTCLIENT_TYPE, plContext),
                    PKIX_OBJECTNOTANLDAPDEFAULTCLIENT);

        client = (PKIX_PL_LdapDefaultClient *)object;

        switch (client->connectStatus) {
        case CONNECT_PENDING:
                break;
        case CONNECTED:
        case BIND_PENDING:
        case BIND_RESPONSE:
        case BIND_RESPONSE_PENDING:
        case BOUND:
        case SEND_PENDING:
        case RECV:
        case RECV_PENDING:
        case RECV_INITIAL:
        case RECV_NONINITIAL:
        case ABANDON_PENDING:
                if (client->bindAPI != NULL) {
                        PKIX_CHECK(pkix_pl_LdapDefaultClient_MakeUnbind
                                (client->arena,
                                ++(client->messageID),
                                &encoded,
                                plContext),
                                PKIX_LDAPDEFAULTCLIENTMAKEUNBINDFAILED);

                        callbackList =
                                (PKIX_PL_Socket_Callback *)(client->callbackList);
                        PKIX_CHECK(callbackList->sendCallback
                                (client->clientSocket,
                                encoded->data,
                                encoded->len,
                                &bytesWritten,
                                plContext),
                                PKIX_SOCKETSENDFAILED);
                }
                break;
        default:
                PKIX_ERROR(PKIX_LDAPDEFAULTCLIENTINILLEGALSTATE);
        }

        PKIX_DECREF(client->cachePtr);
        PKIX_DECREF(client->clientSocket);
        PKIX_DECREF(client->entriesFound);
        PKIX_DECREF(client->currentRequest);
        PKIX_DECREF(client->currentResponse);

        PKIX_CHECK(PKIX_PL_Free
		(client->rcvBuf, plContext), PKIX_FREEFAILED);

        PKIX_PL_NSSCALL
                (LDAPDEFAULTCLIENT,
                PORT_FreeArena,
                (client->arena, PR_FALSE));

cleanup:

        PKIX_RETURN(LDAPDEFAULTCLIENT);
}





static PKIX_Error *
pkix_pl_LdapDefaultClient_Hashcode(
        PKIX_PL_Object *object,
        PKIX_UInt32 *pHashcode,
        void *plContext)
{
        PKIX_PL_LdapDefaultClient *ldapDefaultClient = NULL;
        PKIX_UInt32 tempHash = 0;

        PKIX_ENTER
                (LDAPDEFAULTCLIENT, "pkix_pl_LdapDefaultClient_Hashcode");
        PKIX_NULLCHECK_TWO(object, pHashcode);

        PKIX_CHECK(pkix_CheckType
                (object, PKIX_LDAPDEFAULTCLIENT_TYPE, plContext),
                PKIX_OBJECTNOTANLDAPDEFAULTCLIENT);

        ldapDefaultClient = (PKIX_PL_LdapDefaultClient *)object;

        PKIX_CHECK(PKIX_PL_Object_Hashcode
                ((PKIX_PL_Object *)ldapDefaultClient->clientSocket,
                &tempHash,
                plContext),
                PKIX_SOCKETHASHCODEFAILED);

        if (ldapDefaultClient->bindAPI != NULL) {
                tempHash = (tempHash << 7) +
                        ldapDefaultClient->bindAPI->selector;
        }

        *pHashcode = tempHash;

cleanup:

        PKIX_RETURN(LDAPDEFAULTCLIENT);
}





static PKIX_Error *
pkix_pl_LdapDefaultClient_Equals(
        PKIX_PL_Object *firstObject,
        PKIX_PL_Object *secondObject,
        PKIX_Int32 *pResult,
        void *plContext)
{
        PKIX_PL_LdapDefaultClient *firstClientContext = NULL;
        PKIX_PL_LdapDefaultClient *secondClientContext = NULL;
        PKIX_Int32 compare = 0;

        PKIX_ENTER(LDAPDEFAULTCLIENT, "pkix_pl_LdapDefaultClient_Equals");
        PKIX_NULLCHECK_THREE(firstObject, secondObject, pResult);

        *pResult = PKIX_FALSE;

        PKIX_CHECK(pkix_CheckTypes
                (firstObject,
                secondObject,
                PKIX_LDAPDEFAULTCLIENT_TYPE,
                plContext),
                PKIX_OBJECTNOTANLDAPDEFAULTCLIENT);

        firstClientContext = (PKIX_PL_LdapDefaultClient *)firstObject;
        secondClientContext = (PKIX_PL_LdapDefaultClient *)secondObject;

        if (firstClientContext == secondClientContext) {
                *pResult = PKIX_TRUE;
                goto cleanup;
        }

        PKIX_CHECK(PKIX_PL_Object_Equals
                ((PKIX_PL_Object *)firstClientContext->clientSocket,
                (PKIX_PL_Object *)secondClientContext->clientSocket,
                &compare,
                plContext),
                PKIX_SOCKETEQUALSFAILED);

        if (!compare) {
                goto cleanup;
        }

        if (PKIX_EXACTLY_ONE_NULL
                (firstClientContext->bindAPI, secondClientContext->bindAPI)) {
                goto cleanup;
        }

        if (firstClientContext->bindAPI) {
                if (firstClientContext->bindAPI->selector !=
                    secondClientContext->bindAPI->selector) {
                        goto cleanup;
                }
        }

        *pResult = PKIX_TRUE;

cleanup:

        PKIX_RETURN(LDAPDEFAULTCLIENT);
}















PKIX_Error *
pkix_pl_LdapDefaultClient_RegisterSelf(void *plContext)
{
        extern pkix_ClassTable_Entry systemClasses[PKIX_NUMTYPES];
        pkix_ClassTable_Entry entry;

        PKIX_ENTER
                (LDAPDEFAULTCLIENT,
                "pkix_pl_LdapDefaultClient_RegisterSelf");

        entry.description = "LdapDefaultClient";
        entry.objCounter = 0;
        entry.typeObjectSize = sizeof(PKIX_PL_LdapDefaultClient);
        entry.destructor = pkix_pl_LdapDefaultClient_Destroy;
        entry.equalsFunction = pkix_pl_LdapDefaultClient_Equals;
        entry.hashcodeFunction = pkix_pl_LdapDefaultClient_Hashcode;
        entry.toStringFunction = NULL;
        entry.comparator = NULL;
        entry.duplicateFunction = NULL;

        systemClasses[PKIX_LDAPDEFAULTCLIENT_TYPE] = entry;

        PKIX_RETURN(LDAPDEFAULTCLIENT);
}





















PKIX_Error *
pkix_pl_LdapDefaultClient_GetPollDesc(
        PKIX_PL_LdapDefaultClient *context,
        PRPollDesc **pPollDesc,
        void *plContext)
{
        PKIX_ENTER
                (LDAPDEFAULTCLIENT,
                "pkix_pl_LdapDefaultClient_GetPollDesc");
        PKIX_NULLCHECK_TWO(context, pPollDesc);

        *pPollDesc = &(context->pollDesc);

        PKIX_RETURN(LDAPDEFAULTCLIENT);
}




























static PKIX_Error *
pkix_pl_LdapDefaultClient_ConnectContinue(
        PKIX_PL_LdapDefaultClient *client,
        PKIX_Boolean *pKeepGoing,
        void *plContext)
{
        PKIX_PL_Socket_Callback *callbackList;
        PRErrorCode status;
        PKIX_Boolean keepGoing = PKIX_FALSE;

        PKIX_ENTER
                (LDAPDEFAULTCLIENT,
                "pkix_pl_LdapDefaultClient_ConnectContinue");
        PKIX_NULLCHECK_ONE(client);

        callbackList = (PKIX_PL_Socket_Callback *)(client->callbackList);

        PKIX_CHECK(callbackList->connectcontinueCallback
                (client->clientSocket, &status, plContext),
                PKIX_SOCKETCONNECTCONTINUEFAILED);

        if (status == 0) {
                if (client->bindAPI != NULL) {
                        client->connectStatus = CONNECTED;
                } else {
                        client->connectStatus = BOUND;
                }
                keepGoing = PKIX_FALSE;
        } else if (status != PR_IN_PROGRESS_ERROR) {
                PKIX_ERROR(PKIX_UNEXPECTEDERRORINESTABLISHINGCONNECTION);
        }

        PKIX_CHECK(PKIX_PL_Object_InvalidateCache
                ((PKIX_PL_Object *)client, plContext),
                PKIX_OBJECTINVALIDATECACHEFAILED);

        *pKeepGoing = keepGoing;

cleanup:
        PKIX_RETURN(LDAPDEFAULTCLIENT);
}



























static PKIX_Error *
pkix_pl_LdapDefaultClient_Bind(
        PKIX_PL_LdapDefaultClient *client,
        PKIX_Boolean *pKeepGoing,
        void *plContext)
{
        SECItem *encoded = NULL;
        PKIX_Int32 bytesWritten = 0;
        PKIX_PL_Socket_Callback *callbackList;

        PKIX_ENTER(LDAPDEFAULTCLIENT, "pkix_pl_LdapDefaultClient_Bind");
        PKIX_NULLCHECK_ONE(client);

        
        if (!(client->bindMsg)) {
                PKIX_CHECK(pkix_pl_LdapDefaultClient_MakeBind
                        (client->arena,
                        3,
                        client->bindAPI,
                        client->messageID,
                        &encoded,
                        plContext),
                        PKIX_LDAPDEFAULTCLIENTMAKEBINDFAILED);
                client->bindMsg = encoded->data;
                client->bindMsgLen = encoded->len;
        }

        callbackList = (PKIX_PL_Socket_Callback *)(client->callbackList);

        PKIX_CHECK(callbackList->sendCallback
                (client->clientSocket,
                client->bindMsg,
                client->bindMsgLen,
                &bytesWritten,
                plContext),
                PKIX_SOCKETSENDFAILED);

        client->lastIO = PR_Now();

        if (bytesWritten < 0) {
                client->connectStatus = BIND_PENDING;
                *pKeepGoing = PKIX_FALSE;
        } else {
                client->connectStatus = BIND_RESPONSE;
                *pKeepGoing = PKIX_TRUE;
        }

        PKIX_CHECK(PKIX_PL_Object_InvalidateCache
                ((PKIX_PL_Object *)client, plContext),
                PKIX_OBJECTINVALIDATECACHEFAILED);

cleanup:
        PKIX_RETURN(LDAPDEFAULTCLIENT);
}



























PKIX_Error *pkix_pl_LdapDefaultClient_BindContinue(
        PKIX_PL_LdapDefaultClient *client,
        PKIX_Boolean *pKeepGoing,
        void *plContext)
{
        PKIX_Int32 bytesWritten = 0;
        PKIX_PL_Socket_Callback *callbackList = NULL;

        PKIX_ENTER(LDAPDEFAULTCLIENT, "pkix_pl_LdapDefaultClient_BindContinue");
        PKIX_NULLCHECK_ONE(client);

        *pKeepGoing = PKIX_FALSE;

        callbackList = (PKIX_PL_Socket_Callback *)(client->callbackList);

        PKIX_CHECK(callbackList->pollCallback
                (client->clientSocket, &bytesWritten, NULL, plContext),
                PKIX_SOCKETPOLLFAILED);

        




        if (bytesWritten >= 0) {

                client->connectStatus = BIND_RESPONSE;

                PKIX_CHECK(PKIX_PL_Object_InvalidateCache
                        ((PKIX_PL_Object *)client, plContext),
                        PKIX_OBJECTINVALIDATECACHEFAILED);

                *pKeepGoing = PKIX_TRUE;
        }

cleanup:
        PKIX_RETURN(LDAPDEFAULTCLIENT);
}
































static PKIX_Error *
pkix_pl_LdapDefaultClient_BindResponse(
        PKIX_PL_LdapDefaultClient *client,
        PKIX_Boolean *pKeepGoing,
        void *plContext)
{
        PKIX_Int32 bytesRead = 0;
        PKIX_PL_Socket_Callback *callbackList = NULL;

        PKIX_ENTER(LDAPDEFAULTCLIENT, "pkix_pl_LdapDefaultClient_BindResponse");
        PKIX_NULLCHECK_TWO(client, client->rcvBuf);

        callbackList = (PKIX_PL_Socket_Callback *)(client->callbackList);

        PKIX_CHECK(callbackList->recvCallback
                (client->clientSocket,
                client->rcvBuf,
                client->capacity,
                &bytesRead,
                plContext),
                PKIX_SOCKETRECVFAILED);

        client->lastIO = PR_Now();

        if (bytesRead > 0) {
                PKIX_CHECK(pkix_pl_LdapDefaultClient_VerifyBindResponse
                        (client, bytesRead, plContext),
                        PKIX_LDAPDEFAULTCLIENTVERIFYBINDRESPONSEFAILED);
                



                client->connectStatus = BOUND;
        } else {
                client->connectStatus = BIND_RESPONSE_PENDING;
        }

        PKIX_CHECK(PKIX_PL_Object_InvalidateCache
                ((PKIX_PL_Object *)client, plContext),
                PKIX_OBJECTINVALIDATECACHEFAILED);

        *pKeepGoing = PKIX_TRUE;

cleanup:
        PKIX_RETURN(LDAPDEFAULTCLIENT);
}



























static PKIX_Error *
pkix_pl_LdapDefaultClient_BindResponseContinue(
        PKIX_PL_LdapDefaultClient *client,
        PKIX_Boolean *pKeepGoing,
        void *plContext)
{
        PKIX_Int32 bytesRead = 0;
        PKIX_PL_Socket_Callback *callbackList = NULL;

        PKIX_ENTER
                (LDAPDEFAULTCLIENT,
                "pkix_pl_LdapDefaultClient_BindResponseContinue");
        PKIX_NULLCHECK_ONE(client);

        callbackList = (PKIX_PL_Socket_Callback *)(client->callbackList);

        PKIX_CHECK(callbackList->pollCallback
                (client->clientSocket, NULL, &bytesRead, plContext),
                PKIX_SOCKETPOLLFAILED);

        if (bytesRead > 0) {
                PKIX_CHECK(pkix_pl_LdapDefaultClient_VerifyBindResponse
                        (client, bytesRead, plContext),
                        PKIX_LDAPDEFAULTCLIENTVERIFYBINDRESPONSEFAILED);
                client->connectStatus = BOUND;

                PKIX_CHECK(PKIX_PL_Object_InvalidateCache
                        ((PKIX_PL_Object *)client, plContext),
                        PKIX_OBJECTINVALIDATECACHEFAILED);

                *pKeepGoing = PKIX_TRUE;
        } else {
                *pKeepGoing = PKIX_FALSE;
        }

cleanup:
        PKIX_RETURN(LDAPDEFAULTCLIENT);
}

































static PKIX_Error *
pkix_pl_LdapDefaultClient_Send(
        PKIX_PL_LdapDefaultClient *client,
        PKIX_Boolean *pKeepGoing,
        PKIX_UInt32 *pBytesTransferred,
        void *plContext)
{
        PKIX_Int32 bytesWritten = 0;
        PKIX_PL_Socket_Callback *callbackList = NULL;

        PKIX_ENTER(LDAPDEFAULTCLIENT, "pkix_pl_LdapDefaultClient_Send");
        PKIX_NULLCHECK_THREE(client, pKeepGoing, pBytesTransferred);

        *pKeepGoing = PKIX_FALSE;

        
        if (client->sendBuf) {
                callbackList = (PKIX_PL_Socket_Callback *)(client->callbackList);

                PKIX_CHECK(callbackList->sendCallback
                        (client->clientSocket,
                        client->sendBuf,
                        client->bytesToWrite,
                        &bytesWritten,
                        plContext),
                        PKIX_SOCKETSENDFAILED);

                client->lastIO = PR_Now();

                




                if (bytesWritten >= 0) {
                        client->sendBuf = NULL;
                        client->connectStatus = RECV;
                        *pKeepGoing = PKIX_TRUE;

                } else {
                        *pKeepGoing = PKIX_FALSE;
                        client->connectStatus = SEND_PENDING;
                }

        }

        PKIX_CHECK(PKIX_PL_Object_InvalidateCache
                ((PKIX_PL_Object *)client, plContext),
                PKIX_OBJECTINVALIDATECACHEFAILED);

        *pBytesTransferred = bytesWritten;

cleanup:
        PKIX_RETURN(LDAPDEFAULTCLIENT);
}

































static PKIX_Error *
pkix_pl_LdapDefaultClient_SendContinue(
        PKIX_PL_LdapDefaultClient *client,
        PKIX_Boolean *pKeepGoing,
        PKIX_UInt32 *pBytesTransferred,
        void *plContext)
{
        PKIX_Int32 bytesWritten = 0;
        PKIX_PL_Socket_Callback *callbackList = NULL;

        PKIX_ENTER(LDAPDEFAULTCLIENT, "pkix_pl_LdapDefaultClient_SendContinue");
        PKIX_NULLCHECK_THREE(client, pKeepGoing, pBytesTransferred);

        *pKeepGoing = PKIX_FALSE;

        callbackList = (PKIX_PL_Socket_Callback *)(client->callbackList);

        PKIX_CHECK(callbackList->pollCallback
                (client->clientSocket, &bytesWritten, NULL, plContext),
                PKIX_SOCKETPOLLFAILED);

        




        if (bytesWritten >= 0) {
                client->sendBuf = NULL;
                client->connectStatus = RECV;

                PKIX_CHECK(PKIX_PL_Object_InvalidateCache
                        ((PKIX_PL_Object *)client, plContext),
                        PKIX_OBJECTINVALIDATECACHEFAILED);

                *pKeepGoing = PKIX_TRUE;
        }

        *pBytesTransferred = bytesWritten;

cleanup:
        PKIX_RETURN(LDAPDEFAULTCLIENT);
}


























static PKIX_Error *
pkix_pl_LdapDefaultClient_Recv(
        PKIX_PL_LdapDefaultClient *client,
        PKIX_Boolean *pKeepGoing,
        void *plContext)
{
        PKIX_Int32 bytesRead = 0;
        PKIX_UInt32 bytesToRead = 0;
        PKIX_PL_Socket_Callback *callbackList = NULL;

        PKIX_ENTER(LDAPDEFAULTCLIENT, "pkix_pl_LdapDefaultClient_Recv");
        PKIX_NULLCHECK_THREE(client, pKeepGoing, client->rcvBuf);

        callbackList = (PKIX_PL_Socket_Callback *)(client->callbackList);

        








        if (client->currentResponse) {
                PKIX_CHECK(pkix_pl_LdapResponse_GetCapacity
                        (client->currentResponse, &bytesToRead, plContext),
                        PKIX_LDAPRESPONSEGETCAPACITYFAILED);
                if ((bytesToRead > client->capacity) ||
                    ((bytesToRead + MINIMUM_MSG_LENGTH) < client->capacity)) {
                        bytesToRead = client->capacity;
                }
        } else {
                bytesToRead = client->capacity;
        }

        client->currentBytesAvailable = 0;

        PKIX_CHECK(callbackList->recvCallback
                (client->clientSocket,
                (void *)client->rcvBuf,
                bytesToRead,
                &bytesRead,
                plContext),
                PKIX_SOCKETRECVFAILED);

        client->currentInPtr = client->rcvBuf;
        client->lastIO = PR_Now();

        if (bytesRead > 0) {
                client->currentBytesAvailable = bytesRead;
                client->connectStatus = RECV_INITIAL;
                *pKeepGoing = PKIX_TRUE;
        } else {
                client->connectStatus = RECV_PENDING;
                *pKeepGoing = PKIX_FALSE;
        }

        PKIX_CHECK(PKIX_PL_Object_InvalidateCache
                ((PKIX_PL_Object *)client, plContext),
                PKIX_OBJECTINVALIDATECACHEFAILED);

cleanup:
        PKIX_RETURN(LDAPDEFAULTCLIENT);
}



























static PKIX_Error *
pkix_pl_LdapDefaultClient_RecvContinue(
        PKIX_PL_LdapDefaultClient *client,
        PKIX_Boolean *pKeepGoing,
        void *plContext)
{
        PKIX_Int32 bytesRead = 0;
        PKIX_PL_Socket_Callback *callbackList = NULL;

        PKIX_ENTER(LDAPDEFAULTCLIENT, "pkix_pl_LdapDefaultClient_RecvContinue");
        PKIX_NULLCHECK_TWO(client, pKeepGoing);

        callbackList = (PKIX_PL_Socket_Callback *)(client->callbackList);

        PKIX_CHECK(callbackList->pollCallback
                (client->clientSocket, NULL, &bytesRead, plContext),
                PKIX_SOCKETPOLLFAILED);

        if (bytesRead > 0) {
                client->currentBytesAvailable += bytesRead;
                client->connectStatus = RECV_INITIAL;
                *pKeepGoing = PKIX_TRUE;

                PKIX_CHECK(PKIX_PL_Object_InvalidateCache
                        ((PKIX_PL_Object *)client, plContext),
                        PKIX_OBJECTINVALIDATECACHEFAILED);
        } else {
                *pKeepGoing = PKIX_FALSE;
        }

cleanup:
        PKIX_RETURN(LDAPDEFAULTCLIENT);
}



























static PKIX_Error *
pkix_pl_LdapDefaultClient_AbandonContinue(
        PKIX_PL_LdapDefaultClient *client,
        PKIX_Boolean *pKeepGoing,
        void *plContext)
{
        PKIX_Int32 bytesWritten = 0;
        PKIX_PL_Socket_Callback *callbackList = NULL;

        PKIX_ENTER
                (LDAPDEFAULTCLIENT, "pkix_pl_LdapDefaultClient_AbandonContinue");
        PKIX_NULLCHECK_TWO(client, pKeepGoing);

        callbackList = (PKIX_PL_Socket_Callback *)(client->callbackList);

        PKIX_CHECK(callbackList->pollCallback
                (client->clientSocket, &bytesWritten, NULL, plContext),
                PKIX_SOCKETPOLLFAILED);

        if (bytesWritten > 0) {
                client->connectStatus = BOUND;
                *pKeepGoing = PKIX_TRUE;

                PKIX_CHECK(PKIX_PL_Object_InvalidateCache
                        ((PKIX_PL_Object *)client, plContext),
                        PKIX_OBJECTINVALIDATECACHEFAILED);
        } else {
                *pKeepGoing = PKIX_FALSE;
        }

cleanup:
        PKIX_RETURN(LDAPDEFAULTCLIENT);
}



























static PKIX_Error *
pkix_pl_LdapDefaultClient_RecvInitial(
        PKIX_PL_LdapDefaultClient *client,
        PKIX_Boolean *pKeepGoing,
        void *plContext)
{
        unsigned char *msgBuf = NULL;
        unsigned char *to = NULL;
        unsigned char *from = NULL;
        PKIX_UInt32 dataIndex = 0;
        PKIX_UInt32 messageIdLen = 0;
        PKIX_UInt32 messageLength = 0;
        PKIX_UInt32 sizeofLength = 0;
        PKIX_UInt32 bytesProcessed = 0;
        unsigned char messageChar = 0;
        LDAPMessageType messageType = 0;
        PKIX_Int32 bytesRead = 0;
        PKIX_PL_Socket_Callback *callbackList = NULL;

        PKIX_ENTER(LDAPDEFAULTCLIENT, "pkix_pl_LdapDefaultClient_RecvInitial");
        PKIX_NULLCHECK_TWO(client, pKeepGoing);

        




        if (client->currentResponse) {
                client->connectStatus = RECV_NONINITIAL;
                *pKeepGoing = PKIX_TRUE;
                goto cleanup;
        }
        msgBuf = client->currentInPtr;

        
        if (client->currentBytesAvailable < MINIMUM_MSG_LENGTH) {
                




                to = (unsigned char *)client->rcvBuf;
                from = client->currentInPtr;
                for (dataIndex = 0;
                    dataIndex < client->currentBytesAvailable;
                    dataIndex++) {
                        *to++ = *from++;
                }
                callbackList = (PKIX_PL_Socket_Callback *)(client->callbackList);
                PKIX_CHECK(callbackList->recvCallback
                        (client->clientSocket,
                        (void *)to,
                        client->capacity - client->currentBytesAvailable,
                        &bytesRead,
                        plContext),
                        PKIX_SOCKETRECVFAILED);

                client->currentInPtr = client->rcvBuf;
                client->lastIO = PR_Now();

                if (bytesRead <= 0) {
                        client->connectStatus = RECV_PENDING;
                        *pKeepGoing = PKIX_FALSE;
                        goto cleanup;
                } else {
                        client->currentBytesAvailable += bytesRead;
                }
        }

        






        
        if ((msgBuf[1] & 0x80) != 0) {
                sizeofLength = msgBuf[1] & 0x7F;
                for (dataIndex = 0; dataIndex < sizeofLength; dataIndex++) {
                        messageLength =
                                (messageLength << 8) + msgBuf[dataIndex + 2];
                }
        } else {
                messageLength = msgBuf[1];
        }

        
        messageIdLen = msgBuf[dataIndex + 3];

        messageChar = msgBuf[dataIndex + messageIdLen + 4];

        
        if ((SEC_ASN1_CONSTRUCTED | SEC_ASN1_APPLICATION |
            LDAP_SEARCHRESPONSEENTRY_TYPE) == messageChar) {

                messageType = LDAP_SEARCHRESPONSEENTRY_TYPE;

        } else if ((SEC_ASN1_CONSTRUCTED | SEC_ASN1_APPLICATION |
            LDAP_SEARCHRESPONSERESULT_TYPE) == messageChar) {

                messageType = LDAP_SEARCHRESPONSERESULT_TYPE;

        } else {

                PKIX_ERROR(PKIX_SEARCHRESPONSEPACKETOFUNKNOWNTYPE);

        }

        



        PKIX_CHECK(pkix_pl_LdapResponse_Create
                (messageType,
                messageLength + dataIndex + 2,
                client->currentBytesAvailable,
                msgBuf,
                &bytesProcessed,
                &(client->currentResponse),
                plContext),
                PKIX_LDAPRESPONSECREATEFAILED);

        client->currentBytesAvailable -= bytesProcessed;

        PKIX_CHECK(pkix_pl_LdapDefaultClient_RecvCheckComplete
                (client, bytesProcessed, pKeepGoing, plContext),
                PKIX_LDAPDEFAULTCLIENTRECVCHECKCOMPLETEFAILED);

cleanup:

        PKIX_RETURN(LDAPDEFAULTCLIENT);
}



























static PKIX_Error *
pkix_pl_LdapDefaultClient_RecvNonInitial(
        PKIX_PL_LdapDefaultClient *client,
        PKIX_Boolean *pKeepGoing,
        void *plContext)
{

        PKIX_UInt32 bytesProcessed = 0;

        PKIX_ENTER
                (LDAPDEFAULTCLIENT, "pkix_pl_LdapDefaultClient_RecvNonInitial");
        PKIX_NULLCHECK_TWO(client, pKeepGoing);

        PKIX_CHECK(pkix_pl_LdapResponse_Append
                (client->currentResponse,
                client->currentBytesAvailable,
                client->currentInPtr,
                &bytesProcessed,
                plContext),
                PKIX_LDAPRESPONSEAPPENDFAILED);

        client->currentBytesAvailable -= bytesProcessed;

        PKIX_CHECK(pkix_pl_LdapDefaultClient_RecvCheckComplete
                (client, bytesProcessed, pKeepGoing, plContext),
                PKIX_LDAPDEFAULTCLIENTRECVCHECKCOMPLETEFAILED);

cleanup:

        PKIX_RETURN(LDAPDEFAULTCLIENT);
}






















static PKIX_Error *
pkix_pl_LdapDefaultClient_Dispatch(
        PKIX_PL_LdapDefaultClient *client,
        void *plContext)
{
        PKIX_UInt32 bytesTransferred = 0;
        PKIX_Boolean keepGoing = PKIX_TRUE;

        PKIX_ENTER(LDAPDEFAULTCLIENT, "pkix_pl_LdapDefaultClient_Dispatch");
        PKIX_NULLCHECK_ONE(client);

        while (keepGoing) {
                switch (client->connectStatus) {
                case CONNECT_PENDING:
                        PKIX_CHECK
                                (pkix_pl_LdapDefaultClient_ConnectContinue
                                (client, &keepGoing, plContext),
                                PKIX_LDAPDEFAULTCLIENTCONNECTCONTINUEFAILED);
                        break;
                case CONNECTED:
                        PKIX_CHECK
                                (pkix_pl_LdapDefaultClient_Bind
                                (client, &keepGoing, plContext),
                                PKIX_LDAPDEFAULTCLIENTBINDFAILED);
                        break;
                case BIND_PENDING:
                        PKIX_CHECK
                                (pkix_pl_LdapDefaultClient_BindContinue
                                (client, &keepGoing, plContext),
                                PKIX_LDAPDEFAULTCLIENTBINDCONTINUEFAILED);
                        break;
                case BIND_RESPONSE:
                        PKIX_CHECK
                                (pkix_pl_LdapDefaultClient_BindResponse
                                (client, &keepGoing, plContext),
                                PKIX_LDAPDEFAULTCLIENTBINDRESPONSEFAILED);
                        break;
                case BIND_RESPONSE_PENDING:
                        PKIX_CHECK
                                (pkix_pl_LdapDefaultClient_BindResponseContinue
                                (client, &keepGoing, plContext),
                                PKIX_LDAPDEFAULTCLIENTBINDRESPONSECONTINUEFAILED);
                        break;
                case BOUND:
                        PKIX_CHECK
                                (pkix_pl_LdapDefaultClient_Send
                                (client, &keepGoing, &bytesTransferred, plContext),
                                PKIX_LDAPDEFAULTCLIENTSENDFAILED);
                        break;
                case SEND_PENDING:
                        PKIX_CHECK
                                (pkix_pl_LdapDefaultClient_SendContinue
                                (client, &keepGoing, &bytesTransferred, plContext),
                                PKIX_LDAPDEFAULTCLIENTSENDCONTINUEFAILED);
                        break;
                case RECV:
                        PKIX_CHECK
                                (pkix_pl_LdapDefaultClient_Recv
                                (client, &keepGoing, plContext),
                                PKIX_LDAPDEFAULTCLIENTRECVFAILED);
                        break;
                case RECV_PENDING:
                        PKIX_CHECK
                                (pkix_pl_LdapDefaultClient_RecvContinue
                                (client, &keepGoing, plContext),
                                PKIX_LDAPDEFAULTCLIENTRECVCONTINUEFAILED);
                        break;
                case RECV_INITIAL:
                        PKIX_CHECK
                                (pkix_pl_LdapDefaultClient_RecvInitial
                                (client, &keepGoing, plContext),
                                PKIX_LDAPDEFAULTCLIENTRECVINITIALFAILED);
                        break;
                case RECV_NONINITIAL:
                        PKIX_CHECK
                                (pkix_pl_LdapDefaultClient_RecvNonInitial
                                (client, &keepGoing, plContext),
                                PKIX_LDAPDEFAULTCLIENTRECVNONINITIALFAILED);
                        break;
                case ABANDON_PENDING:
                        PKIX_CHECK
                                (pkix_pl_LdapDefaultClient_AbandonContinue
                                (client, &keepGoing, plContext),
                                PKIX_LDAPDEFAULTCLIENTABANDONCONTINUEFAILED);
                        break;
                default:
                        PKIX_ERROR(PKIX_LDAPCERTSTOREINILLEGALSTATE);
                }
        }

cleanup:

        PKIX_RETURN(LDAPDEFAULTCLIENT);
}



























static PKIX_Error *
pkix_pl_LdapDefaultClient_MakeAndFilter(
        PRArenaPool *arena,
        LDAPNameComponent **nameComponents,
        LDAPFilter **pFilter,
        void *plContext)
{
        LDAPFilter **setOfFilter;
        LDAPFilter *andFilter = NULL;
        LDAPFilter *currentFilter = NULL;
        PKIX_UInt32 componentsPresent = 0;
        void *v = NULL;
        unsigned char *component = NULL;
        LDAPNameComponent **componentP = NULL;

        PKIX_ENTER(CERTSTORE, "pkix_pl_LdapDefaultClient_MakeAndFilter");
        PKIX_NULLCHECK_THREE(arena, nameComponents, pFilter);

        
        for (componentP = nameComponents, componentsPresent = 0;
                *(componentP++) != NULL;
                componentsPresent++) {}

        
        PKIX_PL_NSSCALLRV(CERTSTORE, v, PORT_ArenaZAlloc,
                (arena, (componentsPresent + 1)*sizeof(LDAPFilter *)));
        setOfFilter = (LDAPFilter **)v;

        
        PKIX_PL_NSSCALLRV(CERTSTORE, v, PORT_ArenaZNewArray,
                (arena, LDAPFilter, componentsPresent + 1));
        setOfFilter[0] = (LDAPFilter *)v;

        
        andFilter = setOfFilter[0];

        
        andFilter->selector = LDAP_ANDFILTER_TYPE;
        andFilter->filter.andFilter.filters = setOfFilter;

        currentFilter = andFilter + 1;

        for (componentP = nameComponents, componentsPresent = 0;
                *(componentP) != NULL; componentP++) {
                setOfFilter[componentsPresent++] = currentFilter;
                currentFilter->selector = LDAP_EQUALFILTER_TYPE;
                component = (*componentP)->attrType;
                currentFilter->filter.equalFilter.attrType.data = component;
                currentFilter->filter.equalFilter.attrType.len = 
                        PL_strlen((const char *)component);
                component = (*componentP)->attrValue;
                currentFilter->filter.equalFilter.attrValue.data = component;
                currentFilter->filter.equalFilter.attrValue.len =
                        PL_strlen((const char *)component);
                currentFilter++;
        }

        setOfFilter[componentsPresent] = NULL;

        *pFilter = andFilter;

        PKIX_RETURN(CERTSTORE);

}



























static PKIX_Error *
pkix_pl_LdapDefaultClient_InitiateRequest(
        PKIX_PL_LdapClient *genericClient,
        LDAPRequestParams *requestParams,
        void **pPollDesc,
        PKIX_List **pResponse,
        void *plContext)
{
        PKIX_List *searchResponseList = NULL;
        SECItem *encoded = NULL;
        LDAPFilter *filter = NULL;
        PKIX_PL_LdapDefaultClient *client = 0;

        PKIX_ENTER
                (LDAPDEFAULTCLIENT,
                "pkix_pl_LdapDefaultClient_InitiateRequest");
        PKIX_NULLCHECK_FOUR(genericClient, requestParams, pPollDesc, pResponse);

        PKIX_CHECK(pkix_CheckType
                ((PKIX_PL_Object *)genericClient,
                PKIX_LDAPDEFAULTCLIENT_TYPE,
                plContext),
                PKIX_GENERICCLIENTNOTANLDAPDEFAULTCLIENT);

        client = (PKIX_PL_LdapDefaultClient *)genericClient;

        PKIX_CHECK(pkix_pl_LdapDefaultClient_MakeAndFilter
                (client->arena, requestParams->nc, &filter, plContext),
                PKIX_LDAPDEFAULTCLIENTMAKEANDFILTERFAILED);

        PKIX_CHECK(pkix_pl_LdapRequest_Create
                (client->arena,
                client->messageID++,
                requestParams->baseObject,
                requestParams->scope,
                requestParams->derefAliases,
                requestParams->sizeLimit,
                requestParams->timeLimit,
                PKIX_FALSE,    
                filter,
                requestParams->attributes,
                &client->currentRequest,
                plContext),
                PKIX_LDAPREQUESTCREATEFAILED);

        
        PKIX_CHECK(PKIX_PL_HashTable_Lookup
                (client->cachePtr,
                (PKIX_PL_Object *)(client->currentRequest),
                (PKIX_PL_Object **)&searchResponseList,
                plContext),
                PKIX_HASHTABLELOOKUPFAILED);

        if (searchResponseList != NULL) {
                *pPollDesc = NULL;
                *pResponse = searchResponseList;
                PKIX_DECREF(client->currentRequest);
                goto cleanup;
        }

        

        PKIX_CHECK(pkix_pl_LdapRequest_GetEncoded
                (client->currentRequest, &encoded, plContext),
                PKIX_LDAPREQUESTGETENCODEDFAILED);

        client->sendBuf = encoded->data;
        client->bytesToWrite = encoded->len;

        PKIX_CHECK(pkix_pl_LdapDefaultClient_Dispatch(client, plContext),
                PKIX_LDAPDEFAULTCLIENTDISPATCHFAILED);

        






        if ((client->connectStatus == BOUND) &&
	    (client->entriesFound != NULL)) {
                *pPollDesc = NULL;
                *pResponse = client->entriesFound;
                client->entriesFound = NULL;
                PKIX_DECREF(client->currentRequest);
        } else {
                *pPollDesc = &client->pollDesc;
                *pResponse = NULL;
        }

cleanup:

        PKIX_RETURN(LDAPDEFAULTCLIENT);

}

























static PKIX_Error *
pkix_pl_LdapDefaultClient_ResumeRequest(
        PKIX_PL_LdapClient *genericClient,
        void **pPollDesc,
        PKIX_List **pResponse,
        void *plContext)
{
        PKIX_PL_LdapDefaultClient *client = 0;

        PKIX_ENTER
                (LDAPDEFAULTCLIENT, "pkix_pl_LdapDefaultClient_ResumeRequest");
        PKIX_NULLCHECK_THREE(genericClient, pPollDesc, pResponse);

        PKIX_CHECK(pkix_CheckType
                ((PKIX_PL_Object *)genericClient,
                PKIX_LDAPDEFAULTCLIENT_TYPE,
                plContext),
                PKIX_GENERICCLIENTNOTANLDAPDEFAULTCLIENT);

        client = (PKIX_PL_LdapDefaultClient *)genericClient;

        PKIX_CHECK(pkix_pl_LdapDefaultClient_Dispatch(client, plContext),
                PKIX_LDAPDEFAULTCLIENTDISPATCHFAILED);

        






        if ((client->connectStatus == BOUND) &&
	    (client->entriesFound != NULL)) {
                *pPollDesc = NULL;
                *pResponse = client->entriesFound;
                client->entriesFound = NULL;
                PKIX_DECREF(client->currentRequest);
        } else {
                *pPollDesc = &client->pollDesc;
                *pResponse = NULL;
        }

cleanup:

        PKIX_RETURN(LDAPDEFAULTCLIENT);

}






















PKIX_Error *
PKIX_PL_LdapDefaultClient_AbandonRequest(
        PKIX_PL_LdapDefaultClient *client,
        void *plContext)
{
        PKIX_Int32 bytesWritten = 0;
        PKIX_PL_Socket_Callback *callbackList = NULL;
        SECItem *encoded = NULL;

        PKIX_ENTER(CERTSTORE, "PKIX_PL_LdapDefaultClient_AbandonRequest");
        PKIX_NULLCHECK_ONE(client);

        if (client->connectStatus == RECV_PENDING) {
                PKIX_CHECK(pkix_pl_LdapDefaultClient_MakeAbandon
                        (client->arena,
                        (client->messageID) - 1,
                        &encoded,
                        plContext),
                        PKIX_LDAPDEFAULTCLIENTMAKEABANDONFAILED);

                callbackList = (PKIX_PL_Socket_Callback *)(client->callbackList);
                PKIX_CHECK(callbackList->sendCallback
                        (client->clientSocket,
                        encoded->data,
                        encoded->len,
                        &bytesWritten,
                        plContext),
                        PKIX_SOCKETSENDFAILED);

                if (bytesWritten < 0) {
                        client->connectStatus = ABANDON_PENDING;
                } else {
                        client->connectStatus = BOUND;
                }
        }

        PKIX_DECREF(client->entriesFound);
        PKIX_DECREF(client->currentRequest);
        PKIX_DECREF(client->currentResponse);

cleanup:

        PKIX_DECREF(client);

        PKIX_RETURN(CERTSTORE);
}
