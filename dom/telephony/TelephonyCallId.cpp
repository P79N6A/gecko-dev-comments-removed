





#include "TelephonyCallId.h"

#include "nsITelephonyService.h"

namespace mozilla {
namespace dom {

TelephonyCallId::TelephonyCallId(nsPIDOMWindow* aWindow,
                                 const nsAString& aNumber,
                                 uint16_t aNumberPresentation,
                                 const nsAString& aName,
                                 uint16_t aNamePresentation)
: mWindow(aWindow), mNumber(aNumber), mNumberPresentation(aNumberPresentation),
  mName(aName), mNamePresentation(aNamePresentation)
{
  SetIsDOMBinding();
}

TelephonyCallId::~TelephonyCallId()
{
}

JSObject*
TelephonyCallId::WrapObject(JSContext* aCx)
{
  return TelephonyCallIdBinding::Wrap(aCx, this);
}

CallIdPresentation
TelephonyCallId::GetPresentationStr(uint16_t aPresentation) const
{
  switch (aPresentation) {
    case nsITelephonyService::CALL_PRESENTATION_ALLOWED:
      return CallIdPresentation::Allowed;
    case nsITelephonyService::CALL_PRESENTATION_RESTRICTED:
      return CallIdPresentation::Restricted;
    case nsITelephonyService::CALL_PRESENTATION_UNKNOWN:
      return CallIdPresentation::Unknown;
    case nsITelephonyService::CALL_PRESENTATION_PAYPHONE:
      return CallIdPresentation::Payphone;
    default:
      MOZ_ASSUME_UNREACHABLE("Bad presentation!");
  }
}

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(TelephonyCallId, mWindow)

NS_IMPL_CYCLE_COLLECTING_ADDREF(TelephonyCallId)
NS_IMPL_CYCLE_COLLECTING_RELEASE(TelephonyCallId)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(TelephonyCallId)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END



CallIdPresentation
TelephonyCallId::NumberPresentation() const
{
  return GetPresentationStr(mNumberPresentation);
}

CallIdPresentation
TelephonyCallId::NamePresentation() const
{
  return GetPresentationStr(mNamePresentation);
}

} 
} 
