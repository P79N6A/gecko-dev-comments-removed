





#include "nsNSSCallbacks.h"
#include "pkix/pkixtypes.h"
#include "mozilla/Telemetry.h"
#include "mozilla/TimeStamp.h"
#include "nsNSSComponent.h"
#include "nsNSSIOLayer.h"
#include "nsIWebProgressListener.h"
#include "nsProtectedAuthThread.h"
#include "nsITokenDialogs.h"
#include "nsIUploadChannel.h"
#include "nsIPrompt.h"
#include "nsProxyRelease.h"
#include "PSMRunnable.h"
#include "nsContentUtils.h"
#include "nsIHttpChannelInternal.h"
#include "nsISupportsPriority.h"
#include "nsNetUtil.h"
#include "SharedSSLState.h"
#include "ssl.h"
#include "sslproto.h"

using namespace mozilla;
using namespace mozilla::psm;

#ifdef PR_LOGGING
extern PRLogModuleInfo* gPIPNSSLog;
#endif

static void AccumulateCipherSuite(Telemetry::ID probe,
                                  const SSLChannelInfo& channelInfo);

namespace {




const uint32_t NPN_NOT_NEGOTIATED = 64;
const uint32_t POSSIBLE_VERSION_DOWNGRADE = 4;
const uint32_t POSSIBLE_CIPHER_SUITE_DOWNGRADE = 2;
const uint32_t KEA_NOT_SUPPORTED = 1;

}

class nsHTTPDownloadEvent : public nsRunnable {
public:
  nsHTTPDownloadEvent();
  ~nsHTTPDownloadEvent();

  NS_IMETHOD Run();

  nsNSSHttpRequestSession *mRequestSession;
  
  nsRefPtr<nsHTTPListener> mListener;
  bool mResponsibleForDoneSignal;
  TimeStamp mStartTime;
};

nsHTTPDownloadEvent::nsHTTPDownloadEvent()
:mResponsibleForDoneSignal(true)
{
}

