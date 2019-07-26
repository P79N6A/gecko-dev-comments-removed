





#include "TransportSecurityInfo.h"
#include "nsNSSComponent.h"
#include "nsIWebProgressListener.h"
#include "nsNSSCertificate.h"
#include "nsIX509CertValidity.h"
#include "nsIDateTimeFormat.h"
#include "nsDateTimeFormatCID.h"
#include "nsICertOverrideService.h"
#include "nsIObjectInputStream.h"
#include "nsIObjectOutputStream.h"
#include "nsNSSCertHelper.h"
#include "nsNSSCleaner.h"
#include "nsIProgrammingLanguage.h"
#include "nsIArray.h"
#include "PSMRunnable.h"

#include "secerr.h"


                            
                            

                       
                       
                       
                       
                       

namespace {

NSSCleanupAutoPtrClass(CERTCertificate, CERT_DestroyCertificate)

static NS_DEFINE_CID(kNSSComponentCID, NS_NSSCOMPONENT_CID);

} 

namespace mozilla { namespace psm {

TransportSecurityInfo::TransportSecurityInfo()
  : mMutex("TransportSecurityInfo::mMutex"),
    mSecurityState(nsIWebProgressListener::STATE_IS_INSECURE),
    mSubRequestsHighSecurity(0),
    mSubRequestsLowSecurity(0),
    mSubRequestsBrokenSecurity(0),
    mSubRequestsNoSecurity(0),
    mErrorCode(0),
    mErrorMessageType(PlainErrorMessage),
    mPort(0),
    mIsCertIssuerBlacklisted(false)
{
}

TransportSecurityInfo::~TransportSecurityInfo()
{
  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown())
    return;

