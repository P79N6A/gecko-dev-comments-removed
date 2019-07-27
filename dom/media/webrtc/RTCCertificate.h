





#ifndef mozilla_dom_RTCCertificate_h
#define mozilla_dom_RTCCertificate_h

#include "nsCycleCollectionParticipant.h"
#include "nsWrapperCache.h"
#include "nsIGlobalObject.h"
#include "nsNSSShutDown.h"
#include "prtime.h"
#include "sslt.h"
#include "ScopedNSSTypes.h"

#include "mozilla/ErrorResult.h"
#include "mozilla/UniquePtr.h"
#include "mozilla/RefPtr.h"
#include "mozilla/dom/Date.h"
#include "mozilla/dom/CryptoKey.h"
#include "mtransport/dtlsidentity.h"
#include "js/StructuredClone.h"
#include "js/TypeDecls.h"

namespace mozilla {
namespace dom {

class ObjectOrString;

class RTCCertificate final
    : public nsISupports,
      public nsWrapperCache,
      public nsNSSShutDownObject
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(RTCCertificate)

  
  static already_AddRefed<Promise> GenerateCertificate(
      const GlobalObject& global, const ObjectOrString& keygenAlgorithm,
      ErrorResult& aRv, JSCompartment* aCompartment = nullptr);

  explicit RTCCertificate(nsIGlobalObject* aGlobal);
  RTCCertificate(nsIGlobalObject* aGlobal, SECKEYPrivateKey* aPrivateKey,
                 CERTCertificate* aCertificate, SSLKEAType aAuthType,
                 PRTime aExpires);

  nsIGlobalObject* GetParentObject() const { return mGlobal; }
  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aGivenProto) override;

  
  
  int64_t Expires() const { return mExpires / PR_USEC_PER_MSEC; }

  
  RefPtr<DtlsIdentity> CreateDtlsIdentity() const;
  CERTCertificate* Certificate() const { return mCertificate; }

  
  virtual void virtualDestroyNSSReference() override;
  void destructorSafeDestroyNSSReference();

  
  bool WriteStructuredClone(JSStructuredCloneWriter* aWriter) const;
  bool ReadStructuredClone(JSStructuredCloneReader* aReader);

private:
  ~RTCCertificate();
  void operator=(const RTCCertificate&) = delete;
  RTCCertificate(const RTCCertificate&) = delete;

  bool ReadCertificate(JSStructuredCloneReader* aReader,
                       const nsNSSShutDownPreventionLock& );
  bool ReadPrivateKey(JSStructuredCloneReader* aReader,
                      const nsNSSShutDownPreventionLock& aLockProof);
  bool WriteCertificate(JSStructuredCloneWriter* aWriter,
                        const nsNSSShutDownPreventionLock& ) const;
  bool WritePrivateKey(JSStructuredCloneWriter* aWriter,
                       const nsNSSShutDownPreventionLock& aLockProof) const;

  nsRefPtr<nsIGlobalObject> mGlobal;
  ScopedSECKEYPrivateKey mPrivateKey;
  ScopedCERTCertificate mCertificate;
  SSLKEAType mAuthType;
  PRTime mExpires;
};

} 
} 

#endif 
