










































#include "pkix_pl_aiamgr.h"
extern PKIX_PL_HashTable *aiaConnectionCache;



PKIX_Error *
PKIX_PL_LdapClient_InitiateRequest(
        PKIX_PL_LdapClient *client,
        LDAPRequestParams *requestParams,
        void **pNBIO,
        PKIX_List **pResponse,
        void *plContext)
{
        PKIX_ENTER(LDAPCLIENT, "PKIX_PL_LdapClient_InitiateRequest");
        PKIX_NULLCHECK_TWO(client, client->initiateFcn);

        PKIX_CHECK(client->initiateFcn
                (client, requestParams, pNBIO, pResponse, plContext),
                PKIX_LDAPCLIENTINITIATEREQUESTFAILED);
cleanup:

        PKIX_RETURN(LDAPCLIENT);

}

PKIX_Error *
PKIX_PL_LdapClient_ResumeRequest(
        PKIX_PL_LdapClient *client,
        void **pNBIO,
        PKIX_List **pResponse,
        void *plContext)
{
        PKIX_ENTER(LDAPCLIENT, "PKIX_PL_LdapClient_ResumeRequest");
        PKIX_NULLCHECK_TWO(client, client->resumeFcn);

        PKIX_CHECK(client->resumeFcn
                (client, pNBIO, pResponse, plContext),
                PKIX_LDAPCLIENTRESUMEREQUESTFAILED);
cleanup:

        PKIX_RETURN(LDAPCLIENT);

}







static PKIX_Error *
pkix_pl_AIAMgr_Destroy(
        PKIX_PL_Object *object,
        void *plContext)
{
        PKIX_PL_AIAMgr *aiaMgr = NULL;

        PKIX_ENTER(AIAMGR, "pkix_pl_AIAMgr_Destroy");
        PKIX_NULLCHECK_ONE(object);

        PKIX_CHECK(pkix_CheckType(object, PKIX_AIAMGR_TYPE, plContext),
                PKIX_OBJECTNOTAIAMGR);

        aiaMgr = (PKIX_PL_AIAMgr *)object;

        
        
        aiaMgr->method = 0;
        aiaMgr->aiaIndex = 0;
        aiaMgr->numAias = 0;
        PKIX_DECREF(aiaMgr->aia);
        PKIX_DECREF(aiaMgr->location);
        PKIX_DECREF(aiaMgr->results);
        PKIX_DECREF(aiaMgr->client.ldapClient);

cleanup:

        PKIX_RETURN(AIAMGR);
}












PKIX_Error *
pkix_pl_AIAMgr_RegisterSelf(void *plContext)
{
        extern pkix_ClassTable_Entry systemClasses[PKIX_NUMTYPES];
        pkix_ClassTable_Entry *entry = &systemClasses[PKIX_AIAMGR_TYPE];

        PKIX_ENTER(AIAMGR, "pkix_pl_AIAMgr_RegisterSelf");

        entry->description = "AIAMgr";
        entry->typeObjectSize = sizeof(PKIX_PL_AIAMgr);
        entry->destructor = pkix_pl_AIAMgr_Destroy;

        PKIX_RETURN(AIAMGR);
}





























static PKIX_Error *
pkix_pl_AiaMgr_FindLDAPClient(
        PKIX_PL_AIAMgr *aiaMgr,
        char *domainName,
        PKIX_PL_LdapClient **pClient,
        void *plContext)
{
        PKIX_PL_String *domainString = NULL;
        PKIX_PL_LdapDefaultClient *client = NULL;

        PKIX_ENTER(AIAMGR, "pkix_pl_AiaMgr_FindLDAPClient");
        PKIX_NULLCHECK_THREE(aiaMgr, domainName, pClient);

        
        PKIX_CHECK(PKIX_PL_String_Create
                (PKIX_ESCASCII, domainName, 0, &domainString, plContext),
                PKIX_STRINGCREATEFAILED);

        
        PKIX_CHECK(PKIX_PL_HashTable_Lookup
                (aiaConnectionCache,
                (PKIX_PL_Object *)domainString,
                (PKIX_PL_Object **)&client,
                plContext),
                PKIX_HASHTABLELOOKUPFAILED);

        if (client == NULL) {

                
                PKIX_CHECK(PKIX_PL_LdapDefaultClient_CreateByName
                        (domainName,
                         

                        PR_SecondsToInterval(
                            ((PKIX_PL_NssContext*)plContext)->timeoutSeconds),
                        NULL,
                        &client,
                        plContext),
                        PKIX_LDAPDEFAULTCLIENTCREATEBYNAMEFAILED);

                PKIX_CHECK(PKIX_PL_HashTable_Add
                        (aiaConnectionCache,
                        (PKIX_PL_Object *)domainString,
                        (PKIX_PL_Object *)client,
                        plContext),
                        PKIX_HASHTABLEADDFAILED);

        }

        *pClient = (PKIX_PL_LdapClient *)client;

cleanup:

        PKIX_DECREF(domainString);

        PKIX_RETURN(AIAMGR);
}

