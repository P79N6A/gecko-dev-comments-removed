




#ifndef mozilla_dom_mobilemessage_SmsSegmentInfo_h
#define mozilla_dom_mobilemessage_SmsSegmentInfo_h

#include "nsIDOMSmsSegmentInfo.h"
#include "mozilla/Attributes.h"
#include "mozilla/dom/mobilemessage/SmsTypes.h"

namespace mozilla {
namespace dom {

class SmsSegmentInfo MOZ_FINAL : public nsIDOMMozSmsSegmentInfo
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMMOZSMSSEGMENTINFO

  SmsSegmentInfo(int32_t aSegments,
                 int32_t aCharsPerSegment,
                 int32_t aCharsAvailableInLastSegment);
  SmsSegmentInfo(const mobilemessage::SmsSegmentInfoData& aData);

private:
  mobilemessage::SmsSegmentInfoData mData;
};

} 
} 

#endif 
