



#include "LocalCertService.h"

#include "mozilla/ModuleUtils.h"
#include "mozilla/RefPtr.h"
#include "cert.h"
#include "CryptoTask.h"
#include "nsIPK11Token.h"
#include "nsIPK11TokenDB.h"
#include "nsIX509Cert.h"
#include "nsIX509CertDB.h"
#include "nsIX509CertValidity.h"
#include "nsLiteralString.h"
#include "nsProxyRelease.h"
#include "nsServiceManagerUtils.h"
#include "nsString.h"
#include "pk11pub.h"
#include "ScopedNSSTypes.h"

namespace mozilla {

class LocalCertTask : public CryptoTask
{
protected:
  explicit LocalCertTask(const nsACString& aNickname)
    : mNickname(aNickname)
  {
  }

  nsresult RemoveExisting()
  {
    
    nsresult rv;

    for (;;) {
      ScopedCERTCertificate cert(
        PK11_FindCertFromNickname(mNickname.get(), nullptr));
      if (!cert) {
        return NS_OK; 
      }

      
      if (!cert->isRoot) {
        return NS_ERROR_UNEXPECTED; 
      }

      NS_NAMED_LITERAL_CSTRING(commonNamePrefix, "CN=");
      nsAutoCString subjectNameFromNickname(commonNamePrefix + mNickname);
      if (!subjectNameFromNickname.Equals(cert->subjectName)) {
        return NS_ERROR_UNEXPECTED; 
      }
      if (!subjectNameFromNickname.Equals(cert->issuerName)) {
        return NS_ERROR_UNEXPECTED; 
      }

      rv = MapSECStatus(PK11_DeleteTokenCertAndKey(cert, nullptr));
      if (NS_FAILED(rv)) {
        return rv; 
      }
    }
  }

  nsCString mNickname;
};

class LocalCertGetTask MOZ_FINAL : public LocalCertTask
{
public:
  LocalCertGetTask(const nsACString& aNickname,
                   nsILocalCertGetCallback* aCallback)
    : LocalCertTask(aNickname)
    , mCallback(new nsMainThreadPtrHolder<nsILocalCertGetCallback>(aCallback))
    , mCert(nullptr)
  {
  }

private:
  virtual nsresult CalculateResult() MOZ_OVERRIDE
  {
    
    nsresult rv = GetFromDB();
    
    if (NS_FAILED(rv)) {
      rv = Generate();
    }
    
    if (NS_FAILED(rv)) {
      return rv;
    }

    
    rv = Validate();
    if (NS_FAILED(rv)) {
      rv = Generate();
    }
    
    if (NS_FAILED(rv)) {
      return rv;
    }

    return NS_OK;
  }