PKIX_Error *
pkix_pl_AIAMgr_GetHTTPCerts(
        PKIX_PL_AIAMgr *aiaMgr,
	PKIX_PL_InfoAccess *ia,
	void **pNBIOContext,
	PKIX_List **pCerts,
        void *plContext)
{
        PKIX_PL_GeneralName *location = NULL;
        PKIX_PL_String *locationString = NULL;
	PKIX_UInt32 len = 0;
	PRUint16 port = 0;
	const SEC_HttpClientFcn *httpClient = NULL;
	const SEC_HttpClientFcnV1 *hcv1 = NULL;
	SECStatus rv = SECFailure;
	SEC_HTTP_SERVER_SESSION serverSession = NULL;
	SEC_HTTP_REQUEST_SESSION requestSession = NULL;	
	char *path = NULL;
	char *hostname = NULL;
	char *locationAscii = NULL;
	void *nbio = NULL;
	PRUint16 responseCode = 0;
	const char *responseContentType = NULL;
	const char *responseData = NULL;

        PKIX_ENTER(AIAMGR, "pkix_pl_AIAMgr_GetHTTPCerts");
        PKIX_NULLCHECK_FOUR(aiaMgr, ia, pNBIOContext, pCerts);

        nbio = *pNBIOContext;
        *pNBIOContext = NULL;
        *pCerts = NULL;

        if (nbio == NULL) { 

                PKIX_CHECK(PKIX_PL_InfoAccess_GetLocation
                        (ia, &location, plContext),
                       PKIX_INFOACCESSGETLOCATIONFAILED);

                
		httpClient = SEC_GetRegisteredHttpClient();
		aiaMgr->client.hdata.httpClient = httpClient;
		if (!httpClient)
		    PKIX_ERROR(PKIX_OUTOFMEMORY);

		if (httpClient->version == 1) {

                        PKIX_UInt32 timeout =
                             ((PKIX_PL_NssContext*)plContext)->timeoutSeconds;

			hcv1 = &(httpClient->fcnTable.ftable1);

			
			PKIX_TOSTRING(location, &locationString, plContext,
				PKIX_GENERALNAMETOSTRINGFAILED);

			PKIX_CHECK(PKIX_PL_String_GetEncoded
				(locationString,
				PKIX_ESCASCII,
				(void **)&locationAscii,
				&len,
				plContext),
				PKIX_STRINGGETENCODEDFAILED);

                        rv = CERT_ParseURL(locationAscii, &hostname, &port,
                                            &path);
			if ((rv != SECSuccess) ||
			    (hostname == NULL) ||
			    (path == NULL)) {
				PKIX_ERROR(PKIX_URLPARSINGFAILED);
			}

                        rv = (*hcv1->createSessionFcn)(hostname, port, 
                                                       &serverSession);
	                if (rv != SECSuccess) {
				PKIX_ERROR(PKIX_HTTPCLIENTCREATESESSIONFAILED);
			}

			aiaMgr->client.hdata.serverSession = serverSession;

			
                        rv = (*hcv1->createFcn)(serverSession, "http", path,
                        	"GET", PR_SecondsToInterval(timeout),
                                 &requestSession);
                	if (rv != SECSuccess) {
                        	PKIX_ERROR(PKIX_HTTPSERVERERROR);
                	}

			aiaMgr->client.hdata.requestSession = requestSession;
		} else {
			PKIX_ERROR(PKIX_UNSUPPORTEDVERSIONOFHTTPCLIENT);
		}
	}

	httpClient = aiaMgr->client.hdata.httpClient;

	if (httpClient->version == 1) {
                PRUint32 responseDataLen = 
                   ((PKIX_PL_NssContext*)plContext)->maxResponseLength;

		hcv1 = &(httpClient->fcnTable.ftable1);
		requestSession = aiaMgr->client.hdata.requestSession;

		
                rv = (*hcv1->trySendAndReceiveFcn)(requestSession,
                                 (PRPollDesc **)&nbio,
                                 &responseCode,
                                 (const char **)&responseContentType,
                                 NULL, 
                                 (const char **)&responseData,
                                 &responseDataLen);

                if (rv != SECSuccess) {
                        PKIX_ERROR(PKIX_HTTPSERVERERROR);
                }

                if (nbio != 0) {
                        *pNBIOContext = nbio;
                        goto cleanup;
                }

		PKIX_CHECK(pkix_pl_HttpCertStore_ProcessCertResponse
	                (responseCode,
	                responseContentType,
	                responseData,
	                responseDataLen,
	                pCerts,
	                plContext),
	                PKIX_HTTPCERTSTOREPROCESSCERTRESPONSEFAILED);
                
                
                if (aiaMgr->client.hdata.requestSession != NULL) {
                    (*hcv1->freeFcn)(aiaMgr->client.hdata.requestSession);
                    aiaMgr->client.hdata.requestSession = NULL;
                }
                if (aiaMgr->client.hdata.serverSession != NULL) {
                    (*hcv1->freeSessionFcn)(aiaMgr->client.hdata.serverSession);
                    aiaMgr->client.hdata.serverSession = NULL;
                }
                aiaMgr->client.hdata.httpClient = 0; 

        } else  {
		PKIX_ERROR(PKIX_UNSUPPORTEDVERSIONOFHTTPCLIENT);
	}

cleanup:
        

        if (PKIX_ERROR_RECEIVED && aiaMgr) {
            if (aiaMgr->client.hdata.requestSession != NULL) {
                (*hcv1->freeFcn)(aiaMgr->client.hdata.requestSession);
                aiaMgr->client.hdata.requestSession = NULL;
            }
            if (aiaMgr->client.hdata.serverSession != NULL) {
                (*hcv1->freeSessionFcn)(aiaMgr->client.hdata.serverSession);
                aiaMgr->client.hdata.serverSession = NULL;
            }
            aiaMgr->client.hdata.httpClient = 0; 
        }

        PKIX_DECREF(location);
        PKIX_DECREF(locationString);

        if (locationAscii) {
            PORT_Free(locationAscii);
        }
        if (hostname) {
            PORT_Free(hostname);
        }
        if (path) {
            PORT_Free(path);
        }

        PKIX_RETURN(AIAMGR);
}

