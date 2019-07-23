











































#define MINIMUM_MSG_LENGTH 5

#include "pkix_pl_ldapcertstore.h"





























PKIX_Error *
pkix_pl_LdapCertStore_DecodeCrossCertPair(
        SECItem *derCCPItem,
        PKIX_List *certList,
        void *plContext)
{
        LDAPCertPair certPair = {{ siBuffer, NULL, 0 }, { siBuffer, NULL, 0 }};
        SECStatus rv = SECFailure;

        PRArenaPool *tempArena = NULL;

        PKIX_ENTER(CERTSTORE, "pkix_pl_LdapCertStore_DecodeCrossCertPair");
        PKIX_NULLCHECK_TWO(derCCPItem, certList);

        tempArena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
        if (!tempArena) {
            PKIX_ERROR(PKIX_OUTOFMEMORY);
        }

        rv = SEC_ASN1DecodeItem(tempArena, &certPair, PKIX_PL_LDAPCrossCertPairTemplate,
                                derCCPItem);
        if (rv != SECSuccess) {
                goto cleanup;
        }

        if (certPair.forward.data != NULL) {

            PKIX_CHECK(
                pkix_pl_Cert_CreateToList(&certPair.forward, certList,
                                          plContext),
                PKIX_CERTCREATETOLISTFAILED);
        }

        if (certPair.reverse.data != NULL) {

            PKIX_CHECK(
                pkix_pl_Cert_CreateToList(&certPair.reverse, certList,
                                          plContext),
                PKIX_CERTCREATETOLISTFAILED);
        }

cleanup:
        if (tempArena) {
            PORT_FreeArena(tempArena, PR_FALSE);
        }

        PKIX_RETURN(CERTSTORE);
}
























PKIX_Error *
pkix_pl_LdapCertStore_BuildCertList(
        PKIX_List *responseList,
        PKIX_List **pCerts,
        void *plContext)
{
        PKIX_UInt32 numResponses = 0;
        PKIX_UInt32 respIx = 0;
        LdapAttrMask attrBits = 0;
        PKIX_PL_LdapResponse *response = NULL;
        PKIX_List *certList = NULL;
        LDAPMessage *message = NULL;
        LDAPSearchResponseEntry *sre = NULL;
        LDAPSearchResponseAttr **sreAttrArray = NULL;
        LDAPSearchResponseAttr *sreAttr = NULL;
        SECItem *attrType = NULL;
        SECItem **attrVal = NULL;
        SECItem *derCertItem = NULL;


        PKIX_ENTER(CERTSTORE, "pkix_pl_LdapCertStore_BuildCertList");
        PKIX_NULLCHECK_TWO(responseList, pCerts);

        PKIX_CHECK(PKIX_List_Create(&certList, plContext),
                PKIX_LISTCREATEFAILED);

        
        PKIX_CHECK(PKIX_List_GetLength
                (responseList, &numResponses, plContext),
                PKIX_LISTGETLENGTHFAILED);

        for (respIx = 0; respIx < numResponses; respIx++) {
                PKIX_CHECK(PKIX_List_GetItem
                        (responseList,
                        respIx,
                        (PKIX_PL_Object **)&response,
                        plContext),
                        PKIX_LISTGETITEMFAILED);

                PKIX_CHECK(pkix_pl_LdapResponse_GetMessage
                        (response, &message, plContext),
                        PKIX_LDAPRESPONSEGETMESSAGEFAILED);

                sre = &(message->protocolOp.op.searchResponseEntryMsg);
                sreAttrArray = sre->attributes;

                
                sreAttr = *sreAttrArray++;
                while (sreAttr != NULL) {
                    attrType = &(sreAttr->attrType);
                    PKIX_CHECK(pkix_pl_LdapRequest_AttrTypeToBit
                        (attrType, &attrBits, plContext),
                        PKIX_LDAPREQUESTATTRTYPETOBITFAILED);
                    
                    if (((LDAPATTR_CACERT | LDAPATTR_USERCERT) &
                            attrBits) == attrBits) {
                        attrVal = sreAttr->val;
                        derCertItem = *attrVal++;
                        while (derCertItem != 0) {
                            
                            PKIX_CHECK(pkix_pl_Cert_CreateToList
                                (derCertItem, certList, plContext),
                                PKIX_CERTCREATETOLISTFAILED);
                            derCertItem = *attrVal++;
                        }
                    } else if ((LDAPATTR_CROSSPAIRCERT & attrBits) == attrBits){
                        
                        attrVal = sreAttr->val;
                        derCertItem = *attrVal++;
                        while (derCertItem != 0) {
                            
                            PKIX_CHECK(pkix_pl_LdapCertStore_DecodeCrossCertPair
                                (derCertItem, certList, plContext),
                                PKIX_LDAPCERTSTOREDECODECROSSCERTPAIRFAILED);
                            derCertItem = *attrVal++;
                        }
                    }
                    sreAttr = *sreAttrArray++;
                }
                PKIX_DECREF(response);
        }

        *pCerts = certList;

cleanup:
        if (PKIX_ERROR_RECEIVED) {
                PKIX_DECREF(certList);
        }

        PKIX_DECREF(response);

        PKIX_RETURN(CERTSTORE);
}
























