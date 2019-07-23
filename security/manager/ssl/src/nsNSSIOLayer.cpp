








































#include "nsNSSComponent.h"
#include "nsNSSIOLayer.h"
#include "nsNSSCallbacks.h"

#include "prlog.h"
#include "prnetdb.h"
#include "nsIPrompt.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsIServiceManager.h"
#include "nsIWebProgressListener.h"
#include "nsIChannel.h"
#include "nsNSSCertificate.h"
#include "nsIX509CertValidity.h"
#include "nsIProxyObjectManager.h"
#include "nsProxiedService.h"
#include "nsIDateTimeFormat.h"
#include "nsDateTimeFormatCID.h"
#include "nsIClientAuthDialogs.h"
#include "nsClientAuthRemember.h"
#include "nsICertOverrideService.h"
#include "nsIBadCertListener2.h"
#include "nsISSLErrorListener.h"
#include "nsIObjectInputStream.h"
#include "nsIObjectOutputStream.h"
#include "nsRecentBadCerts.h"
#include "nsISSLCertErrorDialog.h"

#include "nsXPIDLString.h"
#include "nsReadableUtils.h"
#include "nsHashSets.h"
#include "nsCRT.h"
#include "nsAutoPtr.h"
#include "nsPrintfCString.h"
#include "nsAutoLock.h"
#include "nsSSLThread.h"
#include "nsNSSShutDown.h"
#include "nsSSLStatus.h"
#include "nsNSSCertHelper.h"
#include "nsNSSCleaner.h"
#include "nsThreadUtils.h"
#include "nsIDocShell.h"
#include "nsIDocShellTreeItem.h"
#include "nsISecureBrowserUI.h"
#include "nsProxyRelease.h"
#include "nsIClassInfoImpl.h"
#include "nsIProgrammingLanguage.h"

#include "ssl.h"
#include "secerr.h"
#include "sslerr.h"
#include "secder.h"
#include "secasn1.h"
#include "certdb.h"
#include "cert.h"
#include "keyhi.h"
#include "secport.h"



                            
                            

                       
                       
                       
                       
                       

NSSCleanupAutoPtrClass(CERTCertificate, CERT_DestroyCertificate)
NSSCleanupAutoPtrClass(char, PL_strfree)
NSSCleanupAutoPtrClass(void, PR_FREEIF)
NSSCleanupAutoPtrClass_WithParam(PRArenaPool, PORT_FreeArena, FalseParam, PR_FALSE)


typedef enum {ASK, AUTO} SSM_UserCertChoice;


static SECStatus PR_CALLBACK
nsNSS_SSLGetClientAuthData(void *arg, PRFileDesc *socket,
						   CERTDistNames *caNames,
						   CERTCertificate **pRetCert,
						   SECKEYPrivateKey **pRetKey);
static SECStatus PR_CALLBACK
nsNSS_SSLGetClientAuthData(void *arg, PRFileDesc *socket,
						   CERTDistNames *caNames,
						   CERTCertificate **pRetCert,
						   SECKEYPrivateKey **pRetKey);
#ifdef PR_LOGGING
extern PRLogModuleInfo* gPIPNSSLog;
#endif

#if defined(DEBUG_SSL_VERBOSE) && defined (XP_MAC)

#ifdef PR_LOG
#undef PR_LOG
#endif

static PRFileDesc *gMyLogFile = nsnull;
#define MAC_LOG_FILE "MAC PIPNSS Log File"

void MyLogFunction(const char *fmt, ...)
{
  
  va_list ap;
  va_start(ap,fmt);
  if (gMyLogFile == nsnull)
    gMyLogFile = PR_Open(MAC_LOG_FILE, PR_WRONLY | PR_CREATE_FILE | PR_APPEND,
                         0600);
  if (!gMyLogFile)
      return;
  PR_vfprintf(gMyLogFile, fmt, ap);
  va_end(ap);
}

#define PR_LOG(module,level,args) MyLogFunction args
#endif


nsSSLSocketThreadData::nsSSLSocketThreadData()
: mSSLState(ssl_idle)
, mPRErrorCode(PR_SUCCESS)
, mSSLDataBuffer(nsnull)
, mSSLDataBufferAllocatedSize(0)
, mSSLRequestedTransferAmount(0)
, mSSLRemainingReadResultData(nsnull)
, mSSLResultRemainingBytes(0)
, mReplacedSSLFileDesc(nsnull)
, mOneBytePendingFromEarlierWrite(PR_FALSE)
, mThePendingByte(0)
, mOriginalRequestedTransferAmount(0)
{
}

nsSSLSocketThreadData::~nsSSLSocketThreadData()
{
  NS_ASSERTION(mSSLState != ssl_pending_write
               &&
               mSSLState != ssl_pending_read, 
               "oops??? ssl socket is not idle at the time it is being destroyed");
  if (mSSLDataBuffer) {
    nsMemory::Free(mSSLDataBuffer);
  }
}

PRBool nsSSLSocketThreadData::ensure_buffer_size(PRInt32 amount)
{
  if (amount > mSSLDataBufferAllocatedSize) {
    if (mSSLDataBuffer) {
      mSSLDataBuffer = (char*)nsMemory::Realloc(mSSLDataBuffer, amount);
    }
    else {
      mSSLDataBuffer = (char*)nsMemory::Alloc(amount);
    }
    
    if (!mSSLDataBuffer)
      return PR_FALSE;

    mSSLDataBufferAllocatedSize = amount;
  }
  
  return PR_TRUE;
}

nsNSSSocketInfo::nsNSSSocketInfo()
  : mFd(nsnull),
    mBlockingState(blocking_state_unknown),
    mSecurityState(nsIWebProgressListener::STATE_IS_INSECURE),
    mSubRequestsHighSecurity(0),
    mSubRequestsLowSecurity(0),
    mSubRequestsBrokenSecurity(0),
    mSubRequestsNoSecurity(0),
    mDocShellDependentStuffKnown(PR_FALSE),
    mExternalErrorReporting(PR_FALSE),
    mForSTARTTLS(PR_FALSE),
    mHandshakePending(PR_TRUE),
    mCanceled(PR_FALSE),
    mHasCleartextPhase(PR_FALSE),
    mHandshakeInProgress(PR_FALSE),
    mAllowTLSIntoleranceTimeout(PR_TRUE),
    mRememberClientAuthCertificate(PR_FALSE),
    mHandshakeStartTime(0),
    mPort(0)
{
  mThreadData = new nsSSLSocketThreadData;
}

nsNSSSocketInfo::~nsNSSSocketInfo()
{
  delete mThreadData;

  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown())
    return;

  shutdown(calledFromObject);
}

void nsNSSSocketInfo::virtualDestroyNSSReference()
{
}

NS_IMPL_THREADSAFE_ISUPPORTS9(nsNSSSocketInfo,
                              nsITransportSecurityInfo,
                              nsISSLSocketControl,
                              nsIInterfaceRequestor,
                              nsISSLStatusProvider,
                              nsIIdentityInfo,
                              nsIAssociatedContentSecurity,
                              nsISerializable,
                              nsIClassInfo,
                              nsIClientAuthUserDecision)

nsresult
nsNSSSocketInfo::GetHandshakePending(PRBool *aHandshakePending)
{
  *aHandshakePending = mHandshakePending;
  return NS_OK;
}

nsresult
nsNSSSocketInfo::SetHandshakePending(PRBool aHandshakePending)
{
  mHandshakePending = aHandshakePending;
  return NS_OK;
}

nsresult
nsNSSSocketInfo::SetHostName(const char* host)
{
  mHostName.Adopt(host ? NS_strdup(host) : 0);
  return NS_OK;
}

nsresult
nsNSSSocketInfo::GetHostName(char **host)
{
  *host = (mHostName) ? NS_strdup(mHostName) : nsnull;
  return NS_OK;
}

nsresult
nsNSSSocketInfo::SetPort(PRInt32 aPort)
{
  mPort = aPort;
  return NS_OK;
}

nsresult
nsNSSSocketInfo::GetPort(PRInt32 *aPort)
{
  *aPort = mPort;
  return NS_OK;
}

void nsNSSSocketInfo::SetCanceled(PRBool aCanceled)
{
  mCanceled = aCanceled;
}

PRBool nsNSSSocketInfo::GetCanceled()
{
  return mCanceled;
}

NS_IMETHODIMP nsNSSSocketInfo::GetRememberClientAuthCertificate(PRBool *aRememberClientAuthCertificate)
{
  NS_ENSURE_ARG_POINTER(aRememberClientAuthCertificate);
  *aRememberClientAuthCertificate = mRememberClientAuthCertificate;
  return NS_OK;
}

NS_IMETHODIMP nsNSSSocketInfo::SetRememberClientAuthCertificate(PRBool aRememberClientAuthCertificate)
{
  mRememberClientAuthCertificate = aRememberClientAuthCertificate;
  return NS_OK;
}

void nsNSSSocketInfo::SetHasCleartextPhase(PRBool aHasCleartextPhase)
{
  mHasCleartextPhase = aHasCleartextPhase;
}

PRBool nsNSSSocketInfo::GetHasCleartextPhase()
{
  return mHasCleartextPhase;
}

NS_IMETHODIMP
nsNSSSocketInfo::GetNotificationCallbacks(nsIInterfaceRequestor** aCallbacks)
{
  *aCallbacks = mCallbacks;
  NS_IF_ADDREF(*aCallbacks);
  return NS_OK;
}

NS_IMETHODIMP
nsNSSSocketInfo::SetNotificationCallbacks(nsIInterfaceRequestor* aCallbacks)
{
  if (!aCallbacks) {
    mCallbacks = nsnull;
    return NS_OK;
  }

  mCallbacks = aCallbacks;
  mDocShellDependentStuffKnown = PR_FALSE;

  return NS_OK;
}

nsresult
nsNSSSocketInfo::EnsureDocShellDependentStuffKnown()
{
  if (mDocShellDependentStuffKnown)
    return NS_OK;

  if (!mCallbacks || nsSSLThread::exitRequested())
    return NS_ERROR_FAILURE;

  mDocShellDependentStuffKnown = PR_TRUE;

  nsCOMPtr<nsIInterfaceRequestor> proxiedCallbacks;
  NS_GetProxyForObject(NS_PROXY_TO_MAIN_THREAD,
                       NS_GET_IID(nsIInterfaceRequestor),
                       static_cast<nsIInterfaceRequestor*>(mCallbacks),
                       NS_PROXY_SYNC,
                       getter_AddRefs(proxiedCallbacks));

  
  
  
  
  
  
  
  

  nsCOMPtr<nsIDocShell> docshell;

  nsCOMPtr<nsIDocShellTreeItem> item(do_GetInterface(proxiedCallbacks));
  if (item)
  {
    nsCOMPtr<nsIDocShellTreeItem> proxiedItem;
    nsCOMPtr<nsIDocShellTreeItem> rootItem;
    NS_GetProxyForObject(NS_PROXY_TO_MAIN_THREAD,
                         NS_GET_IID(nsIDocShellTreeItem),
                         item.get(),
                         NS_PROXY_SYNC,
                         getter_AddRefs(proxiedItem));

    proxiedItem->GetSameTypeRootTreeItem(getter_AddRefs(rootItem));
    docshell = do_QueryInterface(rootItem);
    NS_ASSERTION(docshell, "rootItem do_QI is null");
  }

  if (docshell)
  {
    nsCOMPtr<nsIDocShell> proxiedDocShell;
    NS_GetProxyForObject(NS_PROXY_TO_MAIN_THREAD,
                         NS_GET_IID(nsIDocShell),
                         docshell.get(),
                         NS_PROXY_SYNC,
                         getter_AddRefs(proxiedDocShell));
    nsISecureBrowserUI* secureUI;
    proxiedDocShell->GetSecurityUI(&secureUI);
    if (secureUI)
    {
      nsCOMPtr<nsIThread> mainThread(do_GetMainThread());
      NS_ProxyRelease(mainThread, secureUI, PR_FALSE);
      mExternalErrorReporting = PR_TRUE;

      
      
      
      
      nsCOMPtr<nsISSLStatusProvider> statprov = do_QueryInterface(secureUI);
      if (statprov) {
        nsCOMPtr<nsISupports> isup_stat;
        statprov->GetSSLStatus(getter_AddRefs(isup_stat));
        if (isup_stat) {
          nsCOMPtr<nsISSLStatus> sslstat = do_QueryInterface(isup_stat);
          if (sslstat) {
            sslstat->GetServerCert(getter_AddRefs(mPreviousCert));
          }
        }
      }
    }
  }

  return NS_OK;
}

nsresult
nsNSSSocketInfo::GetExternalErrorReporting(PRBool* state)
{
  nsresult rv = EnsureDocShellDependentStuffKnown();
  NS_ENSURE_SUCCESS(rv, rv);
  *state = mExternalErrorReporting;
  return NS_OK;
}

nsresult
nsNSSSocketInfo::SetExternalErrorReporting(PRBool aState)
{
  mExternalErrorReporting = aState;
  return NS_OK;
}

NS_IMETHODIMP
nsNSSSocketInfo::GetSecurityState(PRUint32* state)
{
  *state = mSecurityState;
  return NS_OK;
}

nsresult
nsNSSSocketInfo::SetSecurityState(PRUint32 aState)
{
  mSecurityState = aState;
  return NS_OK;
}


