




#include "SmsSegmentInfo.h"
#include "nsIDOMClassInfo.h"

using namespace mozilla::dom::mobilemessage;

DOMCI_DATA(MozSmsSegmentInfo, mozilla::dom::SmsSegmentInfo)

namespace mozilla {
namespace dom {

NS_INTERFACE_MAP_BEGIN(SmsSegmentInfo)
  NS_INTERFACE_MAP_ENTRY(nsIDOMMozSmsSegmentInfo)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(MozSmsSegmentInfo)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(SmsSegmentInfo)
NS_IMPL_RELEASE(SmsSegmentInfo)

SmsSegmentInfo::SmsSegmentInfo(int32_t aSegments,
                               int32_t aCharsPerSegment,
                               int32_t aCharsAvailableInLastSegment)
  : mData(aSegments, aCharsPerSegment, aCharsAvailableInLastSegment)
{
}

SmsSegmentInfo::SmsSegmentInfo(const SmsSegmentInfoData& aData)
  : mData(aData)
{
}

NS_IMETHODIMP
SmsSegmentInfo::GetSegments(int32_t* aSegments)
{
  *aSegments = mData.segments();
  return NS_OK;
}

NS_IMETHODIMP
SmsSegmentInfo::GetCharsPerSegment(int32_t* aCharsPerSegment)
{
  *aCharsPerSegment = mData.charsPerSegment();
  return NS_OK;
}

NS_IMETHODIMP
SmsSegmentInfo::GetCharsAvailableInLastSegment(int32_t* aCharsAvailableInLastSegment)
{
  *aCharsAvailableInLastSegment = mData.charsAvailableInLastSegment();
  return NS_OK;
}

} 
} 
