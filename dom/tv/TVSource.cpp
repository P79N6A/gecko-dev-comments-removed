





#include "mozilla/dom/Promise.h"
#include "TVSource.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_CLASS(TVSource)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(TVSource,
                                                  DOMEventTargetHelper)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(TVSource,
                                                DOMEventTargetHelper)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_ADDREF_INHERITED(TVSource, DOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(TVSource, DOMEventTargetHelper)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(TVSource)
NS_INTERFACE_MAP_END_INHERITING(DOMEventTargetHelper)

TVSource::TVSource(nsPIDOMWindow* aWindow)
  : DOMEventTargetHelper(aWindow)
{
}

TVSource::~TVSource()
{
}

 JSObject*
TVSource::WrapObject(JSContext* aCx)
{
  return TVSourceBinding::Wrap(aCx, this);
}

already_AddRefed<Promise>
TVSource::GetChannels(ErrorResult& aRv)
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
TVSource::SetCurrentChannel(const nsAString& aChannelNumber, ErrorResult& aRv)
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
TVSource::StartScanning(const TVStartScanningOptions& aOptions,
                        ErrorResult& aRv)
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
TVSource::StopScanning(ErrorResult& aRv)
{
  nsCOMPtr<nsIGlobalObject> global = do_QueryInterface(GetOwner());
  MOZ_ASSERT(global);

  nsRefPtr<Promise> promise = Promise::Create(global, aRv);
  if (aRv.Failed()) {
    return nullptr;
  }

  

  return promise.forget();
}

already_AddRefed<TVTuner>
TVSource::Tuner() const
{
  
  return nullptr;
}

TVSourceType
TVSource::Type() const
{
  
  return TVSourceType::Dvb_t;
}

bool
TVSource::IsScanning() const
{
  
  return false;
}

already_AddRefed<TVChannel>
TVSource::GetCurrentChannel() const
{
  
  return nullptr;
}

} 
} 
