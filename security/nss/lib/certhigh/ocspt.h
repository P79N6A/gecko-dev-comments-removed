









#ifndef _OCSPT_H_
#define _OCSPT_H_






typedef struct CERTOCSPRequestStr CERTOCSPRequest;
typedef struct CERTOCSPResponseStr CERTOCSPResponse;





typedef struct CERTOCSPCertIDStr CERTOCSPCertID;
typedef struct CERTOCSPCertStatusStr CERTOCSPCertStatus;
typedef struct CERTOCSPSingleResponseStr CERTOCSPSingleResponse;























typedef void * SEC_HTTP_SERVER_SESSION;
typedef void * SEC_HTTP_REQUEST_SESSION;












typedef SECStatus (*SEC_HttpServer_CreateSessionFcn)(
   const char *host,
   PRUint16 portnum,
   SEC_HTTP_SERVER_SESSION *pSession);













 
typedef SECStatus (*SEC_HttpServer_KeepAliveSessionFcn)(
   SEC_HTTP_SERVER_SESSION session,
   PRPollDesc **pPollDesc);







 
typedef SECStatus (*SEC_HttpServer_FreeSessionFcn)(
   SEC_HTTP_SERVER_SESSION session);



















typedef SECStatus (*SEC_HttpRequest_CreateFcn)(
   SEC_HTTP_SERVER_SESSION session,
   const char *http_protocol_variant, 
   const char *path_and_query_string,
   const char *http_request_method, 
   const PRIntervalTime timeout, 
   SEC_HTTP_REQUEST_SESSION *pRequest);












 
typedef SECStatus (*SEC_HttpRequest_SetPostDataFcn)(
   SEC_HTTP_REQUEST_SESSION request,
   const char *http_data, 
   const PRUint32 http_data_len,
   const char *http_content_type);









 
typedef SECStatus (*SEC_HttpRequest_AddHeaderFcn)(
   SEC_HTTP_REQUEST_SESSION request,
   const char *http_header_name, 
   const char *http_header_value);
















































 
typedef SECStatus (*SEC_HttpRequest_TrySendAndReceiveFcn)(
   SEC_HTTP_REQUEST_SESSION request,
   PRPollDesc **pPollDesc,
   PRUint16 *http_response_code, 
   const char **http_response_content_type, 
   const char **http_response_headers, 
   const char **http_response_data, 
   PRUint32 *http_response_data_len); 










 
typedef SECStatus (*SEC_HttpRequest_CancelFcn)(
   SEC_HTTP_REQUEST_SESSION request);











 
typedef SECStatus (*SEC_HttpRequest_FreeFcn)(
   SEC_HTTP_REQUEST_SESSION request);

typedef struct SEC_HttpClientFcnV1Struct {
   SEC_HttpServer_CreateSessionFcn createSessionFcn;
   SEC_HttpServer_KeepAliveSessionFcn keepAliveSessionFcn;
   SEC_HttpServer_FreeSessionFcn freeSessionFcn;
   SEC_HttpRequest_CreateFcn createFcn;
   SEC_HttpRequest_SetPostDataFcn setPostDataFcn;
   SEC_HttpRequest_AddHeaderFcn addHeaderFcn;
   SEC_HttpRequest_TrySendAndReceiveFcn trySendAndReceiveFcn;
   SEC_HttpRequest_CancelFcn cancelFcn;
   SEC_HttpRequest_FreeFcn freeFcn;
} SEC_HttpClientFcnV1;

typedef struct SEC_HttpClientFcnStruct {
   PRInt16 version;
   union {
      SEC_HttpClientFcnV1 ftable1;
      
      
   } fcnTable;
} SEC_HttpClientFcn;



















typedef enum {
    ocspMode_FailureIsVerificationFailure = 0,
    ocspMode_FailureIsNotAVerificationFailure = 1
} SEC_OcspFailureMode;

#endif 
