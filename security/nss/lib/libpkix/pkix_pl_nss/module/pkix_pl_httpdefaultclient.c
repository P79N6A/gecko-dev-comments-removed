









#include "pkix_pl_httpdefaultclient.h"

static void *plContext = NULL;














static SEC_HttpClientFcnV1 vtable = {
        pkix_pl_HttpDefaultClient_CreateSessionFcn,
        pkix_pl_HttpDefaultClient_KeepAliveSessionFcn,
        pkix_pl_HttpDefaultClient_FreeSessionFcn,
        pkix_pl_HttpDefaultClient_RequestCreateFcn,
        pkix_pl_HttpDefaultClient_SetPostDataFcn,
        pkix_pl_HttpDefaultClient_AddHeaderFcn,
        pkix_pl_HttpDefaultClient_TrySendAndReceiveFcn,
        pkix_pl_HttpDefaultClient_CancelFcn,
        pkix_pl_HttpDefaultClient_FreeFcn
};

static SEC_HttpClientFcn httpClient;

static const char *eohMarker = "\r\n\r\n";
static const PKIX_UInt32 eohMarkLen = 4; 
static const char *crlf = "\r\n";
static const PKIX_UInt32 crlfLen = 2; 
static const char *httpprotocol = "HTTP/";
static const PKIX_UInt32 httpprotocolLen = 5; 


#define HTTP_UNKNOWN_CONTENT_LENGTH -1
































static PKIX_Error *
pkix_pl_HttpDefaultClient_HdrCheckComplete(
        PKIX_PL_HttpDefaultClient *client,
        PKIX_UInt32 bytesRead,
        PKIX_Boolean *pKeepGoing,
        void *plContext)
{
        PKIX_UInt32 alreadyScanned = 0;
        PKIX_UInt32 comp = 0;
        PKIX_UInt32 headerLength = 0;
        PKIX_Int32 contentLength = HTTP_UNKNOWN_CONTENT_LENGTH;
        char *eoh = NULL;
        char *statusLineEnd = NULL;
        char *space = NULL;
        char *nextHeader = NULL;
        const char *httpcode = NULL;
        char *thisHeaderEnd = NULL;
        char *value = NULL;
        char *colon = NULL;
	char *copy = NULL;
        char *body = NULL;

        PKIX_ENTER
                (HTTPDEFAULTCLIENT,
                "pkix_pl_HttpDefaultClient_HdrCheckComplete");
        PKIX_NULLCHECK_TWO(client, pKeepGoing);

        *pKeepGoing = PKIX_FALSE;

        

        
        alreadyScanned = client->filledupBytes;
        




        if (alreadyScanned > eohMarkLen) {
            

            PKIX_UInt32 searchStartPos = alreadyScanned - eohMarkLen;
            eoh = PL_strnstr(&(client->rcvBuf[searchStartPos]), eohMarker,
                             bytesRead + searchStartPos);
        } else {
            
            eoh = PL_strnstr(client->rcvBuf, eohMarker, bytesRead);
        }

        client->filledupBytes += bytesRead;

        if (eoh == NULL) { 
                
                client->connectStatus = HTTP_RECV_HDR;
                *pKeepGoing = PKIX_TRUE;
                goto cleanup;
        }

        
        headerLength = (eoh - client->rcvBuf);

        
        PKIX_CHECK(PKIX_PL_Malloc(headerLength + 1, (void **)&copy, plContext),
                PKIX_MALLOCFAILED);

        
        PORT_Memcpy(copy, client->rcvBuf, headerLength);
	
	copy[headerLength] = '\0';
	client->rcvHeaders = copy;

        
        if (client->rcv_http_headers != NULL) {
                
                *(client->rcv_http_headers) = copy;
        }

        
        statusLineEnd = PL_strnstr(client->rcvBuf, crlf, client->capacity);
        if (statusLineEnd == NULL) {
                client->connectStatus = HTTP_ERROR;
                PORT_SetError(SEC_ERROR_OCSP_BAD_HTTP_RESPONSE);
                goto cleanup;
        }

        *statusLineEnd = '\0';

        space = strchr((const char *)client->rcvBuf, ' ');
        if (space == NULL) {
                client->connectStatus = HTTP_ERROR;
                goto cleanup;
        }

        comp = PORT_Strncasecmp((const char *)client->rcvBuf, httpprotocol,
                                httpprotocolLen);
        if (comp != 0) {
                client->connectStatus = HTTP_ERROR;
                goto cleanup;
        }

        httpcode = space + 1;
        space = strchr(httpcode, ' ');
        if (space == NULL) {
                client->connectStatus = HTTP_ERROR;
                goto cleanup;
        }
        *space = '\0';

        client->responseCode = atoi(httpcode);
        if (client->responseCode != 200) {
                client->connectStatus = HTTP_ERROR;
                goto cleanup;
        }

        
        nextHeader = statusLineEnd + crlfLen;
        *eoh = '\0';
        do {
                thisHeaderEnd = NULL;
                value = NULL;

                colon = strchr(nextHeader, ':');
                if (colon == NULL) {
                        client->connectStatus = HTTP_ERROR;
                        goto cleanup;
                }
                *colon = '\0';
                value = colon + 1;
                if (*value != ' ') {
                        client->connectStatus = HTTP_ERROR;
                        goto cleanup;
                }
                value++;
                thisHeaderEnd = strstr(value, crlf);
                if (thisHeaderEnd != NULL) {
                        *thisHeaderEnd = '\0';
                }
                comp = PORT_Strcasecmp(nextHeader, "content-type");
                if (comp == 0) {
                        client->rcvContentType = PORT_Strdup(value);
                } else {
                        comp = PORT_Strcasecmp(nextHeader, "content-length");
                        if (comp == 0) {
                                contentLength = atoi(value);
                        }
                }
                if (thisHeaderEnd != NULL) {
                        nextHeader = thisHeaderEnd + crlfLen;
                } else {
                        nextHeader = NULL;
                }
        } while ((nextHeader != NULL) && (nextHeader < (eoh + crlfLen)));
                
        
        if (client->rcv_http_content_type != NULL) {
                *(client->rcv_http_content_type) = client->rcvContentType;
        }

        if (client->rcvContentType == NULL) {
                client->connectStatus = HTTP_ERROR;
                goto cleanup;
        }

         
         headerLength += eohMarkLen;
         client->filledupBytes -= headerLength;
 
         



         switch (contentLength) {
         case 0:
             client->rcv_http_data_len = 0;
             client->connectStatus = HTTP_COMPLETE;
             *pKeepGoing = PKIX_FALSE;
             break;
 
         case HTTP_UNKNOWN_CONTENT_LENGTH:
             

             client->rcv_http_data_len = HTTP_UNKNOWN_CONTENT_LENGTH;
             contentLength =                 
                 client->filledupBytes + HTTP_DATA_BUFSIZE;
             if (client->maxResponseLen > 0 &&
                 contentLength > client->maxResponseLen) {
                 if (client->filledupBytes < client->maxResponseLen) {
                     contentLength = client->maxResponseLen;
                 } else {
                     client->connectStatus = HTTP_ERROR;
                     goto cleanup;
                 }
             }
             
             client->capacity = contentLength;
             client->connectStatus = HTTP_RECV_BODY;
             *pKeepGoing = PKIX_TRUE;
             break;
 
         default:
             client->rcv_http_data_len = contentLength;
             if (client->maxResponseLen > 0 &&
                 client->maxResponseLen < contentLength) {
                 client->connectStatus = HTTP_ERROR;
                 goto cleanup;
             }
             
             


             if (client->filledupBytes < contentLength) {
                 client->connectStatus = HTTP_RECV_BODY;
                 *pKeepGoing = PKIX_TRUE;
             } else {
                 client->connectStatus = HTTP_COMPLETE;
                 *pKeepGoing = PKIX_FALSE;
             }
         }
 
         if (contentLength > 0) {
             
             PKIX_CHECK(PKIX_PL_Malloc(contentLength, (void **)&body, plContext),
                        PKIX_MALLOCFAILED);
             
             
             if (client->filledupBytes > 0) {
                 PORT_Memcpy(body, &(client->rcvBuf[headerLength]),
                             client->filledupBytes);
             }
         }
 
         PKIX_CHECK(PKIX_PL_Free(client->rcvBuf, plContext),
                    PKIX_FREEFAILED);
         client->rcvBuf = body;

cleanup:

        PKIX_RETURN(HTTPDEFAULTCLIENT);
}































