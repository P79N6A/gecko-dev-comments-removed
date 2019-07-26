




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


enum DeliveryStatus {
  eDeliveryStatus_NotApplicable = 0,
  eDeliveryStatus_Success,
  eDeliveryStatus_Pending,
  eDeliveryStatus_Error,
  
  eDeliveryStatus_EndGuard
};


enum ReadState {
  eReadState_Unknown = -1,
  eReadState_Unread,
  eReadState_Read,
  
  eReadState_EndGuard
};


enum MessageClass {
  eMessageClass_Normal = 0,
  eMessageClass_Class0,
  eMessageClass_Class1,
  eMessageClass_Class2,
  eMessageClass_Class3,
  
  eMessageClass_EndGuard
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
struct ParamTraits<mozilla::dom::sms::DeliveryStatus>
  : public EnumSerializer<mozilla::dom::sms::DeliveryStatus,
                          mozilla::dom::sms::eDeliveryStatus_NotApplicable,
                          mozilla::dom::sms::eDeliveryStatus_EndGuard>
{};




template <>
struct ParamTraits<mozilla::dom::sms::ReadState>
  : public EnumSerializer<mozilla::dom::sms::ReadState,
                          mozilla::dom::sms::eReadState_Unknown,
                          mozilla::dom::sms::eReadState_EndGuard>
{};




template <>
struct ParamTraits<mozilla::dom::sms::MessageClass>
  : public EnumSerializer<mozilla::dom::sms::MessageClass,
                          mozilla::dom::sms::eMessageClass_Normal,
                          mozilla::dom::sms::eMessageClass_EndGuard>
{};

} 

#endif 