NS_IMETHODIMP nsNSSSocketInfo::GetCountSubRequestsHighSecurity(PRInt32 *aSubRequestsHighSecurity)
{
  *aSubRequestsHighSecurity = mSubRequestsHighSecurity;
  return NS_OK;
}
NS_IMETHODIMP nsNSSSocketInfo::SetCountSubRequestsHighSecurity(PRInt32 aSubRequestsHighSecurity)
{
  mSubRequestsHighSecurity = aSubRequestsHighSecurity;
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP nsNSSSocketInfo::GetCountSubRequestsLowSecurity(PRInt32 *aSubRequestsLowSecurity)
{
  *aSubRequestsLowSecurity = mSubRequestsLowSecurity;
  return NS_OK;
}
NS_IMETHODIMP nsNSSSocketInfo::SetCountSubRequestsLowSecurity(PRInt32 aSubRequestsLowSecurity)
{
  mSubRequestsLowSecurity = aSubRequestsLowSecurity;
  return NS_OK;
}


NS_IMETHODIMP nsNSSSocketInfo::GetCountSubRequestsBrokenSecurity(PRInt32 *aSubRequestsBrokenSecurity)
{
  *aSubRequestsBrokenSecurity = mSubRequestsBrokenSecurity;
  return NS_OK;
}
NS_IMETHODIMP nsNSSSocketInfo::SetCountSubRequestsBrokenSecurity(PRInt32 aSubRequestsBrokenSecurity)
{
  mSubRequestsBrokenSecurity = aSubRequestsBrokenSecurity;
  return NS_OK;
}


NS_IMETHODIMP nsNSSSocketInfo::GetCountSubRequestsNoSecurity(PRInt32 *aSubRequestsNoSecurity)
{
  *aSubRequestsNoSecurity = mSubRequestsNoSecurity;
  return NS_OK;
}
NS_IMETHODIMP nsNSSSocketInfo::SetCountSubRequestsNoSecurity(PRInt32 aSubRequestsNoSecurity)
{
  mSubRequestsNoSecurity = aSubRequestsNoSecurity;
  return NS_OK;
}

NS_IMETHODIMP
nsNSSSocketInfo::GetShortSecurityDescription(PRUnichar** aText) {
  if (mShortDesc.IsEmpty())
    *aText = nsnull;
  else {
    *aText = ToNewUnicode(mShortDesc);
    NS_ENSURE_TRUE(*aText, NS_ERROR_OUT_OF_MEMORY);
  }
  return NS_OK;
}

nsresult
nsNSSSocketInfo::SetShortSecurityDescription(const PRUnichar* aText) {
  mShortDesc.Assign(aText);
  return NS_OK;
}

NS_IMETHODIMP
nsNSSSocketInfo::GetErrorMessage(PRUnichar** aText) {
  if (mErrorMessage.IsEmpty())
    *aText = nsnull;
  else {
    *aText = ToNewUnicode(mErrorMessage);
    NS_ENSURE_TRUE(*aText, NS_ERROR_OUT_OF_MEMORY);
  }
  return NS_OK;
}

nsresult
nsNSSSocketInfo::SetErrorMessage(const PRUnichar* aText) {
  mErrorMessage.Assign(aText);
  return NS_OK;
}


NS_IMETHODIMP nsNSSSocketInfo::GetInterface(const nsIID & uuid, void * *result)
{
  nsresult rv;
  if (!mCallbacks) {
    nsCOMPtr<nsIInterfaceRequestor> ir = new PipUIContext();
    if (!ir)
      return NS_ERROR_OUT_OF_MEMORY;

    rv = ir->GetInterface(uuid, result);
  } else {
    if (nsSSLThread::exitRequested())
      return NS_ERROR_FAILURE;

    nsCOMPtr<nsIInterfaceRequestor> proxiedCallbacks;
    NS_GetProxyForObject(NS_PROXY_TO_MAIN_THREAD,
                         NS_GET_IID(nsIInterfaceRequestor),
                         mCallbacks,
                         NS_PROXY_SYNC,
                         getter_AddRefs(proxiedCallbacks));

    rv = proxiedCallbacks->GetInterface(uuid, result);
  }
  return rv;
}

nsresult
nsNSSSocketInfo::GetForSTARTTLS(PRBool* aForSTARTTLS)
{
  *aForSTARTTLS = mForSTARTTLS;
  return NS_OK;
}

nsresult
nsNSSSocketInfo::SetForSTARTTLS(PRBool aForSTARTTLS)
{
  mForSTARTTLS = aForSTARTTLS;
  return NS_OK;
}

NS_IMETHODIMP
nsNSSSocketInfo::ProxyStartSSL()
{
  return ActivateSSL();
}

NS_IMETHODIMP
nsNSSSocketInfo::StartTLS()
{
  return ActivateSSL();
}

NS_IMETHODIMP
nsNSSSocketInfo::Write(nsIObjectOutputStream* stream) {
  stream->WriteCompoundObject(NS_ISUPPORTS_CAST(nsIX509Cert*, mCert),
                              NS_GET_IID(nsISupports), PR_TRUE);

  
  
  
  
  
  
  PRUint32 version = 2;
  stream->Write32(version | 0xFFFF0000);
  stream->Write32(mSecurityState);
  stream->WriteWStringZ(mShortDesc.get());
  stream->WriteWStringZ(mErrorMessage.get());

  stream->WriteCompoundObject(NS_ISUPPORTS_CAST(nsISSLStatus*, mSSLStatus),
                              NS_GET_IID(nsISupports), PR_TRUE);

  stream->Write32((PRUint32)mSubRequestsHighSecurity);
  stream->Write32((PRUint32)mSubRequestsLowSecurity);
  stream->Write32((PRUint32)mSubRequestsBrokenSecurity);
  stream->Write32((PRUint32)mSubRequestsNoSecurity);
  return NS_OK;
}

NS_IMETHODIMP
nsNSSSocketInfo::Read(nsIObjectInputStream* stream) {
  nsCOMPtr<nsISupports> obj;
  stream->ReadObject(PR_TRUE, getter_AddRefs(obj));
  mCert = reinterpret_cast<nsNSSCertificate*>(obj.get());

  PRUint32 version;
  stream->Read32(&version);
  
  
  
  if ((version & 0xFFFF0000) == 0xFFFF0000) {
      version &= ~0xFFFF0000;
      stream->Read32(&mSecurityState);
  }
  else {
      mSecurityState = version;
      version = 1;
  }
  stream->ReadString(mShortDesc);
  stream->ReadString(mErrorMessage);

  stream->ReadObject(PR_TRUE, getter_AddRefs(obj));
  mSSLStatus = reinterpret_cast<nsSSLStatus*>(obj.get());

  if (version >= 2) {
    stream->Read32((PRUint32*)&mSubRequestsHighSecurity);
    stream->Read32((PRUint32*)&mSubRequestsLowSecurity);
    stream->Read32((PRUint32*)&mSubRequestsBrokenSecurity);
    stream->Read32((PRUint32*)&mSubRequestsNoSecurity);
  }
  else {
    mSubRequestsHighSecurity = 0;
    mSubRequestsLowSecurity = 0;
    mSubRequestsBrokenSecurity = 0;
    mSubRequestsNoSecurity = 0;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsNSSSocketInfo::GetInterfaces(PRUint32 *count, nsIID * **array)
{
  *count = 0;
  *array = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsNSSSocketInfo::GetHelperForLanguage(PRUint32 language, nsISupports **_retval)
{
  *_retval = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsNSSSocketInfo::GetContractID(char * *aContractID)
{
  *aContractID = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsNSSSocketInfo::GetClassDescription(char * *aClassDescription)
{
  *aClassDescription = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsNSSSocketInfo::GetClassID(nsCID * *aClassID)
{
  *aClassID = (nsCID*) nsMemory::Alloc(sizeof(nsCID));
  if (!*aClassID)
    return NS_ERROR_OUT_OF_MEMORY;
  return GetClassIDNoAlloc(*aClassID);
}

NS_IMETHODIMP
nsNSSSocketInfo::GetImplementationLanguage(PRUint32 *aImplementationLanguage)
{
  *aImplementationLanguage = nsIProgrammingLanguage::CPLUSPLUS;
  return NS_OK;
}

NS_IMETHODIMP
nsNSSSocketInfo::GetFlags(PRUint32 *aFlags)
{
  *aFlags = 0;
  return NS_OK;
}

static NS_DEFINE_CID(kNSSSocketInfoCID, NS_NSSSOCKETINFO_CID);

NS_IMETHODIMP
nsNSSSocketInfo::GetClassIDNoAlloc(nsCID *aClassIDNoAlloc)
{
  *aClassIDNoAlloc = kNSSSocketInfoCID;
  return NS_OK;
}

nsresult nsNSSSocketInfo::ActivateSSL()
{
  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown())
    return NS_ERROR_NOT_AVAILABLE;

  nsresult rv = nsSSLThread::requestActivateSSL(this);
  
  if (NS_FAILED(rv))
    return rv;

  mHandshakePending = PR_TRUE;

  return NS_OK;
}

nsresult nsNSSSocketInfo::GetFileDescPtr(PRFileDesc** aFilePtr)
{
  *aFilePtr = mFd;
  return NS_OK;
}

nsresult nsNSSSocketInfo::SetFileDescPtr(PRFileDesc* aFilePtr)
{
  mFd = aFilePtr;
  return NS_OK;
}

nsresult nsNSSSocketInfo::GetPreviousCert(nsIX509Cert** _result)
{
  NS_ENSURE_ARG_POINTER(_result);
  nsresult rv = EnsureDocShellDependentStuffKnown();
  NS_ENSURE_SUCCESS(rv, rv);

  *_result = mPreviousCert;
  NS_IF_ADDREF(*_result);

  return NS_OK;
}

nsresult nsNSSSocketInfo::GetCert(nsIX509Cert** _result)
{
  NS_ENSURE_ARG_POINTER(_result);

  *_result = mCert;
  NS_IF_ADDREF(*_result);

  return NS_OK;
}

nsresult nsNSSSocketInfo::SetCert(nsIX509Cert *aCert)
{
  mCert = aCert;

  return NS_OK;
}

nsresult nsNSSSocketInfo::GetSSLStatus(nsISupports** _result)
{
  NS_ENSURE_ARG_POINTER(_result);

  *_result = NS_ISUPPORTS_CAST(nsISSLStatus*, mSSLStatus);
  NS_IF_ADDREF(*_result);

  return NS_OK;
}

nsresult nsNSSSocketInfo::SetSSLStatus(nsSSLStatus *aSSLStatus)
{
  mSSLStatus = aSSLStatus;

  return NS_OK;
}

void nsNSSSocketInfo::SetHandshakeInProgress(PRBool aIsIn)
{
  mHandshakeInProgress = aIsIn;

  if (mHandshakeInProgress && !mHandshakeStartTime)
  {
    mHandshakeStartTime = PR_IntervalNow();
  }
}

void nsNSSSocketInfo::SetAllowTLSIntoleranceTimeout(PRBool aAllow)
{
  mAllowTLSIntoleranceTimeout = aAllow;
}

#define HANDSHAKE_TIMEOUT_SECONDS 25

PRBool nsNSSSocketInfo::HandshakeTimeout()
{
  if (!mHandshakeInProgress || !mAllowTLSIntoleranceTimeout)
    return PR_FALSE;

  return ((PRIntervalTime)(PR_IntervalNow() - mHandshakeStartTime)
          > PR_SecondsToInterval(HANDSHAKE_TIMEOUT_SECONDS));
}

void nsSSLIOLayerHelpers::Cleanup()
{
  if (mTLSIntolerantSites) {
    delete mTLSIntolerantSites;
    mTLSIntolerantSites = nsnull;
  }

  if (mTLSTolerantSites) {
    delete mTLSTolerantSites;
    mTLSTolerantSites = nsnull;
  }

  if (mSharedPollableEvent)
    PR_DestroyPollableEvent(mSharedPollableEvent);

  if (mutex) {
    PR_DestroyLock(mutex);
    mutex = nsnull;
  }

  if (mHostsWithCertErrors) {
    delete mHostsWithCertErrors;
    mHostsWithCertErrors = nsnull;
  }
}

static nsresult
getErrorMessage(PRInt32 err, 
                const nsString &host,
                PRInt32 port,
                PRBool externalErrorReporting,
                nsINSSComponent *component,
                nsString &returnedMessage)
{
  NS_ENSURE_ARG_POINTER(component);

  const PRUnichar *params[1];
  nsresult rv;

  if (host.Length())
  {
    nsString hostWithPort;

    
    
    
    
    
    

    if (externalErrorReporting && port == 443) {
      params[0] = host.get();
    }
    else {
      hostWithPort = host;
      hostWithPort.AppendLiteral(":");
      hostWithPort.AppendInt(port);
      params[0] = hostWithPort.get();
    }

    nsString formattedString;
    rv = component->PIPBundleFormatStringFromName("SSLConnectionErrorPrefix", 
                                                  params, 1, 
                                                  formattedString);
    if (NS_SUCCEEDED(rv))
    {
      returnedMessage.Append(formattedString);
      returnedMessage.Append(NS_LITERAL_STRING("\n\n"));
    }
  }

  nsString explanation;
  rv = nsNSSErrors::getErrorMessageFromCode(err, component, explanation);
  if (NS_SUCCEEDED(rv))
    returnedMessage.Append(explanation);

  return NS_OK;
}

static void
AppendErrorTextUntrusted(PRErrorCode errTrust,
                         const nsString &host,
                         nsIX509Cert* ix509,
                         nsINSSComponent *component,
                         nsString &returnedMessage)
{
  const char *errorID = nsnull;
  nsCOMPtr<nsIX509Cert3> cert3 = do_QueryInterface(ix509);
  if (cert3) {
    PRBool isSelfSigned;
    if (NS_SUCCEEDED(cert3->GetIsSelfSigned(&isSelfSigned))
        && isSelfSigned) {
      errorID = "certErrorTrust_SelfSigned";
    }
  }

  if (!errorID) {
    switch (errTrust) {
      case SEC_ERROR_UNKNOWN_ISSUER:
        errorID = "certErrorTrust_UnknownIssuer";
        break;
      case SEC_ERROR_INADEQUATE_KEY_USAGE:
        
        
      case SEC_ERROR_CA_CERT_INVALID:
        errorID = "certErrorTrust_CaInvalid";
        break;
      case SEC_ERROR_UNTRUSTED_ISSUER:
        errorID = "certErrorTrust_Issuer";
        break;
      case SEC_ERROR_EXPIRED_ISSUER_CERTIFICATE:
        errorID = "certErrorTrust_ExpiredIssuer";
        break;
      case SEC_ERROR_UNTRUSTED_CERT:
      default:
        errorID = "certErrorTrust_Untrusted";
        break;
    }
  }

  nsString formattedString;
  nsresult rv = component->GetPIPNSSBundleString(errorID, 
                                                 formattedString);
  if (NS_SUCCEEDED(rv))
  {
    returnedMessage.Append(formattedString);
    returnedMessage.Append(NS_LITERAL_STRING("\n"));
  }
}





static PRBool
GetSubjectAltNames(CERTCertificate *nssCert,
                   nsINSSComponent *component,
                   nsString &allNames,
                   PRUint32 &nameCount)
{
  allNames.Truncate();
  nameCount = 0;

  PRArenaPool *san_arena = nsnull;
  SECItem altNameExtension = {siBuffer, NULL, 0 };
  CERTGeneralName *sanNameList = nsnull;
  PRBool ok = PR_FALSE;

  san_arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
  if (!san_arena)
    return ok;

  nsresult rv;
  rv = CERT_FindCertExtension(nssCert, SEC_OID_X509_SUBJECT_ALT_NAME,
                              &altNameExtension);
  if (rv != SECSuccess)
    goto loser;

  sanNameList = CERT_DecodeAltNameExtension(san_arena, &altNameExtension);
  SECITEM_FreeItem(&altNameExtension, PR_FALSE);
  if (!sanNameList)
    goto loser;

  CERTGeneralName *current = sanNameList;
  do {
    nsAutoString name;
    switch (current->type) {
      case certDNSName:
        name.AssignASCII((char*)current->name.other.data, current->name.other.len);
        if (!allNames.IsEmpty()) {
          allNames.Append(NS_LITERAL_STRING(" , "));
        }
        ++nameCount;
        allNames.Append(name);
        break;

      case certIPAddress:
        {
          char buf[INET6_ADDRSTRLEN];
          PRNetAddr addr;
          if (current->name.other.len == 4) {
            addr.inet.family = PR_AF_INET;
            memcpy(&addr.inet.ip, current->name.other.data, current->name.other.len);
            PR_NetAddrToString(&addr, buf, sizeof(buf));
            name.AssignASCII(buf);
          } else if (current->name.other.len == 16) {
            addr.ipv6.family = PR_AF_INET6;
            memcpy(&addr.ipv6.ip, current->name.other.data, current->name.other.len);
            PR_NetAddrToString(&addr, buf, sizeof(buf));
            name.AssignASCII(buf);
          } else {
            
          }
          if (!name.IsEmpty()) {
            if (!allNames.IsEmpty()) {
              allNames.Append(NS_LITERAL_STRING(" , "));
            }
            ++nameCount;
            allNames.Append(name);
          }
          break;
        }

      default: 
        break;
    }
    current = CERT_GetNextGeneralName(current);
  } while (current != sanNameList); 
  ok = PR_TRUE;

loser:
  PORT_FreeArena(san_arena, PR_FALSE);
  return ok;
}

static void
AppendErrorTextMismatch(const nsString &host,
                        nsIX509Cert* ix509,
                        nsINSSComponent *component,
                        PRBool wantsHtml,
                        nsString &returnedMessage)
{
  const PRUnichar *params[1];
  nsresult rv;

  CERTCertificate *nssCert = NULL;
  CERTCertificateCleaner nssCertCleaner(nssCert);

  nsCOMPtr<nsIX509Cert2> cert2 = do_QueryInterface(ix509, &rv);
  if (cert2)
    nssCert = cert2->GetCert();

  if (!nssCert) {
    
    params[0] = host.get();
    nsString formattedString;
    rv = component->PIPBundleFormatStringFromName("certErrorMismatch", 
                                                  params, 1, 
                                                  formattedString);
    if (NS_SUCCEEDED(rv)) {
      returnedMessage.Append(formattedString);
      returnedMessage.Append(NS_LITERAL_STRING("\n"));
    }
    return;
  }

  nsString allNames;
  PRUint32 nameCount = 0;
  PRBool useSAN = PR_FALSE;

  if (nssCert)
    useSAN = GetSubjectAltNames(nssCert, component, allNames, nameCount);

  if (!useSAN) {
    char *certName = nsnull;
    
    
    
    
    if (!certName)
      certName = CERT_GetCommonName(&nssCert->subject);
    if (certName) {
      ++nameCount;
      allNames.AssignASCII(certName);
      PORT_Free(certName);
    }
  }

  if (nameCount > 1) {
    nsString message;
    rv = component->GetPIPNSSBundleString("certErrorMismatchMultiple", 
                                          message);
    if (NS_SUCCEEDED(rv)) {
      returnedMessage.Append(message);
      returnedMessage.Append(NS_LITERAL_STRING("\n  "));
      returnedMessage.Append(allNames);
      returnedMessage.Append(NS_LITERAL_STRING("  \n"));
    }
  }
  else if (nameCount == 1) {
    const PRUnichar *params[1];
    params[0] = allNames.get();

    const char *stringID;
    if (wantsHtml)
      stringID = "certErrorMismatchSingle2";
    else
      stringID = "certErrorMismatchSinglePlain";

    nsString formattedString;
    rv = component->PIPBundleFormatStringFromName(stringID, 
                                                  params, 1, 
                                                  formattedString);
    if (NS_SUCCEEDED(rv)) {
      returnedMessage.Append(formattedString);
      returnedMessage.Append(NS_LITERAL_STRING("\n"));
    }
  }
  else { 
    nsString message;
    nsresult rv = component->GetPIPNSSBundleString("certErrorMismatchNoNames",
                                                   message);
    if (NS_SUCCEEDED(rv)) {
      returnedMessage.Append(message);
      returnedMessage.Append(NS_LITERAL_STRING("\n"));
    }
  }
}

static void
GetDateBoundary(nsIX509Cert* ix509,
                nsString &formattedDate,
                PRBool &trueExpired_falseNotYetValid)
{
  trueExpired_falseNotYetValid = PR_TRUE;
  formattedDate.Truncate();

  PRTime notAfter, notBefore, timeToUse;
  nsCOMPtr<nsIX509CertValidity> validity;
  nsresult rv;

  rv = ix509->GetValidity(getter_AddRefs(validity));
  if (NS_FAILED(rv))
    return;

  rv = validity->GetNotAfter(&notAfter);
  if (NS_FAILED(rv))
    return;

  rv = validity->GetNotBefore(&notBefore);
  if (NS_FAILED(rv))
    return;

  if (LL_CMP(PR_Now(), >, notAfter)) {
    timeToUse = notAfter;
  } else {
    timeToUse = notBefore;
    trueExpired_falseNotYetValid = PR_FALSE;
  }

  nsIDateTimeFormat* aDateTimeFormat;
  rv = CallCreateInstance(NS_DATETIMEFORMAT_CONTRACTID, &aDateTimeFormat);
  if (NS_FAILED(rv))
    return;

  aDateTimeFormat->FormatPRTime(nsnull, kDateFormatShort, 
                                kTimeFormatNoSeconds, timeToUse, 
                                formattedDate);
  NS_IF_RELEASE(aDateTimeFormat);
}

static void
AppendErrorTextTime(nsIX509Cert* ix509,
                    nsINSSComponent *component,
                    nsString &returnedMessage)
{
  nsAutoString formattedDate;
  PRBool trueExpired_falseNotYetValid;
  GetDateBoundary(ix509, formattedDate, trueExpired_falseNotYetValid);

  const PRUnichar *params[1];
  params[0] = formattedDate.get(); 

  const char *key = trueExpired_falseNotYetValid ? 
                    "certErrorExpired" : "certErrorNotYetValid";
  nsresult rv;
  nsString formattedString;
  rv = component->PIPBundleFormatStringFromName(key, params, 
                                                1, formattedString);
  if (NS_SUCCEEDED(rv))
  {
    returnedMessage.Append(formattedString);
    returnedMessage.Append(NS_LITERAL_STRING("\n"));
  }
}

static void
AppendErrorTextCode(PRErrorCode errorCodeToReport,
                    nsINSSComponent *component,
                    nsString &returnedMessage)
{
  const char *codeName = nsNSSErrors::getDefaultErrorStringName(errorCodeToReport);
  if (codeName)
  {
    nsCString error_id(codeName);
    ToLowerCase(error_id);
    NS_ConvertASCIItoUTF16 idU(error_id);

    const PRUnichar *params[1];
    params[0] = idU.get();

    nsString formattedString;
    nsresult rv;
    rv = component->PIPBundleFormatStringFromName("certErrorCodePrefix", 
                                                  params, 1, 
                                                  formattedString);
    if (NS_SUCCEEDED(rv)) {
      returnedMessage.Append(NS_LITERAL_STRING("\n"));
      returnedMessage.Append(formattedString);
      returnedMessage.Append(NS_LITERAL_STRING("\n"));
    }
    else {
      returnedMessage.Append(NS_LITERAL_STRING(" ("));
      returnedMessage.Append(idU);
      returnedMessage.Append(NS_LITERAL_STRING(")"));
    }
  }
}

static nsresult
getInvalidCertErrorMessage(PRUint32 multipleCollectedErrors, 
                           PRErrorCode errorCodeToReport, 
                           PRErrorCode errTrust, 
                           PRErrorCode errMismatch, 
                           PRErrorCode errExpired,
                           const nsString &host,
                           const nsString &hostWithPort,
                           PRInt32 port,
                           nsIX509Cert* ix509,
                           PRBool externalErrorReporting,
                           PRBool wantsHtml,
                           nsINSSComponent *component,
                           nsString &returnedMessage)
{
  NS_ENSURE_ARG_POINTER(component);

  const PRUnichar *params[1];
  nsresult rv;

  
  
  
  
  
  
  
  if (externalErrorReporting && port == 443)
    params[0] = host.get();
  else
    params[0] = hostWithPort.get();

  nsString formattedString;
  rv = component->PIPBundleFormatStringFromName("certErrorIntro", 
                                                params, 1, 
                                                formattedString);
  if (NS_SUCCEEDED(rv))
  {
    returnedMessage.Append(formattedString);
    returnedMessage.Append(NS_LITERAL_STRING("\n\n"));
  }

  if (multipleCollectedErrors & nsICertOverrideService::ERROR_UNTRUSTED)
  {
    AppendErrorTextUntrusted(errTrust, host, ix509, 
                             component, returnedMessage);
  }

  if (multipleCollectedErrors & nsICertOverrideService::ERROR_MISMATCH)
  {
    AppendErrorTextMismatch(host, ix509, component, wantsHtml, returnedMessage);
  }

  if (multipleCollectedErrors & nsICertOverrideService::ERROR_TIME)
  {
    AppendErrorTextTime(ix509, component, returnedMessage);
  }

  AppendErrorTextCode(errorCodeToReport, component, returnedMessage);

  return NS_OK;
}

static nsresult
displayAlert(nsAFlatString &formattedString, nsNSSSocketInfo *infoObject)
{
  
  

  if (nsSSLThread::exitRequested())
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIInterfaceRequestor> proxiedCallbacks;
  NS_GetProxyForObject(NS_PROXY_TO_MAIN_THREAD,
                       NS_GET_IID(nsIInterfaceRequestor),
                       static_cast<nsIInterfaceRequestor*>(infoObject),
                       NS_PROXY_SYNC,
                       getter_AddRefs(proxiedCallbacks));

  nsCOMPtr<nsIPrompt> prompt (do_GetInterface(proxiedCallbacks));
  if (!prompt)
    return NS_ERROR_NO_INTERFACE;

  

  nsCOMPtr<nsIPrompt> proxyPrompt;
  NS_GetProxyForObject(NS_PROXY_TO_MAIN_THREAD,
                       NS_GET_IID(nsIPrompt),
                       prompt,
                       NS_PROXY_SYNC,
                       getter_AddRefs(proxyPrompt));

  proxyPrompt->Alert(nsnull, formattedString.get());
  return NS_OK;
}

static nsresult
nsHandleSSLError(nsNSSSocketInfo *socketInfo, PRInt32 err)
{
  if (socketInfo->GetCanceled()) {
    
    
    
    return NS_OK;
  }

  if (nsSSLThread::exitRequested()) {
    return NS_ERROR_FAILURE;
  }

  nsresult rv;
  NS_DEFINE_CID(nssComponentCID, NS_NSSCOMPONENT_CID);
  nsCOMPtr<nsINSSComponent> nssComponent(do_GetService(nssComponentCID, &rv));
  if (NS_FAILED(rv))
    return rv;

  nsXPIDLCString hostName;
  socketInfo->GetHostName(getter_Copies(hostName));
  NS_ConvertASCIItoUTF16 hostNameU(hostName);

  PRInt32 port;
  socketInfo->GetPort(&port);

  
  nsCOMPtr<nsIInterfaceRequestor> cb;
  socketInfo->GetNotificationCallbacks(getter_AddRefs(cb));
  if (cb) {
    nsCOMPtr<nsIInterfaceRequestor> callbacks;
    NS_GetProxyForObject(NS_PROXY_TO_MAIN_THREAD,
                         NS_GET_IID(nsIInterfaceRequestor),
                         cb,
                         NS_PROXY_SYNC,
                         getter_AddRefs(callbacks));

    nsCOMPtr<nsISSLErrorListener> sel = do_GetInterface(callbacks);
    if (sel) {
      nsISSLErrorListener *proxy_sel = nsnull;
      NS_GetProxyForObject(NS_PROXY_TO_MAIN_THREAD,
                           NS_GET_IID(nsISSLErrorListener),
                           sel,
                           NS_PROXY_SYNC,
                           (void**)&proxy_sel);
      if (proxy_sel) {
        nsIInterfaceRequestor *csi = static_cast<nsIInterfaceRequestor*>(socketInfo);
        PRBool suppressMessage = PR_FALSE;
        nsCString hostWithPortString = hostName;
        hostWithPortString.AppendLiteral(":");
        hostWithPortString.AppendInt(port);
        rv = proxy_sel->NotifySSLError(csi, err, hostWithPortString, 
                                       &suppressMessage);
        if (NS_SUCCEEDED(rv) && suppressMessage)
          return NS_OK;
      }
    }
  }

  PRBool external = PR_FALSE;
  socketInfo->GetExternalErrorReporting(&external);
  
  nsString formattedString;
  rv = getErrorMessage(err, hostNameU, port, external, nssComponent, formattedString);

  if (external)
  {
    socketInfo->SetErrorMessage(formattedString.get());
  }
  else
  {
    nsPSMUITracker tracker;
    if (tracker.isUIForbidden()) {
      rv = NS_ERROR_NOT_AVAILABLE;
    }
    else {
      rv = displayAlert(formattedString, socketInfo);
    }
  }
  return rv;
}

static nsresult
nsHandleInvalidCertError(nsNSSSocketInfo *socketInfo, 
                         PRUint32 multipleCollectedErrors, 
                         const nsACString &host, 
                         const nsACString &hostWithPort,
                         PRInt32 port,
                         PRErrorCode errorCodeToReport,
                         PRErrorCode errTrust, 
                         PRErrorCode errMismatch, 
                         PRErrorCode errExpired,
                         PRBool wantsHtml,
                         nsIX509Cert* ix509)
{
  nsresult rv;
  NS_DEFINE_CID(nssComponentCID, NS_NSSCOMPONENT_CID);
  nsCOMPtr<nsINSSComponent> nssComponent(do_GetService(nssComponentCID, &rv));
  if (NS_FAILED(rv))
    return rv;

  NS_ConvertASCIItoUTF16 hostU(host);
  NS_ConvertASCIItoUTF16 hostWithPortU(hostWithPort);

  
  
  

  PRBool external = PR_FALSE;
  socketInfo->GetExternalErrorReporting(&external);
  
  nsString formattedString;
  rv = getInvalidCertErrorMessage(multipleCollectedErrors, errorCodeToReport,
                                  errTrust, errMismatch, errExpired,
                                  hostU, hostWithPortU, port, 
                                  ix509, external, wantsHtml,
                                  nssComponent, formattedString);

  if (external)
  {
    socketInfo->SetErrorMessage(formattedString.get());
  }
  else
  {
    nsPSMUITracker tracker;
    if (tracker.isUIForbidden()) {
      rv = NS_ERROR_NOT_AVAILABLE;
    }
    else {
      nsISSLCertErrorDialog *dialogs = nsnull;
      rv = getNSSDialogs((void**)&dialogs, 
        NS_GET_IID(nsISSLCertErrorDialog), 
        NS_SSLCERTERRORDIALOG_CONTRACTID);
  
      if (NS_SUCCEEDED(rv)) {
        nsPSMUITracker tracker;
        if (tracker.isUIForbidden()) {
          rv = NS_ERROR_NOT_AVAILABLE;
        }
        else {
          nsCOMPtr<nsISSLStatus> status;
          socketInfo->GetSSLStatus(getter_AddRefs(status));

          nsString empty;

          rv = dialogs->ShowCertError(nsnull, status, ix509, 
                                      formattedString, 
                                      empty, host, port);
        }
  
        NS_RELEASE(dialogs);
      }
    }
  }
  return rv;
}

static PRStatus PR_CALLBACK
nsSSLIOLayerConnect(PRFileDesc* fd, const PRNetAddr* addr,
                    PRIntervalTime timeout)
{
  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("[%p] connecting SSL socket\n", (void*)fd));
  nsNSSShutDownPreventionLock locker;
  if (!fd || !fd->lower)
    return PR_FAILURE;
  
  PRStatus status = PR_SUCCESS;

#if defined(XP_BEOS)
  
  
 
  PRSocketOptionData sockopt;
  sockopt.option = PR_SockOpt_Nonblocking;
  PR_GetSocketOption(fd, &sockopt);
  PRBool oldBlockVal = sockopt.value.non_blocking;
  sockopt.option = PR_SockOpt_Nonblocking;
  sockopt.value.non_blocking = PR_FALSE;
  PR_SetSocketOption(fd, &sockopt);
#endif
  
  status = fd->lower->methods->connect(fd->lower, addr, 
#if defined(XP_BEOS)  
                                       PR_INTERVAL_NO_TIMEOUT);
#else
                                       timeout);
#endif
  if (status != PR_SUCCESS) {
    PR_LOG(gPIPNSSLog, PR_LOG_ERROR, ("[%p] Lower layer connect error: %d\n",
                                      (void*)fd, PR_GetError()));
#if defined(XP_BEOS)  
    goto loser;
#else
    return status;
#endif
  }
  
  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("[%p] Connect\n", (void*)fd));

#if defined(XP_BEOS)  
 loser:
  
  
  NS_ASSERTION(sockopt.option == PR_SockOpt_Nonblocking,
               "sockopt.option was re-set to an unexpected value");
  sockopt.value.non_blocking = oldBlockVal;
  PR_SetSocketOption(fd, &sockopt);
#endif

  return status;
}



nsPSMRememberCertErrorsTable::nsPSMRememberCertErrorsTable()
{
  mErrorHosts.Init(16);
}

nsresult
nsPSMRememberCertErrorsTable::GetHostPortKey(nsNSSSocketInfo* infoObject,
                                             nsCAutoString &result)
{
  nsresult rv;

  result.Truncate();

  nsXPIDLCString hostName;
  rv = infoObject->GetHostName(getter_Copies(hostName));
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 port;
  rv = infoObject->GetPort(&port);
  NS_ENSURE_SUCCESS(rv, rv);

  result.Assign(hostName);
  result.Append(':');
  result.AppendInt(port);

  return NS_OK;
}

void
nsPSMRememberCertErrorsTable::RememberCertHasError(nsNSSSocketInfo* infoObject,
                                                   nsSSLStatus* status,
                                                   SECStatus certVerificationResult)
{
  nsresult rv;

  nsCAutoString hostPortKey;
  rv = GetHostPortKey(infoObject, hostPortKey);
  if (NS_FAILED(rv))
    return;

  if (certVerificationResult != SECSuccess) {
    NS_ASSERTION(status,
        "Must have nsSSLStatus object when remembering flags");

    if (!status)
      return;

    CertStateBits bits;
    bits.mIsDomainMismatch = status->mIsDomainMismatch;
    bits.mIsNotValidAtThisTime = status->mIsNotValidAtThisTime;
    bits.mIsUntrusted = status->mIsUntrusted;
    mErrorHosts.Put(hostPortKey, bits);
  }
  else {
    mErrorHosts.Remove(hostPortKey);
  }
}

void
nsPSMRememberCertErrorsTable::LookupCertErrorBits(nsNSSSocketInfo* infoObject,
                                                  nsSSLStatus* status)
{
  
  
  if (status->mHaveCertErrorBits)
    
    return;

  nsresult rv;

  nsCAutoString hostPortKey;
  rv = GetHostPortKey(infoObject, hostPortKey);
  if (NS_FAILED(rv))
    return;

  CertStateBits bits;
  if (!mErrorHosts.Get(hostPortKey, &bits))
    
    return;

  
  status->mHaveCertErrorBits = PR_TRUE;
  status->mIsDomainMismatch = bits.mIsDomainMismatch;
  status->mIsNotValidAtThisTime = bits.mIsNotValidAtThisTime;
  status->mIsUntrusted = bits.mIsUntrusted;
}

void
nsSSLIOLayerHelpers::getSiteKey(nsNSSSocketInfo *socketInfo, nsCSubstring &key)
{
  PRInt32 port;
  socketInfo->GetPort(&port);

  nsXPIDLCString host;
  socketInfo->GetHostName(getter_Copies(host));

  key = host + NS_LITERAL_CSTRING(":") + nsPrintfCString("%d", port);
}



PRBool
nsSSLIOLayerHelpers::rememberPossibleTLSProblemSite(PRFileDesc* ssl_layer_fd, nsNSSSocketInfo *socketInfo)
{
  PRBool currentlyUsesTLS = PR_FALSE;

  nsCAutoString key;
  getSiteKey(socketInfo, key);

  SSL_OptionGet(ssl_layer_fd, SSL_ENABLE_TLS, &currentlyUsesTLS);
  if (!currentlyUsesTLS) {
    
    
    
    
    removeIntolerantSite(key);
    return PR_FALSE;
  }

  PRBool enableSSL3 = PR_FALSE;
  SSL_OptionGet(ssl_layer_fd, SSL_ENABLE_SSL3, &enableSSL3);
  PRBool enableSSL2 = PR_FALSE;
  SSL_OptionGet(ssl_layer_fd, SSL_ENABLE_SSL2, &enableSSL2);
  if (enableSSL3 || enableSSL2) {
    
    addIntolerantSite(key);
  }
  
  return currentlyUsesTLS;
}

void
nsSSLIOLayerHelpers::rememberTolerantSite(PRFileDesc* ssl_layer_fd, 
                                          nsNSSSocketInfo *socketInfo)
{
  PRBool usingSecurity = PR_FALSE;
  PRBool currentlyUsesTLS = PR_FALSE;
  SSL_OptionGet(ssl_layer_fd, SSL_SECURITY, &usingSecurity);
  SSL_OptionGet(ssl_layer_fd, SSL_ENABLE_TLS, &currentlyUsesTLS);
  if (!usingSecurity || !currentlyUsesTLS) {
    return;
  }

  nsCAutoString key;
  getSiteKey(socketInfo, key);

  nsAutoLock lock(mutex);
  nsSSLIOLayerHelpers::mTLSTolerantSites->Put(key);
}

static PRStatus PR_CALLBACK
nsSSLIOLayerClose(PRFileDesc *fd)
{
  nsNSSShutDownPreventionLock locker;
  if (!fd)
    return PR_FAILURE;

  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("[%p] Shutting down socket\n", (void*)fd));
  
  nsNSSSocketInfo *socketInfo = (nsNSSSocketInfo*)fd->secret;
  NS_ASSERTION(socketInfo,"nsNSSSocketInfo was null for an fd");

  return nsSSLThread::requestClose(socketInfo);
}

PRStatus nsNSSSocketInfo::CloseSocketAndDestroy()
{
  nsNSSShutDownPreventionLock locker;

  nsNSSShutDownList::trackSSLSocketClose();

  PRFileDesc* popped = PR_PopIOLayer(mFd, PR_TOP_IO_LAYER);

  if (GetHandshakeInProgress()) {
    nsSSLIOLayerHelpers::rememberPossibleTLSProblemSite(mFd->lower, this);
  }

  PRStatus status = mFd->methods->close(mFd);
  if (status != PR_SUCCESS) return status;

  popped->identity = PR_INVALID_IO_LAYER;
  NS_RELEASE_THIS();
  popped->dtor(popped);

  return PR_SUCCESS;
}

#if defined(DEBUG_SSL_VERBOSE) && defined(DUMP_BUFFER)



#define DUMPBUF_LINESIZE 24
static void
nsDumpBuffer(unsigned char *buf, PRIntn len)
{
  char hexbuf[DUMPBUF_LINESIZE*3+1];
  char chrbuf[DUMPBUF_LINESIZE+1];
  static const char *hex = "0123456789abcdef";
  PRIntn i = 0, l = 0;
  char ch, *c, *h;
  if (len == 0)
    return;
  hexbuf[DUMPBUF_LINESIZE*3] = '\0';
  chrbuf[DUMPBUF_LINESIZE] = '\0';
  (void) memset(hexbuf, 0x20, DUMPBUF_LINESIZE*3);
  (void) memset(chrbuf, 0x20, DUMPBUF_LINESIZE);
  h = hexbuf;
  c = chrbuf;

  while (i < len)
  {
    ch = buf[i];

    if (l == DUMPBUF_LINESIZE)
    {
      PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("%s%s\n", hexbuf, chrbuf));
      (void) memset(hexbuf, 0x20, DUMPBUF_LINESIZE*3);
      (void) memset(chrbuf, 0x20, DUMPBUF_LINESIZE);
      h = hexbuf;
      c = chrbuf;
      l = 0;
    }

    
    *h++ = hex[(ch >> 4) & 0xf];
    *h++ = hex[ch & 0xf];
    h++;
        
    
    if ((ch >= 0x20) && (ch <= 0x7e))
      *c++ = ch;
    else
      *c++ = '.';
    i++; l++;
  }
  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("%s%s\n", hexbuf, chrbuf));
}

