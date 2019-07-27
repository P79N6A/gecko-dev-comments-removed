






#ifndef mozilla_dom_nfc_MozIsoDepTech_h__
#define mozilla_dom_nfc_MozIsoDepTech_h__

#include "mozilla/dom/MozNFCTagBinding.h"
#include "mozilla/dom/MozIsoDepTechBinding.h"
#include "nsCycleCollectionParticipant.h"
#include "nsWrapperCache.h"
#include "nsISupportsImpl.h"
#include "nsPIDOMWindow.h"

namespace mozilla {
namespace dom {

class Promise;

class MozIsoDepTech : public nsISupports,
                      public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(MozIsoDepTech)

  already_AddRefed<Promise> Transceive(const Uint8Array& aCommand,
                                       ErrorResult& aRv);

  nsPIDOMWindow* GetParentObject() const { return mWindow; }

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  static already_AddRefed<MozIsoDepTech>
  Constructor(const GlobalObject& aGlobal, MozNFCTag& aNFCTag,
              ErrorResult& aRv);

private:
  MozIsoDepTech(nsPIDOMWindow* aWindow, MozNFCTag& aNFCTag);
  virtual ~MozIsoDepTech();

  nsRefPtr<nsPIDOMWindow> mWindow;
  nsRefPtr<MozNFCTag> mTag;

  static const NFCTechType mTechnology;
};

} 
} 

#endif  
