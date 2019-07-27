





#ifndef mozilla_dom_CryptoKey_h
#define mozilla_dom_CryptoKey_h

#include "nsCycleCollectionParticipant.h"
#include "nsWrapperCache.h"
#include "nsIGlobalObject.h"
#include "nsNSSShutDown.h"
#include "pk11pub.h"
#include "keyhi.h"
#include "ScopedNSSTypes.h"
#include "mozilla/ErrorResult.h"
#include "mozilla/dom/CryptoBuffer.h"
#include "mozilla/dom/KeyAlgorithmProxy.h"
#include "js/StructuredClone.h"
#include "js/TypeDecls.h"

#define CRYPTOKEY_SC_VERSION 0x00000001

class nsIGlobalObject;

namespace mozilla {
namespace dom {






























struct JsonWebKey;

class CryptoKey final : public nsISupports,
                            public nsWrapperCache,
                            public nsNSSShutDownObject
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(CryptoKey)

  static const uint32_t CLEAR_EXTRACTABLE = 0xFFFFFFE;
  static const uint32_t EXTRACTABLE = 0x00000001;

  static const uint32_t CLEAR_TYPE = 0xFFFF00FF;
  static const uint32_t TYPE_MASK = 0x0000FF00;
  enum KeyType {
    UNKNOWN = 0x00000000,
    SECRET  = 0x00000100,
    PUBLIC  = 0x00000200,
    PRIVATE = 0x00000300
  };

  static const uint32_t CLEAR_USAGES = 0xFF00FFFF;
  static const uint32_t USAGES_MASK = 0x00FF0000;
  enum KeyUsage {
    ENCRYPT    = 0x00010000,
    DECRYPT    = 0x00020000,
    SIGN       = 0x00040000,
    VERIFY     = 0x00080000,
    DERIVEKEY  = 0x00100000,
    DERIVEBITS = 0x00200000,
    WRAPKEY    = 0x00400000,
    UNWRAPKEY  = 0x00800000
  };

  explicit CryptoKey(nsIGlobalObject* aWindow);

  nsIGlobalObject* GetParentObject() const
  {
    return mGlobal;
  }

  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  
  void GetType(nsString& aRetVal) const;
  bool Extractable() const;
  void GetAlgorithm(JSContext* cx, JS::MutableHandle<JSObject*> aRetVal,
                    ErrorResult& aRv) const;
  void GetUsages(nsTArray<nsString>& aRetVal) const;

  
  

  KeyAlgorithmProxy& Algorithm();
  const KeyAlgorithmProxy& Algorithm() const;
  KeyType GetKeyType() const;
  nsresult SetType(const nsString& aType);
  void SetType(KeyType aType);
  void SetExtractable(bool aExtractable);
  void ClearUsages();
  nsresult AddUsage(const nsString& aUsage);
  nsresult AddUsageIntersecting(const nsString& aUsage, uint32_t aUsageMask);
  void AddUsage(KeyUsage aUsage);
  bool HasAnyUsage();
  bool HasUsage(KeyUsage aUsage);
  bool HasUsageOtherThan(uint32_t aUsages);
  static bool IsRecognizedUsage(const nsString& aUsage);
  static bool AllUsagesRecognized(const Sequence<nsString>& aUsages);

  void SetSymKey(const CryptoBuffer& aSymKey);
  void SetPrivateKey(SECKEYPrivateKey* aPrivateKey);
  void SetPublicKey(SECKEYPublicKey* aPublicKey);

  
  
  
  
  const CryptoBuffer& GetSymKey() const;
  SECKEYPrivateKey* GetPrivateKey() const;
  SECKEYPublicKey* GetPublicKey() const;

  
  virtual void virtualDestroyNSSReference() override;
  void destructorSafeDestroyNSSReference();

  
  
  
  
  
  static SECKEYPrivateKey* PrivateKeyFromPkcs8(CryptoBuffer& aKeyData,
                                               const nsNSSShutDownPreventionLock& );
  static nsresult PrivateKeyToPkcs8(SECKEYPrivateKey* aPrivKey,
                                    CryptoBuffer& aRetVal,
                                    const nsNSSShutDownPreventionLock& );

  static SECKEYPublicKey* PublicKeyFromSpki(CryptoBuffer& aKeyData,
                                            const nsNSSShutDownPreventionLock& );
  static nsresult PublicKeyToSpki(SECKEYPublicKey* aPrivKey,
                                  CryptoBuffer& aRetVal,
                                  const nsNSSShutDownPreventionLock& );

  static SECKEYPrivateKey* PrivateKeyFromJwk(const JsonWebKey& aJwk,
                                             const nsNSSShutDownPreventionLock& );
  static nsresult PrivateKeyToJwk(SECKEYPrivateKey* aPrivKey,
                                  JsonWebKey& aRetVal,
                                  const nsNSSShutDownPreventionLock& );

  static SECKEYPublicKey* PublicKeyFromJwk(const JsonWebKey& aKeyData,
                                           const nsNSSShutDownPreventionLock& );
  static nsresult PublicKeyToJwk(SECKEYPublicKey* aPrivKey,
                                 JsonWebKey& aRetVal,
                                 const nsNSSShutDownPreventionLock& );

  static SECKEYPublicKey* PublicDhKeyFromRaw(CryptoBuffer& aKeyData,
                                             const CryptoBuffer& aPrime,
                                             const CryptoBuffer& aGenerator,
                                             const nsNSSShutDownPreventionLock& );
  static nsresult PublicDhKeyToRaw(SECKEYPublicKey* aPubKey,
                                   CryptoBuffer& aRetVal,
                                   const nsNSSShutDownPreventionLock& );

  static bool PublicKeyValid(SECKEYPublicKey* aPubKey);

  
  bool WriteStructuredClone(JSStructuredCloneWriter* aWriter) const;
  bool ReadStructuredClone(JSStructuredCloneReader* aReader);

private:
  ~CryptoKey();

  nsRefPtr<nsIGlobalObject> mGlobal;
  uint32_t mAttributes; 
  KeyAlgorithmProxy mAlgorithm;

  
  CryptoBuffer mSymKey;
  ScopedSECKEYPrivateKey mPrivateKey;
  ScopedSECKEYPublicKey mPublicKey;
};

} 
} 

#endif 