#define DEBUG_DUMP_BUFFER(buf,len) nsDumpBuffer(buf,len)
#else
#define DEBUG_DUMP_BUFFER(buf,len)
#endif

static PRBool
isNonSSLErrorThatWeAllowToRetry(PRInt32 err, PRBool withInitialCleartext)
{
  switch (err)
  {
    case PR_CONNECT_RESET_ERROR:
      if (!withInitialCleartext)
        return PR_TRUE;
      break;
    
    case PR_END_OF_FILE_ERROR:
      return PR_TRUE;
  }

  return PR_FALSE;
}

static PRBool
isTLSIntoleranceError(PRInt32 err, PRBool withInitialCleartext)
{
  
  
  
  
  
  
  
  

  if (isNonSSLErrorThatWeAllowToRetry(err, withInitialCleartext))
    return PR_TRUE;

  switch (err)
  {
    case SSL_ERROR_BAD_MAC_ALERT:
    case SSL_ERROR_BAD_MAC_READ:
    case SSL_ERROR_HANDSHAKE_FAILURE_ALERT:
    case SSL_ERROR_HANDSHAKE_UNEXPECTED_ALERT:
    case SSL_ERROR_CLIENT_KEY_EXCHANGE_FAILURE:
    case SSL_ERROR_ILLEGAL_PARAMETER_ALERT:
    case SSL_ERROR_NO_CYPHER_OVERLAP:
    case SSL_ERROR_BAD_SERVER:
    case SSL_ERROR_BAD_BLOCK_PADDING:
    case SSL_ERROR_UNSUPPORTED_VERSION:
    case SSL_ERROR_PROTOCOL_VERSION_ALERT:
    case SSL_ERROR_RX_MALFORMED_FINISHED:
    case SSL_ERROR_BAD_HANDSHAKE_HASH_VALUE:
    case SSL_ERROR_DECODE_ERROR_ALERT:
    case SSL_ERROR_RX_UNKNOWN_ALERT:
      return PR_TRUE;
  }
  
  return PR_FALSE;
}