nsHTTPDownloadEvent::~nsHTTPDownloadEvent()
{
  if (mResponsibleForDoneSignal && mListener)
    mListener->send_done_signal();

  mRequestSession->Release();
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
  ios->NewChannel2(mRequestSession->mURL,
                   nullptr,
                   nullptr,
                   nullptr, 
                   nsContentUtils::GetSystemPrincipal(),
                   nullptr, 
                   nsILoadInfo::SEC_NORMAL,
                   nsIContentPolicy::TYPE_OTHER,
                   getter_AddRefs(chan));
  NS_ENSURE_STATE(chan);

  
  
  
  nsCOMPtr<nsISupportsPriority> priorityChannel = do_QueryInterface(chan);
  if (priorityChannel)
    priorityChannel->AdjustPriority(nsISupportsPriority::PRIORITY_HIGHEST);

  chan->SetLoadFlags(nsIRequest::LOAD_ANONYMOUS);

  
  
  nsCOMPtr<nsILoadGroup> lg = do_CreateInstance(NS_LOADGROUP_CONTRACTID);
  chan->SetLoadGroup(lg);

  if (mRequestSession->mHasPostData)
  {
    nsCOMPtr<nsIInputStream> uploadStream;
    rv = NS_NewPostDataStream(getter_AddRefs(uploadStream),
                              false,
                              mRequestSession->mPostData);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIUploadChannel> uploadChannel(do_QueryInterface(chan));
    NS_ENSURE_STATE(uploadChannel);

    rv = uploadChannel->SetUploadStream(uploadStream, 
                                        mRequestSession->mPostContentType,
                                        -1);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  
  
  nsCOMPtr<nsIHttpChannelInternal> internalChannel = do_QueryInterface(chan);
  if (internalChannel) {
    rv = internalChannel->SetAllowSpdy(false);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMPtr<nsIHttpChannel> hchan = do_QueryInterface(chan);
  NS_ENSURE_STATE(hchan);

  rv = hchan->SetAllowSTS(false);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = hchan->SetRequestMethod(mRequestSession->mRequestMethod);
  NS_ENSURE_SUCCESS(rv, rv);

  mResponsibleForDoneSignal = false;
  mListener->mResponsibleForDoneSignal = true;

  mListener->mLoadGroup = lg.get();
  NS_ADDREF(mListener->mLoadGroup);
  mListener->mLoadGroupOwnerThread = PR_GetCurrentThread();

  rv = NS_NewStreamLoader(getter_AddRefs(mListener->mLoader), 
                          mListener);

  if (NS_SUCCEEDED(rv)) {
    mStartTime = TimeStamp::Now();
    rv = hchan->AsyncOpen(mListener->mLoader, nullptr);
  }

  if (NS_FAILED(rv)) {
    mListener->mResponsibleForDoneSignal = false;
    mResponsibleForDoneSignal = true;

    NS_RELEASE(mListener->mLoadGroup);
    mListener->mLoadGroup = nullptr;
    mListener->mLoadGroupOwnerThread = nullptr;
  }

  return NS_OK;
}

struct nsCancelHTTPDownloadEvent : nsRunnable {
  nsRefPtr<nsHTTPListener> mListener;

  NS_IMETHOD Run() {
    mListener->FreeLoadGroup(true);
    mListener = nullptr;
    return NS_OK;
  }
};

SECStatus nsNSSHttpServerSession::createSessionFcn(const char *host,
                                                   uint16_t portnum,
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

  nsNSSHttpServerSession* hss = static_cast<nsNSSHttpServerSession*>(session);
  if (!hss)
    return SECFailure;

  nsNSSHttpRequestSession *rs = new nsNSSHttpRequestSession;
  if (!rs)
    return SECFailure;

  rs->mTimeoutInterval = timeout;

  
  
  uint32_t maxBug404059Timeout = PR_TicksPerSecond() * 10;
  if (timeout > maxBug404059Timeout) {
    rs->mTimeoutInterval = maxBug404059Timeout;
  }

  rs->mURL.Assign(http_protocol_variant);
  rs->mURL.AppendLiteral("://");
  rs->mURL.Append(hss->mHost);
  rs->mURL.Append(':');
  rs->mURL.AppendInt(hss->mPort);
  rs->mURL.Append(path_and_query_string);

  rs->mRequestMethod = http_request_method;

  *pRequest = (void*)rs;
  return SECSuccess;
}

SECStatus nsNSSHttpRequestSession::setPostDataFcn(const char *http_data, 
                                                  const uint32_t http_data_len,
                                                  const char *http_content_type)
{
  mHasPostData = true;
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
                                                        uint16_t *http_response_code, 
                                                        const char **http_response_content_type, 
                                                        const char **http_response_headers, 
                                                        const char **http_response_data, 
                                                        uint32_t *http_response_data_len)
{
  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG,
         ("nsNSSHttpRequestSession::trySendAndReceiveFcn to %s\n", mURL.get()));

  bool onSTSThread;
  nsresult nrv;
  nsCOMPtr<nsIEventTarget> sts
    = do_GetService(NS_SOCKETTRANSPORTSERVICE_CONTRACTID, &nrv);
  if (NS_FAILED(nrv)) {
    NS_ERROR("Could not get STS service");
    PR_SetError(PR_INVALID_STATE_ERROR, 0);
    return SECFailure;
  }

  nrv = sts->IsOnCurrentThread(&onSTSThread);
  if (NS_FAILED(nrv)) {
    NS_ERROR("IsOnCurrentThread failed");
    PR_SetError(PR_INVALID_STATE_ERROR, 0);
    return SECFailure;
  }

  if (onSTSThread) {
    NS_ERROR("nsNSSHttpRequestSession::trySendAndReceiveFcn called on socket "
             "thread; this will not work.");
    PR_SetError(PR_INVALID_STATE_ERROR, 0);
    return SECFailure;
  }

  const int max_retries = 2;
  int retry_count = 0;
  bool retryable_error = false;
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
    retryable_error = false;

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

void
nsNSSHttpRequestSession::AddRef()
{
  ++mRefCount;
}

void
nsNSSHttpRequestSession::Release()
{
  int32_t newRefCount = --mRefCount;
  if (!newRefCount) {
    delete this;
  }
}

SECStatus
nsNSSHttpRequestSession::internal_send_receive_attempt(bool &retryable_error,
                                                       PRPollDesc **pPollDesc,
                                                       uint16_t *http_response_code,
                                                       const char **http_response_content_type,
                                                       const char **http_response_headers,
                                                       const char **http_response_data,
                                                       uint32_t *http_response_data_len)
{
  if (pPollDesc) *pPollDesc = nullptr;
  if (http_response_code) *http_response_code = 0;
  if (http_response_content_type) *http_response_content_type = 0;
  if (http_response_headers) *http_response_headers = 0;
  if (http_response_data) *http_response_data = 0;

  uint32_t acceptableResultSize = 0;

  if (http_response_data_len)
  {
    acceptableResultSize = *http_response_data_len;
    *http_response_data_len = 0;
  }
  
  if (!mListener)
    return SECFailure;

  Mutex& waitLock = mListener->mLock;
  CondVar& waitCondition = mListener->mCondition;
  volatile bool &waitFlag = mListener->mWaitFlag;
  waitFlag = true;

  RefPtr<nsHTTPDownloadEvent> event(new nsHTTPDownloadEvent);
  if (!event)
    return SECFailure;

  event->mListener = mListener;
  this->AddRef();
  event->mRequestSession = this;

  nsresult rv = NS_DispatchToMainThread(event);
  if (NS_FAILED(rv))
  {
    event->mResponsibleForDoneSignal = false;
    return SECFailure;
  }

  bool request_canceled = false;

  {
    MutexAutoLock locker(waitLock);

    const PRIntervalTime start_time = PR_IntervalNow();
    PRIntervalTime wait_interval;

    bool running_on_main_thread = NS_IsMainThread();
    if (running_on_main_thread)
    {
      
      
      
      
      
      NS_WARNING("Security network blocking I/O on Main Thread");

      
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
        
        
        
        
        

        MutexAutoUnlock unlock(waitLock);
        NS_ProcessNextEvent(nullptr);
      }

      waitCondition.Wait(wait_interval);
      
      if (!waitFlag)
        break;

      if (!request_canceled)
      {
        bool timeout = 
          (PRIntervalTime)(PR_IntervalNow() - start_time) > mTimeoutInterval;
 
        if (timeout)
        {
          request_canceled = true;

          RefPtr<nsCancelHTTPDownloadEvent> cancelevent(
            new nsCancelHTTPDownloadEvent);
          cancelevent->mListener = mListener;
          rv = NS_DispatchToMainThread(cancelevent);
          if (NS_FAILED(rv)) {
            NS_WARNING("cannot post cancel event");
          }
          break;
        }
      }
    }
  }

  if (!event->mStartTime.IsNull()) {
    if (request_canceled) {
      Telemetry::Accumulate(Telemetry::CERT_VALIDATION_HTTP_REQUEST_RESULT, 0);
      Telemetry::AccumulateTimeDelta(
        Telemetry::CERT_VALIDATION_HTTP_REQUEST_CANCELED_TIME,
        event->mStartTime, TimeStamp::Now());
    }
    else if (NS_SUCCEEDED(mListener->mResultCode) &&
             mListener->mHttpResponseCode == 200) {
      Telemetry::Accumulate(Telemetry::CERT_VALIDATION_HTTP_REQUEST_RESULT, 1);
      Telemetry::AccumulateTimeDelta(
        Telemetry::CERT_VALIDATION_HTTP_REQUEST_SUCCEEDED_TIME,
        event->mStartTime, TimeStamp::Now());
    }
    else {
      Telemetry::Accumulate(Telemetry::CERT_VALIDATION_HTTP_REQUEST_RESULT, 2);
      Telemetry::AccumulateTimeDelta(
        Telemetry::CERT_VALIDATION_HTTP_REQUEST_FAILED_TIME,
        event->mStartTime, TimeStamp::Now());
    }
  }
  else {
    Telemetry::Accumulate(Telemetry::CERT_VALIDATION_HTTP_REQUEST_RESULT, 3);
  }

  if (request_canceled)
    return SECFailure;

  if (NS_FAILED(mListener->mResultCode))
  {
    if (mListener->mResultCode == NS_ERROR_CONNECTION_REFUSED
        ||
        mListener->mResultCode == NS_ERROR_NET_RESET)
    {
      retryable_error = true;
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
  Release();
  return SECSuccess;
}

nsNSSHttpRequestSession::nsNSSHttpRequestSession()
: mRefCount(1),
  mHasPostData(false),
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

nsHTTPListener::nsHTTPListener()
: mResultData(nullptr),
  mResultLen(0),
  mLock("nsHTTPListener.mLock"),
  mCondition(mLock, "nsHTTPListener.mCondition"),
  mWaitFlag(true),
  mResponsibleForDoneSignal(false),
  mLoadGroup(nullptr),
  mLoadGroupOwnerThread(nullptr)
{
}

nsHTTPListener::~nsHTTPListener()
{
  if (mResponsibleForDoneSignal)
    send_done_signal();

  if (mResultData) {
    free(const_cast<uint8_t *>(mResultData));
  }

  if (mLoader) {
    nsCOMPtr<nsIThread> mainThread(do_GetMainThread());
    NS_ProxyRelease(mainThread, mLoader);
  }
}

NS_IMPL_ISUPPORTS(nsHTTPListener, nsIStreamLoaderObserver)

void
nsHTTPListener::FreeLoadGroup(bool aCancelLoad)
{
  nsILoadGroup *lg = nullptr;

  MutexAutoLock locker(mLock);

  if (mLoadGroup) {
    if (mLoadGroupOwnerThread != PR_GetCurrentThread()) {
      NS_ASSERTION(false,
                   "attempt to access nsHTTPDownloadEvent::mLoadGroup on multiple threads, leaking it!");
    }
    else {
      lg = mLoadGroup;
      mLoadGroup = nullptr;
    }
  }

  if (lg) {
    if (aCancelLoad) {
      lg->Cancel(NS_ERROR_ABORT);
    }
    NS_RELEASE(lg);
  }
}

NS_IMETHODIMP
nsHTTPListener::OnStreamComplete(nsIStreamLoader* aLoader,
                                 nsISupports* aContext,
                                 nsresult aStatus,
                                 uint32_t stringLen,
                                 const uint8_t* string)
{
  mResultCode = aStatus;

  FreeLoadGroup(false);

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
      mHttpRequestSucceeded = false;

    mResultLen = stringLen;
    mResultData = string; 
    aStatus = NS_SUCCESS_ADOPTED_DATA;

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
  mResponsibleForDoneSignal = false;

  {
    MutexAutoLock locker(mLock);
    mWaitFlag = false;
    mCondition.NotifyAll();
  }
}

static char*
ShowProtectedAuthPrompt(PK11SlotInfo* slot, nsIInterfaceRequestor *ir)
{
  if (!NS_IsMainThread()) {
    NS_ERROR("ShowProtectedAuthPrompt called off the main thread");
    return nullptr;
  }

  char* protAuthRetVal = nullptr;

  
  nsITokenDialogs* dialogs = 0;
  nsresult nsrv = getNSSDialogs((void**)&dialogs, 
                                NS_GET_IID(nsITokenDialogs), 
                                NS_TOKENDIALOGS_CONTRACTID);
  if (NS_SUCCEEDED(nsrv))
  {
    nsProtectedAuthThread* protectedAuthRunnable = new nsProtectedAuthThread();
    if (protectedAuthRunnable)
    {
      NS_ADDREF(protectedAuthRunnable);

      protectedAuthRunnable->SetParams(slot);
      
      nsCOMPtr<nsIProtectedAuthThread> runnable = do_QueryInterface(protectedAuthRunnable);
      if (runnable)
      {
        nsrv = dialogs->DisplayProtectedAuth(ir, runnable);
              
        
        
        protectedAuthRunnable->Join();
              
        if (NS_SUCCEEDED(nsrv))
        {
          SECStatus rv = protectedAuthRunnable->GetResult();
          switch (rv)
          {
              case SECSuccess:
                  protAuthRetVal = ToNewCString(nsDependentCString(PK11_PW_AUTHENTICATED));
                  break;
              case SECWouldBlock:
                  protAuthRetVal = ToNewCString(nsDependentCString(PK11_PW_RETRY));
                  break;
              default:
                  protAuthRetVal = nullptr;
                  break;
              
          }
        }
      }

      NS_RELEASE(protectedAuthRunnable);
    }

    NS_RELEASE(dialogs);
  }

  return protAuthRetVal;
}

class PK11PasswordPromptRunnable : public SyncRunnableBase
{
public:
  PK11PasswordPromptRunnable(PK11SlotInfo* slot, 
                             nsIInterfaceRequestor* ir)
    : mResult(nullptr),
      mSlot(slot),
      mIR(ir)
  {
  }
  char * mResult; 
  virtual void RunOnTargetThread();
private:
  PK11SlotInfo* const mSlot; 
  nsIInterfaceRequestor* const mIR; 
};

void PK11PasswordPromptRunnable::RunOnTargetThread()
{
  static NS_DEFINE_CID(kNSSComponentCID, NS_NSSCOMPONENT_CID);

  nsNSSShutDownPreventionLock locker;
  nsresult rv = NS_OK;
  char16_t *password = nullptr;
  bool value = false;
  nsCOMPtr<nsIPrompt> prompt;

  





  if (!mIR)
  {
    nsNSSComponent::GetNewPrompter(getter_AddRefs(prompt));
  }
  else
  {
    prompt = do_GetInterface(mIR);
    NS_ASSERTION(prompt, "callbacks does not implement nsIPrompt");
  }

  if (!prompt)
    return;

  if (PK11_ProtectedAuthenticationPath(mSlot)) {
    mResult = ShowProtectedAuthPrompt(mSlot, mIR);
    return;
  }

  nsAutoString promptString;
  nsCOMPtr<nsINSSComponent> nssComponent(do_GetService(kNSSComponentCID, &rv));

  if (NS_FAILED(rv))
    return; 

  const char16_t* formatStrings[1] = { 
    ToNewUnicode(NS_ConvertUTF8toUTF16(PK11_GetTokenName(mSlot)))
  };
  rv = nssComponent->PIPBundleFormatStringFromName("CertPassPrompt",
                                      formatStrings, 1,
                                      promptString);
  free(const_cast<char16_t*>(formatStrings[0]));

  if (NS_FAILED(rv))
    return;

  {
    nsPSMUITracker tracker;
    if (tracker.isUIForbidden()) {
      rv = NS_ERROR_NOT_AVAILABLE;
    }
    else {
      
      
      bool checkState = false;
      rv = prompt->PromptPassword(nullptr, promptString.get(),
                                  &password, nullptr, &checkState, &value);
    }
  }
  
  if (NS_SUCCEEDED(rv) && value) {
    mResult = ToNewUTF8String(nsDependentString(password));
    free(password);
  }
}

char*
PK11PasswordPrompt(PK11SlotInfo* slot, PRBool retry, void* arg)
{
  RefPtr<PK11PasswordPromptRunnable> runnable(
    new PK11PasswordPromptRunnable(slot,
                                   static_cast<nsIInterfaceRequestor*>(arg)));
  runnable->DispatchToMainThreadAndWait();
  return runnable->mResult;
}


static void
PreliminaryHandshakeDone(PRFileDesc* fd)
{
  nsNSSSocketInfo* infoObject = (nsNSSSocketInfo*) fd->higher->secret;
  if (!infoObject)
    return;

  if (infoObject->IsPreliminaryHandshakeDone())
    return;

  infoObject->SetPreliminaryHandshakeDone();

  SSLChannelInfo channelInfo;
  if (SSL_GetChannelInfo(fd, &channelInfo, sizeof(channelInfo)) == SECSuccess) {
    infoObject->SetSSLVersionUsed(channelInfo.protocolVersion);

    SSLCipherSuiteInfo cipherInfo;
    if (SSL_GetCipherSuiteInfo(channelInfo.cipherSuite, &cipherInfo,
                               sizeof cipherInfo) == SECSuccess) {
      
      RefPtr<nsSSLStatus> status(infoObject->SSLStatus());
      if (!status) {
        status = new nsSSLStatus();
        infoObject->SetSSLStatus(status);
      }

      status->mHaveCipherSuiteAndProtocol = true;
      status->mCipherSuite = channelInfo.cipherSuite;
      status->mProtocolVersion = channelInfo.protocolVersion & 0xFF;
      infoObject->SetKEAUsed(cipherInfo.keaType);
      infoObject->SetKEAKeyBits(channelInfo.keaKeyBits);
      infoObject->SetMACAlgorithmUsed(cipherInfo.macAlgorithm);
    }
  }

  
  SSLNextProtoState state;
  unsigned char npnbuf[256];
  unsigned int npnlen;

  if (SSL_GetNextProto(fd, &state, npnbuf, &npnlen, 256) == SECSuccess) {
    if (state == SSL_NEXT_PROTO_NEGOTIATED ||
        state == SSL_NEXT_PROTO_SELECTED) {
      infoObject->SetNegotiatedNPN(reinterpret_cast<char *>(npnbuf), npnlen);
    }
    else {
      infoObject->SetNegotiatedNPN(nullptr, 0);
    }
    mozilla::Telemetry::Accumulate(Telemetry::SSL_NPN_TYPE, state);
  }
  else {
    infoObject->SetNegotiatedNPN(nullptr, 0);
  }
}

SECStatus
CanFalseStartCallback(PRFileDesc* fd, void* client_data, PRBool *canFalseStart)
{
  *canFalseStart = false;

  nsNSSShutDownPreventionLock locker;

  nsNSSSocketInfo* infoObject = (nsNSSSocketInfo*) fd->higher->secret;
  if (!infoObject) {
    PR_SetError(PR_INVALID_STATE_ERROR, 0);
    return SECFailure;
  }

  infoObject->SetFalseStartCallbackCalled();

  if (infoObject->isAlreadyShutDown()) {
    MOZ_CRASH("SSL socket used after NSS shut down");
    PR_SetError(PR_INVALID_STATE_ERROR, 0);
    return SECFailure;
  }

  PreliminaryHandshakeDone(fd);

  uint32_t reasonsForNotFalseStarting = 0;

  SSLChannelInfo channelInfo;
  if (SSL_GetChannelInfo(fd, &channelInfo, sizeof(channelInfo)) != SECSuccess) {
    return SECSuccess;
  }

  SSLCipherSuiteInfo cipherInfo;
  if (SSL_GetCipherSuiteInfo(channelInfo.cipherSuite, &cipherInfo,
                             sizeof (cipherInfo)) != SECSuccess) {
    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("CanFalseStartCallback [%p] failed - "
                                      " KEA %d\n", fd,
                                      static_cast<int32_t>(cipherInfo.keaType)));
    return SECSuccess;
  }

  nsSSLIOLayerHelpers& helpers = infoObject->SharedState().IOLayerHelpers();

  
  
  if (channelInfo.protocolVersion != SSL_LIBRARY_VERSION_TLS_1_2) {
    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("CanFalseStartCallback [%p] failed - "
                                      "SSL Version must be TLS 1.2, was %x\n", fd,
                                      static_cast<int32_t>(channelInfo.protocolVersion)));
    reasonsForNotFalseStarting |= POSSIBLE_VERSION_DOWNGRADE;
  }

  
  if (cipherInfo.keaType != ssl_kea_ecdh) {
    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("CanFalseStartCallback [%p] failed - "
                                      "unsupported KEA %d\n", fd,
                                      static_cast<int32_t>(cipherInfo.keaType)));
    reasonsForNotFalseStarting |= KEA_NOT_SUPPORTED;
  }

  
  
  
  if (cipherInfo.symCipher != ssl_calg_aes_gcm) {
    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG,
           ("CanFalseStartCallback [%p] failed - Symmetric cipher used, %d, "
            "is not supported with False Start.\n", fd,
            static_cast<int32_t>(cipherInfo.symCipher)));
    reasonsForNotFalseStarting |= POSSIBLE_CIPHER_SUITE_DOWNGRADE;
  }

  
  
  
  
  

  
  
  if (helpers.mFalseStartRequireNPN) {
    nsAutoCString negotiatedNPN;
    if (NS_FAILED(infoObject->GetNegotiatedNPN(negotiatedNPN)) ||
        !negotiatedNPN.Length()) {
      PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("CanFalseStartCallback [%p] failed - "
                                        "NPN cannot be verified\n", fd));
      reasonsForNotFalseStarting |= NPN_NOT_NEGOTIATED;
    }
  }

  Telemetry::Accumulate(Telemetry::SSL_REASONS_FOR_NOT_FALSE_STARTING,
                        reasonsForNotFalseStarting);

  if (reasonsForNotFalseStarting == 0) {
    *canFalseStart = PR_TRUE;
    infoObject->SetFalseStarted();
    infoObject->NoteTimeUntilReady();
    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("CanFalseStartCallback [%p] ok\n", fd));
  }

  return SECSuccess;
}