PKIX_Error *
pkix_pl_LdapCertStore_BuildCrlList(
        PKIX_List *responseList,
        PKIX_List **pCrls,
        void *plContext)
{
        PKIX_UInt32 numResponses = 0;
        PKIX_UInt32 respIx = 0;
        LdapAttrMask attrBits = 0;
        CERTSignedCrl *nssCrl = NULL;
        PKIX_PL_LdapResponse *response = NULL;
        PKIX_List *crlList = NULL;
        PKIX_PL_CRL *crl = NULL;
        LDAPMessage *message = NULL;
        LDAPSearchResponseEntry *sre = NULL;
        LDAPSearchResponseAttr **sreAttrArray = NULL;
        LDAPSearchResponseAttr *sreAttr = NULL;
        SECItem *attrType = NULL;
        SECItem **attrVal = NULL;
        SECItem *derCrlCopy = NULL;
        SECItem *derCrlItem = NULL;

        PKIX_ENTER(CERTSTORE, "pkix_pl_LdapCertStore_BuildCrlList");
        PKIX_NULLCHECK_TWO(responseList, pCrls);

        PKIX_CHECK(PKIX_List_Create(&crlList, plContext),
                PKIX_LISTCREATEFAILED);

        
        PKIX_CHECK(PKIX_List_GetLength
                (responseList, &numResponses, plContext),
                PKIX_LISTGETLENGTHFAILED);

        for (respIx = 0; respIx < numResponses; respIx++) {
                PKIX_CHECK(PKIX_List_GetItem
                        (responseList,
                        respIx,
                        (PKIX_PL_Object **)&response,
                        plContext),
                        PKIX_LISTGETITEMFAILED);

                PKIX_CHECK(pkix_pl_LdapResponse_GetMessage
                        (response, &message, plContext),
                        PKIX_LDAPRESPONSEGETMESSAGEFAILED);

                sre = &(message->protocolOp.op.searchResponseEntryMsg);
                sreAttrArray = sre->attributes;

                
                sreAttr = *sreAttrArray++;
                while (sreAttr != NULL) {
                    attrType = &(sreAttr->attrType);
                    PKIX_CHECK(pkix_pl_LdapRequest_AttrTypeToBit
                        (attrType, &attrBits, plContext),
                        PKIX_LDAPREQUESTATTRTYPETOBITFAILED);
                    
                    if (((LDAPATTR_CERTREVLIST | LDAPATTR_AUTHREVLIST) &
                            attrBits) == attrBits) {
                        attrVal = sreAttr->val;
                        derCrlItem = *attrVal++;
                        while (derCrlItem != 0) {
                            
                            derCrlCopy = SECITEM_DupItem(derCrlItem);
                            if (!derCrlCopy) {
                                PKIX_ERROR(PKIX_ALLOCERROR);
                            }
                            

                            nssCrl =
                                CERT_DecodeDERCrlWithFlags(NULL, derCrlCopy,
                                                       SEC_CRL_TYPE,
                                                       CRL_DECODE_DONT_COPY_DER |
                                                       CRL_DECODE_SKIP_ENTRIES);
                            if (!nssCrl) {
                                SECITEM_FreeItem(derCrlCopy, PKIX_TRUE);
                                continue;
                            }
                            
                            PKIX_CHECK(
                                pkix_pl_CRL_CreateWithSignedCRL(nssCrl, 
                                       derCrlCopy, NULL, &crl, plContext),
                                PKIX_CRLCREATEWITHSIGNEDCRLFAILED);
                            

                            derCrlCopy = NULL;
                            nssCrl = NULL;
                            PKIX_CHECK(PKIX_List_AppendItem
                                       (crlList, (PKIX_PL_Object *) crl, plContext),
                                       PKIX_LISTAPPENDITEMFAILED);
                            PKIX_DECREF(crl);
                            derCrlItem = *attrVal++;
                        }
                        
                        pkixTempErrorReceived = PKIX_FALSE;
                    }
                    sreAttr = *sreAttrArray++;
                }
                PKIX_DECREF(response);
        }

        *pCrls = crlList;
        crlList = NULL;
cleanup:
        if (derCrlCopy) {
            SECITEM_FreeItem(derCrlCopy, PKIX_TRUE);
        }
        if (nssCrl) {
            SEC_DestroyCrl(nssCrl);
        }
        PKIX_DECREF(crl);
        PKIX_DECREF(crlList);
        PKIX_DECREF(response);

        PKIX_RETURN(CERTSTORE);
}





















