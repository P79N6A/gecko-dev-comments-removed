







#include <fcntl.h>
#include "pkix_pl_ldapresponse.h"







static PKIX_Error *
pkix_pl_LdapResponse_Destroy(
        PKIX_PL_Object *object,
        void *plContext)
{
        PKIX_PL_LdapResponse *ldapRsp = NULL;
        LDAPMessage *m = NULL;
        LDAPSearchResponseEntry *entry = NULL;
        LDAPSearchResponseResult *result = NULL;
        LDAPSearchResponseAttr **attributes = NULL;
        LDAPSearchResponseAttr *attr = NULL;
        SECItem **valp = NULL;
        SECItem *val = NULL;

        PKIX_ENTER(LDAPRESPONSE, "pkix_pl_LdapResponse_Destroy");
        PKIX_NULLCHECK_ONE(object);

        PKIX_CHECK(pkix_CheckType(object, PKIX_LDAPRESPONSE_TYPE, plContext),
                    PKIX_OBJECTNOTLDAPRESPONSE);

        ldapRsp = (PKIX_PL_LdapResponse *)object;

        m = &ldapRsp->decoded;

        if (m->messageID.data != NULL) {
                PR_Free(m->messageID.data);
        }

        if (m->protocolOp.selector ==
                LDAP_SEARCHRESPONSEENTRY_TYPE) {
                entry = &m->protocolOp.op.searchResponseEntryMsg;
                if (entry->objectName.data != NULL) {
                        PR_Free(entry->objectName.data);
                }
                if (entry->attributes != NULL) {
                        for (attributes = entry->attributes;
                                *attributes != NULL;
                                attributes++) {
                                attr = *attributes;
                                PR_Free(attr->attrType.data);
                                for (valp = attr->val; *valp != NULL; valp++) {
                                        val = *valp;
                                        if (val->data != NULL) {
                                                PR_Free(val->data);
                                        }
                                        PR_Free(val);
                                }
                                PR_Free(attr->val);
                                PR_Free(attr);
                        }
                        PR_Free(entry->attributes);
                }
        } else if (m->protocolOp.selector ==
                LDAP_SEARCHRESPONSERESULT_TYPE) {
                result = &m->protocolOp.op.searchResponseResultMsg;
                if (result->resultCode.data != NULL) {
                        PR_Free(result->resultCode.data);
                }
        }

        PKIX_FREE(ldapRsp->derEncoded.data);

cleanup:

        PKIX_RETURN(LDAPRESPONSE);
}





static PKIX_Error *
pkix_pl_LdapResponse_Hashcode(
        PKIX_PL_Object *object,
        PKIX_UInt32 *pHashcode,
        void *plContext)
{
        PKIX_UInt32 dataLen = 0;
        PKIX_UInt32 dindex = 0;
        PKIX_UInt32 sizeOfLength = 0;
        PKIX_UInt32 idLen = 0;
        const unsigned char *msgBuf = NULL;
        PKIX_PL_LdapResponse *ldapRsp = NULL;

        PKIX_ENTER(LDAPRESPONSE, "pkix_pl_LdapResponse_Hashcode");
        PKIX_NULLCHECK_TWO(object, pHashcode);

        PKIX_CHECK(pkix_CheckType(object, PKIX_LDAPRESPONSE_TYPE, plContext),
                    PKIX_OBJECTNOTLDAPRESPONSE);

        ldapRsp = (PKIX_PL_LdapResponse *)object;

        *pHashcode = 0;

        



        if (ldapRsp->derEncoded.data) {
                msgBuf = (const unsigned char *)ldapRsp->derEncoded.data;
                
                if ((msgBuf[1] & 0x80) != 0) {
                        sizeOfLength = msgBuf[1] & 0x7F;
                        for (dindex = 0; dindex < sizeOfLength; dindex++) {
                                dataLen = (dataLen << 8) + msgBuf[dindex + 2];
                        }
                } else {
                        dataLen = msgBuf[1];
                }

                
                idLen = msgBuf[dindex + 3] + 2;
                dindex += idLen;
                dataLen -= idLen;
                msgBuf = &msgBuf[dindex + 2];

                PKIX_CHECK(pkix_hash(msgBuf, dataLen, pHashcode, plContext),
                        PKIX_HASHFAILED);
        }

cleanup:

        PKIX_RETURN(LDAPRESPONSE);

}