static void
AccumulateNonECCKeySize(Telemetry::ID probe, uint32_t bits)
{
  unsigned int value = bits <   512 ?  1 : bits ==   512 ?  2
                     : bits <   768 ?  3 : bits ==   768 ?  4
                     : bits <  1024 ?  5 : bits ==  1024 ?  6
                     : bits <  1280 ?  7 : bits ==  1280 ?  8
                     : bits <  1536 ?  9 : bits ==  1536 ? 10
                     : bits <  2048 ? 11 : bits ==  2048 ? 12
                     : bits <  3072 ? 13 : bits ==  3072 ? 14
                     : bits <  4096 ? 15 : bits ==  4096 ? 16
                     : bits <  8192 ? 17 : bits ==  8192 ? 18
                     : bits < 16384 ? 19 : bits == 16384 ? 20
                     : 0;
  Telemetry::Accumulate(probe, value);
}







static void
AccumulateECCCurve(Telemetry::ID probe, uint32_t bits)
{
  unsigned int value = bits == 256 ? 23 
                     : bits == 384 ? 24 
                     : bits == 521 ? 25 
                     : 0; 
  Telemetry::Accumulate(probe, value);
}

static void
AccumulateCipherSuite(Telemetry::ID probe, const SSLChannelInfo& channelInfo)
{
  uint32_t value;
  switch (channelInfo.cipherSuite) {
    
    case TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256: value = 1; break;
    case TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256: value = 2; break;
    case TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA: value = 3; break;
    case TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA: value = 4; break;
    case TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA: value = 5; break;
    case TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA: value = 6; break;
    case TLS_ECDHE_RSA_WITH_3DES_EDE_CBC_SHA: value = 7; break;
    case TLS_ECDHE_RSA_WITH_RC4_128_SHA: value = 8; break;
    case TLS_ECDHE_ECDSA_WITH_RC4_128_SHA: value = 9; break;
    case TLS_ECDHE_ECDSA_WITH_3DES_EDE_CBC_SHA: value = 10; break;
    
    case TLS_DHE_RSA_WITH_AES_128_CBC_SHA: value = 21; break;
    case TLS_DHE_RSA_WITH_CAMELLIA_128_CBC_SHA: value = 22; break;
    case TLS_DHE_RSA_WITH_AES_256_CBC_SHA: value = 23; break;
    case TLS_DHE_RSA_WITH_CAMELLIA_256_CBC_SHA: value = 24; break;
    case TLS_DHE_RSA_WITH_3DES_EDE_CBC_SHA: value = 25; break;
    case TLS_DHE_DSS_WITH_AES_128_CBC_SHA: value = 26; break;
    case TLS_DHE_DSS_WITH_CAMELLIA_128_CBC_SHA: value = 27; break;
    case TLS_DHE_DSS_WITH_AES_256_CBC_SHA: value = 28; break;
    case TLS_DHE_DSS_WITH_CAMELLIA_256_CBC_SHA: value = 29; break;
    case TLS_DHE_DSS_WITH_3DES_EDE_CBC_SHA: value = 30; break;
    
    case TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA: value = 41; break;
    case TLS_ECDH_RSA_WITH_AES_128_CBC_SHA: value = 42; break;
    case TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA: value = 43; break;
    case TLS_ECDH_RSA_WITH_AES_256_CBC_SHA: value = 44; break;
    case TLS_ECDH_ECDSA_WITH_3DES_EDE_CBC_SHA: value = 45; break;
    case TLS_ECDH_RSA_WITH_3DES_EDE_CBC_SHA: value = 46; break;
    case TLS_ECDH_ECDSA_WITH_RC4_128_SHA: value = 47; break;
    case TLS_ECDH_RSA_WITH_RC4_128_SHA: value = 48; break;
    
    case TLS_RSA_WITH_AES_128_CBC_SHA: value = 61; break;
    case TLS_RSA_WITH_CAMELLIA_128_CBC_SHA: value = 62; break;
    case TLS_RSA_WITH_AES_256_CBC_SHA: value = 63; break;
    case TLS_RSA_WITH_CAMELLIA_256_CBC_SHA: value = 64; break;
    case SSL_RSA_FIPS_WITH_3DES_EDE_CBC_SHA: value = 65; break;
    case TLS_RSA_WITH_3DES_EDE_CBC_SHA: value = 66; break;
    case TLS_RSA_WITH_SEED_CBC_SHA: value = 67; break;
    case TLS_RSA_WITH_RC4_128_SHA: value = 68; break;
    case TLS_RSA_WITH_RC4_128_MD5: value = 69; break;
    
    default:
      value = 0;
      break;
  }
  MOZ_ASSERT(value != 0);
  Telemetry::Accumulate(probe, value);
}