static PKIX_Error *
pkix_pl_LdapCertStore_DestroyAVAList(
        LDAPNameComponent **nameComponents,
        void *plContext)
{
        LDAPNameComponent **currentNC = NULL;
        unsigned char *component = NULL;

        PKIX_ENTER(CERTSTORE, "pkix_pl_LdapCertStore_DestroyAVAList");
        PKIX_NULLCHECK_ONE(nameComponents);

        
        currentNC = nameComponents;

        while ((*currentNC) != NULL) {
                component = (*currentNC)->attrValue;
                if (component != NULL) {
                        PORT_Free(component);
                }
                currentNC++;
        }

        PKIX_RETURN(CERTSTORE);
}


































static PKIX_Error *
pkix_pl_LdapCertStore_MakeNameAVAList(
        PRArenaPool *arena,
        PKIX_PL_X500Name *subjectName, 
        LDAPNameComponent ***pList,
        void *plContext)
{
        LDAPNameComponent **setOfNameComponents;
        LDAPNameComponent *currentNameComponent = NULL;
        PKIX_UInt32 componentsPresent = 0;
        void *v = NULL;
        unsigned char *component = NULL;

        PKIX_ENTER(CERTSTORE, "pkix_pl_LdapCertStore_MakeNameAVAList");
        PKIX_NULLCHECK_THREE(arena, subjectName, pList);

        
#define MAX_NUM_COMPONENTS 3

        
        PKIX_PL_NSSCALLRV(CERTSTORE, v, PORT_ArenaZAlloc,
                (arena, (MAX_NUM_COMPONENTS + 1)*sizeof(LDAPNameComponent *)));
        setOfNameComponents = (LDAPNameComponent **)v;

        
        PKIX_PL_NSSCALLRV(CERTSTORE, v, PORT_ArenaZNewArray,
                (arena, LDAPNameComponent, MAX_NUM_COMPONENTS));

        currentNameComponent = (LDAPNameComponent *)v;

        
        PKIX_CHECK(pkix_pl_X500Name_GetCommonName
                (subjectName, &component, plContext),
                PKIX_X500NAMEGETCOMMONNAMEFAILED);
        if (component) {
                setOfNameComponents[componentsPresent] = currentNameComponent;
                currentNameComponent->attrType = (unsigned char *)"cn";
                currentNameComponent->attrValue = component;
                componentsPresent++;
                currentNameComponent++;
        }

        




#if 0
        
        PKIX_CHECK(pkix_pl_X500Name_GetOrgName
                (subjectName, &component, plContext),
                PKIX_X500NAMEGETORGNAMEFAILED);
        if (component) {
                setOfNameComponents[componentsPresent] = currentNameComponent;
                currentNameComponent->attrType = (unsigned char *)"o";
                currentNameComponent->attrValue = component;
                componentsPresent++;
                currentNameComponent++;
        }

        
        PKIX_CHECK(pkix_pl_X500Name_GetCountryName
                (subjectName, &component, plContext),
                PKIX_X500NAMEGETCOUNTRYNAMEFAILED);
        if (component) {
                setOfNameComponents[componentsPresent] = currentNameComponent;
                currentNameComponent->attrType = (unsigned char *)"c";
                currentNameComponent->attrValue = component;
                componentsPresent++;
                currentNameComponent++;
        }
#endif

        setOfNameComponents[componentsPresent] = NULL;

        *pList = setOfNameComponents;

cleanup:

        PKIX_RETURN(CERTSTORE);

}