static PKIX_Error *
pkix_pl_HttpDefaultClient_Create(
        const char *host,
        PRUint16 portnum,
        PKIX_PL_HttpDefaultClient **pClient,
        void *plContext)
{
        PKIX_PL_HttpDefaultClient *client = NULL;

        PKIX_ENTER(HTTPDEFAULTCLIENT, "PKIX_PL_HttpDefaultClient_Create");
        PKIX_NULLCHECK_TWO(pClient, host);

        
        PKIX_CHECK(PKIX_PL_Object_Alloc
                (PKIX_HTTPDEFAULTCLIENT_TYPE,
                sizeof (PKIX_PL_HttpDefaultClient),
                (PKIX_PL_Object **)&client,
                plContext),
                PKIX_COULDNOTCREATEHTTPDEFAULTCLIENTOBJECT);

        

        client->timeout = 0;
        client->connectStatus = HTTP_NOT_CONNECTED;
        client->portnum = portnum;
        client->bytesToWrite = 0;
        client->send_http_data_len = 0;
        client->rcv_http_data_len = 0;
        client->capacity = 0;
        client->filledupBytes = 0;
        client->responseCode = 0;
        client->maxResponseLen = 0;
        client->GETLen = 0;
        client->POSTLen = 0;
        client->pRcv_http_data_len = NULL;
        client->callbackList = NULL;
        client->GETBuf = NULL;
        client->POSTBuf = NULL;
        client->rcvBuf = NULL;
        

        client->host = PORT_Strdup(host);
        if (!client->host) {
            PKIX_ERROR(PKIX_ALLOCERROR);
        }
        client->path = NULL;
        client->rcvContentType = NULL;
        client->rcvHeaders = NULL;
        client->send_http_method = HTTP_POST_METHOD;
        client->send_http_content_type = NULL;
        client->send_http_data = NULL;
        client->rcv_http_response_code = NULL;
        client->rcv_http_content_type = NULL;
        client->rcv_http_headers = NULL;
        client->rcv_http_data = NULL;
        client->socket = NULL;

        



        client->plContext = plContext;

        *pClient = client;

cleanup:
        if (PKIX_ERROR_RECEIVED) {
                PKIX_DECREF(client);
        }

        PKIX_RETURN(HTTPDEFAULTCLIENT);
}





