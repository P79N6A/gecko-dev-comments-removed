





#ifndef mozilla_dom_CryptoKey_h
#define mozilla_dom_CryptoKey_h

#include "nsCycleCollectionParticipant.h"
#include "nsWrapperCache.h"
#include "nsIGlobalObject.h"
#include "nsNSSShutDown.h"
#include "pk11pub.h"
#include "keyhi.h"
#include "ScopedNSSTypes.h"
#include "mozilla/dom/KeyAlgorithm.h"
#include "mozilla/dom/CryptoBuffer.h"
#include "js/StructuredClone.h"
#include "js/TypeDecls.h"

class nsIGlobalObject;

namespace mozilla {
namespace dom {






























struct JsonWebKey;

class CryptoKey MOZ_FINAL : public nsISupports,
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

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  
  void GetType(nsString& aRetVal) const;
  bool Extractable() const;
  KeyAlgorithm* Algorithm() const;
  void GetUsages(nsTArray<nsString>& aRetVal) const;

  
  

  KeyType GetKeyType() const;
  nsresult SetType(const nsString& aType);
  void SetType(KeyType aType);
  void SetExtractable(bool aExtractable);
  void SetAlgorithm(KeyAlgorithm* aAlgorithm);
  void ClearUsages();
  nsresult AddUsage(const nsString& aUsage);
  nsresult AddUsageIntersecting(const nsString& aUsage, uint32_t aUsageMask);
  void AddUsage(KeyUsage aUsage);
  bool HasUsage(KeyUsage aUsage);
  bool HasUsageOtherThan(uint32_t aUsages);

  void SetSymKey(const CryptoBuffer& aSymKey);
  void SetPrivateKey(SECKEYPrivateKey* aPrivateKey);
  void SetPublicKey(SECKEYPublicKey* aPublicKey);

  
  
  
  
  const CryptoBuffer& GetSymKey() const;
  SECKEYPrivateKey* GetPrivateKey() const;
  SECKEYPublicKey* GetPublicKey() const;

  
  virtual void virtualDestroyNSSReference();
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

  
  bool WriteStructuredClone(JSStructuredCloneWriter* aWriter) const;
  bool ReadStructuredClone(JSStructuredCloneReader* aReader);

private:
  ~CryptoKey();

  nsRefPtr<nsIGlobalObject> mGlobal;
  uint32_t mAttributes; 
  nsRefPtr<KeyAlgorithm> mAlgorithm;

  
  CryptoBuffer mSymKey;
  ScopedSECKEYPrivateKey mPrivateKey;
  ScopedSECKEYPublicKey mPublicKey;
};

} 
} 

#endif 
