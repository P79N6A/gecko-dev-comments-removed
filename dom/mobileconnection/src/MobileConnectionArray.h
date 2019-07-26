





#ifndef mozilla_dom_network_MobileConnectionArray_h__
#define mozilla_dom_network_MobileConnectionArray_h__

#include "nsWrapperCache.h"
#include "mozilla/dom/MobileConnection.h"

class nsIDOMMozMobileConnection;

namespace mozilla {
namespace dom {

class MobileConnectionArray MOZ_FINAL : public nsISupports,
                                        public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(MobileConnectionArray)

  MobileConnectionArray(nsPIDOMWindow* aWindow);

  nsPIDOMWindow*
  GetParentObject() const;

  
  virtual JSObject*
  WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  
  nsIDOMMozMobileConnection*
  Item(uint32_t aIndex);

  uint32_t
  Length() const;

  nsIDOMMozMobileConnection*
  IndexedGetter(uint32_t aIndex, bool& aFound);

private:
  ~MobileConnectionArray();

  void
  Init();

  void
  DropConnections();

  bool mInitialized;

  nsCOMPtr<nsPIDOMWindow> mWindow;
  nsTArray<nsRefPtr<MobileConnection>> mMobileConnections;
};

} 
} 

#endif 