static PKIX_Error *
pkix_pl_HttpDefaultClient_Destroy(
        PKIX_PL_Object *object,
        void *plContext)
{
        PKIX_PL_HttpDefaultClient *client = NULL;

        PKIX_ENTER(HTTPDEFAULTCLIENT, "pkix_pl_HttpDefaultClient_Destroy");
        PKIX_NULLCHECK_ONE(object);

        PKIX_CHECK(pkix_CheckType
                    (object, PKIX_HTTPDEFAULTCLIENT_TYPE, plContext),
                    PKIX_OBJECTNOTANHTTPDEFAULTCLIENT);

        client = (PKIX_PL_HttpDefaultClient *)object;

        if (client->rcvHeaders) {
            PKIX_PL_Free(client->rcvHeaders, plContext);
            client->rcvHeaders = NULL;
        }
        if (client->rcvContentType) {
            PORT_Free(client->rcvContentType);
            client->rcvContentType = NULL;
        }
        if (client->GETBuf != NULL) {
                PR_smprintf_free(client->GETBuf);
                client->GETBuf = NULL;
        }
        if (client->POSTBuf != NULL) {
                PKIX_PL_Free(client->POSTBuf, plContext);
                client->POSTBuf = NULL;
        }
        if (client->rcvBuf != NULL) {
                PKIX_PL_Free(client->rcvBuf, plContext);
                client->rcvBuf = NULL;
        }
        if (client->host) {
                PORT_Free(client->host);
                client->host = NULL;
        }
        if (client->path) {
                PORT_Free(client->path);
                client->path = NULL;
        }
        PKIX_DECREF(client->socket);

cleanup:

        PKIX_RETURN(HTTPDEFAULTCLIENT);
}















PKIX_Error *
pkix_pl_HttpDefaultClient_RegisterSelf(void *plContext)
{
        extern pkix_ClassTable_Entry systemClasses[PKIX_NUMTYPES];
        pkix_ClassTable_Entry *entry =
            &systemClasses[PKIX_HTTPDEFAULTCLIENT_TYPE];

        PKIX_ENTER(HTTPDEFAULTCLIENT,
                "pkix_pl_HttpDefaultClient_RegisterSelf");

        entry->description = "HttpDefaultClient";
        entry->typeObjectSize = sizeof(PKIX_PL_HttpDefaultClient);
        entry->destructor = pkix_pl_HttpDefaultClient_Destroy;
 
        httpClient.version = 1;
        httpClient.fcnTable.ftable1 = vtable;
        (void)SEC_RegisterDefaultHttpClient(&httpClient);

        PKIX_RETURN(HTTPDEFAULTCLIENT);
}



























static PKIX_Error *
pkix_pl_HttpDefaultClient_ConnectContinue(
        PKIX_PL_HttpDefaultClient *client,
        PKIX_Boolean *pKeepGoing,
        void *plContext)
{
        PRErrorCode status;
        PKIX_Boolean keepGoing = PKIX_FALSE;
        PKIX_PL_Socket_Callback *callbackList = NULL;

        PKIX_ENTER
                (HTTPDEFAULTCLIENT,
                "pkix_pl_HttpDefaultClient_ConnectContinue");
        PKIX_NULLCHECK_ONE(client);

        callbackList = (PKIX_PL_Socket_Callback *)client->callbackList;

        PKIX_CHECK(callbackList->connectcontinueCallback
                (client->socket, &status, plContext),
                PKIX_SOCKETCONNECTCONTINUEFAILED);

        if (status == 0) {
                client->connectStatus = HTTP_CONNECTED;
                keepGoing = PKIX_TRUE;
        } else if (status != PR_IN_PROGRESS_ERROR) {
                PKIX_ERROR(PKIX_UNEXPECTEDERRORINESTABLISHINGCONNECTION);
        }

        *pKeepGoing = keepGoing;

cleanup:
        PKIX_RETURN(HTTPDEFAULTCLIENT);
}

































static PKIX_Error *
pkix_pl_HttpDefaultClient_Send(
        PKIX_PL_HttpDefaultClient *client,
        PKIX_Boolean *pKeepGoing,
        PKIX_UInt32 *pBytesTransferred,
        void *plContext)
{
        PKIX_Int32 bytesWritten = 0;
        PKIX_Int32 lenToWrite = 0;
        PKIX_PL_Socket_Callback *callbackList = NULL;
        char *dataToWrite = NULL;

        PKIX_ENTER(HTTPDEFAULTCLIENT, "pkix_pl_HttpDefaultClient_Send");
        PKIX_NULLCHECK_THREE(client, pKeepGoing, pBytesTransferred);

        *pKeepGoing = PKIX_FALSE;

        
        if ((client->GETBuf) || (client->POSTBuf)) {

                if (client->GETBuf) {
                        dataToWrite = client->GETBuf;
                        lenToWrite = client->GETLen;
                } else {
                        dataToWrite = client->POSTBuf;
                        lenToWrite = client->POSTLen;
                }

                callbackList = (PKIX_PL_Socket_Callback *)client->callbackList;

                PKIX_CHECK(callbackList->sendCallback
                        (client->socket,
                        dataToWrite,
                        lenToWrite,
                        &bytesWritten,
                        plContext),
                        PKIX_SOCKETSENDFAILED);

                client->rcvBuf = NULL;
                client->capacity = 0;
                client->filledupBytes = 0;

                




                if (bytesWritten >= 0) {
                        client->connectStatus = HTTP_RECV_HDR;
                        *pKeepGoing = PKIX_TRUE;
                } else {
                        client->connectStatus = HTTP_SEND_PENDING;
                        *pKeepGoing = PKIX_FALSE;
                }

        }

        *pBytesTransferred = bytesWritten;

cleanup:
        PKIX_RETURN(HTTPDEFAULTCLIENT);
}

