static PKIX_Error *
pkix_pl_LdapResponse_Equals(
        PKIX_PL_Object *firstObj,
        PKIX_PL_Object *secondObj,
        PKIX_Boolean *pResult,
        void *plContext)
{
        PKIX_PL_LdapResponse *rsp1 = NULL;
        PKIX_PL_LdapResponse *rsp2 = NULL;
        PKIX_UInt32 secondType = 0;
        PKIX_UInt32 firstLen = 0;
        const unsigned char *firstData = NULL;
        const unsigned char *secondData = NULL;
        PKIX_UInt32 sizeOfLength = 0;
        PKIX_UInt32 dindex = 0;
        PKIX_UInt32 i = 0;

        PKIX_ENTER(LDAPRESPONSE, "pkix_pl_LdapResponse_Equals");
        PKIX_NULLCHECK_THREE(firstObj, secondObj, pResult);

        
        PKIX_CHECK(pkix_CheckType(firstObj, PKIX_LDAPRESPONSE_TYPE, plContext),
                    PKIX_FIRSTOBJARGUMENTNOTLDAPRESPONSE);

        



        if (firstObj == secondObj){
                *pResult = PKIX_TRUE;
                goto cleanup;
        }

        



        *pResult = PKIX_FALSE;
        PKIX_CHECK(PKIX_PL_Object_GetType(secondObj, &secondType, plContext),
                PKIX_COULDNOTGETTYPEOFSECONDARGUMENT);
        if (secondType != PKIX_LDAPRESPONSE_TYPE) {
                goto cleanup;
        }

        rsp1 = (PKIX_PL_LdapResponse *)firstObj;
        rsp2 = (PKIX_PL_LdapResponse *)secondObj;

        
        if (!(rsp1->derEncoded.data) || !(rsp2->derEncoded.data)) {
                goto cleanup;
        }

        if (rsp1->derEncoded.len != rsp2->derEncoded.len) {
                goto cleanup;
        }

        firstData = (const unsigned char *)rsp1->derEncoded.data;
        secondData = (const unsigned char *)rsp2->derEncoded.data;

        




        
        if ((firstData[1] & 0x80) != 0) {
                sizeOfLength = firstData[1] & 0x7F;
                for (dindex = 0; dindex < sizeOfLength; dindex++) {
                        firstLen = (firstLen << 8) + firstData[dindex + 2];
                }
        } else {
                firstLen = firstData[1];
        }

        
        i = firstData[dindex + 3] + 2;
        dindex += i;
        firstLen -= i;
        firstData = &firstData[dindex + 2];

        






        secondData = &secondData[dindex + 2];
        
        for (i = 0; i < firstLen; i++) {
                if (firstData[i] != secondData[i]) {
                        goto cleanup;
                }
        }

        *pResult = PKIX_TRUE;

cleanup:

        PKIX_RETURN(LDAPRESPONSE);
}
















PKIX_Error *
pkix_pl_LdapResponse_RegisterSelf(void *plContext)
{
        extern pkix_ClassTable_Entry systemClasses[PKIX_NUMTYPES];
        pkix_ClassTable_Entry entry;

        PKIX_ENTER(LDAPRESPONSE, "pkix_pl_LdapResponse_RegisterSelf");

        entry.description = "LdapResponse";
        entry.objCounter = 0;
        entry.typeObjectSize = sizeof(PKIX_PL_LdapResponse);
        entry.destructor = pkix_pl_LdapResponse_Destroy;
        entry.equalsFunction = pkix_pl_LdapResponse_Equals;
        entry.hashcodeFunction = pkix_pl_LdapResponse_Hashcode;
        entry.toStringFunction = NULL;
        entry.comparator = NULL;
        entry.duplicateFunction = pkix_duplicateImmutable;

        systemClasses[PKIX_LDAPRESPONSE_TYPE] = entry;

        PKIX_RETURN(LDAPRESPONSE);
}














































