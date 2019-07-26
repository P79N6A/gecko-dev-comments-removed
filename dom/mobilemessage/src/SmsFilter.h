




#ifndef mozilla_dom_mobilemessage_SmsFilter_h
#define mozilla_dom_mobilemessage_SmsFilter_h

#include "mozilla/dom/mobilemessage/PSms.h"
#include "nsIDOMSmsFilter.h"
#include "Types.h"
#include "mozilla/Attributes.h"

namespace mozilla {
namespace dom {

class SmsFilter MOZ_FINAL : public nsIDOMMozSmsFilter
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMMOZSMSFILTER

  SmsFilter();
  SmsFilter(const mobilemessage::SmsFilterData& aData);

  const mobilemessage::SmsFilterData& GetData() const;

  static nsresult NewSmsFilter(nsISupports** aSmsFilter);

private:
  mobilemessage::SmsFilterData mData;
};

inline const mobilemessage::SmsFilterData&
SmsFilter::GetData() const {
  return mData;
}

} 
} 

#endif 
