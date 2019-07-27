




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
  eDeliveryState_NotDownloaded,
  
  eDeliveryState_EndGuard
};


enum DeliveryStatus {
  eDeliveryStatus_NotApplicable = 0,
  eDeliveryStatus_Success,
  eDeliveryStatus_Pending,
  eDeliveryStatus_Error,
  eDeliveryStatus_Reject,
  eDeliveryStatus_Manual,
  
  eDeliveryStatus_EndGuard
};


enum ReadStatus {
  eReadStatus_NotApplicable = 0,
  eReadStatus_Success,
  eReadStatus_Pending,
  eReadStatus_Error,
  
  eReadStatus_EndGuard
};


enum MessageClass {
  eMessageClass_Normal = 0,
  eMessageClass_Class0,
  eMessageClass_Class1,
  eMessageClass_Class2,
  eMessageClass_Class3,
  
  eMessageClass_EndGuard
};


enum MessageType {
  eMessageType_SMS = 0,
  eMessageType_MMS,
  
  eMessageType_EndGuard
};

} 
} 
} 

namespace IPC {




template <>
struct ParamTraits<mozilla::dom::mobilemessage::DeliveryState>
  : public ContiguousEnumSerializer<
             mozilla::dom::mobilemessage::DeliveryState,
             mozilla::dom::mobilemessage::eDeliveryState_Sent,
             mozilla::dom::mobilemessage::eDeliveryState_EndGuard>
{};




template <>
struct ParamTraits<mozilla::dom::mobilemessage::DeliveryStatus>
  : public ContiguousEnumSerializer<
             mozilla::dom::mobilemessage::DeliveryStatus,
             mozilla::dom::mobilemessage::eDeliveryStatus_NotApplicable,
             mozilla::dom::mobilemessage::eDeliveryStatus_EndGuard>
{};




template <>
struct ParamTraits<mozilla::dom::mobilemessage::ReadStatus>
  : public ContiguousEnumSerializer<
             mozilla::dom::mobilemessage::ReadStatus,
             mozilla::dom::mobilemessage::eReadStatus_NotApplicable,
             mozilla::dom::mobilemessage::eReadStatus_EndGuard>
{};




template <>
struct ParamTraits<mozilla::dom::mobilemessage::MessageClass>
  : public ContiguousEnumSerializer<
             mozilla::dom::mobilemessage::MessageClass,
             mozilla::dom::mobilemessage::eMessageClass_Normal,
             mozilla::dom::mobilemessage::eMessageClass_EndGuard>
{};




template <>
struct ParamTraits<mozilla::dom::mobilemessage::MessageType>
  : public ContiguousEnumSerializer<
             mozilla::dom::mobilemessage::MessageType,
             mozilla::dom::mobilemessage::eMessageType_SMS,
             mozilla::dom::mobilemessage::eMessageType_EndGuard>
{};

} 

#endif 
