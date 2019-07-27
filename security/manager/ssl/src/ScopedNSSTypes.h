





#ifndef mozilla_ScopedNSSTypes_h
#define mozilla_ScopedNSSTypes_h

#include <limits>

#include "NSSErrorsService.h"
#include "mozilla/Likely.h"
#ifndef MOZ_NO_MOZALLOC
#include "mozilla/mozalloc_oom.h"
#endif
#include "mozilla/Scoped.h"
#include "nsError.h"
#include "nsDebug.h"
#include "prio.h"
#include "cert.h"
#include "cms.h"
#include "keyhi.h"
#include "cryptohi.h"
#include "pk11pub.h"
#include "pkcs12.h"
#include "sechash.h"
#include "secpkcs7.h"
#include "secport.h"
#include "prerror.h"
#include "secmod.h"

namespace mozilla {





inline char *
char_ptr_cast(uint8_t * p) { return reinterpret_cast<char *>(p); }

inline const char *
char_ptr_cast(const uint8_t * p) { return reinterpret_cast<const char *>(p); }

inline uint8_t *
uint8_t_ptr_cast(char * p) { return reinterpret_cast<uint8_t*>(p); }

inline const uint8_t *
uint8_t_ptr_cast(const char * p) { return reinterpret_cast<const uint8_t*>(p); }










inline nsresult
MapSECStatus(SECStatus rv)
{
  if (rv == SECSuccess) {
    return NS_OK;
  }

  return mozilla::psm::GetXPCOMFromNSSError(PR_GetError());
}

#ifdef _MSC_VER


#define MOZ_NON_EXHAUSTIVE_SWITCH __pragma(warning(suppress:4061)) switch
#else
#define MOZ_NON_EXHAUSTIVE_SWITCH switch
#endif


MOZ_TYPE_SPECIFIC_SCOPED_POINTER_TEMPLATE(ScopedPRFileDesc,
                                          PRFileDesc,
                                          PR_Close)
MOZ_TYPE_SPECIFIC_SCOPED_POINTER_TEMPLATE(ScopedCERTCertificate,
                                          CERTCertificate,
                                          CERT_DestroyCertificate)
MOZ_TYPE_SPECIFIC_SCOPED_POINTER_TEMPLATE(ScopedCERTCertificateList,
                                          CERTCertificateList,
                                          CERT_DestroyCertificateList)
MOZ_TYPE_SPECIFIC_SCOPED_POINTER_TEMPLATE(ScopedCERTCertificateRequest,
                                          CERTCertificateRequest,
                                          CERT_DestroyCertificateRequest)
MOZ_TYPE_SPECIFIC_SCOPED_POINTER_TEMPLATE(ScopedCERTCertList,
                                          CERTCertList,
                                          CERT_DestroyCertList)
MOZ_TYPE_SPECIFIC_SCOPED_POINTER_TEMPLATE(ScopedCERTName,
                                          CERTName,
                                          CERT_DestroyName)
MOZ_TYPE_SPECIFIC_SCOPED_POINTER_TEMPLATE(ScopedCERTOidSequence,
                                          CERTOidSequence,
                                          CERT_DestroyOidSequence)
MOZ_TYPE_SPECIFIC_SCOPED_POINTER_TEMPLATE(ScopedCERTCertNicknames,
                                          CERTCertNicknames,
                                          CERT_FreeNicknames)
MOZ_TYPE_SPECIFIC_SCOPED_POINTER_TEMPLATE(ScopedCERTSubjectPublicKeyInfo,
                                          CERTSubjectPublicKeyInfo,
                                          SECKEY_DestroySubjectPublicKeyInfo)
MOZ_TYPE_SPECIFIC_SCOPED_POINTER_TEMPLATE(ScopedCERTValidity,
                                          CERTValidity,
                                          CERT_DestroyValidity)

MOZ_TYPE_SPECIFIC_SCOPED_POINTER_TEMPLATE(ScopedNSSCMSMessage,
                                          NSSCMSMessage,
                                          NSS_CMSMessage_Destroy)
MOZ_TYPE_SPECIFIC_SCOPED_POINTER_TEMPLATE(ScopedNSSCMSSignedData,
                                          NSSCMSSignedData,
                                          NSS_CMSSignedData_Destroy)

namespace psm {

inline void
PK11_DestroyContext_true(PK11Context * ctx) {
  PK11_DestroyContext(ctx, true);
}

inline void
SGN_DestroyContext_true(SGNContext* ctx) {
  SGN_DestroyContext(ctx, true);
}

inline void
VFY_DestroyContext_true(VFYContext * ctx) {
  VFY_DestroyContext(ctx, true);
}

} 

MOZ_TYPE_SPECIFIC_SCOPED_POINTER_TEMPLATE(ScopedPK11Context,
                                          PK11Context,
                                          mozilla::psm::PK11_DestroyContext_true)
MOZ_TYPE_SPECIFIC_SCOPED_POINTER_TEMPLATE(ScopedSGNContext,
                                          SGNContext,
                                          mozilla::psm::SGN_DestroyContext_true)
MOZ_TYPE_SPECIFIC_SCOPED_POINTER_TEMPLATE(ScopedVFYContext,
                                          VFYContext,
                                          mozilla::psm::VFY_DestroyContext_true)




























class Digest
{
public:
  Digest()
  {
    item.type = siBuffer;
    item.data = buf;
    item.len = 0;
  }

