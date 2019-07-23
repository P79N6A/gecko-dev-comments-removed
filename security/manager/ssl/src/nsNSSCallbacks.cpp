







































#include "nsNSSComponent.h" 
#include "nsNSSCallbacks.h"
#include "nsNSSCertificate.h"
#include "nsISSLStatus.h"
#include "nsNSSIOLayer.h" 
#include "nsIWebProgressListener.h"
#include "nsIStringBundle.h"
#include "nsXPIDLString.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsIServiceManager.h"
#include "nsReadableUtils.h"
#include "nsIPrompt.h"
#include "nsProxiedService.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsCRT.h"
#include "nsNSSShutDown.h"
#include "nsIUploadChannel.h"
#include "nsSSLThread.h"
#include "nsThreadUtils.h"
#include "nsAutoLock.h"
#include "nsIThread.h"
#include "nsIWindowWatcher.h"
#include "nsIPrompt.h"

#include "ssl.h"
#include "cert.h"
#include "ocsp.h"

static NS_DEFINE_CID(kNSSComponentCID, NS_NSSCOMPONENT_CID);

#ifdef PR_LOGGING
extern PRLogModuleInfo* gPIPNSSLog;
#endif

struct nsHTTPDownloadEvent : nsRunnable {
  nsHTTPDownloadEvent();
  ~nsHTTPDownloadEvent();

  NS_IMETHOD Run();
  
  nsNSSHttpRequestSession *mRequestSession; 
  
  nsCOMPtr<nsHTTPListener> mListener;
  PRBool mResponsibleForDoneSignal;
};

nsHTTPDownloadEvent::nsHTTPDownloadEvent()
:mResponsibleForDoneSignal(PR_TRUE)
{
}

nsHTTPDownloadEvent::~nsHTTPDownloadEvent()
{
  if (mResponsibleForDoneSignal && mListener)
    mListener->send_done_signal();
}

