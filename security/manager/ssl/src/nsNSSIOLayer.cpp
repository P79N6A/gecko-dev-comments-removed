








































#include "nsNSSComponent.h"
#include "nsNSSIOLayer.h"
#include "nsNSSCallbacks.h"

#include "prlog.h"
#include "prnetdb.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsIServiceManager.h"
#include "nsIWebProgressListener.h"
#include "nsIChannel.h"
#include "nsNSSCertificate.h"
#include "nsIX509CertValidity.h"
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
#include "nsIStrictTransportSecurityService.h"

#include "nsXPIDLString.h"
#include "nsReadableUtils.h"
#include "nsHashSets.h"
#include "nsCRT.h"
#include "nsAutoPtr.h"
#include "nsPrintfCString.h"
#include "SSLServerCertVerification.h"
#include "nsNSSShutDown.h"
#include "nsSSLStatus.h"
#include "nsNSSCertHelper.h"
#include "nsNSSCleaner.h"
#include "nsThreadUtils.h"
#include "nsIDocShell.h"
#include "nsIDocShellTreeItem.h"
#include "nsISecureBrowserUI.h"
#include "nsIClassInfoImpl.h"
#include "nsIProgrammingLanguage.h"
#include "nsIArray.h"
#include "nsCharSeparatedTokenizer.h"
#include "PSMRunnable.h"

#include "ssl.h"
#include "secerr.h"
#include "sslerr.h"
#include "secder.h"
#include "secasn1.h"
#include "certdb.h"
#include "cert.h"
#include "keyhi.h"
#include "secport.h"

#include "mozilla/Util.h"

using namespace mozilla;
using namespace mozilla::psm;


                            
                            

                       
                       
                       
                       
                       


NSSCleanupAutoPtrClass(CERTCertificate, CERT_DestroyCertificate)
NSSCleanupAutoPtrClass(void, PR_FREEIF)
NSSCleanupAutoPtrClass_WithParam(PRArenaPool, PORT_FreeArena, FalseParam, false)

static NS_DEFINE_CID(kNSSComponentCID, NS_NSSCOMPONENT_CID);


typedef enum {ASK, AUTO} SSM_UserCertChoice;

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

nsNSSSocketInfo::nsNSSSocketInfo()
  : mMutex("nsNSSSocketInfo::nsNSSSocketInfo"),
    mFd(nsnull),
    mCertVerificationState(before_cert_verification),
    mSecurityState(nsIWebProgressListener::STATE_IS_INSECURE),
    mSubRequestsHighSecurity(0),
    mSubRequestsLowSecurity(0),
    mSubRequestsBrokenSecurity(0),
    mSubRequestsNoSecurity(0),
    mErrorCode(0),
    mErrorMessageType(PlainErrorMessage),
    mForSTARTTLS(false),
    mHandshakePending(true),
    mHasCleartextPhase(false),
    mHandshakeInProgress(false),
    mAllowTLSIntoleranceTimeout(true),
    mRememberClientAuthCertificate(false),
    mHandshakeStartTime(0),
    mPort(0),
    mIsCertIssuerBlacklisted(false),
    mNPNCompleted(false),
    mHandshakeCompleted(false)
{
}

nsNSSSocketInfo::~nsNSSSocketInfo()
{
  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown())
    return;

  shutdown(calledFromObject);
}

void nsNSSSocketInfo::virtualDestroyNSSReference()
{
}

NS_IMPL_THREADSAFE_ISUPPORTS8(nsNSSSocketInfo,
                              nsITransportSecurityInfo,
                              nsISSLSocketControl,
                              nsIInterfaceRequestor,
                              nsISSLStatusProvider,
                              nsIAssociatedContentSecurity,
                              nsISerializable,
                              nsIClassInfo,
                              nsIClientAuthUserDecision)

nsresult
nsNSSSocketInfo::GetHandshakePending(bool *aHandshakePending)
{
  *aHandshakePending = mHandshakePending;
  return NS_OK;
}

nsresult
nsNSSSocketInfo::SetHandshakePending(bool aHandshakePending)
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

PRErrorCode
nsNSSSocketInfo::GetErrorCode() const
{
  MutexAutoLock lock(mMutex);

  return mErrorCode;
}

void
nsNSSSocketInfo::SetCanceled(PRErrorCode errorCode,
                             SSLErrorMessageType errorMessageType)
{
  MutexAutoLock lock(mMutex);

  mErrorCode = errorCode;
  mErrorMessageType = errorMessageType;
  mErrorMessageCached.Truncate();
}

NS_IMETHODIMP nsNSSSocketInfo::GetRememberClientAuthCertificate(bool *aRememberClientAuthCertificate)
{
  NS_ENSURE_ARG_POINTER(aRememberClientAuthCertificate);
  *aRememberClientAuthCertificate = mRememberClientAuthCertificate;
  return NS_OK;
}

NS_IMETHODIMP nsNSSSocketInfo::SetRememberClientAuthCertificate(bool aRememberClientAuthCertificate)
{
  mRememberClientAuthCertificate = aRememberClientAuthCertificate;
  return NS_OK;
}

void nsNSSSocketInfo::SetHasCleartextPhase(bool aHasCleartextPhase)
{
  mHasCleartextPhase = aHasCleartextPhase;
}

bool nsNSSSocketInfo::GetHasCleartextPhase()
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

  return NS_OK;
}