  nsresult DigestBuf(SECOidTag hashAlg, const uint8_t * buf, uint32_t len)
  {
    if (len > static_cast<uint32_t>(std::numeric_limits<int32_t>::max())) {
      return NS_ERROR_INVALID_ARG;
    }
    nsresult rv = SetLength(hashAlg);
    NS_ENSURE_SUCCESS(rv, rv);
    return MapSECStatus(PK11_HashBuf(hashAlg, item.data, buf,
                                     static_cast<int32_t>(len)));
  }

  nsresult End(SECOidTag hashAlg, ScopedPK11Context & context)
  {
    nsresult rv = SetLength(hashAlg);
    NS_ENSURE_SUCCESS(rv, rv);
    uint32_t len;
    rv = MapSECStatus(PK11_DigestFinal(context, item.data, &len, item.len));
    NS_ENSURE_SUCCESS(rv, rv);
    context = nullptr;
    NS_ENSURE_TRUE(len == item.len, NS_ERROR_UNEXPECTED);
    return NS_OK;
  }

  const SECItem & get() const { return item; }

private:
  nsresult SetLength(SECOidTag hashType)
  {
    MOZ_NON_EXHAUSTIVE_SWITCH (hashType)
    {
      case SEC_OID_SHA1:   item.len = SHA1_LENGTH;   break;
      case SEC_OID_SHA256: item.len = SHA256_LENGTH; break;
      case SEC_OID_SHA384: item.len = SHA384_LENGTH; break;
      case SEC_OID_SHA512: item.len = SHA512_LENGTH; break;
      default:
        return NS_ERROR_INVALID_ARG;
    }

    return NS_OK;
  }