NS_IMETHODIMP
nsHTTPDownloadEvent::Run()
{
  if (!mListener)
    return NS_OK;

  nsresult rv;

  nsCOMPtr<nsIIOService> ios = do_GetIOService();
  NS_ENSURE_STATE(ios);

  nsCOMPtr<nsIChannel> chan;
  ios->NewChannel(mRequestSession->mURL, nsnull, nsnull, getter_AddRefs(chan));
  NS_ENSURE_STATE(chan);

  
  
  nsCOMPtr<nsILoadGroup> loadGroup =
    do_CreateInstance(NS_LOADGROUP_CONTRACTID);
  chan->SetLoadGroup(loadGroup);

  if (mRequestSession->mHasPostData)
  {
    nsCOMPtr<nsIInputStream> uploadStream;
    rv = NS_NewPostDataStream(getter_AddRefs(uploadStream),
                              PR_FALSE,
                              mRequestSession->mPostData,
                              0, ios);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIUploadChannel> uploadChannel(do_QueryInterface(chan));
    NS_ENSURE_STATE(uploadChannel);

    rv = uploadChannel->SetUploadStream(uploadStream, 
                                        mRequestSession->mPostContentType,
                                        -1);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMPtr<nsIHttpChannel> hchan = do_QueryInterface(chan);
  NS_ENSURE_STATE(hchan);

  rv = hchan->SetRequestMethod(mRequestSession->mRequestMethod);
  NS_ENSURE_SUCCESS(rv, rv);

  nsSSLThread::rememberPendingHTTPRequest(loadGroup);

  mResponsibleForDoneSignal = PR_FALSE;
  mListener->mResponsibleForDoneSignal = PR_TRUE;

  rv = NS_NewStreamLoader(getter_AddRefs(mListener->mLoader), 
                          mListener);

  if (NS_SUCCEEDED(rv))
    rv = hchan->AsyncOpen(mListener->mLoader, nsnull);

  if (NS_FAILED(rv)) {
    mListener->mResponsibleForDoneSignal = PR_FALSE;
    mResponsibleForDoneSignal = PR_TRUE;
    
    nsSSLThread::rememberPendingHTTPRequest(nsnull);
  }

  return NS_OK;
}

struct nsCancelHTTPDownloadEvent : nsRunnable {
  NS_IMETHOD Run() {
    nsSSLThread::cancelPendingHTTPRequest();
    return NS_OK;
  }
};

SECStatus nsNSSHttpServerSession::createSessionFcn(const char *host,
                                                   PRUint16 portnum,
                                                   SEC_HTTP_SERVER_SESSION *pSession)
{
  if (!host || !pSession)
    return SECFailure;

  nsNSSHttpServerSession *hss = new nsNSSHttpServerSession;
  if (!hss)
    return SECFailure;

  hss->mHost = host;
  hss->mPort = portnum;

  *pSession = hss;
  return SECSuccess;
}

SECStatus nsNSSHttpRequestSession::createFcn(SEC_HTTP_SERVER_SESSION session,
                                             const char *http_protocol_variant,
                                             const char *path_and_query_string,
                                             const char *http_request_method, 
                                             const PRIntervalTime timeout, 
                                             SEC_HTTP_REQUEST_SESSION *pRequest)
{
  if (!session || !http_protocol_variant || !path_and_query_string || 
      !http_request_method || !pRequest)
    return SECFailure;

  nsNSSHttpServerSession* hss = NS_STATIC_CAST(nsNSSHttpServerSession*, session);
  if (!hss)
    return SECFailure;

  nsNSSHttpRequestSession *rs = new nsNSSHttpRequestSession;
  if (!rs)
    return SECFailure;

  rs->mTimeoutInterval = timeout;

  rs->mURL.Append(nsDependentCString(http_protocol_variant));
  rs->mURL.AppendLiteral("://");
  rs->mURL.Append(hss->mHost);
  rs->mURL.AppendLiteral(":");
  rs->mURL.AppendInt(hss->mPort);
  rs->mURL.Append(path_and_query_string);

  rs->mRequestMethod = nsDependentCString(http_request_method);

  *pRequest = (void*)rs;
  return SECSuccess;
}

SECStatus nsNSSHttpRequestSession::setPostDataFcn(const char *http_data, 
                                                  const PRUint32 http_data_len,
                                                  const char *http_content_type)
{
  mHasPostData = PR_TRUE;
  mPostData.Assign(http_data, http_data_len);
  mPostContentType.Assign(http_content_type);

  return SECSuccess;
}

SECStatus nsNSSHttpRequestSession::addHeaderFcn(const char *http_header_name, 
                                                const char *http_header_value)
{
  return SECFailure; 

  
  
  
  
  
  
  
  
  
}

SECStatus nsNSSHttpRequestSession::trySendAndReceiveFcn(PRPollDesc **pPollDesc,
                                                        PRUint16 *http_response_code, 
                                                        const char **http_response_content_type, 
                                                        const char **http_response_headers, 
                                                        const char **http_response_data, 
                                                        PRUint32 *http_response_data_len)
{
  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG,
         ("nsNSSHttpRequestSession::trySendAndReceiveFcn to %s\n", mURL.get()));

  const int max_retries = 5;
  int retry_count = 0;
  PRBool retryable_error = PR_FALSE;
  SECStatus result_sec_status = SECFailure;

  do
  {
    if (retry_count > 0)
    {
      if (retryable_error)
      {
        PR_LOG(gPIPNSSLog, PR_LOG_DEBUG,
               ("nsNSSHttpRequestSession::trySendAndReceiveFcn - sleeping and retrying: %d of %d\n",
                retry_count, max_retries));
      }

      PR_Sleep( PR_MillisecondsToInterval(300) * retry_count );
    }

    ++retry_count;
    retryable_error = PR_FALSE;

    result_sec_status =
      internal_send_receive_attempt(retryable_error, pPollDesc, http_response_code,
                                    http_response_content_type, http_response_headers,
                                    http_response_data, http_response_data_len);
  }
  while (retryable_error &&
         retry_count < max_retries);

#ifdef PR_LOGGING
  if (retry_count > 1)
  {
    if (retryable_error)
      PR_LOG(gPIPNSSLog, PR_LOG_DEBUG,
             ("nsNSSHttpRequestSession::trySendAndReceiveFcn - still failing, giving up...\n"));
    else
      PR_LOG(gPIPNSSLog, PR_LOG_DEBUG,
             ("nsNSSHttpRequestSession::trySendAndReceiveFcn - success at attempt %d\n",
              retry_count));
  }
#endif

  return result_sec_status;
}