PRInt32
nsSSLThread::checkHandshake(PRInt32 bytesTransfered, 
                            PRBool wasReading,
                            PRFileDesc* ssl_layer_fd, 
                            nsNSSSocketInfo *socketInfo)
{
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  
  
  
  
  
  
  
  
  
  

  
  
  
  

  PRBool handleHandshakeResultNow;
  socketInfo->GetHandshakePending(&handleHandshakeResultNow);

  PRBool wantRetry = PR_FALSE;

  if (0 > bytesTransfered) {
    PRInt32 err = PR_GetError();

    if (handleHandshakeResultNow) {
      if (PR_WOULD_BLOCK_ERROR == err) {
        socketInfo->SetHandshakeInProgress(PR_TRUE);
        return bytesTransfered;
      }

      if (!wantRetry 
          && isTLSIntoleranceError(err, socketInfo->GetHasCleartextPhase()))
      {
        wantRetry = nsSSLIOLayerHelpers::rememberPossibleTLSProblemSite(ssl_layer_fd, socketInfo);
      }
    }
    
    
    
    if (!wantRetry && (IS_SSL_ERROR(err) || IS_SEC_ERROR(err))) {
      nsHandleSSLError(socketInfo, err);
    }
  }
  else if (wasReading && 0 == bytesTransfered) 
  {
    if (handleHandshakeResultNow)
    {
      if (!wantRetry 
          && !socketInfo->GetHasCleartextPhase()) 
      {
        wantRetry = 
          nsSSLIOLayerHelpers::rememberPossibleTLSProblemSite(ssl_layer_fd, socketInfo);
      }
    }
  }

  if (wantRetry) {
    
    PR_SetError(PR_CONNECT_RESET_ERROR, 0);
    if (wasReading)
      bytesTransfered = -1;
  }

  
  
  
  if (handleHandshakeResultNow) {
    socketInfo->SetHandshakePending(PR_FALSE);
    socketInfo->SetHandshakeInProgress(PR_FALSE);
  }
  
  return bytesTransfered;
}

