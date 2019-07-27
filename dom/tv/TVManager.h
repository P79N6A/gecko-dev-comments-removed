





#ifndef mozilla_dom_TVManager_h
#define mozilla_dom_TVManager_h

#include "mozilla/DOMEventTargetHelper.h"

class nsITVService;

namespace mozilla {
namespace dom {

class Promise;
class TVTuner;

class TVManager MOZ_FINAL : public DOMEventTargetHelper
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(TVManager, DOMEventTargetHelper)

  static already_AddRefed<TVManager> Create(nsPIDOMWindow* aWindow);

  

  virtual JSObject* WrapObject(JSContext *aCx) MOZ_OVERRIDE;

  nsresult SetTuners(const nsTArray<nsRefPtr<TVTuner>>& aTuners);

  void RejectPendingGetTunersPromises(nsresult aRv);

  nsresult DispatchTVEvent(nsIDOMEvent* aEvent);

  

  already_AddRefed<Promise> GetTuners(ErrorResult& aRv);

private:
  explicit TVManager(nsPIDOMWindow* aWindow);

  ~TVManager();

  bool Init();

  nsCOMPtr<nsITVService> mTVService;
  nsTArray<nsRefPtr<TVTuner>> mTuners;
  bool mIsReady;
  nsTArray<nsRefPtr<Promise>> mPendingGetTunersPromises;
};

} 
} 

#endif 
