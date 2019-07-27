





#include "mozilla/dom/Promise.h"
#include "TVTuner.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_CLASS(TVTuner)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(TVTuner,
                                                  DOMEventTargetHelper)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(TVTuner,
                                                DOMEventTargetHelper)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_ADDREF_INHERITED(TVTuner, DOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(TVTuner, DOMEventTargetHelper)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(TVTuner)
NS_INTERFACE_MAP_END_INHERITING(DOMEventTargetHelper)

TVTuner::TVTuner(nsPIDOMWindow* aWindow)
  : DOMEventTargetHelper(aWindow)
{
}

TVTuner::~TVTuner()
{
}

 JSObject*
TVTuner::WrapObject(JSContext* aCx)
{
  return TVTunerBinding::Wrap(aCx, this);
}

void
TVTuner::GetSupportedSourceTypes(nsTArray<TVSourceType>& aSourceTypes,
                                 ErrorResult& aRv) const
{
  
}

already_AddRefed<Promise>
TVTuner::GetSources(ErrorResult& aRv)
{
  nsCOMPtr<nsIGlobalObject> global = do_QueryInterface(GetOwner());
  MOZ_ASSERT(global);

  nsRefPtr<Promise> promise = Promise::Create(global, aRv);
  if (aRv.Failed()) {
    return nullptr;
  }

  

  return promise.forget();
}

already_AddRefed<Promise>
TVTuner::SetCurrentSource(const TVSourceType aSourceType, ErrorResult& aRv)
{
  nsCOMPtr<nsIGlobalObject> global = do_QueryInterface(GetOwner());
  MOZ_ASSERT(global);

  nsRefPtr<Promise> promise = Promise::Create(global, aRv);
  if (aRv.Failed()) {
    return nullptr;
  }

  

  return promise.forget();
}

void
TVTuner::GetId(nsAString& aId) const
{
  
}

already_AddRefed<TVSource>
TVTuner::GetCurrentSource() const
{
  
  return nullptr;
}

already_AddRefed<DOMMediaStream>
TVTuner::GetStream() const
{
  
  return nullptr;
}

} 
} 
