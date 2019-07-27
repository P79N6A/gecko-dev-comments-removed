





#include "mozilla/dom/Promise.h"
#include "TVChannel.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_CLASS(TVChannel)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(TVChannel,
                                                  DOMEventTargetHelper)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(TVChannel,
                                                DOMEventTargetHelper)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_ADDREF_INHERITED(TVChannel, DOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(TVChannel, DOMEventTargetHelper)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(TVChannel)
NS_INTERFACE_MAP_END_INHERITING(DOMEventTargetHelper)

TVChannel::TVChannel(nsPIDOMWindow* aWindow)
  : DOMEventTargetHelper(aWindow)
{
}

TVChannel::~TVChannel()
{
}

 JSObject*
TVChannel::WrapObject(JSContext* aCx)
{
  return TVChannelBinding::Wrap(aCx, this);
}

already_AddRefed<Promise>
TVChannel::GetPrograms(const TVGetProgramsOptions& aOptions, ErrorResult& aRv)
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
TVChannel::GetNetworkId(nsAString& aNetworkId) const
{
  
}

void
TVChannel::GetTransportStreamId(nsAString& aTransportStreamId) const
{
  
}

void
TVChannel::GetServiceId(nsAString& aServiceId) const
{
  
}

already_AddRefed<TVSource>
TVChannel::Source() const
{
  
  return nullptr;
}

TVChannelType
TVChannel::Type() const
{
  
  return TVChannelType::Tv;
}

void
TVChannel::GetName(nsAString& aName) const
{
  
}

void
TVChannel::GetNumber(nsAString& aNumber) const
{
  
}

bool
TVChannel::IsEmergency() const
{
  
  return false;
}

bool
TVChannel::IsFree() const
{
  
  return false;
}

already_AddRefed<Promise>
TVChannel::GetCurrentProgram(ErrorResult& aRv)
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