PKIX_Error *
pkix_pl_AIAMgr_GetLDAPCerts(
        PKIX_PL_AIAMgr *aiaMgr,
	PKIX_PL_InfoAccess *ia,
	void **pNBIOContext,
	PKIX_List **pCerts,
        void *plContext)
{
        PKIX_List *result = NULL;
        PKIX_PL_GeneralName *location = NULL;
        PKIX_PL_LdapClient *client = NULL;
        LDAPRequestParams request;
        PRArenaPool *arena = NULL;
        char *domainName = NULL;
	void *nbio = NULL;

        PKIX_ENTER(AIAMGR, "pkix_pl_AIAMgr_GetLDAPCerts");
        PKIX_NULLCHECK_FOUR(aiaMgr, ia, pNBIOContext, pCerts);

        nbio = *pNBIOContext;
        *pNBIOContext = NULL;
        *pCerts = NULL;

        if (nbio == NULL) { 

                

                request.scope = WHOLE_SUBTREE;
                request.derefAliases = NEVER_DEREF;
                request.sizeLimit = 0;
                request.timeLimit = 0;

                PKIX_CHECK(PKIX_PL_InfoAccess_GetLocation
                        (ia, &location, plContext),
                        PKIX_INFOACCESSGETLOCATIONFAILED);

                



                arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
                if (!arena) {
                        PKIX_ERROR_FATAL(PKIX_OUTOFMEMORY);
                }

                PKIX_CHECK(pkix_pl_InfoAccess_ParseLocation
                        (location, arena, &request, &domainName, plContext),
                        PKIX_INFOACCESSPARSELOCATIONFAILED);

                PKIX_DECREF(location);

                
                PKIX_CHECK(pkix_pl_AiaMgr_FindLDAPClient
                        (aiaMgr, domainName, &client, plContext),
                        PKIX_AIAMGRFINDLDAPCLIENTFAILED);

                aiaMgr->client.ldapClient = client;

                PKIX_CHECK(PKIX_PL_LdapClient_InitiateRequest
                        (aiaMgr->client.ldapClient,
			&request,
			&nbio,
			&result,
			plContext),
                        PKIX_LDAPCLIENTINITIATEREQUESTFAILED);

                PKIX_PL_NSSCALL(AIAMGR, PORT_FreeArena, (arena, PR_FALSE));

        } else {

                PKIX_CHECK(PKIX_PL_LdapClient_ResumeRequest
                        (aiaMgr->client.ldapClient, &nbio, &result, plContext),
                        PKIX_LDAPCLIENTRESUMEREQUESTFAILED);

        }

        if (nbio != NULL) { 
                *pNBIOContext = nbio;
                *pCerts = NULL;
                goto cleanup;
        }

	PKIX_DECREF(aiaMgr->client.ldapClient);

	if (result == NULL) {
		*pCerts = NULL;
	} else {
		PKIX_CHECK(pkix_pl_LdapCertStore_BuildCertList
			(result, pCerts, plContext),
			PKIX_LDAPCERTSTOREBUILDCERTLISTFAILED);
	}

	*pNBIOContext = nbio;

cleanup:

        if (arena && (PKIX_ERROR_RECEIVED)) {
                PKIX_PL_NSSCALL(AIAMGR, PORT_FreeArena, (arena, PR_FALSE));
        }

        if (PKIX_ERROR_RECEIVED) {
	        PKIX_DECREF(aiaMgr->client.ldapClient);
	}

        PKIX_DECREF(location);

        PKIX_RETURN(AIAMGR);
}



