void HandshakeCallback(PRFileDesc* fd, void* client_data) {
  nsNSSShutDownPreventionLock locker;
  SECStatus rv;

  nsNSSSocketInfo* infoObject = (nsNSSSocketInfo*) fd->higher->secret;

  
  
  
  PreliminaryHandshakeDone(fd);

  nsSSLIOLayerHelpers& ioLayerHelpers
    = infoObject->SharedState().IOLayerHelpers();

  SSLVersionRange versions(infoObject->GetTLSVersionRange());

  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG,
         ("[%p] HandshakeCallback: succeeded using TLS version range (0x%04x,0x%04x)\n",
          fd, static_cast<unsigned int>(versions.min),
              static_cast<unsigned int>(versions.max)));

  
  ioLayerHelpers.rememberTolerantAtVersion(infoObject->GetHostName(),
                                           infoObject->GetPort(),
                                           versions.max);

  bool usesWeakCipher = false;
  SSLChannelInfo channelInfo;
  rv = SSL_GetChannelInfo(fd, &channelInfo, sizeof(channelInfo));
  MOZ_ASSERT(rv == SECSuccess);
  if (rv == SECSuccess) {
    
    
    unsigned int versionEnum = channelInfo.protocolVersion & 0xFF;
    MOZ_ASSERT(versionEnum > 0);
    Telemetry::Accumulate(Telemetry::SSL_HANDSHAKE_VERSION, versionEnum);
    AccumulateCipherSuite(
      infoObject->IsFullHandshake() ? Telemetry::SSL_CIPHER_SUITE_FULL
                                    : Telemetry::SSL_CIPHER_SUITE_RESUMED,
      channelInfo);

    SSLCipherSuiteInfo cipherInfo;
    rv = SSL_GetCipherSuiteInfo(channelInfo.cipherSuite, &cipherInfo,
                                sizeof cipherInfo);
    MOZ_ASSERT(rv == SECSuccess);
    if (rv == SECSuccess) {
      usesWeakCipher = cipherInfo.symCipher == ssl_calg_rc4;

      
      Telemetry::Accumulate(
        infoObject->IsFullHandshake()
          ? Telemetry::SSL_KEY_EXCHANGE_ALGORITHM_FULL
          : Telemetry::SSL_KEY_EXCHANGE_ALGORITHM_RESUMED,
        cipherInfo.keaType);

      DebugOnly<int16_t> KEAUsed;
      MOZ_ASSERT(NS_SUCCEEDED(infoObject->GetKEAUsed(&KEAUsed)) &&
                 (KEAUsed == cipherInfo.keaType));

      if (infoObject->IsFullHandshake()) {
        switch (cipherInfo.keaType) {
          case ssl_kea_rsa:
            AccumulateNonECCKeySize(Telemetry::SSL_KEA_RSA_KEY_SIZE_FULL,
                                    channelInfo.keaKeyBits);
            break;
          case ssl_kea_dh:
            AccumulateNonECCKeySize(Telemetry::SSL_KEA_DHE_KEY_SIZE_FULL,
                                    channelInfo.keaKeyBits);
            break;
          case ssl_kea_ecdh:
            AccumulateECCCurve(Telemetry::SSL_KEA_ECDHE_CURVE_FULL,
                               channelInfo.keaKeyBits);
            break;
          default:
            MOZ_CRASH("impossible KEA");
            break;
        }

        Telemetry::Accumulate(Telemetry::SSL_AUTH_ALGORITHM_FULL,
                              cipherInfo.authAlgorithm);

        
        if (cipherInfo.keaType != ssl_kea_rsa) {
          switch (cipherInfo.authAlgorithm) {
            case ssl_auth_rsa:
              AccumulateNonECCKeySize(Telemetry::SSL_AUTH_RSA_KEY_SIZE_FULL,
                                      channelInfo.authKeyBits);
              break;
            case ssl_auth_dsa:
              AccumulateNonECCKeySize(Telemetry::SSL_AUTH_DSA_KEY_SIZE_FULL,
                                      channelInfo.authKeyBits);
              break;
            case ssl_auth_ecdsa:
              AccumulateECCCurve(Telemetry::SSL_AUTH_ECDSA_CURVE_FULL,
                                 channelInfo.authKeyBits);
              break;
            default:
              MOZ_CRASH("impossible auth algorithm");
              break;
          }
        }
      }

      Telemetry::Accumulate(
          infoObject->IsFullHandshake()
            ? Telemetry::SSL_SYMMETRIC_CIPHER_FULL
            : Telemetry::SSL_SYMMETRIC_CIPHER_RESUMED,
          cipherInfo.symCipher);
    }
  }

  PRBool siteSupportsSafeRenego;
  rv = SSL_HandshakeNegotiatedExtension(fd, ssl_renegotiation_info_xtn,
                                        &siteSupportsSafeRenego);
  MOZ_ASSERT(rv == SECSuccess);
  if (rv != SECSuccess) {
    siteSupportsSafeRenego = false;
  }
  bool renegotiationUnsafe = !siteSupportsSafeRenego &&
                             ioLayerHelpers.treatUnsafeNegotiationAsBroken();

  uint32_t state;
  if (usesWeakCipher || renegotiationUnsafe) {
    state = nsIWebProgressListener::STATE_IS_BROKEN;
    if (usesWeakCipher) {
      state |= nsIWebProgressListener::STATE_USES_WEAK_CRYPTO;
    }
  } else {
    state = nsIWebProgressListener::STATE_IS_SECURE |
            nsIWebProgressListener::STATE_SECURE_HIGH;
  }
  infoObject->SetSecurityState(state);

  
  
  
  
  
  if (!siteSupportsSafeRenego &&
      ioLayerHelpers.getWarnLevelMissingRFC5746() > 0) {
    nsXPIDLCString hostName;
    infoObject->GetHostName(getter_Copies(hostName));

    nsAutoString msg;
    msg.Append(NS_ConvertASCIItoUTF16(hostName));
    msg.AppendLiteral(" : server does not support RFC 5746, see CVE-2009-3555");

    nsContentUtils::LogSimpleConsoleError(msg, "SSL");
  }

  
  RefPtr<nsSSLStatus> status(infoObject->SSLStatus());
  if (!status) {
    status = new nsSSLStatus();
    infoObject->SetSSLStatus(status);
  }

  RememberCertErrorsTable::GetInstance().LookupCertErrorBits(infoObject,
                                                             status);

  if (status->HasServerCert()) {
    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG,
           ("HandshakeCallback KEEPING existing cert\n"));
  } else {
    ScopedCERTCertificate serverCert(SSL_PeerCertificate(fd));
    RefPtr<nsNSSCertificate> nssc(nsNSSCertificate::Create(serverCert.get()));
    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG,
           ("HandshakeCallback using NEW cert %p\n", nssc.get()));
    status->SetServerCert(nssc, nsNSSCertificate::ev_status_unknown);
  }

  infoObject->NoteTimeUntilReady();
  infoObject->SetHandshakeCompleted();
}
