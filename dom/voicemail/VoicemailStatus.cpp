





#include "mozilla/dom/VoicemailStatus.h"

#include "mozilla/dom/MozVoicemailStatusBinding.h"
#include "nsIVoicemailService.h"
#include "nsPIDOMWindow.h"

namespace mozilla {
namespace dom {


NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(VoicemailStatus, mParent)

NS_IMPL_CYCLE_COLLECTING_ADDREF(VoicemailStatus)
NS_IMPL_CYCLE_COLLECTING_RELEASE(VoicemailStatus)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(VoicemailStatus)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

VoicemailStatus::VoicemailStatus(nsISupports* aParent,
                                 nsIVoicemailProvider* aProvider)
  : mParent(aParent)
  , mProvider(aProvider)
{
  MOZ_ASSERT(mParent);
  MOZ_ASSERT(mProvider);
}

JSObject*
VoicemailStatus::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return MozVoicemailStatusBinding::Wrap(aCx, this, aGivenProto);
}

uint32_t
VoicemailStatus::ServiceId() const
{
  uint32_t result = 0;
  mProvider->GetServiceId(&result);
  return result;
}

bool
VoicemailStatus::HasMessages() const
{
  bool result = false;
  mProvider->GetHasMessages(&result);
  return result;
}

int32_t
VoicemailStatus::MessageCount() const
{
  int32_t result = 0;
  mProvider->GetMessageCount(&result);
  return result;
}

void
VoicemailStatus::GetReturnNumber(nsString& aReturnNumber) const
{
  aReturnNumber.SetIsVoid(true);
  mProvider->GetReturnNumber(aReturnNumber);
}

void
VoicemailStatus::GetReturnMessage(nsString& aReturnMessage) const
{
  aReturnMessage.SetIsVoid(true);
  mProvider->GetReturnMessage(aReturnMessage);
}

} 
} 
