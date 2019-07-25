




































#ifndef mozilla_dom_sms_Constants_h
#define mozilla_dom_sms_Constants_h

namespace mozilla {
namespace dom {
namespace sms {

extern const char* kSmsReceivedObserverTopic;  
extern const char* kSmsSentObserverTopic;      
extern const char* kSmsDeliveredObserverTopic; 

#define DELIVERY_RECEIVED NS_LITERAL_STRING("received")
#define DELIVERY_SENT     NS_LITERAL_STRING("sent")

} 
} 
} 

#endif 
