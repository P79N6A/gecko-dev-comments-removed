




#ifndef mozilla_dom_sms_Types_h
#define mozilla_dom_sms_Types_h

#include "IPCMessageUtils.h"

namespace mozilla {
namespace dom {
namespace sms {




enum DeliveryState {
  eDeliveryState_Sent = 0,
  eDeliveryState_Received,
  eDeliveryState_Unknown,
  
  eDeliveryState_EndGuard
};


enum ReadState {
  eReadState_Unknown = -1,
  eReadState_Unread,
  eReadState_Read,
  
  eReadState_EndGuard
};

} 
} 
} 

namespace IPC {




template <>
struct ParamTraits<mozilla::dom::sms::DeliveryState>
  : public EnumSerializer<mozilla::dom::sms::DeliveryState,
                          mozilla::dom::sms::eDeliveryState_Sent,
                          mozilla::dom::sms::eDeliveryState_EndGuard>
{};




template <>
struct ParamTraits<mozilla::dom::sms::ReadState>
  : public EnumSerializer<mozilla::dom::sms::ReadState,
                          mozilla::dom::sms::eReadState_Unknown,
                          mozilla::dom::sms::eReadState_EndGuard>
{};

} 

#endif 