static PRInt16 PR_CALLBACK
nsSSLIOLayerPoll(PRFileDesc *fd, PRInt16 in_flags, PRInt16 *out_flags)
{
  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("[%p] polling SSL socket\n", (void*)fd));
  nsNSSShutDownPreventionLock locker;

  if (!out_flags)
  {
    NS_WARNING("nsSSLIOLayerPoll called with null out_flags");
    return 0;
  }

  *out_flags = 0;

  if (!fd)
  {
    NS_WARNING("nsSSLIOLayerPoll called with null fd");
    return 0;
  }

  nsNSSSocketInfo *socketInfo = (nsNSSSocketInfo*)fd->secret;
  NS_ASSERTION(socketInfo,"nsNSSSocketInfo was null for an fd");

  return nsSSLThread::requestPoll(socketInfo, in_flags, out_flags);
}

PRBool nsSSLIOLayerHelpers::nsSSLIOLayerInitialized = PR_FALSE;
PRDescIdentity nsSSLIOLayerHelpers::nsSSLIOLayerIdentity;
PRIOMethods nsSSLIOLayerHelpers::nsSSLIOLayerMethods;
PRLock *nsSSLIOLayerHelpers::mutex = nsnull;
nsCStringHashSet *nsSSLIOLayerHelpers::mTLSIntolerantSites = nsnull;
nsCStringHashSet *nsSSLIOLayerHelpers::mTLSTolerantSites = nsnull;
nsPSMRememberCertErrorsTable *nsSSLIOLayerHelpers::mHostsWithCertErrors = nsnull;
PRFileDesc *nsSSLIOLayerHelpers::mSharedPollableEvent = nsnull;
nsNSSSocketInfo *nsSSLIOLayerHelpers::mSocketOwningPollableEvent = nsnull;
PRBool nsSSLIOLayerHelpers::mPollableEventCurrentlySet = PR_FALSE;

static PRIntn _PSM_InvalidInt(void)
{
    PR_ASSERT(!"I/O method is invalid");
    PR_SetError(PR_INVALID_METHOD_ERROR, 0);
    return -1;
}

static PRInt64 _PSM_InvalidInt64(void)
{
    PR_ASSERT(!"I/O method is invalid");
    PR_SetError(PR_INVALID_METHOD_ERROR, 0);
    return -1;
}

static PRStatus _PSM_InvalidStatus(void)
{
    PR_ASSERT(!"I/O method is invalid");
    PR_SetError(PR_INVALID_METHOD_ERROR, 0);
    return PR_FAILURE;
}

static PRFileDesc *_PSM_InvalidDesc(void)
{
    PR_ASSERT(!"I/O method is invalid");
    PR_SetError(PR_INVALID_METHOD_ERROR, 0);
    return NULL;
}

static PRStatus PR_CALLBACK PSMGetsockname(PRFileDesc *fd, PRNetAddr *addr)
{
  nsNSSShutDownPreventionLock locker;
  if (!fd || !fd->lower) {
    return PR_FAILURE;
  }

  nsNSSSocketInfo *socketInfo = (nsNSSSocketInfo*)fd->secret;
  NS_ASSERTION(socketInfo,"nsNSSSocketInfo was null for an fd");

  return nsSSLThread::requestGetsockname(socketInfo, addr);
}

static PRStatus PR_CALLBACK PSMGetpeername(PRFileDesc *fd, PRNetAddr *addr)
{
  nsNSSShutDownPreventionLock locker;
  if (!fd || !fd->lower) {
    return PR_FAILURE;
  }

  nsNSSSocketInfo *socketInfo = (nsNSSSocketInfo*)fd->secret;
  NS_ASSERTION(socketInfo,"nsNSSSocketInfo was null for an fd");

  return nsSSLThread::requestGetpeername(socketInfo, addr);
}

static PRStatus PR_CALLBACK PSMGetsocketoption(PRFileDesc *fd, 
                                        PRSocketOptionData *data)
{
  nsNSSShutDownPreventionLock locker;
  if (!fd || !fd->lower) {
    return PR_FAILURE;
  }

  nsNSSSocketInfo *socketInfo = (nsNSSSocketInfo*)fd->secret;
  NS_ASSERTION(socketInfo,"nsNSSSocketInfo was null for an fd");

  return nsSSLThread::requestGetsocketoption(socketInfo, data);
}

static PRStatus PR_CALLBACK PSMSetsocketoption(PRFileDesc *fd, 
                                        const PRSocketOptionData *data)
{
  nsNSSShutDownPreventionLock locker;
  if (!fd || !fd->lower) {
    return PR_FAILURE;
  }

  nsNSSSocketInfo *socketInfo = (nsNSSSocketInfo*)fd->secret;
  NS_ASSERTION(socketInfo,"nsNSSSocketInfo was null for an fd");

  return nsSSLThread::requestSetsocketoption(socketInfo, data);
}

static PRInt32 PR_CALLBACK PSMRecv(PRFileDesc *fd, void *buf, PRInt32 amount,
    PRIntn flags, PRIntervalTime timeout)
{
  nsNSSShutDownPreventionLock locker;
  if (!fd || !fd->lower) {
    PR_SetError(PR_BAD_DESCRIPTOR_ERROR, 0);
    return -1;
  }

  nsNSSSocketInfo *socketInfo = (nsNSSSocketInfo*)fd->secret;
  NS_ASSERTION(socketInfo,"nsNSSSocketInfo was null for an fd");

  if (flags == PR_MSG_PEEK) {
    return nsSSLThread::requestRecvMsgPeek(socketInfo, buf, amount, flags, timeout);
  }

  if (flags != 0) {
    PR_SetError(PR_INVALID_ARGUMENT_ERROR, 0);
    return -1;
  }

  return nsSSLThread::requestRead(socketInfo, buf, amount, timeout);
}

static PRInt32 PR_CALLBACK PSMSend(PRFileDesc *fd, const void *buf, PRInt32 amount,
    PRIntn flags, PRIntervalTime timeout)
{
  nsNSSShutDownPreventionLock locker;
  if (!fd || !fd->lower) {
    PR_SetError(PR_BAD_DESCRIPTOR_ERROR, 0);
    return -1;
  }

  if (flags != 0) {
    PR_SetError(PR_INVALID_ARGUMENT_ERROR, 0);
    return -1;
  }

  nsNSSSocketInfo *socketInfo = (nsNSSSocketInfo*)fd->secret;
  NS_ASSERTION(socketInfo,"nsNSSSocketInfo was null for an fd");

  return nsSSLThread::requestWrite(socketInfo, buf, amount, timeout);
}

static PRInt32 PR_CALLBACK
nsSSLIOLayerRead(PRFileDesc* fd, void* buf, PRInt32 amount)
{
  return PSMRecv(fd, buf, amount, 0, PR_INTERVAL_NO_TIMEOUT);
}

static PRInt32 PR_CALLBACK
nsSSLIOLayerWrite(PRFileDesc* fd, const void* buf, PRInt32 amount)
{
  return PSMSend(fd, buf, amount, 0, PR_INTERVAL_NO_TIMEOUT);
}

static PRStatus PR_CALLBACK PSMConnectcontinue(PRFileDesc *fd, PRInt16 out_flags)
{
  nsNSSShutDownPreventionLock locker;
  if (!fd || !fd->lower) {
    return PR_FAILURE;
  }

  nsNSSSocketInfo *socketInfo = (nsNSSSocketInfo*)fd->secret;
  NS_ASSERTION(socketInfo,"nsNSSSocketInfo was null for an fd");

  return nsSSLThread::requestConnectcontinue(socketInfo, out_flags);
}

nsresult nsSSLIOLayerHelpers::Init()
{
  if (!nsSSLIOLayerInitialized) {
    nsSSLIOLayerInitialized = PR_TRUE;
    nsSSLIOLayerIdentity = PR_GetUniqueIdentity("NSS layer");
    nsSSLIOLayerMethods  = *PR_GetDefaultIOMethods();

    nsSSLIOLayerMethods.available = (PRAvailableFN)_PSM_InvalidInt;
    nsSSLIOLayerMethods.available64 = (PRAvailable64FN)_PSM_InvalidInt64;
    nsSSLIOLayerMethods.fsync = (PRFsyncFN)_PSM_InvalidStatus;
    nsSSLIOLayerMethods.seek = (PRSeekFN)_PSM_InvalidInt;
    nsSSLIOLayerMethods.seek64 = (PRSeek64FN)_PSM_InvalidInt64;
    nsSSLIOLayerMethods.fileInfo = (PRFileInfoFN)_PSM_InvalidStatus;
    nsSSLIOLayerMethods.fileInfo64 = (PRFileInfo64FN)_PSM_InvalidStatus;
    nsSSLIOLayerMethods.writev = (PRWritevFN)_PSM_InvalidInt;
    nsSSLIOLayerMethods.accept = (PRAcceptFN)_PSM_InvalidDesc;
    nsSSLIOLayerMethods.bind = (PRBindFN)_PSM_InvalidStatus;
    nsSSLIOLayerMethods.listen = (PRListenFN)_PSM_InvalidStatus;
    nsSSLIOLayerMethods.shutdown = (PRShutdownFN)_PSM_InvalidStatus;
    nsSSLIOLayerMethods.recvfrom = (PRRecvfromFN)_PSM_InvalidInt;
    nsSSLIOLayerMethods.sendto = (PRSendtoFN)_PSM_InvalidInt;
    nsSSLIOLayerMethods.acceptread = (PRAcceptreadFN)_PSM_InvalidInt;
    nsSSLIOLayerMethods.transmitfile = (PRTransmitfileFN)_PSM_InvalidInt;
    nsSSLIOLayerMethods.sendfile = (PRSendfileFN)_PSM_InvalidInt;

    nsSSLIOLayerMethods.getsockname = PSMGetsockname;
    nsSSLIOLayerMethods.getpeername = PSMGetpeername;
    nsSSLIOLayerMethods.getsocketoption = PSMGetsocketoption;
    nsSSLIOLayerMethods.setsocketoption = PSMSetsocketoption;
    nsSSLIOLayerMethods.recv = PSMRecv;
    nsSSLIOLayerMethods.send = PSMSend;
    nsSSLIOLayerMethods.connectcontinue = PSMConnectcontinue;

    nsSSLIOLayerMethods.connect = nsSSLIOLayerConnect;
    nsSSLIOLayerMethods.close = nsSSLIOLayerClose;
    nsSSLIOLayerMethods.write = nsSSLIOLayerWrite;
    nsSSLIOLayerMethods.read = nsSSLIOLayerRead;
    nsSSLIOLayerMethods.poll = nsSSLIOLayerPoll;
  }

  mutex = PR_NewLock();
  if (!mutex)
    return NS_ERROR_OUT_OF_MEMORY;

  mSharedPollableEvent = PR_NewPollableEvent();

  

  mTLSIntolerantSites = new nsCStringHashSet();
  if (!mTLSIntolerantSites)
    return NS_ERROR_OUT_OF_MEMORY;

  mTLSIntolerantSites->Init(1);

  mTLSTolerantSites = new nsCStringHashSet();
  if (!mTLSTolerantSites)
    return NS_ERROR_OUT_OF_MEMORY;

  
  
  
  mTLSTolerantSites->Init(16);

  mHostsWithCertErrors = new nsPSMRememberCertErrorsTable();
  if (!mHostsWithCertErrors || !mHostsWithCertErrors->mErrorHosts.IsInitialized())
    return NS_ERROR_OUT_OF_MEMORY;

  return NS_OK;
}

