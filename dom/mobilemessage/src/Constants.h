




#ifndef mozilla_dom_mobilemessage_Constants_h
#define mozilla_dom_mobilemessage_Constants_h

namespace mozilla {
namespace dom {
namespace mobilemessage {


extern const char* kSmsReceivedObserverTopic;
extern const char* kSmsRetrievingObserverTopic;
extern const char* kSmsSendingObserverTopic;
extern const char* kSmsSentObserverTopic;
extern const char* kSmsFailedObserverTopic;
extern const char* kSmsDeliverySuccessObserverTopic;
extern const char* kSmsDeliveryErrorObserverTopic;
extern const char* kSilentSmsReceivedObserverTopic;
extern const char* kSmsReadSuccessObserverTopic;
extern const char* kSmsReadErrorObserverTopic;

#define DELIVERY_RECEIVED       NS_LITERAL_STRING("received")
#define DELIVERY_SENDING        NS_LITERAL_STRING("sending")
#define DELIVERY_SENT           NS_LITERAL_STRING("sent")
#define DELIVERY_ERROR          NS_LITERAL_STRING("error")
#define DELIVERY_NOT_DOWNLOADED NS_LITERAL_STRING("not-downloaded")

#define DELIVERY_STATUS_NOT_APPLICABLE NS_LITERAL_STRING("not-applicable")
#define DELIVERY_STATUS_SUCCESS        NS_LITERAL_STRING("success")
#define DELIVERY_STATUS_PENDING        NS_LITERAL_STRING("pending")
#define DELIVERY_STATUS_ERROR          NS_LITERAL_STRING("error")
#define DELIVERY_STATUS_REJECTED       NS_LITERAL_STRING("rejected")
#define DELIVERY_STATUS_MANUAL         NS_LITERAL_STRING("manual")

#define READ_STATUS_NOT_APPLICABLE NS_LITERAL_STRING("not-applicable")
#define READ_STATUS_SUCCESS        NS_LITERAL_STRING("success")
#define READ_STATUS_PENDING        NS_LITERAL_STRING("pending")
#define READ_STATUS_ERROR          NS_LITERAL_STRING("error")

#define MESSAGE_CLASS_NORMAL  NS_LITERAL_STRING("normal")
#define MESSAGE_CLASS_CLASS_0 NS_LITERAL_STRING("class-0")
#define MESSAGE_CLASS_CLASS_1 NS_LITERAL_STRING("class-1")
#define MESSAGE_CLASS_CLASS_2 NS_LITERAL_STRING("class-2")
#define MESSAGE_CLASS_CLASS_3 NS_LITERAL_STRING("class-3")

#define MESSAGE_TYPE_SMS NS_LITERAL_STRING("sms")
#define MESSAGE_TYPE_MMS NS_LITERAL_STRING("mms")

} 
} 
} 

#endif 
