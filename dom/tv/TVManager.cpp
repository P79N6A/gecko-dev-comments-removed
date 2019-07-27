





#include "mozilla/dom/Promise.h"
#include "mozilla/dom/TVManagerBinding.h"
#include "TVManager.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_CLASS(TVManager)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(TVManager,
                                                  DOMEventTargetHelper)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(TVManager,
                                                DOMEventTargetHelper)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_ADDREF_INHERITED(TVManager, DOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(TVManager, DOMEventTargetHelper)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(TVManager)
NS_INTERFACE_MAP_END_INHERITING(DOMEventTargetHelper)

TVManager::TVManager(nsPIDOMWindow* aWindow)
  : DOMEventTargetHelper(aWindow)
{
}

TVManager::~TVManager()
{
}

 JSObject*
TVManager::WrapObject(JSContext* aCx)
{
  return TVManagerBinding::Wrap(aCx, this);
}

already_AddRefed<Promise>
TVManager::GetTuners(ErrorResult& aRv)
{
  nsCOMPtr<nsIGlobalObject> global = do_QueryInterface(GetOwner());
  MOZ_ASSERT(global);

  nsRefPtr<Promise> promise = Promise::Create(global, aRv);
  if (aRv.Failed()) {
    return nullptr;
  }

  

  return promise.forget();
}

} 
} 
