







#include "pkix_pl_ldaprequest.h"




static const char caAttr[] = "caCertificate;binary";
static unsigned int caAttrLen = sizeof(caAttr) - 1;
static const char uAttr[] = "userCertificate;binary";
static unsigned int uAttrLen = sizeof(uAttr) - 1;
static const char ccpAttr[] = "crossCertificatePair;binary";
static unsigned int ccpAttrLen = sizeof(ccpAttr) - 1;
static const char crlAttr[] = "certificateRevocationList;binary";
static unsigned int crlAttrLen = sizeof(crlAttr) - 1;
static const char arlAttr[] = "authorityRevocationList;binary";
static unsigned int arlAttrLen = sizeof(arlAttr) - 1;

































PKIX_Error *
pkix_pl_LdapRequest_AttrTypeToBit(
        SECItem *attrType,
        LdapAttrMask *pAttrBit,
        void *plContext)
{
        LdapAttrMask attrBit = 0;
        unsigned int attrLen = 0;
        const char *s = NULL;

        PKIX_ENTER(LDAPREQUEST, "pkix_pl_LdapRequest_AttrTypeToBit");
        PKIX_NULLCHECK_TWO(attrType, pAttrBit);

        s = (const char *)attrType->data;
        attrLen = attrType->len;

        







        if (attrLen == caAttrLen) {
                if (PORT_Strncasecmp(caAttr, s, attrLen) == 0) {
                        attrBit = LDAPATTR_CACERT;
                }
        } else if (attrLen == uAttrLen) {
                if (PORT_Strncasecmp(uAttr, s, attrLen) == 0) {
                        attrBit = LDAPATTR_USERCERT;
                }
        } else if (attrLen == ccpAttrLen) {
                if (PORT_Strncasecmp(ccpAttr, s, attrLen) == 0) {
                        attrBit = LDAPATTR_CROSSPAIRCERT;
                }
        } else if (attrLen == crlAttrLen) {
                if (PORT_Strncasecmp(crlAttr, s, attrLen) == 0) {
                        attrBit = LDAPATTR_CERTREVLIST;
                }
        } else if (attrLen == arlAttrLen) {
                if (PORT_Strncasecmp(arlAttr, s, attrLen) == 0) {
                        attrBit = LDAPATTR_AUTHREVLIST;
                }
        }

        *pAttrBit = attrBit;

        PKIX_RETURN(LDAPREQUEST);
}

























PKIX_Error *
pkix_pl_LdapRequest_AttrStringToBit(
        char *attrString,
        LdapAttrMask *pAttrBit,
        void *plContext)
{
        LdapAttrMask attrBit = 0;
        unsigned int attrLen = 0;

        PKIX_ENTER(LDAPREQUEST, "pkix_pl_LdapRequest_AttrStringToBit");
        PKIX_NULLCHECK_TWO(attrString, pAttrBit);

        attrLen = PL_strlen(attrString);

        







        if (attrLen == caAttrLen) {
                if (PORT_Strncasecmp(caAttr, attrString, attrLen) == 0) {
                        attrBit = LDAPATTR_CACERT;
                }
        } else if (attrLen == uAttrLen) {
                if (PORT_Strncasecmp(uAttr, attrString, attrLen) == 0) {
                        attrBit = LDAPATTR_USERCERT;
                }
        } else if (attrLen == ccpAttrLen) {
                if (PORT_Strncasecmp(ccpAttr, attrString, attrLen) == 0) {
                        attrBit = LDAPATTR_CROSSPAIRCERT;
                }
        } else if (attrLen == crlAttrLen) {
                if (PORT_Strncasecmp(crlAttr, attrString, attrLen) == 0) {
                        attrBit = LDAPATTR_CERTREVLIST;
                }
        } else if (attrLen == arlAttrLen) {
                if (PORT_Strncasecmp(arlAttr, attrString, attrLen) == 0) {
                        attrBit = LDAPATTR_AUTHREVLIST;
                }
        }

        *pAttrBit = attrBit;

        PKIX_RETURN(LDAPREQUEST);
}






