SECStatus
nsNSSHttpRequestSession::internal_send_receive_attempt(PRBool &retryable_error,
                                                       PRPollDesc **pPollDesc,
                                                       PRUint16 *http_response_code,
                                                       const char **http_response_content_type,
                                                       const char **http_response_headers,
                                                       const char **http_response_data,
                                                       PRUint32 *http_response_data_len)
{
  if (pPollDesc) *pPollDesc = nsnull;
  if (http_response_code) *http_response_code = 0;
  if (http_response_content_type) *http_response_content_type = 0;
  if (http_response_headers) *http_response_headers = 0;
  if (http_response_data) *http_response_data = 0;

  PRUint32 acceptableResultSize = 0;

  if (http_response_data_len)
  {
    acceptableResultSize = *http_response_data_len;
    *http_response_data_len = 0;
  }
  
  if (!mListener)
    return SECFailure;

  if (NS_FAILED(mListener->InitLocks()))
    return SECFailure;

  PRLock *waitLock = mListener->mLock;
  PRCondVar *waitCondition = mListener->mCondition;
  volatile PRBool &waitFlag = mListener->mWaitFlag;
  waitFlag = PR_TRUE;

  nsRefPtr<nsHTTPDownloadEvent> event = new nsHTTPDownloadEvent;
  if (!event)
    return SECFailure;

  event->mListener = mListener;
  event->mRequestSession = this;

  nsresult rv = NS_DispatchToMainThread(event);
  if (NS_FAILED(rv))
  {
    event->mResponsibleForDoneSignal = PR_FALSE;
    return SECFailure;
  }

  PRBool request_canceled = PR_FALSE;
  PRBool aborted_wait = PR_FALSE;

  {
    nsAutoLock locker(waitLock);

    const PRIntervalTime start_time = PR_IntervalNow();
    PRIntervalTime wait_interval;

    PRBool running_on_main_thread = NS_IsMainThread();
    if (running_on_main_thread)
    {
      
      wait_interval = PR_MicrosecondsToInterval(50);
    }
    else
    { 
      
      
      wait_interval = PR_MillisecondsToInterval(250);
    }

    while (waitFlag)
    {
      if (running_on_main_thread)
      {
        
        
        
        
        

        locker.unlock();
        NS_ProcessNextEvent(nsnull);
        locker.lock();
      }

      PR_WaitCondVar(waitCondition, wait_interval);
      
      if (!waitFlag)
        break;

      if (!request_canceled)
      {
        if ((PRIntervalTime)(PR_IntervalNow() - start_time) > mTimeoutInterval)
        {
          request_canceled = PR_TRUE;
          
          
          nsCOMPtr<nsIRunnable> cancelevent = new nsCancelHTTPDownloadEvent;
          rv = NS_DispatchToMainThread(cancelevent);
          if (NS_FAILED(rv))
          {
            NS_WARNING("cannot post cancel event");
            aborted_wait = PR_TRUE;
            break;
          }
        }
      }
    }
  }

  if (aborted_wait)
  {
    
    nsSSLThread::rememberPendingHTTPRequest(nsnull);
  }

  if (request_canceled)
    return SECFailure;

  if (NS_FAILED(mListener->mResultCode))
  {
    if (mListener->mResultCode == NS_ERROR_CONNECTION_REFUSED
        ||
        mListener->mResultCode == NS_ERROR_NET_RESET)
    {
      retryable_error = PR_TRUE;
    }
    return SECFailure;
  }

  if (http_response_code)
    *http_response_code = mListener->mHttpResponseCode;

  if (mListener->mHttpRequestSucceeded && http_response_data && http_response_data_len) {

    *http_response_data_len = mListener->mResultLen;
  
    
    if (acceptableResultSize != 0
        &&
        acceptableResultSize < mListener->mResultLen)
    {
      return SECFailure;
    }

    
    
    *http_response_data = (const char*)mListener->mResultData;
  }

  if (mListener->mHttpRequestSucceeded && http_response_content_type) {
    if (mListener->mHttpResponseContentType.Length()) {
      *http_response_content_type = mListener->mHttpResponseContentType.get();
    }
  }

  return SECSuccess;
}

SECStatus nsNSSHttpRequestSession::cancelFcn()
{
  
  
  
  return SECSuccess;
}

SECStatus nsNSSHttpRequestSession::freeFcn()
{
  delete this;
  return SECSuccess;
}

nsNSSHttpRequestSession::nsNSSHttpRequestSession()
: mHasPostData(PR_FALSE),
  mTimeoutInterval(0),
  mListener(new nsHTTPListener)
{
}

nsNSSHttpRequestSession::~nsNSSHttpRequestSession()
{
}

SEC_HttpClientFcn nsNSSHttpInterface::sNSSInterfaceTable;