void nsSSLIOLayerHelpers::addIntolerantSite(const nsCString &str)
{
  nsAutoLock lock(mutex);
  
  if (!mTLSTolerantSites->Contains(str))
    nsSSLIOLayerHelpers::mTLSIntolerantSites->Put(str);
}

void nsSSLIOLayerHelpers::removeIntolerantSite(const nsCString &str)
{
  nsAutoLock lock(mutex);
  nsSSLIOLayerHelpers::mTLSIntolerantSites->Remove(str);
}

PRBool nsSSLIOLayerHelpers::isKnownAsIntolerantSite(const nsCString &str)
{
  nsAutoLock lock(mutex);
  return mTLSIntolerantSites->Contains(str);
}

nsresult
nsSSLIOLayerNewSocket(PRInt32 family,
                      const char *host,
                      PRInt32 port,
                      const char *proxyHost,
                      PRInt32 proxyPort,
                      PRFileDesc **fd,
                      nsISupports** info,
                      PRBool forSTARTTLS,
                      PRBool anonymousLoad)
{

  PRFileDesc* sock = PR_OpenTCPSocket(family);
  if (!sock) return NS_ERROR_OUT_OF_MEMORY;

  nsresult rv = nsSSLIOLayerAddToSocket(family, host, port, proxyHost, proxyPort,
                                        sock, info, forSTARTTLS, anonymousLoad);
  if (NS_FAILED(rv)) {
    PR_Close(sock);
    return rv;
  }

  *fd = sock;
  return NS_OK;
}













SECStatus nsConvertCANamesToStrings(PRArenaPool* arena, char** caNameStrings,
                                      CERTDistNames* caNames)
{
    SECItem* dername;
    SECStatus rv;
    int headerlen;
    PRUint32 contentlen;
    SECItem newitem;
    int n;
    char* namestring;

    for (n = 0; n < caNames->nnames; n++) {
        newitem.data = NULL;
        dername = &caNames->names[n];

        rv = DER_Lengths(dername, &headerlen, &contentlen);

        if (rv != SECSuccess) {
            goto loser;
        }

        if (headerlen + contentlen != dername->len) {
            




            if (dername->len <= 127) {
                newitem.data = (unsigned char *) PR_Malloc(dername->len + 2);
                if (newitem.data == NULL) {
                    goto loser;
                }
                newitem.data[0] = (unsigned char)0x30;
                newitem.data[1] = (unsigned char)dername->len;
                (void)memcpy(&newitem.data[2], dername->data, dername->len);
            }
            else if (dername->len <= 255) {
                newitem.data = (unsigned char *) PR_Malloc(dername->len + 3);
                if (newitem.data == NULL) {
                    goto loser;
                }
                newitem.data[0] = (unsigned char)0x30;
                newitem.data[1] = (unsigned char)0x81;
                newitem.data[2] = (unsigned char)dername->len;
                (void)memcpy(&newitem.data[3], dername->data, dername->len);
            }
            else {
                
                newitem.data = (unsigned char *) PR_Malloc(dername->len + 4);
                if (newitem.data == NULL) {
                    goto loser;
                }
                newitem.data[0] = (unsigned char)0x30;
                newitem.data[1] = (unsigned char)0x82;
                newitem.data[2] = (unsigned char)((dername->len >> 8) & 0xff);
                newitem.data[3] = (unsigned char)(dername->len & 0xff);
                memcpy(&newitem.data[4], dername->data, dername->len);
            }
            dername = &newitem;
        }

        namestring = CERT_DerNameToAscii(dername);
        if (namestring == NULL) {
            
            caNameStrings[n] = "";
        }
        else {
            caNameStrings[n] = PORT_ArenaStrdup(arena, namestring);
            PR_Free(namestring);
            if (caNameStrings[n] == NULL) {
                goto loser;
            }
        }

        if (newitem.data != NULL) {
            PR_Free(newitem.data);
        }
    }

    return SECSuccess;
loser:
    if (newitem.data != NULL) {
        PR_Free(newitem.data);
    }
    return SECFailure;
}















typedef struct {
    SECItem derConstraint;
    SECItem derPort;
    CERTGeneralName* constraint; 
    PRIntn port; 
} CERTCertificateScopeEntry;

typedef struct {
    CERTCertificateScopeEntry** entries;
} certCertificateScopeOfUse;


static const SEC_ASN1Template cert_CertificateScopeEntryTemplate[] = {
    { SEC_ASN1_SEQUENCE, 
      0, NULL, sizeof(CERTCertificateScopeEntry) },
    { SEC_ASN1_ANY,
      offsetof(CERTCertificateScopeEntry, derConstraint) },
    { SEC_ASN1_OPTIONAL | SEC_ASN1_INTEGER,
      offsetof(CERTCertificateScopeEntry, derPort) },
    { 0 }
};

static const SEC_ASN1Template cert_CertificateScopeOfUseTemplate[] = {
    { SEC_ASN1_SEQUENCE_OF, 0, cert_CertificateScopeEntryTemplate }
};

#if 0




static
SECStatus cert_DecodeScopeOfUseEntries(PRArenaPool* arena, SECItem* extData,
                                       CERTCertificateScopeEntry*** entries,
                                       int* numEntries)
{
    certCertificateScopeOfUse* scope = NULL;
    SECStatus rv = SECSuccess;
    int i;

    *entries = NULL; 
    *numEntries = 0; 

    scope = (certCertificateScopeOfUse*)
        PORT_ArenaZAlloc(arena, sizeof(certCertificateScopeOfUse));
    if (scope == NULL) {
        goto loser;
    }

    rv = SEC_ASN1DecodeItem(arena, (void*)scope, 
                            cert_CertificateScopeOfUseTemplate, extData);
    if (rv != SECSuccess) {
        goto loser;
    }

    *entries = scope->entries;
    PR_ASSERT(*entries != NULL);

    
    for (i = 0; (*entries)[i] != NULL; i++) ;
    *numEntries = i;

    


    for (i = 0; i < *numEntries; i++) {
        (*entries)[i]->constraint = 
            CERT_DecodeGeneralName(arena, &((*entries)[i]->derConstraint), 
                                   NULL);
        if ((*entries)[i]->derPort.data != NULL) {
            (*entries)[i]->port = 
                (int)DER_GetInteger(&((*entries)[i]->derPort));
        }
        else {
            (*entries)[i]->port = 0;
        }
    }

    goto done;
loser:
    if (rv == SECSuccess) {
        rv = SECFailure;
    }
done:
    return rv;
}

static SECStatus cert_DecodeCertIPAddress(SECItem* genname, 
                                          PRUint32* constraint, PRUint32* mask)
{
    
    *constraint = 0;
    *mask = 0;

    PR_ASSERT(genname->data != NULL);
    if (genname->data == NULL) {
        return SECFailure;
    }
    if (genname->len != 8) {
        
        return SECFailure;
    }

    
    *constraint = PR_ntohl((PRUint32)(*genname->data));
    *mask = PR_ntohl((PRUint32)(*(genname->data + 4)));

    return SECSuccess;
}

static char* _str_to_lower(char* string)
{
#ifdef XP_WIN
    return _strlwr(string);
#else
    int i;
    for (i = 0; string[i] != '\0'; i++) {
        string[i] = tolower(string[i]);
    }
    return string;
#endif
}







static PRBool CERT_MatchesScopeOfUse(CERTCertificate* cert, char* hostname,
                                     char* hostIP, PRIntn port)
{
    PRBool rv = PR_TRUE; 
    SECStatus srv;
    SECItem extData;
    PRArenaPool* arena = NULL;
    CERTCertificateScopeEntry** entries = NULL;
    
    int numEntries = 0;
    int i;
    char* hostLower = NULL;
    PRUint32 hostIPAddr = 0;

    PR_ASSERT((cert != NULL) && (hostname != NULL) && (hostIP != NULL));

    
    srv = CERT_FindCertExtension(cert, SEC_OID_NS_CERT_EXT_SCOPE_OF_USE,
                                 &extData);
    if (srv != SECSuccess) {
        



        goto done;
    }

    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if (arena == NULL) {
        goto done;
    }

    


    srv = cert_DecodeScopeOfUseEntries(arena, &extData, &entries, &numEntries);
    if (srv != SECSuccess) {
        





        goto done;
    }

    
    for (i = 0; i < numEntries; i++) {
        


        CERTGeneralName* genname = entries[i]->constraint;

        
        if (genname == NULL) {
            
            continue;
        }

        switch (genname->type) {
        case certDNSName: {
            


            char* pattern = NULL;
            char* substring = NULL;

            
            genname->name.other.data[genname->name.other.len] = '\0';
            pattern = _str_to_lower((char*)genname->name.other.data);

            if (hostLower == NULL) {
                
                hostLower = _str_to_lower(PL_strdup(hostname));
            }

            
            if (((substring = strstr(hostLower, pattern)) != NULL) &&
                
                (strlen(substring) == strlen(pattern)) &&
                
                ((substring == hostLower) || (*(substring-1) == '.'))) {
                


                rv = PR_TRUE;
            }
            else {
                rv = PR_FALSE;
            }
            
            break;
        }
        case certIPAddress: {
            PRUint32 constraint;
            PRUint32 mask;
            PRNetAddr addr;
            
            if (hostIPAddr == 0) {
                
                PR_StringToNetAddr(hostIP, &addr);
                hostIPAddr = addr.inet.ip;
            }

            if (cert_DecodeCertIPAddress(&(genname->name.other), &constraint, 
                                         &mask) != SECSuccess) {
                continue;
            }
            if ((hostIPAddr & mask) == (constraint & mask)) {
                rv = PR_TRUE;
            }
            else {
                rv = PR_FALSE;
            }
            break;
        }
        default:
            
            continue; 
        }

        if (!rv) {
            
            continue;
        }

        
        if ((entries[i]->port != 0) && (port != entries[i]->port)) {
            
            rv = PR_FALSE;
            continue;
        }

        
        PR_ASSERT(rv);
        break;
    }
done:
    
    if (arena != NULL) {
        PORT_FreeArena(arena, PR_FALSE);
    }
    if (hostLower != NULL) {
        PR_Free(hostLower);
    }
    return rv;
}
#endif

















nsresult nsGetUserCertChoice(SSM_UserCertChoice* certChoice)
{
	char *mode=NULL;
	nsresult ret;

	NS_ENSURE_ARG_POINTER(certChoice);

	nsCOMPtr<nsIPrefBranch> pref = do_GetService(NS_PREFSERVICE_CONTRACTID);

	ret = pref->GetCharPref("security.default_personal_cert", &mode);
	if (NS_FAILED(ret)) {
		goto loser;
	}

    if (PL_strcmp(mode, "Select Automatically") == 0) {
		*certChoice = AUTO;
	}
    else if (PL_strcmp(mode, "Ask Every Time") == 0) {
        *certChoice = ASK;
    }
    else {
      
      
		  *certChoice = ASK;
	}

loser:
	if (mode) {
		nsMemory::Free(mode);
	}
	return ret;
}

static PRBool hasExplicitKeyUsageNonRepudiation(CERTCertificate *cert)
{
  
  if (!cert->extensions)
    return PR_FALSE;

  SECStatus srv;
  SECItem keyUsageItem;
  keyUsageItem.data = NULL;

  srv = CERT_FindKeyUsageExtension(cert, &keyUsageItem);
  if (srv == SECFailure)
    return PR_FALSE;

  unsigned char keyUsage = keyUsageItem.data[0];
  PORT_Free (keyUsageItem.data);

  return !!(keyUsage & KU_NON_REPUDIATION);
}
