#if 0






















PKIX_Error *
pkix_pl_LdapCertStore_ConvertCertResponses(
        PKIX_List *responses,
        PKIX_List **pCerts,
        void *plContext)
{
        PKIX_List *unfiltered = NULL;

        PKIX_ENTER(CERTSTORE, "pkix_pl_LdapCertStore_ConvertCertResponses");
        PKIX_NULLCHECK_TWO(responses, pCerts);

        



        PKIX_CHECK(pkix_pl_LdapCertStore_BuildCertList
                (responses, &unfiltered, plContext),
                PKIX_LDAPCERTSTOREBUILDCERTLISTFAILED);

        *pCerts = unfiltered;

cleanup:

        PKIX_RETURN(CERTSTORE);
}
#endif





PKIX_Error *
pkix_pl_LdapCertStore_GetCert(
        PKIX_CertStore *store,
        PKIX_CertSelector *selector,
        PKIX_VerifyNode *verifyNode,
        void **pNBIOContext,
        PKIX_List **pCertList,
        void *plContext)
{
        PRArenaPool *requestArena = NULL;
        LDAPRequestParams requestParams;
        void *pollDesc = NULL;
        PKIX_Int32 minPathLen = 0;
        PKIX_Boolean cacheFlag = PKIX_FALSE;
        PKIX_ComCertSelParams *params = NULL;
        PKIX_PL_LdapCertStoreContext *lcs = NULL;
        PKIX_List *responses = NULL;
        PKIX_List *unfilteredCerts = NULL;
        PKIX_List *filteredCerts = NULL;
        PKIX_PL_X500Name *subjectName = 0;

        PKIX_ENTER(CERTSTORE, "pkix_pl_LdapCertStore_GetCert");
        PKIX_NULLCHECK_THREE(store, selector, pCertList);

        requestParams.baseObject = "c=US";
        requestParams.scope = WHOLE_SUBTREE;
        requestParams.derefAliases = NEVER_DEREF;
        requestParams.sizeLimit = 0;
        requestParams.timeLimit = 0;

        

        



        requestArena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
        if (!requestArena) {
                PKIX_ERROR_FATAL(PKIX_OUTOFMEMORY);
        }

        PKIX_CHECK(PKIX_CertSelector_GetCommonCertSelectorParams
                (selector, &params, plContext),
                PKIX_CERTSELECTORGETCOMCERTSELPARAMSFAILED);

        



        PKIX_CHECK(PKIX_ComCertSelParams_GetSubject
                (params, &subjectName, plContext),
                PKIX_COMCERTSELPARAMSGETSUBJECTFAILED);

        PKIX_CHECK(PKIX_ComCertSelParams_GetBasicConstraints
                (params, &minPathLen, plContext),
                PKIX_COMCERTSELPARAMSGETBASICCONSTRAINTSFAILED);

        if (subjectName) {
                PKIX_CHECK(pkix_pl_LdapCertStore_MakeNameAVAList
                        (requestArena,
                        subjectName,
                        &(requestParams.nc),
                        plContext),
                        PKIX_LDAPCERTSTOREMAKENAMEAVALISTFAILED);

                if (*requestParams.nc == NULL) {
                        





                        PKIX_PL_NSSCALL(CERTSTORE, PORT_FreeArena,
                                (requestArena, PR_FALSE));

                        PKIX_CHECK(PKIX_List_Create(&filteredCerts, plContext),
                                PKIX_LISTCREATEFAILED);

                        PKIX_CHECK(PKIX_List_SetImmutable
                                (filteredCerts, plContext),
                                PKIX_LISTSETIMMUTABLEFAILED);

                        *pNBIOContext = NULL;
                        *pCertList = filteredCerts;
                        filteredCerts = NULL;
                        goto cleanup;
                }
        } else {
                PKIX_ERROR(PKIX_INSUFFICIENTCRITERIAFORCERTQUERY);
        }

        

        requestParams.attributes = 0;

        if (minPathLen < 0) {
                requestParams.attributes |= LDAPATTR_USERCERT;
        }

        if (minPathLen > -2) {
                requestParams.attributes |=
                        LDAPATTR_CACERT | LDAPATTR_CROSSPAIRCERT;
        }

        

        PKIX_CHECK(PKIX_CertStore_GetCertStoreContext
                (store, (PKIX_PL_Object **)&lcs, plContext),
                PKIX_CERTSTOREGETCERTSTORECONTEXTFAILED);

        PKIX_CHECK(PKIX_PL_LdapClient_InitiateRequest
                ((PKIX_PL_LdapClient *)lcs,
                &requestParams,
                &pollDesc,
                &responses,
                plContext),
                PKIX_LDAPCLIENTINITIATEREQUESTFAILED);

        PKIX_CHECK(pkix_pl_LdapCertStore_DestroyAVAList
                (requestParams.nc, plContext),
                PKIX_LDAPCERTSTOREDESTROYAVALISTFAILED);

        if (requestArena) {
                PKIX_PL_NSSCALL(CERTSTORE, PORT_FreeArena,
                        (requestArena, PR_FALSE));
                requestArena = NULL;
        }

        if (pollDesc != NULL) {
                
                *pNBIOContext = (void *)pollDesc;
                *pCertList = NULL;
                goto cleanup;
        }
        

        if (responses) {
                PKIX_CHECK(PKIX_CertStore_GetCertStoreCacheFlag
                        (store, &cacheFlag, plContext),
                        PKIX_CERTSTOREGETCERTSTORECACHEFLAGFAILED);

                PKIX_CHECK(pkix_pl_LdapCertStore_BuildCertList
                        (responses, &unfilteredCerts, plContext),
                        PKIX_LDAPCERTSTOREBUILDCERTLISTFAILED);

                PKIX_CHECK(pkix_CertSelector_Select
                        (selector, unfilteredCerts, &filteredCerts, plContext),
                        PKIX_CERTSELECTORSELECTFAILED);
        }

        *pNBIOContext = NULL;
        *pCertList = filteredCerts;
        filteredCerts = NULL;

cleanup:

        PKIX_DECREF(params);
        PKIX_DECREF(subjectName);
        PKIX_DECREF(responses);
        PKIX_DECREF(unfilteredCerts);
        PKIX_DECREF(filteredCerts);
        PKIX_DECREF(lcs);

        PKIX_RETURN(CERTSTORE);
}





