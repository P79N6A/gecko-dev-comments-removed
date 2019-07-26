





#ifndef _NSNSSCALLBACKS_H_
#define _NSNSSCALLBACKS_H_

#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "pk11func.h"
#include "nspr.h"
#include "ocspt.h"
#include "nsIStreamLoader.h"
#include "mozilla/CondVar.h"
#include "mozilla/Mutex.h"
#include "mozilla/Attributes.h"
#include "nsString.h"

class nsILoadGroup;

char*
PK11PasswordPrompt(PK11SlotInfo *slot, PRBool retry, void* arg);

void HandshakeCallback(PRFileDesc *fd, void *client_data);
SECStatus CanFalseStartCallback(PRFileDesc* fd, void* client_data,
                                PRBool *canFalseStart);

class nsHTTPListener MOZ_FINAL : public nsIStreamLoaderObserver
{
private:
  
  
  
#ifdef _MSC_VER
  
  __pragma(warning(disable:4265))
#endif
  ~nsHTTPListener();

public:
  nsHTTPListener();

  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSISTREAMLOADEROBSERVER

  nsCOMPtr<nsIStreamLoader> mLoader;

  nsresult mResultCode;

  bool mHttpRequestSucceeded;
  uint16_t mHttpResponseCode;
  nsCString mHttpResponseContentType;

  const uint8_t* mResultData; 
  uint32_t mResultLen;
  
  mozilla::Mutex mLock;
  mozilla::CondVar mCondition;
  volatile bool mWaitFlag;
  
  bool mResponsibleForDoneSignal;
  void send_done_signal();

  
  
  
  
  nsILoadGroup *mLoadGroup;
  PRThread *mLoadGroupOwnerThread;
  void FreeLoadGroup(bool aCancelLoad);
};

class nsNSSHttpServerSession
{
public:
  nsCString mHost;
  uint16_t mPort;  

  static SECStatus createSessionFcn(const char *host,
                                    uint16_t portnum,
                                    SEC_HTTP_SERVER_SESSION *pSession);
};

class nsNSSHttpRequestSession
{
protected:
  mozilla::ThreadSafeAutoRefCnt mRefCount;

public:
  static SECStatus createFcn(SEC_HTTP_SERVER_SESSION session,
                             const char *http_protocol_variant,
                             const char *path_and_query_string,
                             const char *http_request_method, 
                             const PRIntervalTime timeout, 
                             SEC_HTTP_REQUEST_SESSION *pRequest);

  SECStatus setPostDataFcn(const char *http_data, 
                           const uint32_t http_data_len,
                           const char *http_content_type);

  SECStatus addHeaderFcn(const char *http_header_name, 
                         const char *http_header_value);

  SECStatus trySendAndReceiveFcn(PRPollDesc **pPollDesc,
                                 uint16_t *http_response_code, 
                                 const char **http_response_content_type, 
                                 const char **http_response_headers, 
                                 const char **http_response_data, 
                                 uint32_t *http_response_data_len);

  SECStatus cancelFcn();
  SECStatus freeFcn();

  void AddRef();
  void Release();

  nsCString mURL;
  nsCString mRequestMethod;
  
  bool mHasPostData;
  nsCString mPostData;
  nsCString mPostContentType;
  
  PRIntervalTime mTimeoutInterval;
  
  nsRefPtr<nsHTTPListener> mListener;
  
protected:
  nsNSSHttpRequestSession();
  ~nsNSSHttpRequestSession();

  SECStatus internal_send_receive_attempt(bool &retryable_error,
                                          PRPollDesc **pPollDesc,
                                          uint16_t *http_response_code,
                                          const char **http_response_content_type,
                                          const char **http_response_headers,
                                          const char **http_response_data,
                                          uint32_t *http_response_data_len);
};

class nsNSSHttpInterface
{
public:
  static SECStatus createSessionFcn(const char *host,
                                    uint16_t portnum,
                                    SEC_HTTP_SERVER_SESSION *pSession)
  {
    return nsNSSHttpServerSession::createSessionFcn(host, portnum, pSession);
  }

  static SECStatus keepAliveFcn(SEC_HTTP_SERVER_SESSION session,
                                PRPollDesc **pPollDesc)
  {
    
    
    return SECSuccess;
  }

  static SECStatus freeSessionFcn(SEC_HTTP_SERVER_SESSION session)
  {
    delete static_cast<nsNSSHttpServerSession*>(session);
    return SECSuccess;
  }

  static SECStatus createFcn(SEC_HTTP_SERVER_SESSION session,
                             const char *http_protocol_variant,
                             const char *path_and_query_string,
                             const char *http_request_method, 
                             const PRIntervalTime timeout, 
                             SEC_HTTP_REQUEST_SESSION *pRequest)
  {
    return nsNSSHttpRequestSession::createFcn(session, http_protocol_variant,
                                     path_and_query_string, http_request_method, 
                                     timeout, pRequest);
  }

  static SECStatus setPostDataFcn(SEC_HTTP_REQUEST_SESSION request, 
                                  const char *http_data, 
                                  const uint32_t http_data_len,
                                  const char *http_content_type)
  {
    return static_cast<nsNSSHttpRequestSession*>(request)
            ->setPostDataFcn(http_data, http_data_len, http_content_type);
  }

  static SECStatus addHeaderFcn(SEC_HTTP_REQUEST_SESSION request,
                                const char *http_header_name, 
                                const char *http_header_value)
  {
    return static_cast<nsNSSHttpRequestSession*>(request)
            ->addHeaderFcn(http_header_name, http_header_value);
  }

  static SECStatus trySendAndReceiveFcn(SEC_HTTP_REQUEST_SESSION request, 
                                        PRPollDesc **pPollDesc,
                                        uint16_t *http_response_code, 
                                        const char **http_response_content_type, 
                                        const char **http_response_headers, 
                                        const char **http_response_data, 
                                        uint32_t *http_response_data_len)
  {
    return static_cast<nsNSSHttpRequestSession*>(request)
            ->trySendAndReceiveFcn(pPollDesc, http_response_code, http_response_content_type, 
                     http_response_headers, http_response_data, http_response_data_len);
  }

  static SECStatus cancelFcn(SEC_HTTP_REQUEST_SESSION request)
  {
    return static_cast<nsNSSHttpRequestSession*>(request)
            ->cancelFcn();
  }

  static SECStatus freeFcn(SEC_HTTP_REQUEST_SESSION request)
  {
    return static_cast<nsNSSHttpRequestSession*>(request)
            ->freeFcn();
  }

  static void initTable();
  static SEC_HttpClientFcn sNSSInterfaceTable;

  void registerHttpClient();
  void unregisterHttpClient();
};

#endif 