  uint8_t buf[HASH_LENGTH_MAX];
  SECItem item;
};

MOZ_TYPE_SPECIFIC_SCOPED_POINTER_TEMPLATE(ScopedPK11SlotInfo,
                                          PK11SlotInfo,
                                          PK11_FreeSlot)
MOZ_TYPE_SPECIFIC_SCOPED_POINTER_TEMPLATE(ScopedPK11SlotList,
                                          PK11SlotList,
                                          PK11_FreeSlotList)
MOZ_TYPE_SPECIFIC_SCOPED_POINTER_TEMPLATE(ScopedPK11SymKey,
                                          PK11SymKey,
                                          PK11_FreeSymKey)
MOZ_TYPE_SPECIFIC_SCOPED_POINTER_TEMPLATE(ScopedPK11GenericObject,
                                          PK11GenericObject,
                                          PK11_DestroyGenericObject)

MOZ_TYPE_SPECIFIC_SCOPED_POINTER_TEMPLATE(ScopedSEC_PKCS7ContentInfo,
                                          SEC_PKCS7ContentInfo,
                                          SEC_PKCS7DestroyContentInfo)

MOZ_TYPE_SPECIFIC_SCOPED_POINTER_TEMPLATE(ScopedSEC_PKCS12DecoderContext,
                                          SEC_PKCS12DecoderContext,
                                          SEC_PKCS12DecoderFinish)
namespace internal {

inline void
PORT_FreeArena_false(PLArenaPool* arena)
{
  
  
  return PORT_FreeArena(arena, false);
}

} 

MOZ_TYPE_SPECIFIC_SCOPED_POINTER_TEMPLATE(ScopedPLArenaPool,
                                          PLArenaPool,
                                          internal::PORT_FreeArena_false)



inline void
SECITEM_AllocItem(SECItem & item, uint32_t len)
{
  if (MOZ_UNLIKELY(!SECITEM_AllocItem(nullptr, &item, len))) {
#ifndef MOZ_NO_MOZALLOC
    mozalloc_handle_oom(len);
    if (MOZ_UNLIKELY(!SECITEM_AllocItem(nullptr, &item, len)))
#endif
    {
      MOZ_CRASH();
    }
  }
}

class ScopedAutoSECItem final : public SECItem
{
public:
  explicit ScopedAutoSECItem(uint32_t initialAllocatedLen = 0)
  {
    data = nullptr;
    len = 0;
    if (initialAllocatedLen > 0) {
      SECITEM_AllocItem(*this, initialAllocatedLen);
    }
  }

  void reset()
  {
    SECITEM_FreeItem(this, false);
  }

  ~ScopedAutoSECItem()
  {
    reset();
  }
};

namespace internal {

inline void SECITEM_FreeItem_true(SECItem * s)
{
  return SECITEM_FreeItem(s, true);
}

inline void SECOID_DestroyAlgorithmID_true(SECAlgorithmID * a)
{
  return SECOID_DestroyAlgorithmID(a, true);
}

inline void SECKEYEncryptedPrivateKeyInfo_true(SECKEYEncryptedPrivateKeyInfo * epki)
{
  return SECKEY_DestroyEncryptedPrivateKeyInfo(epki, PR_TRUE);
}

} 

MOZ_TYPE_SPECIFIC_SCOPED_POINTER_TEMPLATE(ScopedSECItem,
                                          SECItem,
                                          internal::SECITEM_FreeItem_true)

MOZ_TYPE_SPECIFIC_SCOPED_POINTER_TEMPLATE(ScopedSECKEYPrivateKey,
                                          SECKEYPrivateKey,
                                          SECKEY_DestroyPrivateKey)
MOZ_TYPE_SPECIFIC_SCOPED_POINTER_TEMPLATE(ScopedSECKEYEncryptedPrivateKeyInfo,
                                          SECKEYEncryptedPrivateKeyInfo,
                                          internal::SECKEYEncryptedPrivateKeyInfo_true)
MOZ_TYPE_SPECIFIC_SCOPED_POINTER_TEMPLATE(ScopedSECKEYPublicKey,
                                          SECKEYPublicKey,
                                          SECKEY_DestroyPublicKey)
MOZ_TYPE_SPECIFIC_SCOPED_POINTER_TEMPLATE(ScopedSECAlgorithmID,
                                          SECAlgorithmID,
                                          internal::SECOID_DestroyAlgorithmID_true)
MOZ_TYPE_SPECIFIC_SCOPED_POINTER_TEMPLATE(ScopedSECMODModule, SECMODModule,
                                          SECMOD_DestroyModule)


} 

#endif 