PKIX_Error *
pkix_pl_LdapCertStore_GetCertContinue(
        PKIX_CertStore *store,
        PKIX_CertSelector *selector,
        PKIX_VerifyNode *verifyNode,
        void **pNBIOContext,
        PKIX_List **pCertList,
        void *plContext)
{
        PKIX_Boolean cacheFlag = PKIX_FALSE;
        PKIX_PL_LdapCertStoreContext *lcs = NULL;
        void *pollDesc = NULL;
        PKIX_List *responses = NULL;
        PKIX_List *unfilteredCerts = NULL;
        PKIX_List *filteredCerts = NULL;

        PKIX_ENTER(CERTSTORE, "pkix_pl_LdapCertStore_GetCertContinue");
        PKIX_NULLCHECK_THREE(store, selector, pCertList);

        PKIX_CHECK(PKIX_CertStore_GetCertStoreContext
                (store, (PKIX_PL_Object **)&lcs, plContext),
                PKIX_CERTSTOREGETCERTSTORECONTEXTFAILED);

        PKIX_CHECK(PKIX_PL_LdapClient_ResumeRequest
                ((PKIX_PL_LdapClient *)lcs, &pollDesc, &responses, plContext),
                PKIX_LDAPCLIENTRESUMEREQUESTFAILED);

        if (pollDesc != NULL) {
                
                *pNBIOContext = (void *)pollDesc;
                *pCertList = NULL;
                goto cleanup;
        }
        

        if (responses) {
                PKIX_CHECK(PKIX_CertStore_GetCertStoreCacheFlag
                        (store, &cacheFlag, plContext),
                        PKIX_CERTSTOREGETCERTSTORECACHEFLAGFAILED);

                PKIX_CHECK(pkix_pl_LdapCertStore_BuildCertList
                        (responses, &unfilteredCerts, plContext),
                        PKIX_LDAPCERTSTOREBUILDCERTLISTFAILED);

                PKIX_CHECK(pkix_CertSelector_Select
                        (selector, unfilteredCerts, &filteredCerts, plContext),
                        PKIX_CERTSELECTORSELECTFAILED);
        }

        *pNBIOContext = NULL;
        *pCertList = filteredCerts;

cleanup:

        PKIX_DECREF(responses);
        PKIX_DECREF(unfilteredCerts);
        PKIX_DECREF(lcs);

        PKIX_RETURN(CERTSTORE);
}