static PKIX_Error *
pkix_pl_HttpDefaultClient_SendContinue(
        PKIX_PL_HttpDefaultClient *client,
        PKIX_Boolean *pKeepGoing,
        PKIX_UInt32 *pBytesTransferred,
        void *plContext)
{
        PKIX_Int32 bytesWritten = 0;
        PKIX_PL_Socket_Callback *callbackList = NULL;

        PKIX_ENTER(HTTPDEFAULTCLIENT, "pkix_pl_HttpDefaultClient_SendContinue");
        PKIX_NULLCHECK_THREE(client, pKeepGoing, pBytesTransferred);

        *pKeepGoing = PKIX_FALSE;

        callbackList = (PKIX_PL_Socket_Callback *)client->callbackList;

        PKIX_CHECK(callbackList->pollCallback
                (client->socket, &bytesWritten, NULL, plContext),
                PKIX_SOCKETPOLLFAILED);

        




        if (bytesWritten >= 0) {
                       client->connectStatus = HTTP_RECV_HDR;
                *pKeepGoing = PKIX_TRUE;
        }

        *pBytesTransferred = bytesWritten;

cleanup:
        PKIX_RETURN(HTTPDEFAULTCLIENT);
}


























static PKIX_Error *
pkix_pl_HttpDefaultClient_RecvHdr(
        PKIX_PL_HttpDefaultClient *client,
        PKIX_Boolean *pKeepGoing,
        void *plContext)
{
        PKIX_UInt32 bytesToRead = 0;
        PKIX_Int32 bytesRead = 0;
        PKIX_PL_Socket_Callback *callbackList = NULL;

        PKIX_ENTER(HTTPDEFAULTCLIENT, "pkix_pl_HttpDefaultClient_RecvHdr");
        PKIX_NULLCHECK_TWO(client, pKeepGoing);

        






        client->capacity += HTTP_HEADER_BUFSIZE;
        PKIX_CHECK(PKIX_PL_Realloc
                (client->rcvBuf,
                client->capacity,
                (void **)&(client->rcvBuf),
                plContext),
                PKIX_REALLOCFAILED);

        bytesToRead = client->capacity - client->filledupBytes;

        callbackList = (PKIX_PL_Socket_Callback *)client->callbackList;

        PKIX_CHECK(callbackList->recvCallback
                (client->socket,
                (void *)&(client->rcvBuf[client->filledupBytes]),
                bytesToRead,
                &bytesRead,
                plContext),
                PKIX_SOCKETRECVFAILED);

        if (bytesRead > 0) {
            

            PKIX_CHECK(
                pkix_pl_HttpDefaultClient_HdrCheckComplete(client, bytesRead,
                                                           pKeepGoing,
                                                           plContext),
                       PKIX_HTTPDEFAULTCLIENTHDRCHECKCOMPLETEFAILED);
        } else {
            client->connectStatus = HTTP_RECV_HDR_PENDING;
            *pKeepGoing = PKIX_FALSE;
        }

cleanup:
        PKIX_RETURN(HTTPDEFAULTCLIENT);
}


























static PKIX_Error *
pkix_pl_HttpDefaultClient_RecvHdrContinue(
        PKIX_PL_HttpDefaultClient *client,
        PKIX_Boolean *pKeepGoing,
        void *plContext)
{
        PKIX_Int32 bytesRead = 0;
        PKIX_PL_Socket_Callback *callbackList = NULL;

        PKIX_ENTER
                (HTTPDEFAULTCLIENT,
                "pkix_pl_HttpDefaultClient_RecvHdrContinue");
        PKIX_NULLCHECK_TWO(client, pKeepGoing);

        callbackList = (PKIX_PL_Socket_Callback *)client->callbackList;

        PKIX_CHECK(callbackList->pollCallback
                (client->socket, NULL, &bytesRead, plContext),
                PKIX_SOCKETPOLLFAILED);

        if (bytesRead > 0) {
                client->filledupBytes += bytesRead;

                PKIX_CHECK(pkix_pl_HttpDefaultClient_HdrCheckComplete
                        (client, bytesRead, pKeepGoing, plContext),
                        PKIX_HTTPDEFAULTCLIENTHDRCHECKCOMPLETEFAILED);

        } else {

                *pKeepGoing = PKIX_FALSE;

        }

cleanup:
        PKIX_RETURN(HTTPDEFAULTCLIENT);
}



























