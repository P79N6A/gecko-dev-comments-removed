




#ifndef mozilla_dom_sms_Constants_h
#define mozilla_dom_sms_Constants_h

namespace mozilla {
namespace dom {
namespace sms {

extern const char* kSmsReceivedObserverTopic;        
extern const char* kSmsSentObserverTopic;            
extern const char* kSmsDeliverySuccessObserverTopic; 
extern const char* kSmsDeliveryErrorObserverTopic;   

#define DELIVERY_RECEIVED NS_LITERAL_STRING("received")
#define DELIVERY_SENT     NS_LITERAL_STRING("sent")

#define DELIVERY_STATUS_NOT_APPLICABLE NS_LITERAL_STRING("not-applicable")
#define DELIVERY_STATUS_SUCCESS        NS_LITERAL_STRING("success")
#define DELIVERY_STATUS_PENDING        NS_LITERAL_STRING("pending")
#define DELIVERY_STATUS_ERROR          NS_LITERAL_STRING("error")

} 
} 
} 

#endif 