static PKIX_Error *
pkix_pl_LdapRequest_EncodeAttrs(
        PKIX_PL_LdapRequest *request,
        void *plContext)
{
        SECItem **attrArray = NULL;
        PKIX_UInt32 attrIndex = 0;
        LdapAttrMask attrBits;

        PKIX_ENTER(LDAPREQUEST, "pkix_pl_LdapRequest_EncodeAttrs");
        PKIX_NULLCHECK_ONE(request);

        
        attrBits = request->attrBits;
        attrArray = request->attrArray;
        if ((attrBits & LDAPATTR_CACERT) == LDAPATTR_CACERT) {
                attrArray[attrIndex] = &(request->attributes[attrIndex]);
                request->attributes[attrIndex].type = siAsciiString;
                request->attributes[attrIndex].data = (unsigned char *)caAttr;
                request->attributes[attrIndex].len = caAttrLen;
                attrIndex++;
        }
        if ((attrBits & LDAPATTR_USERCERT) == LDAPATTR_USERCERT) {
                attrArray[attrIndex] = &(request->attributes[attrIndex]);
                request->attributes[attrIndex].type = siAsciiString;
                request->attributes[attrIndex].data = (unsigned char *)uAttr;
                request->attributes[attrIndex].len = uAttrLen;
                attrIndex++;
        }
        if ((attrBits & LDAPATTR_CROSSPAIRCERT) == LDAPATTR_CROSSPAIRCERT) {
                attrArray[attrIndex] = &(request->attributes[attrIndex]);
                request->attributes[attrIndex].type = siAsciiString;
                request->attributes[attrIndex].data = (unsigned char *)ccpAttr;
                request->attributes[attrIndex].len = ccpAttrLen;
                attrIndex++;
        }
        if ((attrBits & LDAPATTR_CERTREVLIST) == LDAPATTR_CERTREVLIST) {
                attrArray[attrIndex] = &(request->attributes[attrIndex]);
                request->attributes[attrIndex].type = siAsciiString;
                request->attributes[attrIndex].data = (unsigned char *)crlAttr;
                request->attributes[attrIndex].len = crlAttrLen;
                attrIndex++;
        }
        if ((attrBits & LDAPATTR_AUTHREVLIST) == LDAPATTR_AUTHREVLIST) {
                attrArray[attrIndex] = &(request->attributes[attrIndex]);
                request->attributes[attrIndex].type = siAsciiString;
                request->attributes[attrIndex].data = (unsigned char *)arlAttr;
                request->attributes[attrIndex].len = arlAttrLen;
                attrIndex++;
        }
        attrArray[attrIndex] = (SECItem *)NULL;

        PKIX_RETURN(LDAPREQUEST);
}





static PKIX_Error *
pkix_pl_LdapRequest_Destroy(
        PKIX_PL_Object *object,
        void *plContext)
{
        PKIX_PL_LdapRequest *ldapRq = NULL;

        PKIX_ENTER(LDAPREQUEST, "pkix_pl_LdapRequest_Destroy");
        PKIX_NULLCHECK_ONE(object);

        PKIX_CHECK(pkix_CheckType(object, PKIX_LDAPREQUEST_TYPE, plContext),
                    PKIX_OBJECTNOTLDAPREQUEST);

        ldapRq = (PKIX_PL_LdapRequest *)object;

        




cleanup:

        PKIX_RETURN(LDAPREQUEST);
}