static PKIX_Error *
pkix_pl_HttpDefaultClient_RecvBody(
        PKIX_PL_HttpDefaultClient *client,
        PKIX_Boolean *pKeepGoing,
        void *plContext)
{
        PKIX_Int32 bytesRead = 0;
        PKIX_Int32 bytesToRead = 0;
        PKIX_PL_Socket_Callback *callbackList = NULL;

        PKIX_ENTER(HTTPDEFAULTCLIENT, "pkix_pl_HttpDefaultClient_RecvBody");
        PKIX_NULLCHECK_TWO(client, pKeepGoing);

        callbackList = (PKIX_PL_Socket_Callback *)client->callbackList;

        if (client->rcv_http_data_len != HTTP_UNKNOWN_CONTENT_LENGTH) {
            bytesToRead = client->rcv_http_data_len -
                client->filledupBytes;
        } else {
            
            

            int freeBuffSize = client->capacity - client->filledupBytes;
            if (freeBuffSize < HTTP_MIN_AVAILABLE_BUFFER_SIZE) {
                

                int currBuffSize = client->capacity;
                
                int newLength = currBuffSize + HTTP_DATA_BUFSIZE;
                if (client->maxResponseLen > 0 &&
                    newLength > client->maxResponseLen) {
                        newLength = client->maxResponseLen;
                }
                

                if (newLength <= client->filledupBytes) {
                    client->rcv_http_data_len = client->filledupBytes;
                    client->connectStatus = HTTP_ERROR;
                    *pKeepGoing = PKIX_FALSE;
                    goto cleanup;
                }
                if (client->capacity < newLength) {
                    client->capacity = newLength;
                    PKIX_CHECK(
                        PKIX_PL_Realloc(client->rcvBuf, newLength,
                                        (void**)&client->rcvBuf, plContext),
                        PKIX_REALLOCFAILED);
                    freeBuffSize = client->capacity -
                        client->filledupBytes;
                }
            }
            bytesToRead = freeBuffSize;
        }

        
        if (client->connectStatus == HTTP_RECV_BODY_PENDING) {
            PKIX_CHECK(callbackList->pollCallback
                       (client->socket, NULL, &bytesRead, plContext),
                       PKIX_SOCKETPOLLFAILED);
        } else {
            PKIX_CHECK(callbackList->recvCallback
                       (client->socket,
                        (void *)&(client->rcvBuf[client->filledupBytes]),
                        bytesToRead,
                        &bytesRead,
                        plContext),
                       PKIX_SOCKETRECVFAILED);
        }

        


        
        if (bytesRead == 0) {
            client->connectStatus = HTTP_RECV_BODY_PENDING;
            *pKeepGoing = PKIX_TRUE;
            goto cleanup;
        }
        
        
        client->filledupBytes += bytesRead;

        

        if (bytesToRead > bytesRead ||
            client->rcv_http_data_len == HTTP_UNKNOWN_CONTENT_LENGTH) {
            *pKeepGoing = PKIX_TRUE;
            goto cleanup;
        }
        client->connectStatus = HTTP_COMPLETE;
        *pKeepGoing = PKIX_FALSE;

cleanup:
        if (pkixErrorResult && pkixErrorResult->errCode ==
            PKIX_PRRECVREPORTSNETWORKCONNECTIONCLOSED) {
            if (client->rcv_http_data_len == HTTP_UNKNOWN_CONTENT_LENGTH) {
                client->rcv_http_data_len = client->filledupBytes;
                client->connectStatus = HTTP_COMPLETE;
                *pKeepGoing = PKIX_FALSE;
                PKIX_DECREF(pkixErrorResult);
            } else {
                client->connectStatus = HTTP_ERROR;
            }
        }

        PKIX_RETURN(HTTPDEFAULTCLIENT);
}






















static PKIX_Error *
pkix_pl_HttpDefaultClient_Dispatch(
        PKIX_PL_HttpDefaultClient *client,
        void *plContext)
{
        PKIX_UInt32 bytesTransferred = 0;
        PKIX_Boolean keepGoing = PKIX_TRUE;

        PKIX_ENTER(HTTPDEFAULTCLIENT, "pkix_pl_HttpDefaultClient_Dispatch");
        PKIX_NULLCHECK_ONE(client);

        while (keepGoing) {
                switch (client->connectStatus) {
                case HTTP_CONNECT_PENDING:
                    PKIX_CHECK(pkix_pl_HttpDefaultClient_ConnectContinue
                        (client, &keepGoing, plContext),
                        PKIX_HTTPDEFAULTCLIENTCONNECTCONTINUEFAILED);
                    break;
                case HTTP_CONNECTED:
                    PKIX_CHECK(pkix_pl_HttpDefaultClient_Send
                        (client, &keepGoing, &bytesTransferred, plContext),
                        PKIX_HTTPDEFAULTCLIENTSENDFAILED);
                    break;
                case HTTP_SEND_PENDING:
                    PKIX_CHECK(pkix_pl_HttpDefaultClient_SendContinue
                        (client, &keepGoing, &bytesTransferred, plContext),
                        PKIX_HTTPDEFAULTCLIENTSENDCONTINUEFAILED);
                    break;
                case HTTP_RECV_HDR:
                    PKIX_CHECK(pkix_pl_HttpDefaultClient_RecvHdr
                        (client, &keepGoing, plContext),
                        PKIX_HTTPDEFAULTCLIENTRECVHDRFAILED);
                    break;
                case HTTP_RECV_HDR_PENDING:
                    PKIX_CHECK(pkix_pl_HttpDefaultClient_RecvHdrContinue
                        (client, &keepGoing, plContext),
                        PKIX_HTTPDEFAULTCLIENTRECVHDRCONTINUEFAILED);
                    break;
                case HTTP_RECV_BODY:
                case HTTP_RECV_BODY_PENDING:
                    PKIX_CHECK(pkix_pl_HttpDefaultClient_RecvBody
                        (client, &keepGoing, plContext),
                        PKIX_HTTPDEFAULTCLIENTRECVBODYFAILED);
                    break;
                case HTTP_ERROR:
                case HTTP_COMPLETE:
                    keepGoing = PKIX_FALSE;
                    break;
                case HTTP_NOT_CONNECTED:
                default:
                    PKIX_ERROR(PKIX_HTTPDEFAULTCLIENTINILLEGALSTATE);
                }
        }

cleanup:

        PKIX_RETURN(HTTPDEFAULTCLIENT);
}







PKIX_Error *
pkix_pl_HttpDefaultClient_CreateSession(
        const char *host,
        PRUint16 portnum,
        SEC_HTTP_SERVER_SESSION *pSession,
        void *plContext)
{
        PKIX_PL_HttpDefaultClient *client = NULL;

        PKIX_ENTER
                (HTTPDEFAULTCLIENT, "pkix_pl_HttpDefaultClient_CreateSession");
        PKIX_NULLCHECK_TWO(host, pSession);

        PKIX_CHECK(pkix_pl_HttpDefaultClient_Create
                (host, portnum, &client, plContext),
                PKIX_HTTPDEFAULTCLIENTCREATEFAILED);

        *pSession = (SEC_HTTP_SERVER_SESSION)client;

cleanup:

        PKIX_RETURN(HTTPDEFAULTCLIENT);

}

