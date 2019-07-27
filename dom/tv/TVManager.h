





#ifndef mozilla_dom_TVManager_h__
#define mozilla_dom_TVManager_h__

#include "mozilla/DOMEventTargetHelper.h"

namespace mozilla {
namespace dom {

class Promise;

class TVManager MOZ_FINAL : public DOMEventTargetHelper
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(TVManager, DOMEventTargetHelper)

  explicit TVManager(nsPIDOMWindow* aWindow);

  

  virtual JSObject* WrapObject(JSContext *aCx) MOZ_OVERRIDE;

  

  already_AddRefed<Promise> GetTuners(ErrorResult& aRv);

private:
  ~TVManager();

};

} 
} 

#endif 