void nsNSSHttpInterface::initTable()
{
  sNSSInterfaceTable.version = 1;
  SEC_HttpClientFcnV1 &v1 = sNSSInterfaceTable.fcnTable.ftable1;
  v1.createSessionFcn = createSessionFcn;
  v1.keepAliveSessionFcn = keepAliveFcn;
  v1.freeSessionFcn = freeSessionFcn;
  v1.createFcn = createFcn;
  v1.setPostDataFcn = setPostDataFcn;
  v1.addHeaderFcn = addHeaderFcn;
  v1.trySendAndReceiveFcn = trySendAndReceiveFcn;
  v1.cancelFcn = cancelFcn;
  v1.freeFcn = freeFcn;
}

void nsNSSHttpInterface::registerHttpClient()
{
  SEC_RegisterDefaultHttpClient(&sNSSInterfaceTable);
}

void nsNSSHttpInterface::unregisterHttpClient()
{
  SEC_RegisterDefaultHttpClient(nsnull);
}

nsHTTPListener::nsHTTPListener()
: mResultData(nsnull),
  mResultLen(0),
  mLock(nsnull),
  mCondition(nsnull),
  mWaitFlag(PR_TRUE),
  mResponsibleForDoneSignal(PR_FALSE)
{
}

nsresult nsHTTPListener::InitLocks()
{
  mLock = PR_NewLock();
  if (!mLock)
    return NS_ERROR_OUT_OF_MEMORY;
  
  mCondition = PR_NewCondVar(mLock);
  if (!mCondition)
  {
    PR_DestroyLock(mLock);
    mLock = nsnull;
    return NS_ERROR_OUT_OF_MEMORY;
  }
  
  return NS_OK;
}

nsHTTPListener::~nsHTTPListener()
{
  if (mResponsibleForDoneSignal)
    send_done_signal();

  if (mCondition)
    PR_DestroyCondVar(mCondition);
  
  if (mLock)
    PR_DestroyLock(mLock);
}

NS_IMPL_THREADSAFE_ISUPPORTS1(nsHTTPListener, nsIStreamLoaderObserver)

NS_IMETHODIMP
nsHTTPListener::OnStreamComplete(nsIStreamLoader* aLoader,
                                 nsISupports* aContext,
                                 nsresult aStatus,
                                 PRUint32 stringLen,
                                 const PRUint8* string)
{
  mResultCode = aStatus;

  nsCOMPtr<nsIRequest> req;
  nsCOMPtr<nsIHttpChannel> hchan;

  nsresult rv = aLoader->GetRequest(getter_AddRefs(req));
  
#ifdef PR_LOGGING
  if (NS_FAILED(aStatus))
  {
    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG,
           ("nsHTTPListener::OnStreamComplete status failed %d", aStatus));
  }
#endif

  if (NS_SUCCEEDED(rv))
    hchan = do_QueryInterface(req, &rv);

  if (NS_SUCCEEDED(rv))
  {
    rv = hchan->GetRequestSucceeded(&mHttpRequestSucceeded);
    if (NS_FAILED(rv))
      mHttpRequestSucceeded = PR_FALSE;

    mResultLen = stringLen;
    mResultData = string; 

    unsigned int rcode;
    rv = hchan->GetResponseStatus(&rcode);
    if (NS_FAILED(rv))
      mHttpResponseCode = 500;
    else
      mHttpResponseCode = rcode;

    hchan->GetResponseHeader(NS_LITERAL_CSTRING("Content-Type"), 
                                    mHttpResponseContentType);
  }

  if (mResponsibleForDoneSignal)
    send_done_signal();
  
  return aStatus;
}

void nsHTTPListener::send_done_signal()
{
  nsSSLThread::rememberPendingHTTPRequest(nsnull);
  
  mResponsibleForDoneSignal = PR_FALSE;

  {
    nsAutoLock locker(mLock);
    mWaitFlag = PR_FALSE;
    PR_NotifyAllCondVar(mCondition);
  }
}


class nsSSLStatus
  : public nsISSLStatus
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISSLSTATUS

  nsSSLStatus();
  virtual ~nsSSLStatus();

  
  nsCOMPtr<nsIX509Cert> mServerCert;
  PRUint32 mKeyLength;
  PRUint32 mSecretKeyLength;
  nsXPIDLCString mCipherName;
};

