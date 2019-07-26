




#ifndef mozilla_dom_mobilemessage_MobileMessageThread_h
#define mozilla_dom_mobilemessage_MobileMessageThread_h

#include "mozilla/Attributes.h"
#include "mozilla/dom/mobilemessage/SmsTypes.h"
#include "nsIDOMMozMobileMessageThread.h"
#include "nsString.h"
#include "jspubtd.h"

namespace mozilla {
namespace dom {

class MobileMessageThread MOZ_FINAL : public nsIDOMMozMobileMessageThread
{
private:
  typedef mobilemessage::ThreadData ThreadData;

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMMOZMOBILEMESSAGETHREAD

  MobileMessageThread(const uint64_t aId,
                      const nsTArray<nsString>& aParticipants,
                      const uint64_t aTimestamp,
                      const nsString& aBody,
                      const uint64_t aUnreadCount);

  MobileMessageThread(const ThreadData& aData);

  static nsresult Create(const uint64_t aId,
                         const JS::Value& aParticipants,
                         const JS::Value& aTimestamp,
                         const nsAString& aBody,
                         const uint64_t aUnreadCount,
                         JSContext* aCx,
                         nsIDOMMozMobileMessageThread** aThread);

  const ThreadData& GetData() const { return mData; }

private:
  
  MobileMessageThread() MOZ_DELETE;

  ThreadData mData;
};

} 
} 

#endif 