PKIX_Error *
PKIX_PL_AIAMgr_Create(
        PKIX_PL_AIAMgr **pAIAMgr,
        void *plContext)
{
        PKIX_PL_AIAMgr *aiaMgr = NULL;

        PKIX_ENTER(AIAMGR, "PKIX_PL_AIAMgr_Create");
        PKIX_NULLCHECK_ONE(pAIAMgr);

        PKIX_CHECK(PKIX_PL_Object_Alloc
                (PKIX_AIAMGR_TYPE,
                sizeof(PKIX_PL_AIAMgr),
                (PKIX_PL_Object **)&aiaMgr,
                plContext),
                PKIX_COULDNOTCREATEAIAMGROBJECT);
        
        
        aiaMgr->method = 0;
        aiaMgr->aiaIndex = 0;
        aiaMgr->numAias = 0;
        aiaMgr->aia = NULL;
        aiaMgr->location = NULL;
        aiaMgr->results = NULL;
        aiaMgr->client.hdata.httpClient = NULL;
	aiaMgr->client.hdata.serverSession = NULL;
	aiaMgr->client.hdata.requestSession = NULL;

        *pAIAMgr = aiaMgr;

cleanup:

        PKIX_RETURN(AIAMGR);
}






PKIX_Error *
PKIX_PL_AIAMgr_GetAIACerts(
        PKIX_PL_AIAMgr *aiaMgr,
        PKIX_PL_Cert *prevCert,
        void **pNBIOContext,
        PKIX_List **pCerts,
        void *plContext)
{
        PKIX_UInt32 numAias = 0;
        PKIX_UInt32 aiaIndex = 0;
        PKIX_UInt32 iaType = PKIX_INFOACCESS_LOCATION_UNKNOWN;
        PKIX_List *certs = NULL;
        PKIX_PL_InfoAccess *ia = NULL;
        void *nbio = NULL;

        PKIX_ENTER(AIAMGR, "PKIX_PL_AIAMgr_GetAIACerts");
        PKIX_NULLCHECK_FOUR(aiaMgr, prevCert, pNBIOContext, pCerts);

        nbio = *pNBIOContext;
        *pCerts = NULL;
        *pNBIOContext = NULL;

        if (nbio == NULL) { 

                
                PKIX_CHECK(PKIX_PL_Cert_GetAuthorityInfoAccess
                        (prevCert, &aiaMgr->aia, plContext),
                        PKIX_CERTGETAUTHORITYINFOACCESSFAILED);

                if (aiaMgr->aia != NULL) {
                        PKIX_CHECK(PKIX_List_GetLength
                                (aiaMgr->aia, &numAias, plContext),
                                PKIX_LISTGETLENGTHFAILED);
                }

                
                if ((aiaMgr->aia == NULL) || (numAias == 0)) {
                        *pCerts = NULL;
                        goto cleanup;
                }

                aiaMgr->aiaIndex = 0;
                aiaMgr->numAias = numAias;
                aiaMgr->results = NULL;

        }

        for (aiaIndex = aiaMgr->aiaIndex;
                aiaIndex < aiaMgr->numAias;
                aiaIndex ++) {
                PKIX_UInt32 method = 0;

                PKIX_CHECK(PKIX_List_GetItem
                        (aiaMgr->aia,
                        aiaIndex,
                        (PKIX_PL_Object **)&ia,
                        plContext),
                        PKIX_LISTGETITEMFAILED);

                PKIX_CHECK(PKIX_PL_InfoAccess_GetMethod
                        (ia, &method, plContext),
                        PKIX_INFOACCESSGETMETHODFAILED);

                if (method != PKIX_INFOACCESS_CA_ISSUERS &&
                    method != PKIX_INFOACCESS_CA_REPOSITORY) {
                    PKIX_DECREF(ia);
                    continue;
                }
                
                PKIX_CHECK(PKIX_PL_InfoAccess_GetLocationType
                        (ia, &iaType, plContext),
                        PKIX_INFOACCESSGETLOCATIONTYPEFAILED);

                if (iaType == PKIX_INFOACCESS_LOCATION_HTTP) {
			PKIX_CHECK(pkix_pl_AIAMgr_GetHTTPCerts
				(aiaMgr, ia, &nbio, &certs, plContext),
				PKIX_AIAMGRGETHTTPCERTSFAILED);
                } else if (iaType == PKIX_INFOACCESS_LOCATION_LDAP) {
			PKIX_CHECK(pkix_pl_AIAMgr_GetLDAPCerts
				(aiaMgr, ia, &nbio, &certs, plContext),
				PKIX_AIAMGRGETLDAPCERTSFAILED);
                } else {
                        
			PKIX_ERROR(PKIX_UNKNOWNINFOACCESSTYPE);
                }

                if (nbio != NULL) { 
                        aiaMgr->aiaIndex = aiaIndex;
                        *pNBIOContext = nbio;
                        *pCerts = NULL;
                        goto cleanup;
                }

                



                if (aiaMgr->results == NULL) {
                        PKIX_CHECK(PKIX_List_Create
                                (&(aiaMgr->results), plContext),
                                PKIX_LISTCREATEFAILED);
                }
                PKIX_CHECK(pkix_List_AppendList
                        (aiaMgr->results, certs, plContext),
                        PKIX_APPENDLISTFAILED);
                PKIX_DECREF(certs);

                PKIX_DECREF(ia);
        }

        PKIX_DECREF(aiaMgr->aia);

        *pNBIOContext = NULL;
        *pCerts = aiaMgr->results;
        aiaMgr->results = NULL;

cleanup:

        if (PKIX_ERROR_RECEIVED) {
                PKIX_DECREF(aiaMgr->aia);
                PKIX_DECREF(aiaMgr->results);
                PKIX_DECREF(aiaMgr->client.ldapClient);
        }

        PKIX_DECREF(certs);
        PKIX_DECREF(ia);

        PKIX_RETURN(AIAMGR);
}