PKIX_Error *
pkix_pl_LdapCertStore_GetCRL(
        PKIX_CertStore *store,
        PKIX_CRLSelector *selector,
        void **pNBIOContext,
        PKIX_List **pCrlList,
        void *plContext)
{
        LDAPRequestParams requestParams;
        void *pollDesc = NULL;
        PRArenaPool *requestArena = NULL;
        PKIX_UInt32 numNames = 0;
        PKIX_UInt32 thisName = 0;
        PKIX_PL_CRL *candidate = NULL;
        PKIX_List *responses = NULL;
        PKIX_List *issuerNames = NULL;
        PKIX_List *filteredCRLs = NULL;
        PKIX_List *unfilteredCRLs = NULL;
        PKIX_PL_X500Name *issuer = NULL;
        PKIX_PL_LdapCertStoreContext *lcs = NULL;
        PKIX_ComCRLSelParams *params = NULL;

        PKIX_ENTER(CERTSTORE, "pkix_pl_LdapCertStore_GetCRL");
        PKIX_NULLCHECK_THREE(store, selector, pCrlList);

        requestParams.baseObject = "c=US";
        requestParams.scope = WHOLE_SUBTREE;
        requestParams.derefAliases = NEVER_DEREF;
        requestParams.sizeLimit = 0;
        requestParams.timeLimit = 0;
        requestParams.attributes = LDAPATTR_CERTREVLIST | LDAPATTR_AUTHREVLIST;
        

        
        
        
        
        
        
        
        
        
        



        



        PKIX_PL_NSSCALLRV
            (CERTSTORE, requestArena, PORT_NewArena, (DER_DEFAULT_CHUNKSIZE));

        if (!requestArena) {
                PKIX_ERROR_FATAL(PKIX_OUTOFMEMORY);
        }

        PKIX_CHECK(PKIX_CRLSelector_GetCommonCRLSelectorParams
                (selector, &params, plContext),
                PKIX_CRLSELECTORGETCOMCERTSELPARAMSFAILED);

        PKIX_CHECK(PKIX_ComCRLSelParams_GetIssuerNames
                (params, &issuerNames, plContext),
                PKIX_COMCRLSELPARAMSGETISSUERNAMESFAILED);

        





        if (issuerNames) {

                PKIX_CHECK(PKIX_List_GetLength
                        (issuerNames, &numNames, plContext),
                        PKIX_LISTGETLENGTHFAILED);

                if (numNames > 0) {
                        for (thisName = 0; thisName < numNames; thisName++) {
                                PKIX_CHECK(PKIX_List_GetItem
                                (issuerNames,
                                thisName,
                                (PKIX_PL_Object **)&issuer,
                                plContext),
                                PKIX_LISTGETITEMFAILED);

                                PKIX_CHECK
                                        (pkix_pl_LdapCertStore_MakeNameAVAList
                                        (requestArena,
                                        issuer,
                                        &(requestParams.nc),
                                        plContext),
                                        PKIX_LDAPCERTSTOREMAKENAMEAVALISTFAILED);

                                PKIX_DECREF(issuer);

                                if (*requestParams.nc == NULL) {
                                        







                                        PKIX_PL_NSSCALL
                                                (CERTSTORE, PORT_FreeArena,
                                                (requestArena, PR_FALSE));

                                        PKIX_CHECK(PKIX_List_Create
                                                (&filteredCRLs, plContext),
                                                PKIX_LISTCREATEFAILED);

                                        PKIX_CHECK(PKIX_List_SetImmutable
                                                (filteredCRLs, plContext),
                                               PKIX_LISTSETIMMUTABLEFAILED);

                                        *pNBIOContext = NULL;
                                        *pCrlList = filteredCRLs;
                                        goto cleanup;
                                }

                                



                                break;
                        }
                } else {
                        PKIX_ERROR(PKIX_IMPOSSIBLECRITERIONFORCRLQUERY);
                }
        } else {
                PKIX_ERROR(PKIX_IMPOSSIBLECRITERIONFORCRLQUERY);
        }

        

        PKIX_CHECK(PKIX_CertStore_GetCertStoreContext
                (store, (PKIX_PL_Object **)&lcs, plContext),
                PKIX_CERTSTOREGETCERTSTORECONTEXTFAILED);

        PKIX_CHECK(PKIX_PL_LdapClient_InitiateRequest
                ((PKIX_PL_LdapClient *)lcs,
                &requestParams,
                &pollDesc,
                &responses,
                plContext),
                PKIX_LDAPCLIENTINITIATEREQUESTFAILED);

        PKIX_CHECK(pkix_pl_LdapCertStore_DestroyAVAList
                (requestParams.nc, plContext),
                PKIX_LDAPCERTSTOREDESTROYAVALISTFAILED);

        if (requestArena) {
                PKIX_PL_NSSCALL(CERTSTORE, PORT_FreeArena,
                        (requestArena, PR_FALSE));
        }

        if (pollDesc != NULL) {
                
                *pNBIOContext = (void *)pollDesc;
                *pCrlList = NULL;
                goto cleanup;
        }
        

        if (responses) {

                



                PKIX_CHECK(pkix_pl_LdapCertStore_BuildCrlList
                        (responses, &unfilteredCRLs, plContext),
                        PKIX_LDAPCERTSTOREBUILDCRLLISTFAILED);

                PKIX_CHECK(pkix_CRLSelector_Select
                        (selector, unfilteredCRLs, &filteredCRLs, plContext),
                        PKIX_CRLSELECTORSELECTFAILED);

        }

        
        pkixTempErrorReceived = PKIX_FALSE;

        *pNBIOContext = NULL;
        *pCrlList = filteredCRLs;

cleanup:

        if (PKIX_ERROR_RECEIVED) {
                PKIX_DECREF(filteredCRLs);
        }

        PKIX_DECREF(params);
        PKIX_DECREF(issuerNames);
        PKIX_DECREF(issuer);
        PKIX_DECREF(candidate);
        PKIX_DECREF(responses);
        PKIX_DECREF(unfilteredCRLs);
        PKIX_DECREF(lcs);

        PKIX_RETURN(CERTSTORE);
}





