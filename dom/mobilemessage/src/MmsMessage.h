




#ifndef mozilla_dom_mobilemessage_MmsMessage_h
#define mozilla_dom_mobilemessage_MmsMessage_h

#include "nsIDOMMozMmsMessage.h"
#include "nsString.h"
#include "jspubtd.h"
#include "mozilla/dom/mobilemessage/Types.h"
#include "mozilla/Attributes.h"
#include "DictionaryHelpers.h"

namespace mozilla {
namespace dom {

namespace mobilemessage {
class MmsMessageData;
} 

class ContentParent;

class MmsMessage MOZ_FINAL : public nsIDOMMozMmsMessage
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMMOZMMSMESSAGE

  MmsMessage(int32_t                                        aId,
             const uint64_t                                 aThreadId,
             mobilemessage::DeliveryState                   aDelivery,
             const nsTArray<mobilemessage::DeliveryStatus>& aDeliveryStatus,
             const nsAString&                               aSender,
             const nsTArray<nsString>&                      aReceivers,
             uint64_t                                       aTimestamp,
             bool                                           aRead,
             const nsAString&                               aSubject,
             const nsAString&                               aSmil,
             const nsTArray<idl::MmsAttachment>&            aAttachments);

  MmsMessage(const mobilemessage::MmsMessageData& aData);

  static nsresult Create(int32_t               aId,
                         const uint64_t        aThreadId,
                         const nsAString&      aDelivery,
                         const JS::Value&      aDeliveryStatus,
                         const nsAString&      aSender,
                         const JS::Value&      aReceivers,
                         const JS::Value&      aTimestamp,
                         bool                  aRead,
                         const nsAString&      aSubject,
                         const nsAString&      aSmil,
                         const JS::Value&      aAttachments,
                         JSContext*            aCx,
                         nsIDOMMozMmsMessage** aMessage);

  bool GetData(ContentParent* aParent,
               mobilemessage::MmsMessageData& aData);

private:

  int32_t                                 mId;
  uint64_t                                mThreadId;
  mobilemessage::DeliveryState            mDelivery;
  nsTArray<mobilemessage::DeliveryStatus> mDeliveryStatus;
  nsString                                mSender;
  nsTArray<nsString>                      mReceivers;
  uint64_t                                mTimestamp;
  bool                                    mRead;
  nsString                                mSubject;
  nsString                                mSmil;
  nsTArray<idl::MmsAttachment>            mAttachments;
};

} 
} 

#endif 