NS_IMETHODIMP
nsSSLStatus::GetServerCert(nsIX509Cert** _result)
{
  NS_ASSERTION(_result, "non-NULL destination required");

  *_result = mServerCert;
  NS_IF_ADDREF(*_result);

  return NS_OK;
}

NS_IMETHODIMP
nsSSLStatus::GetKeyLength(PRUint32* _result)
{
  NS_ASSERTION(_result, "non-NULL destination required");

  *_result = mKeyLength;

  return NS_OK;
}

NS_IMETHODIMP
nsSSLStatus::GetSecretKeyLength(PRUint32* _result)
{
  NS_ASSERTION(_result, "non-NULL destination required");

  *_result = mSecretKeyLength;

  return NS_OK;
}

NS_IMETHODIMP
nsSSLStatus::GetCipherName(char** _result)
{
  NS_ASSERTION(_result, "non-NULL destination required");

  *_result = PL_strdup(mCipherName.get());

  return NS_OK;
}

nsSSLStatus::nsSSLStatus()
: mKeyLength(0), mSecretKeyLength(0)
{
}

NS_IMPL_THREADSAFE_ISUPPORTS1(nsSSLStatus, nsISSLStatus)

nsSSLStatus::~nsSSLStatus()
{
}


char* PR_CALLBACK
PK11PasswordPrompt(PK11SlotInfo* slot, PRBool retry, void* arg) {
  nsNSSShutDownPreventionLock locker;
  nsresult rv = NS_OK;
  PRUnichar *password = nsnull;
  PRBool value = PR_FALSE;
  nsIInterfaceRequestor *ir = NS_STATIC_CAST(nsIInterfaceRequestor*, arg);
  nsCOMPtr<nsIPrompt> proxyPrompt;

  





  if (!ir)
  {
    nsCOMPtr<nsIWindowWatcher> wwatch(do_GetService(NS_WINDOWWATCHER_CONTRACTID));
    if (!wwatch)
      return nsnull;

    nsCOMPtr<nsIPrompt> prompter;
    wwatch->GetNewPrompter(0, getter_AddRefs(prompter));
    if (!prompter)
      return nsnull;

    NS_GetProxyForObject(NS_PROXY_TO_MAIN_THREAD,
                         NS_GET_IID(nsIPrompt),
                         prompter, NS_PROXY_SYNC,
                         getter_AddRefs(proxyPrompt));
    if (!proxyPrompt)
      return nsnull;
  }
  else
  {
    
    
  
    nsCOMPtr<nsIInterfaceRequestor> proxiedCallbacks;
    NS_GetProxyForObject(NS_PROXY_TO_MAIN_THREAD,
                         NS_GET_IID(nsIInterfaceRequestor),
                         ir,
                         NS_PROXY_SYNC,
                         getter_AddRefs(proxiedCallbacks));
  
    
    nsCOMPtr<nsIPrompt> prompt(do_GetInterface(proxiedCallbacks));
    if (!prompt) {
      NS_ASSERTION(PR_FALSE, "callbacks does not implement nsIPrompt");
      return nsnull;
    }
  
    
    NS_GetProxyForObject(NS_PROXY_TO_MAIN_THREAD,
                         NS_GET_IID(nsIPrompt),
                         prompt,
                         NS_PROXY_SYNC,
                         getter_AddRefs(proxyPrompt));
  }

  nsAutoString promptString;
  nsCOMPtr<nsINSSComponent> nssComponent(do_GetService(kNSSComponentCID, &rv));

  if (NS_FAILED(rv))
    return nsnull; 

  const PRUnichar* formatStrings[1] = { ToNewUnicode(NS_ConvertUTF8toUTF16(PK11_GetTokenName(slot))) };
  rv = nssComponent->PIPBundleFormatStringFromName("CertPassPrompt",
                                      formatStrings, 1,
                                      promptString);
  nsMemory::Free(NS_CONST_CAST(PRUnichar*, formatStrings[0]));

  if (NS_FAILED(rv))
    return nsnull;

  {
    nsPSMUITracker tracker;
    if (tracker.isUIForbidden()) {
      rv = NS_ERROR_NOT_AVAILABLE;
    }
    else {
      rv = proxyPrompt->PromptPassword(nsnull, promptString.get(),
                                       &password, nsnull, nsnull, &value);
    }
  }
  
  if (NS_SUCCEEDED(rv) && value) {
    char* str = ToNewUTF8String(nsDependentString(password));
    Recycle(password);
    return str;
  }

  return nsnull;
}

