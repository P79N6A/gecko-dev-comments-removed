




































#ifndef mozilla_dom_sms_Types_h
#define mozilla_dom_sms_Types_h

namespace mozilla {
namespace dom {
namespace sms {


enum DeliveryState {
  eDeliveryState_Sent,
  eDeliveryState_Received,
  
  eDeliveryState_Unknown
};

} 
} 
} 

namespace IPC {




template <>
struct ParamTraits<mozilla::dom::sms::DeliveryState>
  : public EnumSerializer<mozilla::dom::sms::DeliveryState,
                          mozilla::dom::sms::eDeliveryState_Sent,
                          mozilla::dom::sms::eDeliveryState_Unknown>
{};

} 

#endif 
