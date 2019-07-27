



#ifndef mozilla_dom_IccManager_h
#define mozilla_dom_IccManager_h

#include "mozilla/DOMEventTargetHelper.h"
#include "nsCycleCollectionParticipant.h"
#include "nsTArrayHelpers.h"

namespace mozilla {
namespace dom {

class Icc;
class IccListener;

class IccManager final : public DOMEventTargetHelper
{
public:
  NS_DECL_ISUPPORTS_INHERITED

  NS_REALLY_FORWARD_NSIDOMEVENTTARGET(DOMEventTargetHelper)

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(IccManager, DOMEventTargetHelper)

  explicit IccManager(nsPIDOMWindow* aWindow);

  void
  Shutdown();

  nsresult
  NotifyIccAdd(const nsAString& aIccId);

  nsresult
  NotifyIccRemove(const nsAString& aIccId);

  IMPL_EVENT_HANDLER(iccdetected)
  IMPL_EVENT_HANDLER(iccundetected)

  void
  GetIccIds(nsTArray<nsString>& aIccIds);

  Icc*
  GetIccById(const nsAString& aIccId) const;

  nsPIDOMWindow*
  GetParentObject() const { return GetOwner(); }

  virtual JSObject*
  WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

private:
  ~IccManager();

private:
  nsTArray<nsRefPtr<IccListener>> mIccListeners;
};

} 
} 

#endif 