  shutdown(calledFromObject);
}

void
TransportSecurityInfo::virtualDestroyNSSReference()
{
}

NS_IMPL_THREADSAFE_ISUPPORTS6(TransportSecurityInfo,
                              nsITransportSecurityInfo,
                              nsIInterfaceRequestor,
                              nsISSLStatusProvider,
                              nsIAssociatedContentSecurity,
                              nsISerializable,
                              nsIClassInfo)

nsresult
TransportSecurityInfo::SetHostName(const char* host)
{
  mHostName.Adopt(host ? NS_strdup(host) : 0);
  return NS_OK;
}

nsresult
TransportSecurityInfo::GetHostName(char **host)
{
  *host = (mHostName) ? NS_strdup(mHostName) : nullptr;
  return NS_OK;
}

nsresult
TransportSecurityInfo::SetPort(int32_t aPort)
{
  mPort = aPort;
  return NS_OK;
}

nsresult
TransportSecurityInfo::GetPort(int32_t *aPort)
{
  *aPort = mPort;
  return NS_OK;
}

PRErrorCode
TransportSecurityInfo::GetErrorCode() const
{
  MutexAutoLock lock(mMutex);

  return mErrorCode;
}

void
TransportSecurityInfo::SetCanceled(PRErrorCode errorCode,
                                   SSLErrorMessageType errorMessageType)
{
  MutexAutoLock lock(mMutex);

  mErrorCode = errorCode;
  mErrorMessageType = errorMessageType;
  mErrorMessageCached.Truncate();
}

NS_IMETHODIMP
TransportSecurityInfo::GetSecurityState(uint32_t* state)
{
  *state = mSecurityState;
  return NS_OK;
}

nsresult
TransportSecurityInfo::SetSecurityState(uint32_t aState)
{
  mSecurityState = aState;
  return NS_OK;
}


NS_IMETHODIMP
TransportSecurityInfo::GetCountSubRequestsHighSecurity(
  int32_t *aSubRequestsHighSecurity)
{
  *aSubRequestsHighSecurity = mSubRequestsHighSecurity;
  return NS_OK;
}

NS_IMETHODIMP
TransportSecurityInfo::SetCountSubRequestsHighSecurity(
  int32_t aSubRequestsHighSecurity)
{
  mSubRequestsHighSecurity = aSubRequestsHighSecurity;
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
TransportSecurityInfo::GetCountSubRequestsLowSecurity(
  int32_t *aSubRequestsLowSecurity)
{
  *aSubRequestsLowSecurity = mSubRequestsLowSecurity;
  return NS_OK;
}

NS_IMETHODIMP
TransportSecurityInfo::SetCountSubRequestsLowSecurity(
  int32_t aSubRequestsLowSecurity)
{
  mSubRequestsLowSecurity = aSubRequestsLowSecurity;
  return NS_OK;
}


NS_IMETHODIMP
TransportSecurityInfo::GetCountSubRequestsBrokenSecurity(
  int32_t *aSubRequestsBrokenSecurity)
{
  *aSubRequestsBrokenSecurity = mSubRequestsBrokenSecurity;
  return NS_OK;
}

NS_IMETHODIMP
TransportSecurityInfo::SetCountSubRequestsBrokenSecurity(
  int32_t aSubRequestsBrokenSecurity)
{
  mSubRequestsBrokenSecurity = aSubRequestsBrokenSecurity;
  return NS_OK;
}


NS_IMETHODIMP
TransportSecurityInfo::GetCountSubRequestsNoSecurity(
  int32_t *aSubRequestsNoSecurity)
{
  *aSubRequestsNoSecurity = mSubRequestsNoSecurity;
  return NS_OK;
}

NS_IMETHODIMP
TransportSecurityInfo::SetCountSubRequestsNoSecurity(
  int32_t aSubRequestsNoSecurity)
{
  mSubRequestsNoSecurity = aSubRequestsNoSecurity;
  return NS_OK;
}

NS_IMETHODIMP
TransportSecurityInfo::Flush()
{
  return NS_OK;
}

NS_IMETHODIMP
TransportSecurityInfo::GetShortSecurityDescription(PRUnichar** aText)
{
  if (mShortDesc.IsEmpty())
    *aText = nullptr;
  else {
    *aText = ToNewUnicode(mShortDesc);
    NS_ENSURE_TRUE(*aText, NS_ERROR_OUT_OF_MEMORY);
  }
  return NS_OK;
}

nsresult
TransportSecurityInfo::SetShortSecurityDescription(const PRUnichar* aText)
{
  mShortDesc.Assign(aText);
  return NS_OK;
}

NS_IMETHODIMP
TransportSecurityInfo::GetErrorMessage(PRUnichar** aText)
{
  NS_ENSURE_ARG_POINTER(aText);
  *aText = nullptr;

  if (!NS_IsMainThread()) {
    NS_ERROR("nsNSSSocketInfo::GetErrorMessage called off the main thread");
    return NS_ERROR_NOT_SAME_THREAD;
  }

  MutexAutoLock lock(mMutex);

  if (mErrorMessageCached.IsEmpty()) {
    nsresult rv = formatErrorMessage(lock, 
                                     mErrorCode, mErrorMessageType,
                                     true, true, mErrorMessageCached);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  *aText = ToNewUnicode(mErrorMessageCached);
  return *aText != nullptr ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

void
TransportSecurityInfo::GetErrorLogMessage(PRErrorCode errorCode,
                                          SSLErrorMessageType errorMessageType,
                                          nsString &result)
{
  if (!NS_IsMainThread()) {
    NS_ERROR("nsNSSSocketInfo::GetErrorLogMessage called off the main thread");
    return;
  }

  MutexAutoLock lock(mMutex);
  (void) formatErrorMessage(lock, errorCode, errorMessageType,
                            false, false, result);
}

static nsresult
formatPlainErrorMessage(nsXPIDLCString const & host, int32_t port,
                        PRErrorCode err, 
                        bool suppressPort443,
                        nsString &returnedMessage);

static nsresult
formatOverridableCertErrorMessage(nsISSLStatus & sslStatus,
                                  PRErrorCode errorCodeToReport, 
                                  const nsXPIDLCString & host, int32_t port,
                                  bool suppressPort443,
                                  bool wantsHtml,
                                  nsString & returnedMessage);



nsresult
TransportSecurityInfo::formatErrorMessage(MutexAutoLock const & proofOfLock, 
                                          PRErrorCode errorCode,
                                          SSLErrorMessageType errorMessageType,
                                          bool wantsHtml, bool suppressPort443, 
                                          nsString &result)
{
  if (errorCode == 0) {
    result.Truncate();
    return NS_OK;
  }

  nsresult rv;
  NS_ConvertASCIItoUTF16 hostNameU(mHostName);
  NS_ASSERTION(errorMessageType != OverridableCertErrorMessage || 
                (mSSLStatus && mSSLStatus->mServerCert &&
                 mSSLStatus->mHaveCertErrorBits),
                "GetErrorLogMessage called for cert error without cert");
  if (errorMessageType == OverridableCertErrorMessage && 
      mSSLStatus && mSSLStatus->mServerCert) {
    rv = formatOverridableCertErrorMessage(*mSSLStatus, errorCode,
                                           mHostName, mPort,
                                           suppressPort443,
                                           wantsHtml,
                                           result);
  } else {
    rv = formatPlainErrorMessage(mHostName, mPort, 
                                 errorCode,
                                 suppressPort443,
                                 result);
  }

  if (NS_FAILED(rv)) {
    result.Truncate();
  }

  return rv;
}


NS_IMETHODIMP
TransportSecurityInfo::GetInterface(const nsIID & uuid, void * *result)
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

static NS_DEFINE_CID(kNSSCertificateCID, NS_X509CERT_CID);
#define TRANSPORTSECURITYINFOMAGIC { 0xa9863a23, 0x26b8, 0x4a9c, \
  { 0x83, 0xf1, 0xe9, 0xda, 0xdb, 0x36, 0xb8, 0x30 } }
static NS_DEFINE_CID(kTransportSecurityInfoMagic, TRANSPORTSECURITYINFOMAGIC);

NS_IMETHODIMP
TransportSecurityInfo::Write(nsIObjectOutputStream* stream)
{
  stream->WriteID(kTransportSecurityInfoMagic);

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

  
  
  
  
  
  
  uint32_t version = 3;
  stream->Write32(version | 0xFFFF0000);
  stream->Write32(mSecurityState);
  stream->WriteWStringZ(mShortDesc.get());

  
  nsresult rv = formatErrorMessage(lock, 
                                   mErrorCode, mErrorMessageType,
                                   true, true, mErrorMessageCached);
  NS_ENSURE_SUCCESS(rv, rv);
  stream->WriteWStringZ(mErrorMessageCached.get());

  stream->WriteCompoundObject(NS_ISUPPORTS_CAST(nsISSLStatus*, status),
                              NS_GET_IID(nsISupports), true);

  stream->Write32((uint32_t)mSubRequestsHighSecurity);
  stream->Write32((uint32_t)mSubRequestsLowSecurity);
  stream->Write32((uint32_t)mSubRequestsBrokenSecurity);
  stream->Write32((uint32_t)mSubRequestsNoSecurity);
  return NS_OK;
}

static bool CheckUUIDEquals(uint32_t m0,
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
TransportSecurityInfo::Read(nsIObjectInputStream* stream)
{
  nsresult rv;

  uint32_t version;
  bool certificatePresent;

  
  uint32_t UUID_0;
  stream->Read32(&UUID_0);
  if (UUID_0 == kTransportSecurityInfoMagic.m0) {
    
    if (!CheckUUIDEquals(UUID_0, stream, kTransportSecurityInfoMagic))
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
    stream->Read32((uint32_t*)&mSubRequestsHighSecurity);
    stream->Read32((uint32_t*)&mSubRequestsLowSecurity);
    stream->Read32((uint32_t*)&mSubRequestsBrokenSecurity);
    stream->Read32((uint32_t*)&mSubRequestsNoSecurity);
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
TransportSecurityInfo::GetInterfaces(uint32_t *count, nsIID * **array)
{
  *count = 0;
  *array = nullptr;
  return NS_OK;
}

NS_IMETHODIMP
TransportSecurityInfo::GetHelperForLanguage(uint32_t language,
                                            nsISupports **_retval)
{
  *_retval = nullptr;
  return NS_OK;
}

NS_IMETHODIMP
TransportSecurityInfo::GetContractID(char * *aContractID)
{
  *aContractID = nullptr;
  return NS_OK;
}

NS_IMETHODIMP
TransportSecurityInfo::GetClassDescription(char * *aClassDescription)
{
  *aClassDescription = nullptr;
  return NS_OK;
}

NS_IMETHODIMP
TransportSecurityInfo::GetClassID(nsCID * *aClassID)
{
  *aClassID = (nsCID*) nsMemory::Alloc(sizeof(nsCID));
  if (!*aClassID)
    return NS_ERROR_OUT_OF_MEMORY;
  return GetClassIDNoAlloc(*aClassID);
}

NS_IMETHODIMP
TransportSecurityInfo::GetImplementationLanguage(
  uint32_t *aImplementationLanguage)
{
  *aImplementationLanguage = nsIProgrammingLanguage::CPLUSPLUS;
  return NS_OK;
}

NS_IMETHODIMP
TransportSecurityInfo::GetFlags(uint32_t *aFlags)
{
  *aFlags = 0;
  return NS_OK;
}

static NS_DEFINE_CID(kNSSSocketInfoCID, TRANSPORTSECURITYINFO_CID);

NS_IMETHODIMP
TransportSecurityInfo::GetClassIDNoAlloc(nsCID *aClassIDNoAlloc)
{
  *aClassIDNoAlloc = kNSSSocketInfoCID;
  return NS_OK;
}

nsresult
TransportSecurityInfo::GetSSLStatus(nsISSLStatus** _result)
{
  NS_ENSURE_ARG_POINTER(_result);

  *_result = mSSLStatus;
  NS_IF_ADDREF(*_result);

  return NS_OK;
}

nsresult
TransportSecurityInfo::SetSSLStatus(nsSSLStatus *aSSLStatus)
{
  mSSLStatus = aSSLStatus;

  return NS_OK;
}






static nsresult
formatPlainErrorMessage(const nsXPIDLCString &host, int32_t port,
                        PRErrorCode err, 
                        bool suppressPort443,
                        nsString &returnedMessage)
{
  const PRUnichar *params[1];
  nsresult rv;

  nsCOMPtr<nsINSSComponent> component = do_GetService(kNSSComponentCID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  if (host.Length())
  {
    nsString hostWithPort;

    
    
    
    
    
    

    hostWithPort.AssignASCII(host);
    if (!suppressPort443 || port != 443) {
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
  const char *errorID = nullptr;
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
        uint32_t length = 0;
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
      case SEC_ERROR_CERT_SIGNATURE_ALGORITHM_DISABLED:
        errorID = "certErrorTrust_SignatureAlgorithmDisabled";
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
                   uint32_t &nameCount)
{
  allNames.Truncate();
  nameCount = 0;

  PRArenaPool *san_arena = nullptr;
  SECItem altNameExtension = {siBuffer, NULL, 0 };
  CERTGeneralName *sanNameList = nullptr;

  SECStatus rv = CERT_FindCertExtension(nssCert, SEC_OID_X509_SUBJECT_ALT_NAME,
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
                        bool wantsHtml,
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
  uint32_t nameCount = 0;
  bool useSAN = false;

  if (nssCert)
    useSAN = GetSubjectAltNames(nssCert, component, allNames, nameCount);

  if (!useSAN) {
    char *certName = nullptr;
    
    
    
    
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
  if (now > notAfter) {
    timeToUse = notAfter;
  } else {
    timeToUse = notBefore;
    trueExpired_falseNotYetValid = false;
  }

  nsCOMPtr<nsIDateTimeFormat> dateTimeFormat(do_CreateInstance(NS_DATETIMEFORMAT_CONTRACTID, &rv));
  if (NS_FAILED(rv))
    return;

  dateTimeFormat->FormatPRTime(nullptr, kDateFormatShort, 
                               kTimeFormatNoSeconds, timeToUse, 
                               formattedDate);
  dateTimeFormat->FormatPRTime(nullptr, kDateFormatShort,
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
                                  const nsXPIDLCString & host, int32_t port,
                                  bool suppressPort443,
                                  bool wantsHtml,
                                  nsString & returnedMessage)
{
  const PRUnichar *params[1];
  nsresult rv;
  nsAutoString hostWithPort;
  nsAutoString hostWithoutPort;

  
  
  
  
  
  
  
  hostWithoutPort.AppendASCII(host);
  if (suppressPort443 && port == 443) {
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
    AppendErrorTextMismatch(hostWithoutPort, ix509, component, wantsHtml, returnedMessage);
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



 RememberCertErrorsTable*
RememberCertErrorsTable::sInstance = nullptr;

RememberCertErrorsTable::RememberCertErrorsTable()
  : mMutex("RememberCertErrorsTable::mMutex")
{
  mErrorHosts.Init(16);
}

static nsresult
GetHostPortKey(TransportSecurityInfo* infoObject, nsAutoCString &result)
{
  nsresult rv;

  result.Truncate();

  nsXPIDLCString hostName;
  rv = infoObject->GetHostName(getter_Copies(hostName));
  NS_ENSURE_SUCCESS(rv, rv);

  int32_t port;
  rv = infoObject->GetPort(&port);
  NS_ENSURE_SUCCESS(rv, rv);

  result.Assign(hostName);
  result.Append(':');
  result.AppendInt(port);

  return NS_OK;
}

void
RememberCertErrorsTable::RememberCertHasError(TransportSecurityInfo* infoObject,
                                              nsSSLStatus* status,
                                              SECStatus certVerificationResult)
{
  nsresult rv;

  nsAutoCString hostPortKey;
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

    MutexAutoLock lock(mMutex);
    mErrorHosts.Put(hostPortKey, bits);
  }
  else {
    MutexAutoLock lock(mMutex);
    mErrorHosts.Remove(hostPortKey);
  }
}

void
RememberCertErrorsTable::LookupCertErrorBits(TransportSecurityInfo* infoObject,
                                             nsSSLStatus* status)
{
  
  
  if (status->mHaveCertErrorBits)
    
    return;

  nsresult rv;

  nsAutoCString hostPortKey;
  rv = GetHostPortKey(infoObject, hostPortKey);
  if (NS_FAILED(rv))
    return;

  CertStateBits bits;
  {
    MutexAutoLock lock(mMutex);
    if (!mErrorHosts.Get(hostPortKey, &bits))
      
      return;
  }

  
  status->mHaveCertErrorBits = true;
  status->mIsDomainMismatch = bits.mIsDomainMismatch;
  status->mIsNotValidAtThisTime = bits.mIsNotValidAtThisTime;
  status->mIsUntrusted = bits.mIsUntrusted;
}

void
TransportSecurityInfo::SetStatusErrorBits(nsIX509Cert & cert,
                                          uint32_t collected_errors)
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

  RememberCertErrorsTable::GetInstance().RememberCertHasError(this,
                                                              mSSLStatus,
                                                              SECFailure);
}

} } 