  nsresult Generate()
  {
    nsresult rv;

    
    ScopedPK11SlotInfo slot(PK11_GetInternalKeySlot());
    if (!slot) {
      return mozilla::psm::GetXPCOMFromNSSError(PR_GetError());
    }

    
    rv = RemoveExisting();
    if (NS_FAILED(rv)) {
      return rv;
    }

    
    NS_NAMED_LITERAL_CSTRING(commonNamePrefix, "CN=");
    nsAutoCString subjectNameStr(commonNamePrefix + mNickname);
    ScopedCERTName subjectName(CERT_AsciiToName(subjectNameStr.get()));
    if (!subjectName) {
      return mozilla::psm::GetXPCOMFromNSSError(PR_GetError());
    }

    
    SECOidData* curveOidData = SECOID_FindOIDByTag(SEC_OID_SECG_EC_SECP256R1);
    if (!curveOidData) {
      return mozilla::psm::GetXPCOMFromNSSError(PR_GetError());
    }

    
    ScopedAutoSECItem keyParams(2 + curveOidData->oid.len);
    keyParams.data[0] = SEC_ASN1_OBJECT_ID;
    keyParams.data[1] = curveOidData->oid.len;
    memcpy(keyParams.data + 2, curveOidData->oid.data, curveOidData->oid.len);

    
    ScopedSECKEYPrivateKey privateKey;
    ScopedSECKEYPublicKey publicKey;
    SECKEYPublicKey* tempPublicKey;
    privateKey = PK11_GenerateKeyPair(slot, CKM_EC_KEY_PAIR_GEN, &keyParams,
                                      &tempPublicKey, true ,
                                      true , nullptr);
    if (!privateKey) {
      return mozilla::psm::GetXPCOMFromNSSError(PR_GetError());
    }
    publicKey = tempPublicKey;

    
    ScopedCERTSubjectPublicKeyInfo spki(
      SECKEY_CreateSubjectPublicKeyInfo(publicKey));
    if (!spki) {
      return mozilla::psm::GetXPCOMFromNSSError(PR_GetError());
    }
    ScopedCERTCertificateRequest certRequest(
      CERT_CreateCertificateRequest(subjectName, spki, nullptr));
    if (!certRequest) {
      return mozilla::psm::GetXPCOMFromNSSError(PR_GetError());
    }

    
    static const PRTime oneDay = PRTime(PR_USEC_PER_SEC)
                               * PRTime(60)  
                               * PRTime(60)  
                               * PRTime(24); 

    PRTime now = PR_Now();
    PRTime notBefore = now - oneDay;
    PRTime notAfter = now + (PRTime(365) * oneDay);
    ScopedCERTValidity validity(CERT_CreateValidity(notBefore, notAfter));
    if (!validity) {
      return mozilla::psm::GetXPCOMFromNSSError(PR_GetError());
    }

    
    unsigned long serial;
    
    rv = MapSECStatus(
           PK11_GenerateRandomOnSlot(slot,
                                     reinterpret_cast<unsigned char *>(&serial),
                                     sizeof(serial)));
    if (NS_FAILED(rv)) {
      return rv;
    }

    
    ScopedCERTCertificate cert(
      CERT_CreateCertificate(serial, subjectName, validity, certRequest));
    if (!cert) {
      return mozilla::psm::GetXPCOMFromNSSError(PR_GetError());
    }

    
    if (!cert->version.data) {
      return NS_ERROR_INVALID_POINTER;
    }
    *(cert->version.data) = SEC_CERTIFICATE_VERSION_3;
    cert->version.len = 1;

    
    PLArenaPool* arena = cert->arena;
    if (!arena) {
      return NS_ERROR_INVALID_POINTER;
    }
    rv = MapSECStatus(
           SECOID_SetAlgorithmID(arena, &cert->signature,
                                 SEC_OID_ANSIX962_ECDSA_SHA256_SIGNATURE, 0));
    if (NS_FAILED(rv)) {
      return rv;
    }

    
    ScopedSECItem certDER(
      SEC_ASN1EncodeItem(nullptr, nullptr, cert,
                         SEC_ASN1_GET(CERT_CertificateTemplate)));
    if (!certDER) {
      return mozilla::psm::GetXPCOMFromNSSError(PR_GetError());
    }
    rv = MapSECStatus(
           SEC_DerSignData(arena, &cert->derCert, certDER->data, certDER->len,
                           privateKey,
                           SEC_OID_ANSIX962_ECDSA_SHA256_SIGNATURE));
    if (NS_FAILED(rv)) {
      return rv;
    }

    
    ScopedCERTCertificate certFromDER(
      CERT_NewTempCertificate(CERT_GetDefaultCertDB(), &cert->derCert, nullptr,
                              true , true ));
    if (!certFromDER) {
      return mozilla::psm::GetXPCOMFromNSSError(PR_GetError());
    }

    
    rv = MapSECStatus(PK11_ImportCert(slot, certFromDER, CK_INVALID_HANDLE,
                                      mNickname.get(), false ));
    if (NS_FAILED(rv)) {
      return rv;
    }

    
    return GetFromDB();
  }