void PR_CALLBACK HandshakeCallback(PRFileDesc* fd, void* client_data) {
  nsNSSShutDownPreventionLock locker;
  PRInt32 sslStatus;
  char* signer = nsnull;
  char* cipherName = nsnull;
  PRInt32 keyLength;
  nsresult rv;
  PRInt32 encryptBits;

  if (SECSuccess != SSL_SecurityStatus(fd, &sslStatus, &cipherName, &keyLength,
                                       &encryptBits, &signer, nsnull)) {
    return;
  }

  PRInt32 secStatus;
  if (sslStatus == SSL_SECURITY_STATUS_OFF)
    secStatus = nsIWebProgressListener::STATE_IS_BROKEN;
  else if (encryptBits >= 90)
    secStatus = (nsIWebProgressListener::STATE_IS_SECURE |
                 nsIWebProgressListener::STATE_SECURE_HIGH);
  else
    secStatus = (nsIWebProgressListener::STATE_IS_SECURE |
                 nsIWebProgressListener::STATE_SECURE_LOW);

  CERTCertificate *peerCert = SSL_PeerCertificate(fd);
  char* caName = CERT_GetOrgName(&peerCert->issuer);
  CERT_DestroyCertificate(peerCert);
  if (!caName) {
    caName = signer;
  }

  
  
  if (nsCRT::strcmp((const char*)caName, "RSA Data Security, Inc.") == 0) {
    
    
    
    
    NS_ASSERTION(caName != signer, "caName was equal to caName when it shouldn't be");
    PR_Free(caName);
    caName = PL_strdup("Verisign, Inc.");
  }

  nsAutoString shortDesc;
  const PRUnichar* formatStrings[1] = { ToNewUnicode(NS_ConvertUTF8toUTF16(caName)) };
  nsCOMPtr<nsINSSComponent> nssComponent(do_GetService(kNSSComponentCID, &rv));
  if (NS_SUCCEEDED(rv)) {
    rv = nssComponent->PIPBundleFormatStringFromName("SignedBy",
                                                   formatStrings, 1,
                                                   shortDesc);

    nsMemory::Free(NS_CONST_CAST(PRUnichar*, formatStrings[0]));

    nsNSSSocketInfo* infoObject = (nsNSSSocketInfo*) fd->higher->secret;
    infoObject->SetSecurityState(secStatus);
    infoObject->SetShortSecurityDescription(shortDesc.get());

    
    nsCOMPtr<nsSSLStatus> status = new nsSSLStatus();

    CERTCertificate *serverCert = SSL_PeerCertificate(fd);
    if (serverCert) {
      status->mServerCert = new nsNSSCertificate(serverCert);
      CERT_DestroyCertificate(serverCert);
    }

    status->mKeyLength = keyLength;
    status->mSecretKeyLength = encryptBits;
    status->mCipherName.Adopt(cipherName);

    infoObject->SetSSLStatus(status);
  }

  if (caName != signer) {
    PR_Free(caName);
  }
  PR_Free(signer);
}

SECStatus PR_CALLBACK AuthCertificateCallback(void* client_data, PRFileDesc* fd,
                                              PRBool checksig, PRBool isServer) {
  nsNSSShutDownPreventionLock locker;

  
  SECStatus rv = SSL_AuthCertificate(CERT_GetDefaultCertDB(), fd, checksig, isServer);

  
  
  
  
  if (SECSuccess == rv) {
    CERTCertificate *serverCert = SSL_PeerCertificate(fd);
    if (serverCert) {
      CERTCertList *certList = CERT_GetCertChainFromCert(serverCert, PR_Now(), certUsageSSLCA);

      nsCOMPtr<nsINSSComponent> nssComponent;
      
      for (CERTCertListNode *node = CERT_LIST_HEAD(certList);
           !CERT_LIST_END(node, certList);
           node = CERT_LIST_NEXT(node)) {

        if (node->cert->slot) {
          
          continue;
        }

        if (node->cert->isperm) {
          
          continue;
        }
        
        if (node->cert == serverCert) {
          
          
          continue;
        }
        
        

        if (!nssComponent) {
          
          nsresult rv;
          nssComponent = do_GetService(kNSSComponentCID, &rv);
        }
        
        if (nssComponent) {
          nssComponent->RememberCert(node->cert);
        }
      }

      CERT_DestroyCertList(certList);
      CERT_DestroyCertificate(serverCert);
    }
  }

  return rv;
}
