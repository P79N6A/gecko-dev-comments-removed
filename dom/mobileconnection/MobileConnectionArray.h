





#ifndef mozilla_dom_network_MobileConnectionArray_h__
#define mozilla_dom_network_MobileConnectionArray_h__

#include "mozilla/dom/MobileConnection.h"
#include "nsWrapperCache.h"

namespace mozilla {
namespace dom {

class MobileConnectionArray MOZ_FINAL : public nsISupports
                                      , public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(MobileConnectionArray)

  explicit MobileConnectionArray(nsPIDOMWindow* aWindow);

  nsPIDOMWindow*
  GetParentObject() const;

  
  virtual JSObject*
  WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  
  MobileConnection*
  Item(uint32_t aIndex);

  uint32_t
  Length();

  MobileConnection*
  IndexedGetter(uint32_t aIndex, bool& aFound);

private:
  ~MobileConnectionArray();

  bool mLengthInitialized;

  nsCOMPtr<nsPIDOMWindow> mWindow;
  nsTArray<nsRefPtr<MobileConnection>> mMobileConnections;
};

} 
} 

#endif 