PKIX_Error *
pkix_pl_HttpDefaultClient_KeepAliveSession(
        SEC_HTTP_SERVER_SESSION session,
        PRPollDesc **pPollDesc,
        void *plContext)
{
        PKIX_ENTER
                (HTTPDEFAULTCLIENT,
                "pkix_pl_HttpDefaultClient_KeepAliveSession");
        PKIX_NULLCHECK_TWO(session, pPollDesc);

        PKIX_CHECK(pkix_CheckType
                ((PKIX_PL_Object *)session,
                PKIX_HTTPDEFAULTCLIENT_TYPE,
                plContext),
                PKIX_SESSIONNOTANHTTPDEFAULTCLIENT);

        

cleanup:

        PKIX_RETURN(HTTPDEFAULTCLIENT);

}

PKIX_Error *
pkix_pl_HttpDefaultClient_RequestCreate(
        SEC_HTTP_SERVER_SESSION session,
        const char *http_protocol_variant, 
        const char *path_and_query_string,
        const char *http_request_method, 
        const PRIntervalTime timeout, 
        SEC_HTTP_REQUEST_SESSION *pRequest,
        void *plContext)
{
        PKIX_PL_HttpDefaultClient *client = NULL;
        PKIX_PL_Socket *socket = NULL;
        PKIX_PL_Socket_Callback *callbackList = NULL;
        PRFileDesc *fileDesc = NULL;
        PRErrorCode status = 0;

        PKIX_ENTER
                (HTTPDEFAULTCLIENT, "pkix_pl_HttpDefaultClient_RequestCreate");
        PKIX_NULLCHECK_TWO(session, pRequest);

        PKIX_CHECK(pkix_CheckType
                ((PKIX_PL_Object *)session,
                PKIX_HTTPDEFAULTCLIENT_TYPE,
                plContext),
                PKIX_SESSIONNOTANHTTPDEFAULTCLIENT);

        client = (PKIX_PL_HttpDefaultClient *)session;

        
        if (PORT_Strncasecmp(http_protocol_variant, "http", 4) != 0) {
                PKIX_ERROR(PKIX_UNRECOGNIZEDPROTOCOLREQUESTED);
        }

        if (PORT_Strncasecmp(http_request_method, "POST", 4) == 0) {
                client->send_http_method = HTTP_POST_METHOD;
        } else if (PORT_Strncasecmp(http_request_method, "GET", 3) == 0) {
                client->send_http_method = HTTP_GET_METHOD;
        } else {
                
                PKIX_ERROR(PKIX_UNRECOGNIZEDREQUESTMETHOD);
        }

        if (path_and_query_string) {
            


            client->path = PORT_Strdup(path_and_query_string);
            if (!client->path) {
                PKIX_ERROR(PKIX_ALLOCERROR);
            }
        }

        client->timeout = timeout;

#if 0
	PKIX_CHECK(pkix_HttpCertStore_FindSocketConnection
                (timeout,
                "variation.red.iplanet.com", 
                2001,   
                &status,
                &socket,
                plContext),
		PKIX_HTTPCERTSTOREFINDSOCKETCONNECTIONFAILED);
#else
	PKIX_CHECK(pkix_HttpCertStore_FindSocketConnection
                (timeout,
                (char *)client->host,
                client->portnum,
                &status,
                &socket,
                plContext),
		PKIX_HTTPCERTSTOREFINDSOCKETCONNECTIONFAILED);
#endif

        client->socket = socket;

        PKIX_CHECK(pkix_pl_Socket_GetCallbackList
                (socket, &callbackList, plContext),
                PKIX_SOCKETGETCALLBACKLISTFAILED);

        client->callbackList = (void *)callbackList;

        PKIX_CHECK(pkix_pl_Socket_GetPRFileDesc
                (socket, &fileDesc, plContext),
                PKIX_SOCKETGETPRFILEDESCFAILED);

        client->pollDesc.fd = fileDesc;
        client->pollDesc.in_flags = 0;
        client->pollDesc.out_flags = 0;

        client->send_http_data = NULL;
        client->send_http_data_len = 0;
        client->send_http_content_type = NULL;

        client->connectStatus =
                 ((status == 0) ? HTTP_CONNECTED : HTTP_CONNECT_PENDING);

        
        PKIX_INCREF(client);
        *pRequest = client;

cleanup:

        PKIX_RETURN(HTTPDEFAULTCLIENT);

}

PKIX_Error *
pkix_pl_HttpDefaultClient_SetPostData(
        SEC_HTTP_REQUEST_SESSION request,
        const char *http_data, 
        const PRUint32 http_data_len,
        const char *http_content_type,
        void *plContext)
{
        PKIX_PL_HttpDefaultClient *client = NULL;

        PKIX_ENTER
                (HTTPDEFAULTCLIENT,
                "pkix_pl_HttpDefaultClient_SetPostData");
        PKIX_NULLCHECK_ONE(request);

        PKIX_CHECK(pkix_CheckType
                ((PKIX_PL_Object *)request,
                PKIX_HTTPDEFAULTCLIENT_TYPE,
                plContext),
                PKIX_REQUESTNOTANHTTPDEFAULTCLIENT);

        client = (PKIX_PL_HttpDefaultClient *)request;

        client->send_http_data = http_data;
        client->send_http_data_len = http_data_len;
        client->send_http_content_type = http_content_type;

        
        if ((client->send_http_content_type == NULL) ||
            (*(client->send_http_content_type) == '\0')) {
                client->send_http_content_type = "application/ocsp-request";
        }

cleanup:

        PKIX_RETURN(HTTPDEFAULTCLIENT);

}

