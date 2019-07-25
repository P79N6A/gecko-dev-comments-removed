




































#ifndef mozilla_dom_sms_SmsFilter_h
#define mozilla_dom_sms_SmsFilter_h

#include "mozilla/dom/sms/PSms.h"
#include "nsIDOMSmsFilter.h"
#include "Types.h"

namespace mozilla {
namespace dom {
namespace sms {

class SmsFilter : public nsIDOMMozSmsFilter
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMMOZSMSFILTER

  SmsFilter();
  SmsFilter(const SmsFilterData& aData);

  const SmsFilterData& GetData() const;

  static nsresult NewSmsFilter(nsISupports** aSmsFilter);

private:
  SmsFilterData mData;
};

inline const SmsFilterData&
SmsFilter::GetData() const {
  return mData;
}

} 
} 
} 

#endif 
