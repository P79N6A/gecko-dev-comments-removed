




#ifndef mozilla_dom_mobilemessage_Types_h
#define mozilla_dom_mobilemessage_Types_h

#include "IPCMessageUtils.h"

namespace mozilla {
namespace dom {
namespace mobilemessage {




enum DeliveryState {
  eDeliveryState_Sent = 0,
  eDeliveryState_Received,
  eDeliveryState_Sending,
  eDeliveryState_Error,
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
struct ParamTraits<mozilla::dom::mobilemessage::DeliveryState>
  : public EnumSerializer<mozilla::dom::mobilemessage::DeliveryState,
                          mozilla::dom::mobilemessage::eDeliveryState_Sent,
                          mozilla::dom::mobilemessage::eDeliveryState_EndGuard>
{};




template <>
struct ParamTraits<mozilla::dom::mobilemessage::DeliveryStatus>
  : public EnumSerializer<mozilla::dom::mobilemessage::DeliveryStatus,
                          mozilla::dom::mobilemessage::eDeliveryStatus_NotApplicable,
                          mozilla::dom::mobilemessage::eDeliveryStatus_EndGuard>
{};




template <>
struct ParamTraits<mozilla::dom::mobilemessage::ReadState>
  : public EnumSerializer<mozilla::dom::mobilemessage::ReadState,
                          mozilla::dom::mobilemessage::eReadState_Unknown,
                          mozilla::dom::mobilemessage::eReadState_EndGuard>
{};




template <>
struct ParamTraits<mozilla::dom::mobilemessage::MessageClass>
  : public EnumSerializer<mozilla::dom::mobilemessage::MessageClass,
                          mozilla::dom::mobilemessage::eMessageClass_Normal,
                          mozilla::dom::mobilemessage::eMessageClass_EndGuard>
{};

} 

#endif 
