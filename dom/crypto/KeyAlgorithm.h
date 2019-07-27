





#ifndef mozilla_dom_KeyAlgorithm_h
#define mozilla_dom_KeyAlgorithm_h

#include "nsCycleCollectionParticipant.h"
#include "nsIGlobalObject.h"
#include "nsWrapperCache.h"
#include "pk11pub.h"
#include "mozilla/dom/CryptoBuffer.h"
#include "js/StructuredClone.h"
#include "js/TypeDecls.h"

namespace mozilla {
namespace dom {

class CryptoKey;
class KeyAlgorithm;

enum KeyAlgorithmStructuredCloneTags {
  SCTAG_KEYALG,
  SCTAG_AESKEYALG,
  SCTAG_ECKEYALG,
  SCTAG_HMACKEYALG,
  SCTAG_RSAKEYALG,
  SCTAG_RSAHASHEDKEYALG
};

}

namespace dom {

class KeyAlgorithm : public nsISupports,
                     public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(KeyAlgorithm)

public:
  KeyAlgorithm(nsIGlobalObject* aGlobal, const nsString& aName);

  nsIGlobalObject* GetParentObject() const
  {
    return mGlobal;
  }

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  void GetName(nsString& aRetVal) const;

  virtual nsString ToJwkAlg() const;

  
  virtual bool WriteStructuredClone(JSStructuredCloneWriter* aWriter) const;
  static KeyAlgorithm* Create(nsIGlobalObject* aGlobal,
                              JSStructuredCloneReader* aReader);

  
  
  CK_MECHANISM_TYPE Mechanism() const {
    return mMechanism;
  }

protected:
  virtual ~KeyAlgorithm();

  nsRefPtr<nsIGlobalObject> mGlobal;
  nsString mName;
  CK_MECHANISM_TYPE mMechanism;
};

} 
} 

#endif 