PKIX_Error *
pkix_pl_LdapCertStore_GetCRLContinue(
        PKIX_CertStore *store,
        PKIX_CRLSelector *selector,
        void **pNBIOContext,
        PKIX_List **pCrlList,
        void *plContext)
{
        void *nbio = NULL;
        PKIX_PL_CRL *candidate = NULL;
        PKIX_List *responses = NULL;
        PKIX_PL_LdapCertStoreContext *lcs = NULL;
        PKIX_List *filteredCRLs = NULL;
        PKIX_List *unfilteredCRLs = NULL;

        PKIX_ENTER(CERTSTORE, "pkix_pl_LdapCertStore_GetCRLContinue");
        PKIX_NULLCHECK_FOUR(store, selector, pNBIOContext, pCrlList);

        PKIX_CHECK(PKIX_CertStore_GetCertStoreContext
                (store, (PKIX_PL_Object **)&lcs, plContext),
                PKIX_CERTSTOREGETCERTSTORECONTEXTFAILED);

        PKIX_CHECK(PKIX_PL_LdapClient_ResumeRequest
                ((PKIX_PL_LdapClient *)lcs, &nbio, &responses, plContext),
                PKIX_LDAPCLIENTRESUMEREQUESTFAILED);

        if (nbio != NULL) {
                
                *pNBIOContext = (void *)nbio;
                *pCrlList = NULL;
                goto cleanup;
        }
        

        if (responses) {

                



                PKIX_CHECK(pkix_pl_LdapCertStore_BuildCrlList
                        (responses, &unfilteredCRLs, plContext),
                        PKIX_LDAPCERTSTOREBUILDCRLLISTFAILED);

                PKIX_CHECK(pkix_CRLSelector_Select
                        (selector, unfilteredCRLs, &filteredCRLs, plContext),
                        PKIX_CRLSELECTORSELECTFAILED);

                PKIX_CHECK(PKIX_List_SetImmutable(filteredCRLs, plContext),
                        PKIX_LISTSETIMMUTABLEFAILED);

        }

        
        pkixTempErrorReceived = PKIX_FALSE;

        *pCrlList = filteredCRLs;

cleanup:
        if (PKIX_ERROR_RECEIVED) {
                PKIX_DECREF(filteredCRLs);
        }

        PKIX_DECREF(candidate);
        PKIX_DECREF(responses);
        PKIX_DECREF(unfilteredCRLs);
        PKIX_DECREF(lcs);

        PKIX_RETURN(CERTSTORE);
}







PKIX_Error *
PKIX_PL_LdapCertStore_Create(
        PKIX_PL_LdapClient *client,
        PKIX_CertStore **pCertStore,
        void *plContext)
{
        PKIX_CertStore *certStore = NULL;

        PKIX_ENTER(CERTSTORE, "PKIX_PL_LdapCertStore_Create");
        PKIX_NULLCHECK_TWO(client, pCertStore);

        PKIX_CHECK(PKIX_CertStore_Create
                (pkix_pl_LdapCertStore_GetCert,
                pkix_pl_LdapCertStore_GetCRL,
                pkix_pl_LdapCertStore_GetCertContinue,
                pkix_pl_LdapCertStore_GetCRLContinue,
                NULL,       
                NULL,      
                NULL,      
                (PKIX_PL_Object *)client,
                PKIX_TRUE,  
                PKIX_FALSE, 
                &certStore,
                plContext),
                PKIX_CERTSTORECREATEFAILED);

        *pCertStore = certStore;

cleanup:

        PKIX_RETURN(CERTSTORE);
}