static void
getSecureBrowserUI(nsIInterfaceRequestor * callbacks,
                   nsISecureBrowserUI ** result)
{
  NS_ASSERTION(result != nsnull, "result parameter to getSecureBrowserUI is null");
  *result = nsnull;

  NS_ASSERTION(NS_IsMainThread(),
               "getSecureBrowserUI called off the main thread");

  if (!callbacks)
    return;

  nsCOMPtr<nsISecureBrowserUI> secureUI = do_GetInterface(callbacks);
  if (secureUI) {
    secureUI.forget(result);
    return;
  }

  nsCOMPtr<nsIDocShellTreeItem> item = do_GetInterface(callbacks);
  if (item) {
    nsCOMPtr<nsIDocShellTreeItem> rootItem;
    (void) item->GetSameTypeRootTreeItem(getter_AddRefs(rootItem));
      
    nsCOMPtr<nsIDocShell> docShell = do_QueryInterface(rootItem);
    if (docShell) {
      (void) docShell->GetSecurityUI(result);
    }
  }
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
NS_IMETHODIMP nsNSSSocketInfo::Flush()
{
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
nsNSSSocketInfo::GetErrorMessage(PRUnichar** aText)
{
  NS_ENSURE_ARG_POINTER(aText);
  *aText = nsnull;

  if (!NS_IsMainThread()) {
    NS_ERROR("nsNSSSocketInfo::GetErrorMessage called off the main thread");
    return NS_ERROR_NOT_SAME_THREAD;
  }

  MutexAutoLock lock(mMutex);

  nsresult rv = formatErrorMessage(lock);
  NS_ENSURE_SUCCESS(rv, rv);

  *aText = ToNewUnicode(mErrorMessageCached);
  return *aText != nsnull ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

void
nsNSSSocketInfo::SetNegotiatedNPN(const char *value, PRUint32 length)
{
  if (!value)
    mNegotiatedNPN.Truncate();
  else
    mNegotiatedNPN.Assign(value, length);
  mNPNCompleted = true;
}

NS_IMETHODIMP
nsNSSSocketInfo::GetNegotiatedNPN(nsACString &aNegotiatedNPN)
{
  if (!mNPNCompleted)
    return NS_ERROR_NOT_CONNECTED;

  aNegotiatedNPN = mNegotiatedNPN;
  return NS_OK;
}

NS_IMETHODIMP
nsNSSSocketInfo::JoinConnection(const nsACString & npnProtocol,
                                const nsACString & hostname,
                                PRInt32 port,
                                bool *_retval NS_OUTPARAM)
{
  *_retval = false;

  
  if (port != mPort)
    return NS_OK;

  
  if (!mNPNCompleted || !mNegotiatedNPN.Equals(npnProtocol))
    return NS_OK;

  
  
  if (mHostName && hostname.Equals(mHostName)) {
    *_retval = true;
    return NS_OK;
  }

  
  
  if (!mHandshakeCompleted || !SSLStatus() || !SSLStatus()->mServerCert)
    return NS_OK;

  
  
  
  if (SSLStatus()->mHaveCertErrorBits)
    return NS_OK;

  
  

  CERTCertificate *nssCert = nsnull;
  CERTCertificateCleaner nsscertCleaner(nssCert);

  nsCOMPtr<nsIX509Cert2> cert2 = do_QueryInterface(SSLStatus()->mServerCert);
  if (cert2)
    nssCert = cert2->GetCert();

  if (!nssCert)
    return NS_OK;

  if (CERT_VerifyCertName(nssCert, PromiseFlatCString(hostname).get()) !=
      SECSuccess)
    return NS_OK;

  
  *_retval = true;
  return NS_OK;
}

static nsresult
formatPlainErrorMessage(nsXPIDLCString const & host, PRInt32 port,
                        PRErrorCode err, nsString &returnedMessage);

static nsresult
formatOverridableCertErrorMessage(nsISSLStatus & sslStatus,
                                  PRErrorCode errorCodeToReport, 
                                  const nsXPIDLCString & host, PRInt32 port,
                                  nsString & returnedMessage);





nsresult
nsNSSSocketInfo::formatErrorMessage(MutexAutoLock const & proofOfLock)
{
  if (mErrorCode == 0 || !mErrorMessageCached.IsEmpty()) {
    return NS_OK;
  }

  nsresult rv;
  NS_ConvertASCIItoUTF16 hostNameU(mHostName);
  NS_ASSERTION(mErrorMessageType != OverridableCertErrorMessage || 
                (mSSLStatus && mSSLStatus->mServerCert &&
                 mSSLStatus->mHaveCertErrorBits),
                "GetErrorMessage called for cert error without cert");
  if (mErrorMessageType == OverridableCertErrorMessage && 
      mSSLStatus && mSSLStatus->mServerCert) {
    rv = formatOverridableCertErrorMessage(*mSSLStatus, mErrorCode,
                                           mHostName, mPort,
                                           mErrorMessageCached);
  } else {
    rv = formatPlainErrorMessage(mHostName, mPort, mErrorCode,
                                 mErrorMessageCached);
  }

  if (NS_FAILED(rv)) {
    mErrorMessageCached.Truncate();
  }

  return rv;
}


NS_IMETHODIMP nsNSSSocketInfo::GetInterface(const nsIID & uuid, void * *result)
{
  if (!NS_IsMainThread()) {
    NS_ERROR("nsNSSSocketInfo::GetInterface called off the main thread");
    return NS_ERROR_NOT_SAME_THREAD;
  }

  nsresult rv;
  if (!mCallbacks) {
    nsCOMPtr<nsIInterfaceRequestor> ir = new PipUIContext();
    if (!ir)
      return NS_ERROR_OUT_OF_MEMORY;

    rv = ir->GetInterface(uuid, result);
  } else {
    rv = mCallbacks->GetInterface(uuid, result);
  }
  return rv;
}

nsresult
nsNSSSocketInfo::GetForSTARTTLS(bool* aForSTARTTLS)
{
  *aForSTARTTLS = mForSTARTTLS;
  return NS_OK;
}

nsresult
nsNSSSocketInfo::SetForSTARTTLS(bool aForSTARTTLS)
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
nsNSSSocketInfo::SetNPNList(nsTArray<nsCString> &protocolArray)
{
  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown())
    return NS_ERROR_NOT_AVAILABLE;
  if (!mFd)
    return NS_ERROR_FAILURE;

  
  nsCString npnList;

  for (PRUint32 index = 0; index < protocolArray.Length(); ++index) {
    if (protocolArray[index].IsEmpty() ||
        protocolArray[index].Length() > 255)
      return NS_ERROR_ILLEGAL_VALUE;

    npnList.Append(protocolArray[index].Length());
    npnList.Append(protocolArray[index]);
  }
  
  if (SSL_SetNextProtoNego(
        mFd,
        reinterpret_cast<const unsigned char *>(npnList.get()),
        npnList.Length()) != SECSuccess)
    return NS_ERROR_FAILURE;

  return NS_OK;
}

static NS_DEFINE_CID(kNSSCertificateCID, NS_X509CERT_CID);
#define NSSSOCKETINFOMAGIC { 0xa9863a23, 0x26b8, 0x4a9c, \
  { 0x83, 0xf1, 0xe9, 0xda, 0xdb, 0x36, 0xb8, 0x30 } }
static NS_DEFINE_CID(kNSSSocketInfoMagic, NSSSOCKETINFOMAGIC);

NS_IMETHODIMP
nsNSSSocketInfo::Write(nsIObjectOutputStream* stream) {
  stream->WriteID(kNSSSocketInfoMagic);

  MutexAutoLock lock(mMutex);

  nsRefPtr<nsSSLStatus> status = mSSLStatus;
  nsCOMPtr<nsISerializable> certSerializable;

  
  
  
  
  
  

  if (status) {
    nsCOMPtr<nsIX509Cert> cert = status->mServerCert;
    certSerializable = do_QueryInterface(cert);

    if (!certSerializable) {
      NS_ERROR("certificate is missing or isn't serializable");
      return NS_ERROR_UNEXPECTED;
    }
  } else {
    NS_WARNING("Serializing nsNSSSocketInfo without mSSLStatus");
  }

  
  stream->WriteBoolean(certSerializable);
  if (certSerializable) {
    stream->WriteID(kNSSCertificateCID);
    stream->WriteID(NS_GET_IID(nsISupports));
    certSerializable->Write(stream);
  }

  
  
  
  
  
  
  PRUint32 version = 3;
  stream->Write32(version | 0xFFFF0000);
  stream->Write32(mSecurityState);
  stream->WriteWStringZ(mShortDesc.get());

  
  nsresult rv = formatErrorMessage(lock); 
  NS_ENSURE_SUCCESS(rv, rv);
  stream->WriteWStringZ(mErrorMessageCached.get());

  stream->WriteCompoundObject(NS_ISUPPORTS_CAST(nsISSLStatus*, status),
                              NS_GET_IID(nsISupports), true);

  stream->Write32((PRUint32)mSubRequestsHighSecurity);
  stream->Write32((PRUint32)mSubRequestsLowSecurity);
  stream->Write32((PRUint32)mSubRequestsBrokenSecurity);
  stream->Write32((PRUint32)mSubRequestsNoSecurity);
  return NS_OK;
}

static bool CheckUUIDEquals(PRUint32 m0,
                            nsIObjectInputStream* stream,
                            const nsCID& id)
{
  nsID tempID;
  tempID.m0 = m0;
  stream->Read16(&tempID.m1);
  stream->Read16(&tempID.m2);
  for (int i = 0; i < 8; ++i)
    stream->Read8(&tempID.m3[i]);
  return tempID.Equals(id);
}

NS_IMETHODIMP
nsNSSSocketInfo::Read(nsIObjectInputStream* stream) {
  nsresult rv;

  PRUint32 version;
  bool certificatePresent;

  
  PRUint32 UUID_0;
  stream->Read32(&UUID_0);
  if (UUID_0 == kNSSSocketInfoMagic.m0) {
    
    if (!CheckUUIDEquals(UUID_0, stream, kNSSSocketInfoMagic))
      return NS_ERROR_FAILURE;

    
    
    stream->ReadBoolean(&certificatePresent);
    stream->Read32(&UUID_0);
  }
  else {
    
    
    
    certificatePresent = true;
  }

  if (certificatePresent && UUID_0 == kNSSCertificateCID.m0) {
    
    
    if (!CheckUUIDEquals(UUID_0, stream, kNSSCertificateCID))
      return NS_ERROR_FAILURE;

    
    nsID tempID;
    stream->ReadID(&tempID);
    if (!tempID.Equals(NS_GET_IID(nsISupports)))
      return NS_ERROR_FAILURE;

    nsCOMPtr<nsISerializable> serializable =
        do_CreateInstance(kNSSCertificateCID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    
    serializable->Read(stream);

    
    
    stream->Read32(&version);
  }
  else {
    
    version = UUID_0;
  }

  MutexAutoLock lock(mMutex);

  
  
  
  if ((version & 0xFFFF0000) == 0xFFFF0000) {
    version &= ~0xFFFF0000;
    stream->Read32(&mSecurityState);
  }
  else {
    mSecurityState = version;
    version = 1;
  }
  stream->ReadString(mShortDesc);
  stream->ReadString(mErrorMessageCached);
  mErrorCode = 0;

  nsCOMPtr<nsISupports> obj;
  stream->ReadObject(true, getter_AddRefs(obj));
  
  mSSLStatus = reinterpret_cast<nsSSLStatus*>(obj.get());

  if (!mSSLStatus) {
    NS_WARNING("deserializing nsNSSSocketInfo without mSSLStatus");
  }

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

  if (SECSuccess != SSL_OptionSet(mFd, SSL_SECURITY, true))
    return NS_ERROR_FAILURE;		
  if (SECSuccess != SSL_ResetHandshake(mFd, false))
    return NS_ERROR_FAILURE;

  mHandshakePending = true;

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

class PreviousCertRunnable : public SyncRunnableBase
{
public:
  PreviousCertRunnable(nsIInterfaceRequestor * callbacks)
    : mCallbacks(callbacks)
  {
  }

  virtual void RunOnTargetThread()
  {
    nsCOMPtr<nsISecureBrowserUI> secureUI;
    getSecureBrowserUI(mCallbacks, getter_AddRefs(secureUI));
    nsCOMPtr<nsISSLStatusProvider> statusProvider = do_QueryInterface(secureUI);
    if (statusProvider) {
      nsCOMPtr<nsISSLStatus> status;
      (void) statusProvider->GetSSLStatus(getter_AddRefs(status));
      if (status) {
        (void) status->GetServerCert(getter_AddRefs(mPreviousCert));
      }
    }
  }

  nsCOMPtr<nsIX509Cert> mPreviousCert; 
private:
  nsCOMPtr<nsIInterfaceRequestor> mCallbacks; 
};

void nsNSSSocketInfo::GetPreviousCert(nsIX509Cert** _result)
{
  NS_ASSERTION(_result, "_result parameter to GetPreviousCert is null");
  *_result = nsnull;

  if (NS_IsMainThread()) {
    NS_ERROR("nsNSSSocketInfo::GetPreviousCert called on the main thread");
    return;
  }

  nsRefPtr<PreviousCertRunnable> runnable = new PreviousCertRunnable(mCallbacks);
  nsresult rv = runnable->DispatchToMainThreadAndWait();
  NS_ASSERTION(NS_SUCCEEDED(rv), "runnable->DispatchToMainThreadAndWait() failed");
  runnable->mPreviousCert.forget(_result);
}

void
nsNSSSocketInfo::SetCertVerificationWaiting()
{
  
  
  
  NS_ASSERTION(mCertVerificationState != waiting_for_cert_verification,
               "Invalid state transition to waiting_for_cert_verification");
  mCertVerificationState = waiting_for_cert_verification;
}

void
nsNSSSocketInfo::SetCertVerificationResult(PRErrorCode errorCode,
                                           SSLErrorMessageType errorMessageType)
{
  NS_ASSERTION(mCertVerificationState == waiting_for_cert_verification,
               "Invalid state transition to cert_verification_finished");

  if (errorCode != 0) {
    SetCanceled(errorCode, errorMessageType);
  } else if (mFd) {
    
    SECStatus rv = SSL_RestartHandshakeAfterAuthCertificate(mFd);
    if (rv != SECSuccess) {
      errorCode = PR_GetError();
      if (errorCode == 0) {
        NS_ERROR("SSL_RestartHandshakeAfterAuthCertificate didn't set error code");
        errorCode = PR_INVALID_STATE_ERROR;
      }
      SetCanceled(errorCode, PlainErrorMessage);
    }
  } else {
    
  }

  mCertVerificationState = after_cert_verification;
}

nsresult nsNSSSocketInfo::GetSSLStatus(nsISSLStatus** _result)
{
  NS_ENSURE_ARG_POINTER(_result);

  *_result = mSSLStatus;
  NS_IF_ADDREF(*_result);

  return NS_OK;
}

nsresult nsNSSSocketInfo::SetSSLStatus(nsSSLStatus *aSSLStatus)
{
  mSSLStatus = aSSLStatus;

  return NS_OK;
}

void nsNSSSocketInfo::SetHandshakeInProgress(bool aIsIn)
{
  mHandshakeInProgress = aIsIn;

  if (mHandshakeInProgress && !mHandshakeStartTime)
  {
    mHandshakeStartTime = PR_IntervalNow();
  }
}

void nsNSSSocketInfo::SetAllowTLSIntoleranceTimeout(bool aAllow)
{
  mAllowTLSIntoleranceTimeout = aAllow;
}

#define HANDSHAKE_TIMEOUT_SECONDS 25

bool nsNSSSocketInfo::HandshakeTimeout()
{
  if (!mHandshakeInProgress || !mAllowTLSIntoleranceTimeout)
    return false;

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

  if (mRenegoUnrestrictedSites) {
    delete mRenegoUnrestrictedSites;
    mRenegoUnrestrictedSites = nsnull;
  }

  if (mutex) {
    delete mutex;
    mutex = nsnull;
  }

  if (mHostsWithCertErrors) {
    delete mHostsWithCertErrors;
    mHostsWithCertErrors = nsnull;
  }
}






static nsresult
formatPlainErrorMessage(const nsXPIDLCString &host, PRInt32 port,
                        PRErrorCode err, nsString &returnedMessage)
{
  const PRUnichar *params[1];
  nsresult rv;

  nsCOMPtr<nsINSSComponent> component = do_GetService(kNSSComponentCID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  if (host.Length())
  {
    nsString hostWithPort;

    
    
    
    
    
    

    hostWithPort.AssignASCII(host);
    if (port != 443) {
      hostWithPort.AppendLiteral(":");
      hostWithPort.AppendInt(port);
    }
    params[0] = hostWithPort.get();

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
    bool isSelfSigned;
    if (NS_SUCCEEDED(cert3->GetIsSelfSigned(&isSelfSigned))
        && isSelfSigned) {
      errorID = "certErrorTrust_SelfSigned";
    }
  }

  if (!errorID) {
    switch (errTrust) {
      case SEC_ERROR_UNKNOWN_ISSUER:
      {
        nsCOMPtr<nsIArray> chain;
        ix509->GetChain(getter_AddRefs(chain));
        PRUint32 length = 0;
        if (chain && NS_FAILED(chain->GetLength(&length)))
          length = 0;
        if (length == 1)
          errorID = "certErrorTrust_MissingChain";
        else
          errorID = "certErrorTrust_UnknownIssuer";
        break;
      }
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





static bool
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

  nsresult rv;
  rv = CERT_FindCertExtension(nssCert, SEC_OID_X509_SUBJECT_ALT_NAME,
                              &altNameExtension);
  if (rv != SECSuccess)
    return false;

  san_arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
  if (!san_arena)
    return false;

  sanNameList = CERT_DecodeAltNameExtension(san_arena, &altNameExtension);
  if (!sanNameList)
    return false;

  SECITEM_FreeItem(&altNameExtension, false);

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

  PORT_FreeArena(san_arena, false);
  return true;
}

static void
AppendErrorTextMismatch(const nsString &host,
                        nsIX509Cert* ix509,
                        nsINSSComponent *component,
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
  bool useSAN = false;

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

    nsString formattedString;
    rv = component->PIPBundleFormatStringFromName("certErrorMismatchSingle2", 
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
                nsString &nowDate,
                bool &trueExpired_falseNotYetValid)
{
  trueExpired_falseNotYetValid = true;
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

  PRTime now = PR_Now();
  if (LL_CMP(now, >, notAfter)) {
    timeToUse = notAfter;
  } else {
    timeToUse = notBefore;
    trueExpired_falseNotYetValid = false;
  }

  nsCOMPtr<nsIDateTimeFormat> dateTimeFormat(do_CreateInstance(NS_DATETIMEFORMAT_CONTRACTID, &rv));
  if (NS_FAILED(rv))
    return;

  dateTimeFormat->FormatPRTime(nsnull, kDateFormatShort, 
                               kTimeFormatNoSeconds, timeToUse, 
                               formattedDate);
  dateTimeFormat->FormatPRTime(nsnull, kDateFormatShort,
                               kTimeFormatNoSeconds, now,
                               nowDate);
}

static void
AppendErrorTextTime(nsIX509Cert* ix509,
                    nsINSSComponent *component,
                    nsString &returnedMessage)
{
  nsAutoString formattedDate, nowDate;
  bool trueExpired_falseNotYetValid;
  GetDateBoundary(ix509, formattedDate, nowDate, trueExpired_falseNotYetValid);

  const PRUnichar *params[2];
  params[0] = formattedDate.get(); 
  params[1] = nowDate.get();

  const char *key = trueExpired_falseNotYetValid ? 
                    "certErrorExpiredNow" : "certErrorNotYetValidNow";
  nsresult rv;
  nsString formattedString;
  rv = component->PIPBundleFormatStringFromName(
           key,
           params, 
           ArrayLength(params),
           formattedString);
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
formatOverridableCertErrorMessage(nsISSLStatus & sslStatus,
                                  PRErrorCode errorCodeToReport, 
                                  const nsXPIDLCString & host, PRInt32 port,
                                  nsString & returnedMessage)
{
  const PRUnichar *params[1];
  nsresult rv;
  nsAutoString hostWithPort;
  nsAutoString hostWithoutPort;

  
  
  
  
  
  
  
  hostWithoutPort.AppendASCII(host);
  if (port == 443) {
    params[0] = hostWithoutPort.get();
  } else {
    hostWithPort.AppendASCII(host);
    hostWithPort.Append(':');
    hostWithPort.AppendInt(port);
    params[0] = hostWithPort.get();
  }

  nsCOMPtr<nsINSSComponent> component = do_GetService(kNSSComponentCID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  returnedMessage.Truncate();
  rv = component->PIPBundleFormatStringFromName("certErrorIntro", params, 1,
                                                returnedMessage);
  NS_ENSURE_SUCCESS(rv, rv);

  returnedMessage.Append(NS_LITERAL_STRING("\n\n"));

  nsRefPtr<nsIX509Cert> ix509;
  rv = sslStatus.GetServerCert(getter_AddRefs(ix509));
  NS_ENSURE_SUCCESS(rv, rv);

  bool isUntrusted;
  rv = sslStatus.GetIsUntrusted(&isUntrusted);
  NS_ENSURE_SUCCESS(rv, rv);
  if (isUntrusted) {
    AppendErrorTextUntrusted(errorCodeToReport, hostWithoutPort, ix509, 
                             component, returnedMessage);
  }

  bool isDomainMismatch;
  rv = sslStatus.GetIsDomainMismatch(&isDomainMismatch);
  NS_ENSURE_SUCCESS(rv, rv);
  if (isDomainMismatch) {
    AppendErrorTextMismatch(hostWithoutPort, ix509, component, returnedMessage);
  }

  bool isNotValidAtThisTime;
  rv = sslStatus.GetIsNotValidAtThisTime(&isNotValidAtThisTime);
  NS_ENSURE_SUCCESS(rv, rv);
  if (isNotValidAtThisTime) {
    AppendErrorTextTime(ix509, component, returnedMessage);
  }

  AppendErrorTextCode(errorCodeToReport, component, returnedMessage);

  return NS_OK;
}

static void
nsHandleSSLError(nsNSSSocketInfo *socketInfo, PRErrorCode err)
{
  if (!NS_IsMainThread()) {
    NS_ERROR("nsHandleSSLError called off the main thread");
    return;
  }

  
  
  
  
  if (socketInfo->GetErrorCode()) {
    
    
    return;
  }

  nsresult rv;
  NS_DEFINE_CID(nssComponentCID, NS_NSSCOMPONENT_CID);
  nsCOMPtr<nsINSSComponent> nssComponent(do_GetService(nssComponentCID, &rv));
  if (NS_FAILED(rv))
    return;

  nsXPIDLCString hostName;
  socketInfo->GetHostName(getter_Copies(hostName));

  PRInt32 port;
  socketInfo->GetPort(&port);

  
  nsCOMPtr<nsIInterfaceRequestor> cb;
  socketInfo->GetNotificationCallbacks(getter_AddRefs(cb));
  if (cb) {
    nsCOMPtr<nsISSLErrorListener> sel = do_GetInterface(cb);
    if (sel) {
      nsIInterfaceRequestor *csi = static_cast<nsIInterfaceRequestor*>(socketInfo);
      nsCString hostWithPortString = hostName;
      hostWithPortString.AppendLiteral(":");
      hostWithPortString.AppendInt(port);
    
      bool suppressMessage = false; 
      rv = sel->NotifySSLError(csi, err, hostWithPortString, &suppressMessage);
    }
  }

  socketInfo->SetCanceled(err, PlainErrorMessage);
}

namespace {

nsNSSSocketInfo *
getSocketInfoIfRunning(PRFileDesc * fd,
                       const nsNSSShutDownPreventionLock & )
{
  if (!fd || !fd->lower || !fd->secret ||
      fd->identity != nsSSLIOLayerHelpers::nsSSLIOLayerIdentity) {
    NS_ERROR("bad file descriptor passed to getSocketInfoIfRunning");
    PR_SetError(PR_BAD_DESCRIPTOR_ERROR, 0);
    return nsnull;
  }

  nsNSSSocketInfo *socketInfo = (nsNSSSocketInfo*)fd->secret;

  if (socketInfo->isAlreadyShutDown() || socketInfo->isPK11LoggedOut()) {
    PR_SetError(PR_SOCKET_SHUTDOWN_ERROR, 0);
    return nsnull;
  }

  if (socketInfo->GetErrorCode()) {
    PRErrorCode err = socketInfo->GetErrorCode();
    
    
    PR_SetError(err, 0);
    return nsnull;
  }

  return socketInfo;
}

} 

static PRStatus PR_CALLBACK
nsSSLIOLayerConnect(PRFileDesc* fd, const PRNetAddr* addr,
                    PRIntervalTime timeout)
{
  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("[%p] connecting SSL socket\n", (void*)fd));
  nsNSSShutDownPreventionLock locker;
  if (!getSocketInfoIfRunning(fd, locker))
    return PR_FAILURE;
  
  PRStatus status = fd->lower->methods->connect(fd->lower, addr, timeout);
  if (status != PR_SUCCESS) {
    PR_LOG(gPIPNSSLog, PR_LOG_ERROR, ("[%p] Lower layer connect error: %d\n",
                                      (void*)fd, PR_GetError()));
    return status;
  }

  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("[%p] Connect\n", (void*)fd));
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

  
  status->mHaveCertErrorBits = true;
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



bool
nsSSLIOLayerHelpers::rememberPossibleTLSProblemSite(PRFileDesc* ssl_layer_fd, nsNSSSocketInfo *socketInfo)
{
  PRBool currentlyUsesTLS = false;

  nsCAutoString key;
  getSiteKey(socketInfo, key);

  SSL_OptionGet(ssl_layer_fd, SSL_ENABLE_TLS, &currentlyUsesTLS);
  if (!currentlyUsesTLS) {
    
    
    
    
    removeIntolerantSite(key);
    return false;
  }

  PRBool enableSSL3 = false;
  SSL_OptionGet(ssl_layer_fd, SSL_ENABLE_SSL3, &enableSSL3);
  if (enableSSL3) {
    
    addIntolerantSite(key);
  }
  
  return currentlyUsesTLS;
}

void
nsSSLIOLayerHelpers::rememberTolerantSite(PRFileDesc* ssl_layer_fd, 
                                          nsNSSSocketInfo *socketInfo)
{
  PRBool usingSecurity = false;
  PRBool currentlyUsesTLS = false;
  SSL_OptionGet(ssl_layer_fd, SSL_SECURITY, &usingSecurity);
  SSL_OptionGet(ssl_layer_fd, SSL_ENABLE_TLS, &currentlyUsesTLS);
  if (!usingSecurity || !currentlyUsesTLS) {
    return;
  }

  nsCAutoString key;
  getSiteKey(socketInfo, key);

  MutexAutoLock lock(*mutex);
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

  return socketInfo->CloseSocketAndDestroy(locker);
}

PRStatus nsNSSSocketInfo::CloseSocketAndDestroy(
  const nsNSSShutDownPreventionLock & )
{
  nsNSSShutDownList::trackSSLSocketClose();

  PRFileDesc* popped = PR_PopIOLayer(mFd, PR_TOP_IO_LAYER);

  PRStatus status = mFd->methods->close(mFd);
  
  
  
  
  
  mFd = nsnull;
  
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

static bool
isNonSSLErrorThatWeAllowToRetry(PRInt32 err, bool withInitialCleartext)
{
  switch (err)
  {
    case PR_CONNECT_RESET_ERROR:
      if (!withInitialCleartext)
        return true;
      break;
    
    case PR_END_OF_FILE_ERROR:
      return true;
  }

  return false;
}

static bool
isTLSIntoleranceError(PRInt32 err, bool withInitialCleartext)
{
  
  
  
  
  
  
  
  

  if (isNonSSLErrorThatWeAllowToRetry(err, withInitialCleartext))
    return true;

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
      return true;
  }
  
  return false;
}

class SSLErrorRunnable : public SyncRunnableBase
{
 public:
  SSLErrorRunnable(nsNSSSocketInfo * infoObject, PRErrorCode errorCode)
    : mInfoObject(infoObject), mErrorCode(errorCode)
  {
  }

  virtual void RunOnTargetThread()
  {
    nsHandleSSLError(mInfoObject, mErrorCode);
  }
  
  nsRefPtr<nsNSSSocketInfo> mInfoObject;
  const PRErrorCode mErrorCode;
};

PRInt32 checkHandshake(PRInt32 bytesTransfered, bool wasReading,
                       PRFileDesc* ssl_layer_fd,
                       nsNSSSocketInfo *socketInfo)
{
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  
  
  
  
  
  
  
  
  
  

  
  
  
  

  bool handleHandshakeResultNow;
  socketInfo->GetHandshakePending(&handleHandshakeResultNow);

  bool wantRetry = false;

  if (0 > bytesTransfered) {
    PRInt32 err = PR_GetError();

    if (handleHandshakeResultNow) {
      if (PR_WOULD_BLOCK_ERROR == err) {
        socketInfo->SetHandshakeInProgress(true);
        return bytesTransfered;
      }

      if (!wantRetry 
          && isTLSIntoleranceError(err, socketInfo->GetHasCleartextPhase()))
      {
        wantRetry = nsSSLIOLayerHelpers::rememberPossibleTLSProblemSite(ssl_layer_fd, socketInfo);
      }
    }
    
    
    
    
    
    
    
    
    
    
    if (!wantRetry && (IS_SSL_ERROR(err) || IS_SEC_ERROR(err)) &&
        !socketInfo->GetErrorCode()) {
      nsRefPtr<SyncRunnableBase> runnable = new SSLErrorRunnable(socketInfo,
                                                                 err);
      (void) runnable->DispatchToMainThreadAndWait();
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
    socketInfo->SetHandshakePending(false);
    socketInfo->SetHandshakeInProgress(false);
  }
  
  return bytesTransfered;
}

static PRInt16 PR_CALLBACK
nsSSLIOLayerPoll(PRFileDesc * fd, PRInt16 in_flags, PRInt16 *out_flags)
{
  nsNSSShutDownPreventionLock locker;

  if (!out_flags)
  {
    NS_WARNING("nsSSLIOLayerPoll called with null out_flags");
    return 0;
  }

  *out_flags = 0;

  nsNSSSocketInfo * socketInfo = getSocketInfoIfRunning(fd, locker);
  if (!socketInfo) {
    
    
    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG,
            ("[%p] polling SSL socket right after certificate verification failed "
                  "or NSS shutdown or SDR logout %d\n",
             fd, (int) in_flags));

    NS_ASSERTION(in_flags & PR_POLL_EXCEPT,
                 "caller did not poll for EXCEPT (canceled)");
    
    
    
    *out_flags = in_flags | PR_POLL_EXCEPT; 
    return in_flags;
  }

  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG,
         (socketInfo->IsWaitingForCertVerification()
            ?  "[%p] polling SSL socket during certificate verification using lower %d\n"
            :  "[%p] poll SSL socket using lower %d\n",
         fd, (int) in_flags));

  if (socketInfo->HandshakeTimeout()) {
    NS_ASSERTION(in_flags & PR_POLL_EXCEPT,
                 "caller did not poll for EXCEPT (handshake timeout)");
    *out_flags = in_flags | PR_POLL_EXCEPT;
    return in_flags;
  }

  
  
  
  
  PRInt16 result = fd->lower->methods->poll(fd->lower, in_flags, out_flags);
  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("[%p] poll SSL socket returned %d\n",
                                    (void*)fd, (int) result));
  return result;
}

bool nsSSLIOLayerHelpers::nsSSLIOLayerInitialized = false;
PRDescIdentity nsSSLIOLayerHelpers::nsSSLIOLayerIdentity;
PRIOMethods nsSSLIOLayerHelpers::nsSSLIOLayerMethods;
Mutex *nsSSLIOLayerHelpers::mutex = nsnull;
nsCStringHashSet *nsSSLIOLayerHelpers::mTLSIntolerantSites = nsnull;
nsCStringHashSet *nsSSLIOLayerHelpers::mTLSTolerantSites = nsnull;
nsPSMRememberCertErrorsTable *nsSSLIOLayerHelpers::mHostsWithCertErrors = nsnull;
nsCStringHashSet *nsSSLIOLayerHelpers::mRenegoUnrestrictedSites = nsnull;
bool nsSSLIOLayerHelpers::mTreatUnsafeNegotiationAsBroken = false;
PRInt32 nsSSLIOLayerHelpers::mWarnLevelMissingRFC5746 = 1;

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
  if (!getSocketInfoIfRunning(fd, locker))
    return PR_FAILURE;

  return fd->lower->methods->getsockname(fd->lower, addr);
}

static PRStatus PR_CALLBACK PSMGetpeername(PRFileDesc *fd, PRNetAddr *addr)
{
  nsNSSShutDownPreventionLock locker;
  if (!getSocketInfoIfRunning(fd, locker))
    return PR_FAILURE;

  return fd->lower->methods->getpeername(fd->lower, addr);
}

static PRStatus PR_CALLBACK PSMGetsocketoption(PRFileDesc *fd, 
                                        PRSocketOptionData *data)
{
  nsNSSShutDownPreventionLock locker;
  if (!getSocketInfoIfRunning(fd, locker))
    return PR_FAILURE;

  return fd->lower->methods->getsocketoption(fd, data);
}

static PRStatus PR_CALLBACK PSMSetsocketoption(PRFileDesc *fd, 
                                        const PRSocketOptionData *data)
{
  nsNSSShutDownPreventionLock locker;
  if (!getSocketInfoIfRunning(fd, locker))
    return PR_FAILURE;

  return fd->lower->methods->setsocketoption(fd, data);
}

static PRInt32 PR_CALLBACK PSMRecv(PRFileDesc *fd, void *buf, PRInt32 amount,
    PRIntn flags, PRIntervalTime timeout)
{
  nsNSSShutDownPreventionLock locker;
  nsNSSSocketInfo *socketInfo = getSocketInfoIfRunning(fd, locker);
  if (!socketInfo)
    return -1;

  if (flags != PR_MSG_PEEK && flags != 0) {
    PR_SetError(PR_INVALID_ARGUMENT_ERROR, 0);
    return -1;
  }

  PRInt32 bytesRead = fd->lower->methods->recv(fd->lower, buf, amount, flags,
                                               timeout);

  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("[%p] read %d bytes\n", (void*)fd, bytesRead));

#ifdef DEBUG_SSL_VERBOSE
  DEBUG_DUMP_BUFFER((unsigned char*)buf, bytesRead);
#endif

  return checkHandshake(bytesRead, true, fd, socketInfo);
}

static PRInt32 PR_CALLBACK PSMSend(PRFileDesc *fd, const void *buf, PRInt32 amount,
    PRIntn flags, PRIntervalTime timeout)
{
  nsNSSShutDownPreventionLock locker;
  nsNSSSocketInfo *socketInfo = getSocketInfoIfRunning(fd, locker);
  if (!socketInfo)
    return -1;

  if (flags != 0) {
    PR_SetError(PR_INVALID_ARGUMENT_ERROR, 0);
    return -1;
  }

#ifdef DEBUG_SSL_VERBOSE
  DEBUG_DUMP_BUFFER((unsigned char*)buf, amount);
#endif

  PRInt32 bytesWritten = fd->lower->methods->send(fd->lower, buf, amount,
                                                  flags, timeout);

  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("[%p] wrote %d bytes\n",
         fd, bytesWritten));

  return checkHandshake(bytesWritten, false, fd, socketInfo);
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
  if (!getSocketInfoIfRunning(fd, locker)) {
    return PR_FAILURE;
  }

  return fd->lower->methods->connectcontinue(fd, out_flags);
}

nsresult nsSSLIOLayerHelpers::Init()
{
  if (!nsSSLIOLayerInitialized) {
    nsSSLIOLayerInitialized = true;
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

  mutex = new Mutex("nsSSLIOLayerHelpers.mutex");

  mTLSIntolerantSites = new nsCStringHashSet();
  if (!mTLSIntolerantSites)
    return NS_ERROR_OUT_OF_MEMORY;

  mTLSIntolerantSites->Init(1);

  mTLSTolerantSites = new nsCStringHashSet();
  if (!mTLSTolerantSites)
    return NS_ERROR_OUT_OF_MEMORY;

  
  
  
  mTLSTolerantSites->Init(16);

  mRenegoUnrestrictedSites = new nsCStringHashSet();
  if (!mRenegoUnrestrictedSites)
    return NS_ERROR_OUT_OF_MEMORY;

  mRenegoUnrestrictedSites->Init(1);

  mTreatUnsafeNegotiationAsBroken = false;
  
  mHostsWithCertErrors = new nsPSMRememberCertErrorsTable();
  if (!mHostsWithCertErrors || !mHostsWithCertErrors->mErrorHosts.IsInitialized())
    return NS_ERROR_OUT_OF_MEMORY;

  return NS_OK;
}

void nsSSLIOLayerHelpers::addIntolerantSite(const nsCString &str)
{
  MutexAutoLock lock(*mutex);
  
  if (!mTLSTolerantSites->Contains(str))
    nsSSLIOLayerHelpers::mTLSIntolerantSites->Put(str);
}

void nsSSLIOLayerHelpers::removeIntolerantSite(const nsCString &str)
{
  MutexAutoLock lock(*mutex);
  nsSSLIOLayerHelpers::mTLSIntolerantSites->Remove(str);
}

bool nsSSLIOLayerHelpers::isKnownAsIntolerantSite(const nsCString &str)
{
  MutexAutoLock lock(*mutex);
  return mTLSIntolerantSites->Contains(str);
}

void nsSSLIOLayerHelpers::setRenegoUnrestrictedSites(const nsCString &str)
{
  MutexAutoLock lock(*mutex);
  
  if (mRenegoUnrestrictedSites) {
    delete mRenegoUnrestrictedSites;
    mRenegoUnrestrictedSites = nsnull;
  }

  mRenegoUnrestrictedSites = new nsCStringHashSet();
  if (!mRenegoUnrestrictedSites)
    return;
  
  mRenegoUnrestrictedSites->Init(1);
  
  nsCCharSeparatedTokenizer toker(str, ',');

  while (toker.hasMoreTokens()) {
    const nsCSubstring &host = toker.nextToken();
    if (!host.IsEmpty()) {
      mRenegoUnrestrictedSites->Put(host);
    }
  }
}

bool nsSSLIOLayerHelpers::isRenegoUnrestrictedSite(const nsCString &str)
{
  MutexAutoLock lock(*mutex);
  return mRenegoUnrestrictedSites->Contains(str);
}

void nsSSLIOLayerHelpers::setTreatUnsafeNegotiationAsBroken(bool broken)
{
  MutexAutoLock lock(*mutex);
  mTreatUnsafeNegotiationAsBroken = broken;
}

bool nsSSLIOLayerHelpers::treatUnsafeNegotiationAsBroken()
{
  MutexAutoLock lock(*mutex);
  return mTreatUnsafeNegotiationAsBroken;
}

void nsSSLIOLayerHelpers::setWarnLevelMissingRFC5746(PRInt32 level)
{
  MutexAutoLock lock(*mutex);
  mWarnLevelMissingRFC5746 = level;
}

PRInt32 nsSSLIOLayerHelpers::getWarnLevelMissingRFC5746()
{
  MutexAutoLock lock(*mutex);
  return mWarnLevelMissingRFC5746;
}

nsresult
nsSSLIOLayerNewSocket(PRInt32 family,
                      const char *host,
                      PRInt32 port,
                      const char *proxyHost,
                      PRInt32 proxyPort,
                      PRFileDesc **fd,
                      nsISupports** info,
                      bool forSTARTTLS,
                      bool anonymousLoad)
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
            
            caNameStrings[n] = const_cast<char*>("");
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







static bool CERT_MatchesScopeOfUse(CERTCertificate* cert, char* hostname,
                                     char* hostIP, PRIntn port)
{
    bool rv = true; 
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
                


                rv = true;
            }
            else {
                rv = false;
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
                rv = true;
            }
            else {
                rv = false;
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
            
            rv = false;
            continue;
        }

        
        PR_ASSERT(rv);
        break;
    }
done:
    
    if (arena != NULL) {
        PORT_FreeArena(arena, false);
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

static bool hasExplicitKeyUsageNonRepudiation(CERTCertificate *cert)
{
  
  if (!cert->extensions)
    return false;

  SECStatus srv;
  SECItem keyUsageItem;
  keyUsageItem.data = NULL;

  srv = CERT_FindKeyUsageExtension(cert, &keyUsageItem);
  if (srv == SECFailure)
    return false;

  unsigned char keyUsage = keyUsageItem.data[0];
  PORT_Free (keyUsageItem.data);

  return !!(keyUsage & KU_NON_REPUDIATION);
}

class ClientAuthDataRunnable : public SyncRunnableBase
{
public:
  ClientAuthDataRunnable(CERTDistNames* caNames,
                         CERTCertificate** pRetCert,
                         SECKEYPrivateKey** pRetKey,
                         nsNSSSocketInfo * info,
                         CERTCertificate * serverCert) 
    : mRV(SECFailure)
    , mErrorCodeToReport(SEC_ERROR_NO_MEMORY)
    , mCANames(caNames)
    , mPRetCert(pRetCert)
    , mPRetKey(pRetKey)
    , mSocketInfo(info)
    , mServerCert(serverCert)
  {
  }

  SECStatus mRV;                        
  PRErrorCode mErrorCodeToReport;       
protected:
  virtual void RunOnTargetThread();
private:
  CERTDistNames* const mCANames;        
  CERTCertificate** const mPRetCert;    
  SECKEYPrivateKey** const mPRetKey;    
  nsNSSSocketInfo * const mSocketInfo;  
  CERTCertificate * const mServerCert;  
};
















SECStatus nsNSS_SSLGetClientAuthData(void* arg, PRFileDesc* socket,
								   CERTDistNames* caNames,
								   CERTCertificate** pRetCert,
								   SECKEYPrivateKey** pRetKey)
{
  nsNSSShutDownPreventionLock locker;

  if (!socket || !caNames || !pRetCert || !pRetKey) {
    PR_SetError(PR_INVALID_ARGUMENT_ERROR, 0);
    return SECFailure;
  }

  nsRefPtr<nsNSSSocketInfo> info
        = reinterpret_cast<nsNSSSocketInfo*>(socket->higher->secret);

  CERTCertificate* serverCert = SSL_PeerCertificate(socket);
  if (!serverCert) {
    NS_NOTREACHED("Missing server certificate should have been detected during "
                  "server cert authentication.");
    PR_SetError(SSL_ERROR_NO_CERTIFICATE, 0);
    return SECFailure;
  }

  
  nsRefPtr<ClientAuthDataRunnable> runnable =
    new ClientAuthDataRunnable(caNames, pRetCert, pRetKey, info, serverCert);
  nsresult rv = runnable->DispatchToMainThreadAndWait();
  if (NS_FAILED(rv)) {
    PR_SetError(SEC_ERROR_NO_MEMORY, 0);
    return SECFailure;
  }
  
  if (runnable->mRV != SECSuccess) {
    PR_SetError(runnable->mErrorCodeToReport, 0);
  }

  return runnable->mRV;
}

void ClientAuthDataRunnable::RunOnTargetThread()
{
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
  void * wincx = mSocketInfo;

  
  arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
  if (arena == NULL) {
    goto loser;
  }

  caNameStrings = (char**)PORT_ArenaAlloc(arena, 
                                          sizeof(char*)*(mCANames->nnames));
  if (caNameStrings == NULL) {
    goto loser;
  }

  mRV = nsConvertCANamesToStrings(arena, caNameStrings, mCANames);
  if (mRV != SECSuccess) {
    goto loser;
  }

  
  if (NS_FAILED(nsGetUserCertChoice(&certChoice))) {
    goto loser;
  }

  	
  if (certChoice == AUTO) {
    

    
    certList = CERT_FindUserCertsByUsage(CERT_GetDefaultCertDB(), 
                                         certUsageSSLClient, false,
                                         true, wincx);
    if (certList == NULL) {
      goto noCert;
    }

    
    mRV = CERT_FilterCertListByCANames(certList, mCANames->nnames,
                                       caNameStrings, certUsageSSLClient);
    if (mRV != SECSuccess) {
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
      if (!CERT_MatchesScopeOfUse(node->cert, mSocketInfo->GetHostName,
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
    

    nsXPIDLCString hostname;
    mSocketInfo->GetHostName(getter_Copies(hostname));

    nsresult rv;
    NS_DEFINE_CID(nssComponentCID, NS_NSSCOMPONENT_CID);
    nsCOMPtr<nsINSSComponent> nssComponent(do_GetService(nssComponentCID, &rv));
    nsRefPtr<nsClientAuthRememberService> cars;
    if (nssComponent) {
      nssComponent->GetClientAuthRememberService(getter_AddRefs(cars));
    }

    bool hasRemembered = false;
    nsCString rememberedDBKey;
    if (cars) {
      bool found;
      nsresult rv = cars->HasRememberedDecision(hostname, mServerCert,
                                                rememberedDBKey, &found);
      if (NS_SUCCEEDED(rv) && found) {
        hasRemembered = true;
      }
    }

    bool canceled = false;

if (hasRemembered)
{
    if (rememberedDBKey.IsEmpty())
    {
      canceled = true;
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
          hasRemembered = false;
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
                                         certUsageSSLClient, false, 
                                         false, wincx);
    if (certList == NULL) {
      goto noCert;
    }

    if (mCANames->nnames != 0) {
      


      mRV = CERT_FilterCertListByCANames(certList, mCANames->nnames, 
                                        caNameStrings, 
                                        certUsageSSLClient);
      if (mRV != SECSuccess) {
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

    
    char *ccn = CERT_GetCommonName(&mServerCert->subject);
    void *v = ccn;
    voidCleaner ccnCleaner(v);
    NS_ConvertUTF8toUTF16 cn(ccn);

    PRInt32 port;
    mSocketInfo->GetPort(&port);

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

    char *corg = CERT_GetOrgName(&mServerCert->subject);
    NS_ConvertUTF8toUTF16 org(corg);
    if (corg) PORT_Free(corg);

    char *cissuer = CERT_GetOrgName(&mServerCert->issuer);
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
      nsRefPtr<nsNSSCertificate> tempCert = nsNSSCertificate::Create(node->cert);

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
        rv = dialogs->ChooseCertificate(mSocketInfo, cn_host_port.get(),
                                        org.get(), issuer.get(), 
                                        (const PRUnichar**)certNicknameList,
                                        (const PRUnichar**)certDetailsList,
                                        CertsToUse, &selectedIndex, &canceled);
      }
    }

    NS_RELEASE(dialogs);
    NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(CertsToUse, certNicknameList);
    NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(CertsToUse, certDetailsList);
    
    if (NS_FAILED(rv)) goto loser;

    
    bool wantRemember = false;
    mSocketInfo->GetRememberClientAuthCertificate(&wantRemember);

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
      cars->RememberDecision(hostname, mServerCert, canceled ? 0 : cert);
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
  if (mRV == SECSuccess) {
    mRV = SECFailure;
  }
  if (cert != NULL) {
    CERT_DestroyCertificate(cert);
    cert = NULL;
  }
done:
  int error = PR_GetError();

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
    PORT_FreeArena(arena, false);
  }

  *mPRetCert = cert;
  *mPRetKey = privKey;

  if (mRV == SECFailure) {
    mErrorCodeToReport = error;
  }
}

class CertErrorRunnable : public SyncRunnableBase
{
 public:
  CertErrorRunnable(const void * fdForLogging,
                    nsIX509Cert * cert,
                    nsNSSSocketInfo * infoObject,
                    PRErrorCode defaultErrorCodeToReport,
                    PRUint32 collectedErrors,
                    PRErrorCode errorCodeTrust,
                    PRErrorCode errorCodeMismatch,
                    PRErrorCode errorCodeExpired)
    : mFdForLogging(fdForLogging), mCert(cert), mInfoObject(infoObject),
      mDefaultErrorCodeToReport(defaultErrorCodeToReport),
      mCollectedErrors(collectedErrors),
      mErrorCodeTrust(errorCodeTrust),
      mErrorCodeMismatch(errorCodeMismatch),
      mErrorCodeExpired(errorCodeExpired)
  {
  }

  NS_DECL_NSIRUNNABLE
  virtual void RunOnTargetThread();
  nsCOMPtr<nsIRunnable> mResult; 
private:
  SSLServerCertVerificationResult* CheckCertOverrides();
  
  const void * const mFdForLogging; 
  const nsCOMPtr<nsIX509Cert> mCert;
  const nsRefPtr<nsNSSSocketInfo> mInfoObject;
  const PRErrorCode mDefaultErrorCodeToReport;
  const PRUint32 mCollectedErrors;
  const PRErrorCode mErrorCodeTrust;
  const PRErrorCode mErrorCodeMismatch;
  const PRErrorCode mErrorCodeExpired;
};

namespace mozilla { namespace psm {








SECStatus
HandleBadCertificate(PRErrorCode defaultErrorCodeToReport,
                    nsNSSSocketInfo * socketInfo, CERTCertificate & cert,
                    const void * fdForLogging,
                    const nsNSSShutDownPreventionLock & )
{
  
  if (defaultErrorCodeToReport == SEC_ERROR_REVOKED_CERTIFICATE) {
    PR_SetError(SEC_ERROR_REVOKED_CERTIFICATE, 0);
    return SECFailure;
  }

  if (defaultErrorCodeToReport == 0) {
    NS_ERROR("No error code set during certificate validation failure.");
    PR_SetError(PR_INVALID_STATE_ERROR, 0);
    return SECFailure;
  }

  nsRefPtr<nsNSSCertificate> nssCert;
  nssCert = nsNSSCertificate::Create(&cert);
  if (!nssCert) {
    NS_ERROR("nsNSSCertificate::Create failed in DispatchCertErrorRunnable");
    PR_SetError(SEC_ERROR_NO_MEMORY, 0);
    return SECFailure;
  }

  SECStatus srv;
  nsresult nsrv;

  nsCOMPtr<nsINSSComponent> inss = do_GetService(kNSSComponentCID, &nsrv);
  if (!inss) {
    NS_ERROR("do_GetService(kNSSComponentCID) failed in DispatchCertErrorRunnable");
    PR_SetError(defaultErrorCodeToReport, 0);
    return SECFailure;
  }

  nsRefPtr<nsCERTValInParamWrapper> survivingParams;
  nsrv = inss->GetDefaultCERTValInParam(survivingParams);
  if (NS_FAILED(nsrv)) {
    NS_ERROR("GetDefaultCERTValInParam failed in DispatchCertErrorRunnable");
    PR_SetError(defaultErrorCodeToReport, 0);
    return SECFailure;
  }
  
  PRArenaPool *log_arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
  PRArenaPoolCleanerFalseParam log_arena_cleaner(log_arena);
  if (!log_arena) {
    NS_ERROR("PORT_NewArena failed in DispatchCertErrorRunnable");
    return SECFailure; 
  }

  CERTVerifyLog *verify_log = PORT_ArenaZNew(log_arena, CERTVerifyLog);
  if (!verify_log) {
    NS_ERROR("PORT_ArenaZNew failed in DispatchCertErrorRunnable");
    return SECFailure; 
  }
  CERTVerifyLogContentsCleaner verify_log_cleaner(verify_log);
  verify_log->arena = log_arena;

  if (!nsNSSComponent::globalConstFlagUsePKIXVerification) {
    srv = CERT_VerifyCertificate(CERT_GetDefaultCertDB(), &cert,
                                true, certificateUsageSSLServer,
                                PR_Now(), static_cast<void*>(socketInfo),
                                verify_log, NULL);
  }
  else {
    CERTValOutParam cvout[2];
    cvout[0].type = cert_po_errorLog;
    cvout[0].value.pointer.log = verify_log;
    cvout[1].type = cert_po_end;

    srv = CERT_PKIXVerifyCert(&cert, certificateUsageSSLServer,
                              survivingParams->GetRawPointerForNSS(),
                              cvout, static_cast<void*>(socketInfo));
  }

  
  
  
  
  

  PRErrorCode errorCodeMismatch = 0;
  PRErrorCode errorCodeTrust = 0;
  PRErrorCode errorCodeExpired = 0;

  PRUint32 collected_errors = 0;

  if (socketInfo->IsCertIssuerBlacklisted()) {
    collected_errors |= nsICertOverrideService::ERROR_UNTRUSTED;
    errorCodeTrust = defaultErrorCodeToReport;
  }

  
  if (CERT_VerifyCertName(&cert, socketInfo->GetHostName()) != SECSuccess) {
    collected_errors |= nsICertOverrideService::ERROR_MISMATCH;
    errorCodeMismatch = SSL_ERROR_BAD_CERT_DOMAIN;
  }

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
        PR_SetError(i_node->error, 0);
        return SECFailure;
    }
  }

  if (!collected_errors)
  {
    
    
    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("[%p] !collected_errors: %d\n",
           fdForLogging, static_cast<int>(defaultErrorCodeToReport)));
    PR_SetError(defaultErrorCodeToReport, 0);
    return SECFailure;
  }

  socketInfo->SetStatusErrorBits(*nssCert, collected_errors);

  nsRefPtr<CertErrorRunnable> runnable =
    new CertErrorRunnable(fdForLogging, 
                          static_cast<nsIX509Cert*>(nssCert.get()),
                          socketInfo, defaultErrorCodeToReport, 
                          collected_errors, errorCodeTrust, 
                          errorCodeMismatch, errorCodeExpired);

  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG,
          ("[%p][%p] Before dispatching CertErrorRunnable\n",
          fdForLogging, runnable.get()));

  nsresult nrv;
  nsCOMPtr<nsIEventTarget> stsTarget
    = do_GetService(NS_SOCKETTRANSPORTSERVICE_CONTRACTID, &nrv);
  if (NS_SUCCEEDED(nrv)) {
    nrv = stsTarget->Dispatch(runnable, NS_DISPATCH_NORMAL);
  }
  if (NS_FAILED(nrv)) {
    NS_ERROR("Failed to dispatch CertErrorRunnable");
    PR_SetError(defaultErrorCodeToReport, 0);
    return SECFailure;
  }

  return SECSuccess;
}

} } 

void
nsNSSSocketInfo::SetStatusErrorBits(nsIX509Cert & cert,
                                    PRUint32 collected_errors)
{
  MutexAutoLock lock(mMutex);

  if (!mSSLStatus)
    mSSLStatus = new nsSSLStatus();

  mSSLStatus->mServerCert = &cert;

  mSSLStatus->mHaveCertErrorBits = true;
  mSSLStatus->mIsDomainMismatch = 
    collected_errors & nsICertOverrideService::ERROR_MISMATCH;
  mSSLStatus->mIsNotValidAtThisTime = 
    collected_errors & nsICertOverrideService::ERROR_TIME;
  mSSLStatus->mIsUntrusted = 
    collected_errors & nsICertOverrideService::ERROR_UNTRUSTED;

  nsSSLIOLayerHelpers::mHostsWithCertErrors->RememberCertHasError(
    this, mSSLStatus, SECFailure);
}

SSLServerCertVerificationResult *
CertErrorRunnable::CheckCertOverrides()
{
  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("[%p][%p] top of CertErrorRunnable::Run\n",
                                    mFdForLogging, this));

  if (!NS_IsMainThread()) {
    NS_ERROR("CertErrorRunnable::CheckCertOverrides called off main thread");
    return new SSLServerCertVerificationResult(*mInfoObject,
                                               mDefaultErrorCodeToReport);
  }

  PRInt32 port;
  mInfoObject->GetPort(&port);

  nsCString hostWithPortString;
  hostWithPortString.AppendASCII(mInfoObject->GetHostName());
  hostWithPortString.AppendLiteral(":");
  hostWithPortString.AppendInt(port);

  PRUint32 remaining_display_errors = mCollectedErrors;

  nsresult nsrv;

  
  
  
  bool strictTransportSecurityEnabled = false;
  nsCOMPtr<nsIStrictTransportSecurityService> stss
    = do_GetService(NS_STSSERVICE_CONTRACTID, &nsrv);
  if (NS_SUCCEEDED(nsrv)) {
    nsrv = stss->IsStsHost(mInfoObject->GetHostName(),
                           &strictTransportSecurityEnabled);
  }
  if (NS_FAILED(nsrv)) {
    return new SSLServerCertVerificationResult(*mInfoObject,
                                               mDefaultErrorCodeToReport);
  }

  if (!strictTransportSecurityEnabled) {
    nsCOMPtr<nsICertOverrideService> overrideService =
      do_GetService(NS_CERTOVERRIDE_CONTRACTID);
    

    PRUint32 overrideBits = 0;

    if (overrideService)
    {
      bool haveOverride;
      bool isTemporaryOverride; 
      nsCString hostString(mInfoObject->GetHostName());
      nsrv = overrideService->HasMatchingOverride(hostString, port,
                                                  mCert,
                                                  &overrideBits,
                                                  &isTemporaryOverride, 
                                                  &haveOverride);
      if (NS_SUCCEEDED(nsrv) && haveOverride) 
      {
       
        remaining_display_errors -= overrideBits;
      }
    }

    if (!remaining_display_errors) {
      
      PR_LOG(gPIPNSSLog, PR_LOG_DEBUG,
             ("[%p][%p] All errors covered by override rules\n",
             mFdForLogging, this));
      return new SSLServerCertVerificationResult(*mInfoObject, 0);
    }
  } else {
    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG,
           ("[%p][%p] Strict-Transport-Security is violated: untrusted "
            "transport layer\n", mFdForLogging, this));
  }

  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG,
         ("[%p][%p] Certificate error was not overridden\n",
         mFdForLogging, this));

  
  

  
  nsCOMPtr<nsIInterfaceRequestor> cb;
  mInfoObject->GetNotificationCallbacks(getter_AddRefs(cb));
  if (cb) {
    nsCOMPtr<nsIBadCertListener2> bcl = do_GetInterface(cb);
    if (bcl) {
      nsIInterfaceRequestor *csi = static_cast<nsIInterfaceRequestor*>(mInfoObject);
      bool suppressMessage = false; 
      nsrv = bcl->NotifyCertProblem(csi, mInfoObject->SSLStatus(),
                                    hostWithPortString, &suppressMessage);
    }
  }

  nsCOMPtr<nsIRecentBadCertsService> recentBadCertsService = 
    do_GetService(NS_RECENTBADCERTS_CONTRACTID);
 
  if (recentBadCertsService) {
    NS_ConvertUTF8toUTF16 hostWithPortStringUTF16(hostWithPortString);
    recentBadCertsService->AddBadCert(hostWithPortStringUTF16,
                                      mInfoObject->SSLStatus());
  }

  
  PRErrorCode errorCodeToReport = mErrorCodeTrust    ? mErrorCodeTrust
                                : mErrorCodeMismatch ? mErrorCodeMismatch
                                : mErrorCodeExpired  ? mErrorCodeExpired
                                : mDefaultErrorCodeToReport;

  return new SSLServerCertVerificationResult(*mInfoObject, errorCodeToReport,
                                             OverridableCertErrorMessage);
}

NS_IMETHODIMP
CertErrorRunnable::Run()
{
  
  
  
  
  
  if (!NS_IsMainThread()) {
    
    
    DispatchToMainThreadAndWait(); 

    
    if (!mResult) {
      
      NS_ERROR("Did not create a SSLServerCertVerificationResult");
      mResult = new SSLServerCertVerificationResult(*mInfoObject,
                                                    PR_INVALID_STATE_ERROR);
    }
    return mResult->Run(); 
  } else {
    
    
    return SyncRunnableBase::Run(); 
  }
}

void 
CertErrorRunnable::RunOnTargetThread()
{
  
  
  
  
  
  
  
  mResult = CheckCertOverrides(); 
}
 
static PRFileDesc*
nsSSLIOLayerImportFD(PRFileDesc *fd,
                     nsNSSSocketInfo *infoObject,
                     const char *host,
                     bool anonymousLoad)
{
  nsNSSShutDownPreventionLock locker;
  PRFileDesc* sslSock = SSL_ImportFD(nsnull, fd);
  if (!sslSock) {
    NS_ASSERTION(false, "NSS: Error importing socket");
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
  if (SECSuccess != SSL_AuthCertificateHook(sslSock, AuthCertificateHook,
                                            infoObject)) {
    NS_NOTREACHED("failed to configure AuthCertificateHook");
    goto loser;
  }

  if (SECSuccess != SSL_SetURL(sslSock, host)) {
    NS_NOTREACHED("SSL_SetURL failed");
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
nsSSLIOLayerSetOptions(PRFileDesc *fd, bool forSTARTTLS, 
                       const char *proxyHost, const char *host, PRInt32 port,
                       bool anonymousLoad, nsNSSSocketInfo *infoObject)
{
  nsNSSShutDownPreventionLock locker;
  if (forSTARTTLS || proxyHost) {
    if (SECSuccess != SSL_OptionSet(fd, SSL_SECURITY, false)) {
      return NS_ERROR_FAILURE;
    }
    infoObject->SetHasCleartextPhase(true);
  }

  
  
  nsCAutoString key;
  key = nsDependentCString(host) + NS_LITERAL_CSTRING(":") + nsPrintfCString("%d", port);

  if (nsSSLIOLayerHelpers::isKnownAsIntolerantSite(key)) {
    if (SECSuccess != SSL_OptionSet(fd, SSL_ENABLE_TLS, false))
      return NS_ERROR_FAILURE;

    infoObject->SetAllowTLSIntoleranceTimeout(false);
      
    
    
    
    
    
    
  }

  if (SECSuccess != SSL_OptionSet(fd, SSL_HANDSHAKE_AS_CLIENT, true)) {
    return NS_ERROR_FAILURE;
  }
  
  if (nsSSLIOLayerHelpers::isRenegoUnrestrictedSite(nsDependentCString(host))) {
    if (SECSuccess != SSL_OptionSet(fd, SSL_REQUIRE_SAFE_NEGOTIATION, false)) {
      return NS_ERROR_FAILURE;
    }
    if (SECSuccess != SSL_OptionSet(fd, SSL_ENABLE_RENEGOTIATION, SSL_RENEGOTIATE_UNRESTRICTED)) {
      return NS_ERROR_FAILURE;
    }
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
                        bool forSTARTTLS,
                        bool anonymousLoad)
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
    NS_ASSERTION(false, "NSS: Error importing socket");
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
    infoObject->SetHandshakePending(false);
  }

  return NS_OK;
 loser:
  NS_IF_RELEASE(infoObject);
  if (layer) {
    layer->dtor(layer);
  }
  return NS_ERROR_FAILURE;
}