PKIX_Error *
pkix_pl_HttpDefaultClient_TrySendAndReceive(
        SEC_HTTP_REQUEST_SESSION request,
        PRUint16 *http_response_code, 
        const char **http_response_content_type, 
        const char **http_response_headers, 
        const char **http_response_data, 
        PRUint32 *http_response_data_len, 
        PRPollDesc **pPollDesc,
        SECStatus *pSECReturn,
        void *plContext)        
{
        PKIX_PL_HttpDefaultClient *client = NULL;
        PKIX_UInt32 postLen = 0;
        PRPollDesc *pollDesc = NULL;
        char *sendbuf = NULL;
        char portstr[16];

        PKIX_ENTER
                (HTTPDEFAULTCLIENT,
                "pkix_pl_HttpDefaultClient_TrySendAndReceive");

        PKIX_NULLCHECK_ONE(request);

        PKIX_CHECK(pkix_CheckType
                ((PKIX_PL_Object *)request,
                PKIX_HTTPDEFAULTCLIENT_TYPE,
                plContext),
                PKIX_REQUESTNOTANHTTPDEFAULTCLIENT);

        client = (PKIX_PL_HttpDefaultClient *)request;

        if (!pPollDesc && client->timeout == 0) {
            PKIX_ERROR_FATAL(PKIX_NULLARGUMENT);
        }

        if (pPollDesc) {
            pollDesc = *pPollDesc;
        }

        
        if (pollDesc == NULL) {

                if (!((client->connectStatus == HTTP_CONNECTED) ||
                     (client->connectStatus == HTTP_CONNECT_PENDING))) {
                        PKIX_ERROR(PKIX_HTTPCLIENTININVALIDSTATE);
                }

                
                if (http_response_data_len != NULL) {
                        client->pRcv_http_data_len = http_response_data_len;
                        client->maxResponseLen = *http_response_data_len;
                }

                client->rcv_http_response_code = http_response_code;
                client->rcv_http_content_type = http_response_content_type;
                client->rcv_http_headers = http_response_headers;
                client->rcv_http_data = http_response_data;

                
                portstr[0] = '\0';
                if (client->portnum != 80) {
                        PR_snprintf(portstr, sizeof(portstr), ":%d", 
                                    client->portnum);
                }
                
                if (client->send_http_method == HTTP_POST_METHOD) {
                        sendbuf = PR_smprintf
                            ("POST %s HTTP/1.0\r\nHost: %s%s\r\n"
                            "Content-Type: %s\r\nContent-Length: %u\r\n\r\n",
                            client->path,
                            client->host,
                            portstr,
                            client->send_http_content_type,
                            client->send_http_data_len);
                        postLen = PORT_Strlen(sendbuf);
                            
                        client->POSTLen = postLen + client->send_http_data_len;

                        
                        PKIX_CHECK(PKIX_PL_Malloc
                                (client->POSTLen,
                                (void **)&(client->POSTBuf),
                                plContext),
                                PKIX_MALLOCFAILED);

                        
                        PORT_Memcpy(client->POSTBuf, sendbuf, postLen);

                        
                        PORT_Memcpy(&client->POSTBuf[postLen],
                                    client->send_http_data,
                                    client->send_http_data_len);

                        
                        PR_smprintf_free(sendbuf);
                        sendbuf = NULL;
                        
                } else if (client->send_http_method == HTTP_GET_METHOD) {
                        client->GETBuf = PR_smprintf
                            ("GET %s HTTP/1.0\r\nHost: %s%s\r\n\r\n",
                            client->path,
                            client->host,
                            portstr);
                        client->GETLen = PORT_Strlen(client->GETBuf);
                }

        }

        
        PKIX_CHECK(pkix_pl_HttpDefaultClient_Dispatch(client, plContext),
                PKIX_HTTPDEFAULTCLIENTDISPATCHFAILED);

        switch (client->connectStatus) {
                case HTTP_CONNECT_PENDING:
                case HTTP_SEND_PENDING:
                case HTTP_RECV_HDR_PENDING:
                case HTTP_RECV_BODY_PENDING:
                        pollDesc = &(client->pollDesc);
                        *pSECReturn = SECWouldBlock;
                        break;
                case HTTP_ERROR:
                        
                        if (client->pRcv_http_data_len != NULL) {
                                
                                if (client->rcv_http_data_len !=
                                            HTTP_UNKNOWN_CONTENT_LENGTH &&
                                    client->maxResponseLen >=
                                                client->rcv_http_data_len) {
                                        
                                        *(client->pRcv_http_data_len) =
                                                 client->rcv_http_data_len;
                                } else {
                                        
                                        *(client->pRcv_http_data_len) = 0;
                                }
                        }

                        pollDesc = NULL;
                        *pSECReturn = SECFailure;
                        break;
                case HTTP_COMPLETE:
                        *(client->rcv_http_response_code) =
                                client->responseCode;
                        if (client->pRcv_http_data_len != NULL) {
                                *http_response_data_len =
                                         client->rcv_http_data_len;
                        }
                        if (client->rcv_http_data != NULL) {
                                *(client->rcv_http_data) = client->rcvBuf;
                        }
                        pollDesc = NULL;
                        *pSECReturn = SECSuccess;
                        break;
                case HTTP_NOT_CONNECTED:
                case HTTP_CONNECTED:
                case HTTP_RECV_HDR:
                case HTTP_RECV_BODY:
                default:
                        pollDesc = NULL;
                        *pSECReturn = SECFailure;
                        PKIX_ERROR(PKIX_HTTPCLIENTININVALIDSTATE);
                        break;
        }

        if (pPollDesc) {
            *pPollDesc = pollDesc;
        }

cleanup:
        if (sendbuf) {
            PR_smprintf_free(sendbuf);
        }

        PKIX_RETURN(HTTPDEFAULTCLIENT);

}