  nsresult GetFromDB()
  {
    nsCOMPtr<nsIX509CertDB> certDB = do_GetService(NS_X509CERTDB_CONTRACTID);
    if (!certDB) {
      return NS_ERROR_FAILURE;
    }

    nsCOMPtr<nsIX509Cert> certFromDB;
    nsresult rv;
    rv = certDB->FindCertByNickname(nullptr, NS_ConvertASCIItoUTF16(mNickname),
                                    getter_AddRefs(certFromDB));
    if (NS_FAILED(rv)) {
      return rv;
    }
    mCert = certFromDB;
    return NS_OK;
  }

  nsresult Validate()
  {
    
    bool selfSigned;
    nsresult rv = mCert->GetIsSelfSigned(&selfSigned);
    if (NS_FAILED(rv)) {
      return rv;
    }
    if (!selfSigned) {
      return NS_ERROR_FAILURE;
    }

    
    nsXPIDLString subjectName;
    nsXPIDLString issuerName;
    mCert->GetSubjectName(subjectName);
    mCert->GetIssuerName(issuerName);
    if (!subjectName.Equals(issuerName)) {
      return NS_ERROR_FAILURE;
    }
    NS_NAMED_LITERAL_STRING(commonNamePrefix, "CN=");
    nsAutoString subjectNameFromNickname(
      commonNamePrefix + NS_ConvertASCIItoUTF16(mNickname));
    if (!subjectName.Equals(subjectNameFromNickname)) {
      return NS_ERROR_FAILURE;
    }

    nsCOMPtr<nsIX509CertValidity> validity;
    mCert->GetValidity(getter_AddRefs(validity));

    PRTime notBefore, notAfter;
    validity->GetNotBefore(&notBefore);
    validity->GetNotAfter(&notAfter);

    
    static const PRTime oneDay = PRTime(PR_USEC_PER_SEC)
                               * PRTime(60)  
                               * PRTime(60)  
                               * PRTime(24); 
    PRTime now = PR_Now();
    if (notBefore > now ||
        notAfter < (now - oneDay)) {
      return NS_ERROR_FAILURE;
    }

    return NS_OK;
  }

  virtual void ReleaseNSSResources() {}

  virtual void CallCallback(nsresult rv)
  {
    (void) mCallback->HandleCert(mCert, rv);
  }

  nsMainThreadPtrHandle<nsILocalCertGetCallback> mCallback;
  nsCOMPtr<nsIX509Cert> mCert; 
};

class LocalCertRemoveTask MOZ_FINAL : public LocalCertTask
{
public:
  LocalCertRemoveTask(const nsACString& aNickname,
                      nsILocalCertCallback* aCallback)
    : LocalCertTask(aNickname)
    , mCallback(new nsMainThreadPtrHolder<nsILocalCertCallback>(aCallback))
  {
  }

private:
  virtual nsresult CalculateResult() MOZ_OVERRIDE
  {
    return RemoveExisting();
  }

  virtual void ReleaseNSSResources() {}

  virtual void CallCallback(nsresult rv)
  {
    (void) mCallback->HandleResult(rv);
  }

