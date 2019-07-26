





#ifndef mozilla_dom_MessagePortList_h
#define mozilla_dom_MessagePortList_h

#include "mozilla/Attributes.h"
#include "mozilla/ErrorResult.h"
#include "nsCycleCollectionParticipant.h"
#include "mozilla/dom/MessagePort.h"
#include "nsWrapperCache.h"
#include "nsAutoPtr.h"
#include "nsTArray.h"

namespace mozilla {
namespace dom {

class MessagePortList MOZ_FINAL : public nsISupports
                                , public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(MessagePortList)

public:
  MessagePortList(nsISupports* aOwner, nsTArray<nsRefPtr<MessagePortBase>>& aPorts)
    : mOwner(aOwner)
    , mPorts(aPorts)
  {
    SetIsDOMBinding();
  }

  nsISupports*
  GetParentObject() const
  {
    return mOwner;
  }

  virtual JSObject*
  WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  uint32_t
  Length() const
  {
    return mPorts.Length();
  }

  MessagePortBase*
  Item(uint32_t aIndex)
  {
    return mPorts.SafeElementAt(aIndex);
  }

  MessagePortBase*
  IndexedGetter(uint32_t aIndex, bool &aFound)
  {
    aFound = aIndex < mPorts.Length();
    if (!aFound) {
      return nullptr;
    }
    return mPorts[aIndex];
  }

public:
  nsCOMPtr<nsISupports> mOwner;
  nsTArray<nsRefPtr<MessagePortBase>> mPorts;
};

} 
} 

#endif 