PKIX_Error *
pkix_pl_LdapResponse_Create(
        LDAPMessageType responseType,
        PKIX_UInt32 totalLength,
        PKIX_UInt32 bytesAvailable,
        void *partialData,
        PKIX_UInt32 *pBytesConsumed,
        PKIX_PL_LdapResponse **pLdapResponse,
        void *plContext)
{
        PKIX_UInt32 bytesConsumed = 0;
        PKIX_PL_LdapResponse *ldapResponse = NULL;
        void *data = NULL;

        PKIX_ENTER(LDAPRESPONSE, "PKIX_PL_LdapResponse_Create");
        PKIX_NULLCHECK_ONE(pLdapResponse);

        if (bytesAvailable <= totalLength) {
                bytesConsumed = bytesAvailable;
        } else {
                bytesConsumed = totalLength;
        }

        
        PKIX_CHECK(PKIX_PL_Object_Alloc
                    (PKIX_LDAPRESPONSE_TYPE,
                    sizeof (PKIX_PL_LdapResponse),
                    (PKIX_PL_Object **)&ldapResponse,
                    plContext),
                    PKIX_COULDNOTCREATEOBJECT);

        ldapResponse->decoded.protocolOp.selector = responseType;
        ldapResponse->totalLength = totalLength;
        ldapResponse->partialLength = bytesConsumed;

        if (totalLength != 0){
                
                PKIX_NULLCHECK_ONE(partialData);

                PKIX_CHECK(PKIX_PL_Malloc
                    (totalLength,
                    &data,
                    plContext),
                    PKIX_MALLOCFAILED);

                PKIX_PL_NSSCALL
                    (LDAPRESPONSE,
                    PORT_Memcpy,
                    (data, partialData, bytesConsumed));
        }

        ldapResponse->derEncoded.type = siBuffer;
        ldapResponse->derEncoded.data = data;
        ldapResponse->derEncoded.len = totalLength;
        *pBytesConsumed = bytesConsumed;
        *pLdapResponse = ldapResponse;

cleanup:

        if (PKIX_ERROR_RECEIVED){
                PKIX_DECREF(ldapResponse);
        }

        PKIX_RETURN(LDAPRESPONSE);
}






























PKIX_Error *
pkix_pl_LdapResponse_Append(
        PKIX_PL_LdapResponse *response,
        PKIX_UInt32 incrLength,
        void *incrData,
        PKIX_UInt32 *pBytesConsumed,
        void *plContext)
{
        PKIX_UInt32 newPartialLength = 0;
        PKIX_UInt32 bytesConsumed = 0;
        void *dest = NULL;

        PKIX_ENTER(LDAPRESPONSE, "PKIX_PL_LdapResponse_Append");
        PKIX_NULLCHECK_TWO(response, pBytesConsumed);

        if (incrLength > 0) {

                
                bytesConsumed =
                        response->totalLength - response->partialLength;

                if (bytesConsumed > incrLength) {
                        bytesConsumed = incrLength;
                }

                newPartialLength = response->partialLength + bytesConsumed;

                PKIX_NULLCHECK_ONE(incrData);

                dest = &(((char *)response->derEncoded.data)[
                        response->partialLength]);

                PKIX_PL_NSSCALL
                        (LDAPRESPONSE,
                        PORT_Memcpy,
                        (dest, incrData, bytesConsumed));

                response->partialLength = newPartialLength;
        }

        *pBytesConsumed = bytesConsumed;

        PKIX_RETURN(LDAPRESPONSE);
}






























PKIX_Error *
pkix_pl_LdapResponse_IsComplete(
        PKIX_PL_LdapResponse *response,
        PKIX_Boolean *pIsComplete,
        void *plContext)
{
        PKIX_ENTER(LDAPRESPONSE, "PKIX_PL_LdapResponse_IsComplete");
        PKIX_NULLCHECK_TWO(response, pIsComplete);

        if (response->totalLength == response->partialLength) {
                *pIsComplete = PKIX_TRUE;
        } else {
                *pIsComplete = PKIX_FALSE;
        }

        PKIX_RETURN(LDAPRESPONSE);
}





