PKIX_Error *
pkix_pl_HttpDefaultClient_Cancel(
        SEC_HTTP_REQUEST_SESSION request,
        void *plContext)
{
        PKIX_PL_HttpDefaultClient *client = NULL;

        PKIX_ENTER(HTTPDEFAULTCLIENT, "pkix_pl_HttpDefaultClient_Cancel");
        PKIX_NULLCHECK_ONE(request);

        PKIX_CHECK(pkix_CheckType
                ((PKIX_PL_Object *)request,
                PKIX_HTTPDEFAULTCLIENT_TYPE,
                plContext),
                PKIX_REQUESTNOTANHTTPDEFAULTCLIENT);

        client = (PKIX_PL_HttpDefaultClient *)request;

        

cleanup:

        PKIX_RETURN(HTTPDEFAULTCLIENT);

}

SECStatus
pkix_pl_HttpDefaultClient_CreateSessionFcn(
        const char *host,
        PRUint16 portnum,
        SEC_HTTP_SERVER_SESSION *pSession)
{
        PKIX_Error *err = pkix_pl_HttpDefaultClient_CreateSession
                (host, portnum, pSession, plContext);

        if (err) {
                PKIX_PL_Object_DecRef((PKIX_PL_Object *)err, plContext);
                return SECFailure;
        }
        return SECSuccess;
}

SECStatus
pkix_pl_HttpDefaultClient_KeepAliveSessionFcn(
        SEC_HTTP_SERVER_SESSION session,
        PRPollDesc **pPollDesc)
{
        PKIX_Error *err = pkix_pl_HttpDefaultClient_KeepAliveSession
                (session, pPollDesc, plContext);

        if (err) {
                PKIX_PL_Object_DecRef((PKIX_PL_Object *)err, plContext);
                return SECFailure;
        }
        return SECSuccess;
}

SECStatus
pkix_pl_HttpDefaultClient_FreeSessionFcn(
        SEC_HTTP_SERVER_SESSION session)
{
        PKIX_Error *err =
            PKIX_PL_Object_DecRef((PKIX_PL_Object *)(session), plContext);

        if (err) {
                PKIX_PL_Object_DecRef((PKIX_PL_Object *)err, plContext);
                return SECFailure;
        }
        return SECSuccess;
}

SECStatus
pkix_pl_HttpDefaultClient_RequestCreateFcn(
        SEC_HTTP_SERVER_SESSION session,
        const char *http_protocol_variant, 
        const char *path_and_query_string,
        const char *http_request_method, 
        const PRIntervalTime timeout, 
        SEC_HTTP_REQUEST_SESSION *pRequest)
{
        PKIX_Error *err = pkix_pl_HttpDefaultClient_RequestCreate
                (session,
                http_protocol_variant,
                path_and_query_string,
                http_request_method,
                timeout,
                pRequest,
                plContext);

        if (err) {
                PKIX_PL_Object_DecRef((PKIX_PL_Object *)err, plContext);
                return SECFailure;
        }
        return SECSuccess;
}

SECStatus
pkix_pl_HttpDefaultClient_SetPostDataFcn(
        SEC_HTTP_REQUEST_SESSION request,
        const char *http_data, 
        const PRUint32 http_data_len,
        const char *http_content_type)
{
        PKIX_Error *err =
            pkix_pl_HttpDefaultClient_SetPostData(request, http_data,
                                                  http_data_len,
                                                  http_content_type,
                                                  plContext);
        if (err) {
                PKIX_PL_Object_DecRef((PKIX_PL_Object *)err, plContext);
                return SECFailure;
        }
        return SECSuccess;
}

SECStatus
pkix_pl_HttpDefaultClient_AddHeaderFcn(
        SEC_HTTP_REQUEST_SESSION request,
        const char *http_header_name, 
        const char *http_header_value)
{
        
        return SECFailure;
}

SECStatus
pkix_pl_HttpDefaultClient_TrySendAndReceiveFcn(
        SEC_HTTP_REQUEST_SESSION request,
        PRPollDesc **pPollDesc,
        PRUint16 *http_response_code, 
        const char **http_response_content_type, 
        const char **http_response_headers, 
        const char **http_response_data, 
        PRUint32 *http_response_data_len) 
{
        SECStatus rv = SECFailure;

        PKIX_Error *err = pkix_pl_HttpDefaultClient_TrySendAndReceive
                (request,
                http_response_code, 
                http_response_content_type, 
                http_response_headers, 
                http_response_data, 
                http_response_data_len,
                pPollDesc,
                &rv,
                plContext);

        if (err) {
                PKIX_PL_Object_DecRef((PKIX_PL_Object *)err, plContext);
                return rv;
        }
        return SECSuccess;
}

SECStatus
pkix_pl_HttpDefaultClient_CancelFcn(
        SEC_HTTP_REQUEST_SESSION request)
{
        PKIX_Error *err = pkix_pl_HttpDefaultClient_Cancel(request, plContext);

        if (err) {
                PKIX_PL_Object_DecRef((PKIX_PL_Object *)err, plContext);
                return SECFailure;
        }
        return SECSuccess;
}

SECStatus
pkix_pl_HttpDefaultClient_FreeFcn(
        SEC_HTTP_REQUEST_SESSION request)
{
        PKIX_Error *err =
            PKIX_PL_Object_DecRef((PKIX_PL_Object *)(request), plContext);

        if (err) {
                PKIX_PL_Object_DecRef((PKIX_PL_Object *)err, plContext);
                return SECFailure;
        }
        return SECSuccess;
}