SECStatus nsNSS_SSLGetClientAuthData(void* arg, PRFileDesc* socket,
								   CERTDistNames* caNames,
								   CERTCertificate** pRetCert,
								   SECKEYPrivateKey** pRetKey)
{
  nsNSSShutDownPreventionLock locker;
  void* wincx = NULL;
  SECStatus ret = SECFailure;
  nsNSSSocketInfo* info = NULL;
  PRArenaPool* arena = NULL;
  char** caNameStrings;
  CERTCertificate* cert = NULL;
  SECKEYPrivateKey* privKey = NULL;
  CERTCertList* certList = NULL;
  CERTCertListNode* node;
  CERTCertNicknames* nicknames = NULL;
  char* extracted = NULL;
  PRIntn keyError = 0; 
  SSM_UserCertChoice certChoice;
  PRInt32 NumberOfCerts = 0;
	
  
  if (socket == NULL || caNames == NULL || pRetCert == NULL ||
      pRetKey == NULL) {
    PR_SetError(PR_INVALID_ARGUMENT_ERROR, 0);
    return SECFailure;
  }

  
  wincx = SSL_RevealPinArg(socket);
  if (wincx == NULL) {
    return SECFailure;
  }

  
  info = (nsNSSSocketInfo*)socket->higher->secret;

  
  arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
  if (arena == NULL) {
    goto loser;
  }

  caNameStrings = (char**)PORT_ArenaAlloc(arena, 
                                          sizeof(char*)*(caNames->nnames));
  if (caNameStrings == NULL) {
    goto loser;
  }


  ret = nsConvertCANamesToStrings(arena, caNameStrings, caNames);
  if (ret != SECSuccess) {
    goto loser;
  }

  
  if (NS_FAILED(nsGetUserCertChoice(&certChoice))) {
    goto loser;
  }

  	
  if (certChoice == AUTO) {
    

    
    certList = CERT_FindUserCertsByUsage(CERT_GetDefaultCertDB(), 
                                         certUsageSSLClient, PR_FALSE,
                                         PR_TRUE, wincx);
    if (certList == NULL) {
      goto noCert;
    }

    
    ret = CERT_FilterCertListByCANames(certList, caNames->nnames,
                                       caNameStrings, certUsageSSLClient);
    if (ret != SECSuccess) {
      goto noCert;
    }

    
    node = CERT_LIST_HEAD(certList);
    if (CERT_LIST_END(node, certList)) {
      goto noCert;
    }

    CERTCertificate* low_prio_nonrep_cert = NULL;
    CERTCertificateCleaner low_prio_cleaner(low_prio_nonrep_cert);

    
    while (!CERT_LIST_END(node, certList)) {
      


#if 0		
      if (!CERT_MatchesScopeOfUse(node->cert, info->GetHostName,
                                  info->GetHostIP, info->GetHostPort)) {
          node = CERT_LIST_NEXT(node);
          continue;
      }
#endif

      privKey = PK11_FindKeyByAnyCert(node->cert, wincx);
      if (privKey != NULL) {
        if (hasExplicitKeyUsageNonRepudiation(node->cert)) {
          SECKEY_DestroyPrivateKey(privKey);
          privKey = NULL;
          
          if (!low_prio_nonrep_cert) 
            low_prio_nonrep_cert = CERT_DupCertificate(node->cert);
        }
        else {
          
          cert = CERT_DupCertificate(node->cert);
          break;
        }
      }
      keyError = PR_GetError();
      if (keyError == SEC_ERROR_BAD_PASSWORD) {
          
          goto loser;
      }

      node = CERT_LIST_NEXT(node);
    }

    if (!cert && low_prio_nonrep_cert) {
      cert = low_prio_nonrep_cert;
      low_prio_nonrep_cert = NULL; 
      privKey = PK11_FindKeyByAnyCert(cert, wincx);
    }

    if (cert == NULL) {
        goto noCert;
    }
  }
  else { 
    
    CERTCertificate* serverCert = NULL;
    CERTCertificateCleaner serverCertCleaner(serverCert);
    serverCert = SSL_PeerCertificate(socket);
    if (serverCert == NULL) {
      
      goto loser;
    }

    nsXPIDLCString hostname;
    info->GetHostName(getter_Copies(hostname));

    nsresult rv;
    NS_DEFINE_CID(nssComponentCID, NS_NSSCOMPONENT_CID);
    nsCOMPtr<nsINSSComponent> nssComponent(do_GetService(nssComponentCID, &rv));
    nsRefPtr<nsClientAuthRememberService> cars;
    if (nssComponent) {
      nssComponent->GetClientAuthRememberService(getter_AddRefs(cars));
    }

    PRBool hasRemembered = PR_FALSE;
    nsCString rememberedDBKey;
    if (cars) {
      PRBool found;
      nsresult rv = cars->HasRememberedDecision(hostname, 
                                                serverCert,
                                                rememberedDBKey, &found);
      if (NS_SUCCEEDED(rv) && found) {
        hasRemembered = PR_TRUE;
      }
    }

    PRBool canceled = PR_FALSE;

if (hasRemembered)
{
    if (rememberedDBKey.IsEmpty())
    {
      canceled = PR_TRUE;
    }
    else
    {
      nsCOMPtr<nsIX509CertDB> certdb;
      certdb = do_GetService(NS_X509CERTDB_CONTRACTID);
      if (certdb)
      {
        nsCOMPtr<nsIX509Cert> found_cert;
        nsresult find_rv = 
          certdb->FindCertByDBKey(rememberedDBKey.get(), nsnull,
                                  getter_AddRefs(found_cert));
        if (NS_SUCCEEDED(find_rv) && found_cert) {
          nsNSSCertificate *obj_cert = reinterpret_cast<nsNSSCertificate *>(found_cert.get());
          if (obj_cert) {
            cert = obj_cert->GetCert();

#ifdef DEBUG_kaie
            nsAutoString nick, nickWithSerial, details;
            if (NS_SUCCEEDED(obj_cert->FormatUIStrings(nick, 
                                                       nickWithSerial, 
                                                       details))) {
              NS_LossyConvertUTF16toASCII asc(nickWithSerial);
              fprintf(stderr, "====> remembered serial %s\n", asc.get());
            }
#endif

          }
        }
        
        if (!cert) {
          hasRemembered = PR_FALSE;
        }
      }
    }
}

if (!hasRemembered)
{
    
    nsIClientAuthDialogs *dialogs = NULL;
    PRInt32 selectedIndex = -1;
    PRUnichar **certNicknameList = NULL;
    PRUnichar **certDetailsList = NULL;

    
    
    certList = CERT_FindUserCertsByUsage(CERT_GetDefaultCertDB(), 
                                         certUsageSSLClient, PR_FALSE, 
                                         PR_FALSE, wincx);
    if (certList == NULL) {
      goto noCert;
    }

    if (caNames->nnames != 0) {
      


      ret = CERT_FilterCertListByCANames(certList, caNames->nnames, 
                                        caNameStrings, 
                                        certUsageSSLClient);
      if (ret != SECSuccess) {
        goto loser;
      }
    }

    if (CERT_LIST_END(CERT_LIST_HEAD(certList), certList)) {
      
      goto noCert;
    }

    
    node = CERT_LIST_HEAD(certList);
    while (!CERT_LIST_END(node, certList)) {
      ++NumberOfCerts;
#if 0 
      if (!CERT_MatchesScopeOfUse(node->cert, conn->hostName,
                                  conn->hostIP, conn->port)) {
        CERTCertListNode* removed = node;
        node = CERT_LIST_NEXT(removed);
        CERT_RemoveCertListNode(removed);
      }
      else {
        node = CERT_LIST_NEXT(node);
      }
#endif
      node = CERT_LIST_NEXT(node);
    }
    if (CERT_LIST_END(CERT_LIST_HEAD(certList), certList)) {
      goto noCert;
    }

    nicknames = getNSSCertNicknamesFromCertList(certList);

    if (nicknames == NULL) {
      goto loser;
    }

    NS_ASSERTION(nicknames->numnicknames == NumberOfCerts, "nicknames->numnicknames != NumberOfCerts");

    
    char *ccn = CERT_GetCommonName(&serverCert->subject);
    void *v = ccn;
    voidCleaner ccnCleaner(v);
    NS_ConvertUTF8toUTF16 cn(ccn);

    PRInt32 port;
    info->GetPort(&port);

    nsString cn_host_port;
    if (ccn && strcmp(ccn, hostname) == 0) {
      cn_host_port.Append(cn);
      cn_host_port.AppendLiteral(":");
      cn_host_port.AppendInt(port);
    }
    else {
      cn_host_port.Append(cn);
      cn_host_port.AppendLiteral(" (");
      cn_host_port.AppendLiteral(":");
      cn_host_port.AppendInt(port);
      cn_host_port.AppendLiteral(")");
    }

    char *corg = CERT_GetOrgName(&serverCert->subject);
    NS_ConvertUTF8toUTF16 org(corg);
    if (corg) PORT_Free(corg);

    char *cissuer = CERT_GetOrgName(&serverCert->issuer);
    NS_ConvertUTF8toUTF16 issuer(cissuer);
    if (cissuer) PORT_Free(cissuer);

    certNicknameList = (PRUnichar **)nsMemory::Alloc(sizeof(PRUnichar *) * nicknames->numnicknames);
    if (!certNicknameList)
      goto loser;
    certDetailsList = (PRUnichar **)nsMemory::Alloc(sizeof(PRUnichar *) * nicknames->numnicknames);
    if (!certDetailsList) {
      nsMemory::Free(certNicknameList);
      goto loser;
    }

    PRInt32 CertsToUse;
    for (CertsToUse = 0, node = CERT_LIST_HEAD(certList);
         !CERT_LIST_END(node, certList) && CertsToUse < nicknames->numnicknames;
         node = CERT_LIST_NEXT(node)
        )
    {
      nsRefPtr<nsNSSCertificate> tempCert = new nsNSSCertificate(node->cert);

      if (!tempCert)
        continue;
      
      NS_ConvertUTF8toUTF16 i_nickname(nicknames->nicknames[CertsToUse]);
      nsAutoString nickWithSerial, details;
      
      if (NS_FAILED(tempCert->FormatUIStrings(i_nickname, nickWithSerial, details)))
        continue;

      certNicknameList[CertsToUse] = ToNewUnicode(nickWithSerial);
      if (!certNicknameList[CertsToUse])
        continue;
      certDetailsList[CertsToUse] = ToNewUnicode(details);
      if (!certDetailsList[CertsToUse]) {
        nsMemory::Free(certNicknameList[CertsToUse]);
        continue;
      }

      ++CertsToUse;
    }

    
    rv = getNSSDialogs((void**)&dialogs, 
                       NS_GET_IID(nsIClientAuthDialogs),
                       NS_CLIENTAUTHDIALOGS_CONTRACTID);

    if (NS_FAILED(rv)) {
      NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(CertsToUse, certNicknameList);
      NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(CertsToUse, certDetailsList);
      goto loser;
    }

    {
      nsPSMUITracker tracker;
      if (tracker.isUIForbidden()) {
        rv = NS_ERROR_NOT_AVAILABLE;
      }
      else {
        rv = dialogs->ChooseCertificate(info, cn_host_port.get(), org.get(), issuer.get(), 
          (const PRUnichar**)certNicknameList, (const PRUnichar**)certDetailsList,
          CertsToUse, &selectedIndex, &canceled);
      }
    }

    NS_RELEASE(dialogs);
    NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(CertsToUse, certNicknameList);
    NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(CertsToUse, certDetailsList);
    
    if (NS_FAILED(rv)) goto loser;

    
    PRBool wantRemember = PR_FALSE;
    info->GetRememberClientAuthCertificate(&wantRemember);

    int i;
    if (!canceled)
    for (i = 0, node = CERT_LIST_HEAD(certList);
         !CERT_LIST_END(node, certList);
         ++i, node = CERT_LIST_NEXT(node)) {

      if (i == selectedIndex) {
        cert = CERT_DupCertificate(node->cert);
        break;
      }
    }

    if (cars && wantRemember) {
      cars->RememberDecision(hostname, 
                             serverCert, 
                             canceled ? 0 : cert);
    }
}

    if (canceled) { rv = NS_ERROR_NOT_AVAILABLE; goto loser; }

    if (cert == NULL) {
      goto loser;
    }

    
    privKey = PK11_FindKeyByAnyCert(cert, wincx);
    if (privKey == NULL) {
      keyError = PR_GetError();
      if (keyError == SEC_ERROR_BAD_PASSWORD) {
          
          goto loser;
      }
      else {
          goto noCert;
      }
    }
  }
  goto done;

noCert:
loser:
  if (ret == SECSuccess) {
    ret = SECFailure;
  }
  if (cert != NULL) {
    CERT_DestroyCertificate(cert);
    cert = NULL;
  }
done:
  if (extracted != NULL) {
    PR_Free(extracted);
  }
  if (nicknames != NULL) {
    CERT_FreeNicknames(nicknames);
  }
  if (certList != NULL) {
    CERT_DestroyCertList(certList);
  }
  if (arena != NULL) {
    PORT_FreeArena(arena, PR_FALSE);
  }

  *pRetCert = cert;
  *pRetKey = privKey;

  return ret;
}

static SECStatus
cancel_and_failure(nsNSSSocketInfo* infoObject)
{
  infoObject->SetCanceled(PR_TRUE);
  return SECFailure;
}