PKIX_Error *
pkix_pl_LdapResponse_Decode(
        PRArenaPool *arena,
        PKIX_PL_LdapResponse *response,
        SECStatus *pStatus,
        void *plContext)
{
        LDAPMessage *msg;
        SECStatus rv = SECFailure;

        PKIX_ENTER(LDAPRESPONSE, "PKIX_PL_LdapResponse_Decode");
        PKIX_NULLCHECK_THREE(arena, response, pStatus);

        if (response->totalLength != response->partialLength) {
                PKIX_ERROR(PKIX_ATTEMPTTODECODEANINCOMPLETERESPONSE);
        }

        msg = &(response->decoded);

        PKIX_PL_NSSCALL
                (LDAPRESPONSE, PORT_Memset, (msg, 0, sizeof (LDAPMessage)));

        PKIX_PL_NSSCALLRV(LDAPRESPONSE, rv, SEC_ASN1DecodeItem,
            (NULL, msg, PKIX_PL_LDAPMessageTemplate, &(response->derEncoded)));

        *pStatus = rv;
cleanup:

        PKIX_RETURN(LDAPRESPONSE);
}























PKIX_Error *
pkix_pl_LdapResponse_GetMessage(
        PKIX_PL_LdapResponse *response,
        LDAPMessage **pMessage,
        void *plContext)
{
        PKIX_ENTER(LDAPRESPONSE, "PKIX_PL_LdapResponse_GetMessage");
        PKIX_NULLCHECK_TWO(response, pMessage);

        *pMessage = &response->decoded;

        PKIX_RETURN(LDAPRESPONSE);
}


























PKIX_Error *
pkix_pl_LdapResponse_GetCapacity(
        PKIX_PL_LdapResponse *response,
        PKIX_UInt32 *pCapacity,
        void *plContext)
{
        PKIX_ENTER(LDAPRESPONSE, "PKIX_PL_LdapResponse_GetCapacity");
        PKIX_NULLCHECK_TWO(response, pCapacity);

        *pCapacity = response->totalLength - response->partialLength;

        PKIX_RETURN(LDAPRESPONSE);
}























PKIX_Error *
pkix_pl_LdapResponse_GetMessageType(
        PKIX_PL_LdapResponse *response,
        LDAPMessageType *pMessageType,
        void *plContext)
{
        PKIX_ENTER(LDAPRESPONSE, "PKIX_PL_LdapResponse_GetMessageType");
        PKIX_NULLCHECK_TWO(response, pMessageType);

        *pMessageType = response->decoded.protocolOp.selector;

        PKIX_RETURN(LDAPRESPONSE);
}
























PKIX_Error *
pkix_pl_LdapResponse_GetResultCode(
        PKIX_PL_LdapResponse *response,
        LDAPResultCode *pResultCode,
        void *plContext)
{
        LDAPMessageType messageType = 0;
        LDAPSearchResponseResult *resultMsg = NULL;

        PKIX_ENTER(LDAPRESPONSE, "PKIX_PL_LdapResponse_GetResultCode");
        PKIX_NULLCHECK_TWO(response, pResultCode);

        messageType = response->decoded.protocolOp.selector;

        if (messageType != LDAP_SEARCHRESPONSERESULT_TYPE) {
                PKIX_ERROR(PKIX_GETRESULTCODECALLEDFORNONRESULTMESSAGE);
        }

        resultMsg = &response->decoded.protocolOp.op.searchResponseResultMsg;

        *pResultCode = *(char *)(resultMsg->resultCode.data);

cleanup:

        PKIX_RETURN(LDAPRESPONSE);
}
























PKIX_Error *
pkix_pl_LdapResponse_GetAttributes(
        PKIX_PL_LdapResponse *response,
        LDAPSearchResponseAttr ***pAttributes,
        void *plContext)
{
        LDAPMessageType messageType = 0;

        PKIX_ENTER(LDAPRESPONSE, "PKIX_PL_LdapResponse_GetResultCode");
        PKIX_NULLCHECK_TWO(response, pAttributes);

        messageType = response->decoded.protocolOp.selector;

        if (messageType != LDAP_SEARCHRESPONSEENTRY_TYPE) {
                PKIX_ERROR(PKIX_GETATTRIBUTESCALLEDFORNONENTRYMESSAGE);
        }

        *pAttributes = response->
                decoded.protocolOp.op.searchResponseEntryMsg.attributes;

cleanup:

        PKIX_RETURN(LDAPRESPONSE);
}