static PKIX_Error *
pkix_pl_LdapRequest_Hashcode(
        PKIX_PL_Object *object,
        PKIX_UInt32 *pHashcode,
        void *plContext)
{
        PKIX_UInt32 dataLen = 0;
        PKIX_UInt32 dindex = 0;
        PKIX_UInt32 sizeOfLength = 0;
        PKIX_UInt32 idLen = 0;
        const unsigned char *msgBuf = NULL;
        PKIX_PL_LdapRequest *ldapRq = NULL;

        PKIX_ENTER(LDAPREQUEST, "pkix_pl_LdapRequest_Hashcode");
        PKIX_NULLCHECK_TWO(object, pHashcode);

        PKIX_CHECK(pkix_CheckType(object, PKIX_LDAPREQUEST_TYPE, plContext),
                    PKIX_OBJECTNOTLDAPREQUEST);

        ldapRq = (PKIX_PL_LdapRequest *)object;

        *pHashcode = 0;

        



        if (ldapRq->encoded) {
                msgBuf = (const unsigned char *)ldapRq->encoded->data;
                
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

        PKIX_RETURN(LDAPREQUEST);

}





static PKIX_Error *
pkix_pl_LdapRequest_Equals(
        PKIX_PL_Object *firstObj,
        PKIX_PL_Object *secondObj,
        PKIX_Boolean *pResult,
        void *plContext)
{
        PKIX_PL_LdapRequest *firstReq = NULL;
        PKIX_PL_LdapRequest *secondReq = NULL;
        PKIX_UInt32 secondType = 0;
        PKIX_UInt32 firstLen = 0;
        const unsigned char *firstData = NULL;
        const unsigned char *secondData = NULL;
        PKIX_UInt32 sizeOfLength = 0;
        PKIX_UInt32 dindex = 0;
        PKIX_UInt32 i = 0;

        PKIX_ENTER(LDAPREQUEST, "pkix_pl_LdapRequest_Equals");
        PKIX_NULLCHECK_THREE(firstObj, secondObj, pResult);

        
        PKIX_CHECK(pkix_CheckType(firstObj, PKIX_LDAPREQUEST_TYPE, plContext),
                    PKIX_FIRSTOBJARGUMENTNOTLDAPREQUEST);

        



        if (firstObj == secondObj){
                *pResult = PKIX_TRUE;
                goto cleanup;
        }

        



        *pResult = PKIX_FALSE;
        PKIX_CHECK(PKIX_PL_Object_GetType
                    (secondObj, &secondType, plContext),
                    PKIX_COULDNOTGETTYPEOFSECONDARGUMENT);
        if (secondType != PKIX_LDAPREQUEST_TYPE) {
                goto cleanup;
        }

        firstReq = (PKIX_PL_LdapRequest *)firstObj;
        secondReq = (PKIX_PL_LdapRequest *)secondObj;

        
        if (!(firstReq->encoded) || !(secondReq->encoded)) {
                goto cleanup;
        }

        if (firstReq->encoded->len != secondReq->encoded->len) {
                goto cleanup;
        }

        firstData = (const unsigned char *)firstReq->encoded->data;
        secondData = (const unsigned char *)secondReq->encoded->data;

        




        
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

        PKIX_RETURN(LDAPREQUEST);
}
















PKIX_Error *
pkix_pl_LdapRequest_RegisterSelf(void *plContext)
{
        extern pkix_ClassTable_Entry systemClasses[PKIX_NUMTYPES];
        pkix_ClassTable_Entry entry;

        PKIX_ENTER(LDAPREQUEST, "pkix_pl_LdapRequest_RegisterSelf");

        entry.description = "LdapRequest";
        entry.objCounter = 0;
        entry.typeObjectSize = sizeof(PKIX_PL_LdapRequest);
        entry.destructor = pkix_pl_LdapRequest_Destroy;
        entry.equalsFunction = pkix_pl_LdapRequest_Equals;
        entry.hashcodeFunction = pkix_pl_LdapRequest_Hashcode;
        entry.toStringFunction = NULL;
        entry.comparator = NULL;
        entry.duplicateFunction = pkix_duplicateImmutable;

        systemClasses[PKIX_LDAPREQUEST_TYPE] = entry;

        PKIX_RETURN(LDAPREQUEST);
}































































































































PKIX_Error *
pkix_pl_LdapRequest_Create(
        PRArenaPool *arena,
        PKIX_UInt32 msgnum,
        char *issuerDN,
        ScopeType scope,
        DerefType derefAliases,
        PKIX_UInt32 sizeLimit,
        PKIX_UInt32 timeLimit,
        char attrsOnly,
        LDAPFilter *filter,
        LdapAttrMask attrBits,
        PKIX_PL_LdapRequest **pRequestMsg,
        void *plContext)
{
        LDAPMessage msg;
        LDAPSearch *search;
        PKIX_PL_LdapRequest *ldapRequest = NULL;
        char scopeTypeAsChar;
        char derefAliasesTypeAsChar;
        SECItem *attrArray[MAX_LDAPATTRS + 1];

        PKIX_ENTER(LDAPREQUEST, "pkix_pl_LdapRequest_Create");
        PKIX_NULLCHECK_THREE(arena, issuerDN, pRequestMsg);

        
        PKIX_CHECK(PKIX_PL_Object_Alloc
                    (PKIX_LDAPREQUEST_TYPE,
                    sizeof (PKIX_PL_LdapRequest),
                    (PKIX_PL_Object **)&ldapRequest,
                    plContext),
                    PKIX_COULDNOTCREATEOBJECT);

        ldapRequest->arena = arena;
        ldapRequest->msgnum = msgnum;
        ldapRequest->issuerDN = issuerDN;
        ldapRequest->scope = scope;
        ldapRequest->derefAliases = derefAliases;
        ldapRequest->sizeLimit = sizeLimit;
        ldapRequest->timeLimit = timeLimit;
        ldapRequest->attrsOnly = attrsOnly;
        ldapRequest->filter = filter;
        ldapRequest->attrBits = attrBits;

        ldapRequest->attrArray = attrArray;

        PKIX_CHECK(pkix_pl_LdapRequest_EncodeAttrs
                (ldapRequest, plContext),
                PKIX_LDAPREQUESTENCODEATTRSFAILED);

        PKIX_PL_NSSCALL
                (LDAPREQUEST, PORT_Memset, (&msg, 0, sizeof (LDAPMessage)));

        msg.messageID.type = siUnsignedInteger;
        msg.messageID.data = (void*)&msgnum;
        msg.messageID.len = sizeof (msgnum);

        msg.protocolOp.selector = LDAP_SEARCH_TYPE;

        search = &(msg.protocolOp.op.searchMsg);

        search->baseObject.type = siAsciiString;
        search->baseObject.data = (void *)issuerDN;
        search->baseObject.len = PL_strlen(issuerDN);
        scopeTypeAsChar = (char)scope;
        search->scope.type = siUnsignedInteger;
        search->scope.data = (void *)&scopeTypeAsChar;
        search->scope.len = sizeof (scopeTypeAsChar);
        derefAliasesTypeAsChar = (char)derefAliases;
        search->derefAliases.type = siUnsignedInteger;
        search->derefAliases.data =
                (void *)&derefAliasesTypeAsChar;
        search->derefAliases.len =
                sizeof (derefAliasesTypeAsChar);
        search->sizeLimit.type = siUnsignedInteger;
        search->sizeLimit.data = (void *)&sizeLimit;
        search->sizeLimit.len = sizeof (PKIX_UInt32);
        search->timeLimit.type = siUnsignedInteger;
        search->timeLimit.data = (void *)&timeLimit;
        search->timeLimit.len = sizeof (PKIX_UInt32);
        search->attrsOnly.type = siBuffer;
        search->attrsOnly.data = (void *)&attrsOnly;
        search->attrsOnly.len = sizeof (attrsOnly);

        PKIX_PL_NSSCALL
                (LDAPREQUEST,
                PORT_Memcpy,
                (&search->filter, filter, sizeof (LDAPFilter)));

        search->attributes = attrArray;

        PKIX_PL_NSSCALLRV
                (LDAPREQUEST, ldapRequest->encoded, SEC_ASN1EncodeItem,
                (arena, NULL, (void *)&msg, PKIX_PL_LDAPMessageTemplate));

        if (!(ldapRequest->encoded)) {
                PKIX_ERROR(PKIX_FAILEDINENCODINGSEARCHREQUEST);
        }

        *pRequestMsg = ldapRequest;

cleanup:

        if (PKIX_ERROR_RECEIVED) {
                PKIX_DECREF(ldapRequest);
        }

        PKIX_RETURN(LDAPREQUEST);
}
























PKIX_Error *
pkix_pl_LdapRequest_GetEncoded(
        PKIX_PL_LdapRequest *request,
        SECItem **pRequestBuf,
        void *plContext)
{
        PKIX_ENTER(LDAPREQUEST, "pkix_pl_LdapRequest_GetEncoded");
        PKIX_NULLCHECK_TWO(request, pRequestBuf);

        *pRequestBuf = request->encoded;

        PKIX_RETURN(LDAPREQUEST);
}
