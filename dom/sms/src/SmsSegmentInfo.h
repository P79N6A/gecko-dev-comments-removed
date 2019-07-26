




#ifndef mozilla_dom_sms_SmsSegmentInfo_h
#define mozilla_dom_sms_SmsSegmentInfo_h

#include "nsIDOMSmsSegmentInfo.h"
#include "mozilla/Attributes.h"
#include "mozilla/dom/sms/SmsTypes.h"

namespace mozilla {
namespace dom {
namespace sms {

class SmsSegmentInfo MOZ_FINAL : public nsIDOMMozSmsSegmentInfo
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMMOZSMSSEGMENTINFO

  SmsSegmentInfo(int32_t aSegments,
                 int32_t aCharsPerSegment,
                 int32_t aCharsAvailableInLastSegment);
  SmsSegmentInfo(const SmsSegmentInfoData& aData);

private:
  SmsSegmentInfoData mData;
};

} 
} 
} 

#endif 
