





#ifndef mozilla_dom_telephony_CallsList_h__
#define mozilla_dom_telephony_CallsList_h__

#include "mozilla/dom/telephony/TelephonyCommon.h"

#include "nsWrapperCache.h"

namespace mozilla {
namespace dom {

class CallsList MOZ_FINAL : public nsISupports,
                            public nsWrapperCache
{
  nsRefPtr<Telephony> mTelephony;
  nsRefPtr<TelephonyCallGroup> mGroup;

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(CallsList)

  explicit CallsList(Telephony* aTelephony, TelephonyCallGroup* aGroup = nullptr);

  nsPIDOMWindow*
  GetParentObject() const;

  
  virtual JSObject*
  WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  
  already_AddRefed<TelephonyCall>
  Item(uint32_t aIndex) const;

  uint32_t
  Length() const;

  already_AddRefed<TelephonyCall>
  IndexedGetter(uint32_t aIndex, bool& aFound) const;

private:
  ~CallsList();
};

} 
} 

#endif 