static SECStatus
nsNSSBadCertHandler(void *arg, PRFileDesc *sslSocket)
{
  nsNSSShutDownPreventionLock locker;
  nsNSSSocketInfo* infoObject = (nsNSSSocketInfo *)arg;
  if (!infoObject)
    return SECFailure;

  if (nsSSLThread::exitRequested())
    return cancel_and_failure(infoObject);

  CERTCertificate *peerCert = nsnull;
  CERTCertificateCleaner peerCertCleaner(peerCert);
  peerCert = SSL_PeerCertificate(sslSocket);
  if (!peerCert)
    return cancel_and_failure(infoObject);

  nsRefPtr<nsNSSCertificate> nssCert;
  nssCert = new nsNSSCertificate(peerCert);
  if (!nssCert)
    return cancel_and_failure(infoObject);

  nsCOMPtr<nsIX509Cert> ix509 = static_cast<nsIX509Cert*>(nssCert.get());

  SECStatus srv;
  nsresult nsrv;
  PRUint32 collected_errors = 0;
  PRUint32 remaining_display_errors = 0;

  PRErrorCode errorCodeTrust = SECSuccess;
  PRErrorCode errorCodeMismatch = SECSuccess;
  PRErrorCode errorCodeExpired = SECSuccess;
  
  char *hostname = SSL_RevealURL(sslSocket);
  charCleaner hostnameCleaner(hostname); 
  nsDependentCString hostString(hostname);

  PRInt32 port;
  infoObject->GetPort(&port);

  nsCString hostWithPortString = hostString;
  hostWithPortString.AppendLiteral(":");
  hostWithPortString.AppendInt(port);

  NS_ConvertUTF8toUTF16 hostWithPortStringUTF16(hostWithPortString);

  
  if (hostname && hostname[0] &&
      CERT_VerifyCertName(peerCert, hostname) != SECSuccess) {
    collected_errors |= nsICertOverrideService::ERROR_MISMATCH;
    errorCodeMismatch = SSL_ERROR_BAD_CERT_DOMAIN;
  }

  {
    PRArenaPool *log_arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if (!log_arena)    
      return cancel_and_failure(infoObject);

    PRArenaPoolCleanerFalseParam log_arena_cleaner(log_arena);

    CERTVerifyLog *verify_log = PORT_ArenaZNew(log_arena, CERTVerifyLog);
    if (!verify_log)
      return cancel_and_failure(infoObject);

    CERTVerifyLogContentsCleaner verify_log_cleaner(verify_log);

    verify_log->arena = log_arena;

    srv = CERT_VerifyCertificate(CERT_GetDefaultCertDB(), peerCert,
                                 PR_TRUE, certificateUsageSSLServer,
                                 PR_Now(), (void*)infoObject, 
                                 verify_log, NULL);

    
    
    
    
    

    CERTVerifyLogNode *i_node;
    for (i_node = verify_log->head; i_node; i_node = i_node->next)
    {
      switch (i_node->error)
      {
        case SEC_ERROR_UNKNOWN_ISSUER:
        case SEC_ERROR_CA_CERT_INVALID:
        case SEC_ERROR_UNTRUSTED_ISSUER:
        case SEC_ERROR_EXPIRED_ISSUER_CERTIFICATE:
        case SEC_ERROR_UNTRUSTED_CERT:
        case SEC_ERROR_INADEQUATE_KEY_USAGE:
          
          collected_errors |= nsICertOverrideService::ERROR_UNTRUSTED;
          if (errorCodeTrust == SECSuccess) {
            errorCodeTrust = i_node->error;
          }
          break;
        case SSL_ERROR_BAD_CERT_DOMAIN:
          collected_errors |= nsICertOverrideService::ERROR_MISMATCH;
          if (errorCodeMismatch == SECSuccess) {
            errorCodeMismatch = i_node->error;
          }
          break;
        case SEC_ERROR_EXPIRED_CERTIFICATE:
          collected_errors |= nsICertOverrideService::ERROR_TIME;
          if (errorCodeExpired == SECSuccess) {
            errorCodeExpired = i_node->error;
          }
          break;
        default:
          
          nsHandleSSLError(infoObject, i_node->error);
          
          
          PR_SetError(i_node->error, 0);
          return cancel_and_failure(infoObject);
      }
    }
  }

  if (!collected_errors)
  {
    NS_NOTREACHED("why did NSS call our bad cert handler if all looks good? Let's cancel the connection");
    return SECFailure;
  }

  nsRefPtr<nsSSLStatus> status = infoObject->SSLStatus();
  if (!status) {
    status = new nsSSLStatus();
    infoObject->SetSSLStatus(status);
  }

  if (status) {
    if (!status->mServerCert) {
      status->mServerCert = nssCert;
    }

    status->mHaveCertErrorBits = PR_TRUE;
    status->mIsDomainMismatch = collected_errors & nsICertOverrideService::ERROR_MISMATCH;
    status->mIsNotValidAtThisTime = collected_errors & nsICertOverrideService::ERROR_TIME;
    status->mIsUntrusted = collected_errors & nsICertOverrideService::ERROR_UNTRUSTED;

    nsSSLIOLayerHelpers::mHostsWithCertErrors->RememberCertHasError(
      infoObject, status, SECFailure);
  }

  remaining_display_errors = collected_errors;

  nsCOMPtr<nsICertOverrideService> overrideService = 
    do_GetService(NS_CERTOVERRIDE_CONTRACTID);
  

  PRUint32 overrideBits = 0; 

  if (overrideService)
  {
    PRBool haveOverride;
    PRBool isTemporaryOverride; 
  
    nsrv = overrideService->HasMatchingOverride(hostString, port,
                                                ix509, 
                                                &overrideBits,
                                                &isTemporaryOverride, 
                                                &haveOverride);
    if (NS_SUCCEEDED(nsrv) && haveOverride) 
    {
      
      remaining_display_errors -= overrideBits;
    }
  }

  if (!remaining_display_errors) {
    
    return SECSuccess;
  }

  
  
  

  PRBool suppressMessage = PR_FALSE;
  nsresult rv;

  
  nsCOMPtr<nsIInterfaceRequestor> cb;
  infoObject->GetNotificationCallbacks(getter_AddRefs(cb));
  if (cb) {
    nsCOMPtr<nsIInterfaceRequestor> callbacks;
    NS_GetProxyForObject(NS_PROXY_TO_MAIN_THREAD,
                         NS_GET_IID(nsIInterfaceRequestor),
                         cb,
                         NS_PROXY_SYNC,
                         getter_AddRefs(callbacks));

    nsCOMPtr<nsIBadCertListener2> bcl = do_GetInterface(callbacks);
    if (bcl) {
      nsCOMPtr<nsIBadCertListener2> proxy_bcl;
      NS_GetProxyForObject(NS_PROXY_TO_MAIN_THREAD,
                           NS_GET_IID(nsIBadCertListener2),
                           bcl,
                           NS_PROXY_SYNC,
                           getter_AddRefs(proxy_bcl));
      if (proxy_bcl) {
        nsIInterfaceRequestor *csi = static_cast<nsIInterfaceRequestor*>(infoObject);
        rv = proxy_bcl->NotifyCertProblem(csi, status, hostWithPortString, 
                                          &suppressMessage);
      }
    }
  }

  nsCOMPtr<nsIRecentBadCertsService> recentBadCertsService = 
    do_GetService(NS_RECENTBADCERTS_CONTRACTID);

  if (recentBadCertsService) {
    recentBadCertsService->AddBadCert(hostWithPortStringUTF16, status);
  }

  
  PRErrorCode errorCodeToReport = SECSuccess;
  if (remaining_display_errors & nsICertOverrideService::ERROR_UNTRUSTED)
    errorCodeToReport = errorCodeTrust;
  else if (remaining_display_errors & nsICertOverrideService::ERROR_MISMATCH)
    errorCodeToReport = errorCodeMismatch;
  else if (remaining_display_errors & nsICertOverrideService::ERROR_TIME)
    errorCodeToReport = errorCodeExpired;

  if (!suppressMessage) {
    PRBool external = PR_FALSE;
    infoObject->GetExternalErrorReporting(&external);

    nsHandleInvalidCertError(infoObject,
                             remaining_display_errors,
                             hostString,
                             hostWithPortString,
                             port,
                             errorCodeToReport,
                             errorCodeTrust,
                             errorCodeMismatch,
                             errorCodeExpired,
                             external, 
                             ix509);
  }

  PR_SetError(errorCodeToReport, 0);
  return cancel_and_failure(infoObject);
}

static PRFileDesc*
nsSSLIOLayerImportFD(PRFileDesc *fd,
                     nsNSSSocketInfo *infoObject,
                     const char *host,
                     PRBool anonymousLoad)
{
  nsNSSShutDownPreventionLock locker;
  PRFileDesc* sslSock = SSL_ImportFD(nsnull, fd);
  if (!sslSock) {
    NS_ASSERTION(PR_FALSE, "NSS: Error importing socket");
    return nsnull;
  }
  SSL_SetPKCS11PinArg(sslSock, (nsIInterfaceRequestor*)infoObject);
  SSL_HandshakeCallback(sslSock, HandshakeCallback, infoObject);

  
  if (anonymousLoad) {
      SSL_GetClientAuthDataHook(sslSock, NULL, infoObject);
  } else {
      SSL_GetClientAuthDataHook(sslSock, 
                            (SSLGetClientAuthData)nsNSS_SSLGetClientAuthData,
                            infoObject);
  }
  SSL_AuthCertificateHook(sslSock, AuthCertificateCallback, 0);

  PRInt32 ret = SSL_SetURL(sslSock, host);
  if (ret == -1) {
    NS_ASSERTION(PR_FALSE, "NSS: Error setting server name");
    goto loser;
  }
  return sslSock;
loser:
  if (sslSock) {
    PR_Close(sslSock);
  }
  return nsnull;
}

static nsresult
nsSSLIOLayerSetOptions(PRFileDesc *fd, PRBool forSTARTTLS, 
                       const char *proxyHost, const char *host, PRInt32 port,
                       PRBool anonymousLoad, nsNSSSocketInfo *infoObject)
{
  nsNSSShutDownPreventionLock locker;
  if (forSTARTTLS || proxyHost) {
    if (SECSuccess != SSL_OptionSet(fd, SSL_SECURITY, PR_FALSE)) {
      return NS_ERROR_FAILURE;
    }
    infoObject->SetHasCleartextPhase(PR_TRUE);
  }

  if (forSTARTTLS) {
    if (SECSuccess != SSL_OptionSet(fd, SSL_ENABLE_SSL2, PR_FALSE)) {
      return NS_ERROR_FAILURE;
    }
    if (SECSuccess != SSL_OptionSet(fd, SSL_V2_COMPATIBLE_HELLO, PR_FALSE)) {
      return NS_ERROR_FAILURE;
    }
  }

  
  
  nsCAutoString key;
  key = nsDependentCString(host) + NS_LITERAL_CSTRING(":") + nsPrintfCString("%d", port);

  if (nsSSLIOLayerHelpers::isKnownAsIntolerantSite(key)) {
    if (SECSuccess != SSL_OptionSet(fd, SSL_ENABLE_TLS, PR_FALSE))
      return NS_ERROR_FAILURE;

    infoObject->SetAllowTLSIntoleranceTimeout(PR_FALSE);
      
    
    
    
    
    
    
    
    if (!forSTARTTLS &&
        SECSuccess != SSL_OptionSet(fd, SSL_V2_COMPATIBLE_HELLO, PR_TRUE))
      return NS_ERROR_FAILURE;
  }

  if (SECSuccess != SSL_OptionSet(fd, SSL_HANDSHAKE_AS_CLIENT, PR_TRUE)) {
    return NS_ERROR_FAILURE;
  }
  if (SECSuccess != SSL_BadCertHook(fd, (SSLBadCertHandler) nsNSSBadCertHandler,
                                    infoObject)) {
    return NS_ERROR_FAILURE;
  }

  
  char *peerId;
  if (anonymousLoad) {  
      peerId = PR_smprintf("anon:%s:%d", host, port);
  } else {
      peerId = PR_smprintf("%s:%d", host, port);
  }
  
  if (SECSuccess != SSL_SetSockPeerID(fd, peerId)) {
    PR_smprintf_free(peerId);
    return NS_ERROR_FAILURE;
  }

  PR_smprintf_free(peerId);
  return NS_OK;
}

nsresult
nsSSLIOLayerAddToSocket(PRInt32 family,
                        const char* host,
                        PRInt32 port,
                        const char* proxyHost,
                        PRInt32 proxyPort,
                        PRFileDesc* fd,
                        nsISupports** info,
                        PRBool forSTARTTLS,
                        PRBool anonymousLoad)
{
  nsNSSShutDownPreventionLock locker;
  PRFileDesc* layer = nsnull;
  nsresult rv;

  nsNSSSocketInfo* infoObject = new nsNSSSocketInfo();
  if (!infoObject) return NS_ERROR_FAILURE;
  
  NS_ADDREF(infoObject);
  infoObject->SetForSTARTTLS(forSTARTTLS);
  infoObject->SetHostName(host);
  infoObject->SetPort(port);

  PRFileDesc *sslSock = nsSSLIOLayerImportFD(fd, infoObject, host, anonymousLoad);
  if (!sslSock) {
    NS_ASSERTION(PR_FALSE, "NSS: Error importing socket");
    goto loser;
  }

  infoObject->SetFileDescPtr(sslSock);

  rv = nsSSLIOLayerSetOptions(sslSock,
                              forSTARTTLS, proxyHost, host, port, anonymousLoad,
                              infoObject);

  if (NS_FAILED(rv))
    goto loser;

  
  layer = PR_CreateIOLayerStub(nsSSLIOLayerHelpers::nsSSLIOLayerIdentity,
                               &nsSSLIOLayerHelpers::nsSSLIOLayerMethods);
  if (!layer)
    goto loser;
  
  layer->secret = (PRFilePrivate*) infoObject;
  rv = PR_PushIOLayer(sslSock, PR_GetLayersIdentity(sslSock), layer);
  
  if (NS_FAILED(rv)) {
    goto loser;
  }
  
  nsNSSShutDownList::trackSSLSocketCreate();

  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("[%p] Socket set up\n", (void*)sslSock));
  infoObject->QueryInterface(NS_GET_IID(nsISupports), (void**) (info));

  
  if (forSTARTTLS || proxyHost) {
    infoObject->SetHandshakePending(PR_FALSE);
  }

  return NS_OK;
 loser:
  NS_IF_RELEASE(infoObject);
  if (layer) {
    layer->dtor(layer);
  }
  return NS_ERROR_FAILURE;
}