  nsMainThreadPtrHandle<nsILocalCertCallback> mCallback;
};

NS_IMPL_ISUPPORTS(LocalCertService, nsILocalCertService)

LocalCertService::LocalCertService()
{
}

LocalCertService::~LocalCertService()
{
}

nsresult
LocalCertService::LoginToKeySlot()
{
  nsresult rv;

  
  ScopedPK11SlotInfo slot(PK11_GetInternalKeySlot());
  if (!slot) {
    return mozilla::psm::GetXPCOMFromNSSError(PR_GetError());
  }

  
  if (PK11_NeedUserInit(slot)) {
    rv = MapSECStatus(PK11_InitPin(slot, "", ""));
    if (NS_FAILED(rv)) {
      return rv;
    }
  }

  
  if (PK11_NeedLogin(slot) && !PK11_IsLoggedIn(slot, nullptr)) {
    
    nsCOMPtr<nsIPK11TokenDB> tokenDB =
      do_GetService(NS_PK11TOKENDB_CONTRACTID);
    if (!tokenDB) {
      return NS_ERROR_FAILURE;
    }
    nsCOMPtr<nsIPK11Token> keyToken;
    tokenDB->GetInternalKeyToken(getter_AddRefs(keyToken));
    if (!keyToken) {
      return NS_ERROR_FAILURE;
    }
    
    return keyToken->Login(false );
  }

  return NS_OK;
}

NS_IMETHODIMP
LocalCertService::GetOrCreateCert(const nsACString& aNickname,
                                  nsILocalCertGetCallback* aCallback)
{
  if (NS_WARN_IF(aNickname.IsEmpty())) {
    return NS_ERROR_INVALID_ARG;
  }
  if (NS_WARN_IF(!aCallback)) {
    return NS_ERROR_INVALID_POINTER;
  }

  
  nsresult rv = LoginToKeySlot();
  if (NS_FAILED(rv)) {
    aCallback->HandleCert(nullptr, rv);
    return NS_OK;
  }

  RefPtr<LocalCertGetTask> task(new LocalCertGetTask(aNickname, aCallback));
  return task->Dispatch("LocalCertGet");
}

NS_IMETHODIMP
LocalCertService::RemoveCert(const nsACString& aNickname,
                             nsILocalCertCallback* aCallback)
{
  if (NS_WARN_IF(aNickname.IsEmpty())) {
    return NS_ERROR_INVALID_ARG;
  }
  if (NS_WARN_IF(!aCallback)) {
    return NS_ERROR_INVALID_POINTER;
  }

  
  nsresult rv = LoginToKeySlot();
  if (NS_FAILED(rv)) {
    aCallback->HandleResult(rv);
    return NS_OK;
  }

  RefPtr<LocalCertRemoveTask> task(
    new LocalCertRemoveTask(aNickname, aCallback));
  return task->Dispatch("LocalCertRm");
}

NS_IMETHODIMP
LocalCertService::GetLoginPromptRequired(bool* aRequired)
{
  nsresult rv;

  
  ScopedPK11SlotInfo slot(PK11_GetInternalKeySlot());
  if (!slot) {
    return mozilla::psm::GetXPCOMFromNSSError(PR_GetError());
  }

  
  if (PK11_NeedUserInit(slot)) {
    rv = MapSECStatus(PK11_InitPin(slot, "", ""));
    if (NS_FAILED(rv)) {
      return rv;
    }
  }

  *aRequired = PK11_NeedLogin(slot) && !PK11_IsLoggedIn(slot, nullptr);
  return NS_OK;
}

#define LOCALCERTSERVICE_CID \
{ 0x47402be2, 0xe653, 0x45d0, \
  { 0x8d, 0xaa, 0x9f, 0x0d, 0xce, 0x0a, 0xc1, 0x48 } }

NS_GENERIC_FACTORY_CONSTRUCTOR(LocalCertService)

NS_DEFINE_NAMED_CID(LOCALCERTSERVICE_CID);

static const Module::CIDEntry kLocalCertServiceCIDs[] = {
  { &kLOCALCERTSERVICE_CID, false, nullptr, LocalCertServiceConstructor },
  { nullptr }
};

static const Module::ContractIDEntry kLocalCertServiceContracts[] = {
  { LOCALCERTSERVICE_CONTRACTID, &kLOCALCERTSERVICE_CID },
  { nullptr }
};

static const Module kLocalCertServiceModule = {
  Module::kVersion,
  kLocalCertServiceCIDs,
  kLocalCertServiceContracts
};

NSMODULE_DEFN(LocalCertServiceModule) = &kLocalCertServiceModule;

} 
