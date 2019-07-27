





#ifndef mozilla_dom_telephony_TelephonyIPCSerializer_h
#define mozilla_dom_telephony_TelephonyIPCSerializer_h

#include "ipc/IPCMessageUtils.h"
#include "mozilla/dom/telephony/TelephonyCallInfo.h"
#include "nsITelephonyCallInfo.h"

using mozilla::AutoJSContext;
using mozilla::dom::telephony::TelephonyCallInfo;

typedef nsITelephonyCallInfo* nsTelephonyCallInfo;

namespace IPC {




template <>
struct ParamTraits<nsITelephonyCallInfo*>
{
  typedef nsITelephonyCallInfo* paramType;

  static void Write(Message* aMsg, const paramType& aParam) {
    bool isNull = !aParam;
    WriteParam(aMsg, isNull);
    
    if (isNull) {
      return;
    }

    uint32_t clientId;
    uint32_t callIndex;
    uint16_t callState;
    nsString number;
    uint16_t numberPresentation;
    nsString name;
    uint16_t namePresentation;
    bool isOutgoing;
    bool isEmergency;
    bool isConference;
    bool isSwitchable;
    bool isMergeable;

    aParam->GetClientId(&clientId);
    aParam->GetCallIndex(&callIndex);
    aParam->GetCallState(&callState);
    aParam->GetNumber(number);
    aParam->GetNumberPresentation(&numberPresentation);
    aParam->GetName(name);
    aParam->GetNamePresentation(&namePresentation);
    aParam->GetIsOutgoing(&isOutgoing);
    aParam->GetIsEmergency(&isEmergency);
    aParam->GetIsConference(&isConference);
    aParam->GetIsSwitchable(&isSwitchable);
    aParam->GetIsMergeable(&isMergeable);

    WriteParam(aMsg, clientId);
    WriteParam(aMsg, callIndex);
    WriteParam(aMsg, callState);
    WriteParam(aMsg, number);
    WriteParam(aMsg, numberPresentation);
    WriteParam(aMsg, name);
    WriteParam(aMsg, namePresentation);
    WriteParam(aMsg, isOutgoing);
    WriteParam(aMsg, isEmergency);
    WriteParam(aMsg, isConference);
    WriteParam(aMsg, isSwitchable);
    WriteParam(aMsg, isMergeable);
  }

  static bool Read(const Message* aMsg, void** aIter, paramType* aResult)
  {
    
    bool isNull;
    if (!ReadParam(aMsg, aIter, &isNull)) {
      return false;
    }

    if (isNull) {
      *aResult = nullptr;
      return true;
    }

    uint32_t clientId;
    uint32_t callIndex;
    uint16_t callState;
    nsString disconnectedReason;

    nsString number;
    uint16_t numberPresentation;
    nsString name;
    uint16_t namePresentation;

    bool isOutgoing;
    bool isEmergency;
    bool isConference;
    bool isSwitchable;
    bool isMergeable;

    
    if (!(ReadParam(aMsg, aIter, &clientId) &&
          ReadParam(aMsg, aIter, &callIndex) &&
          ReadParam(aMsg, aIter, &callState) &&
          ReadParam(aMsg, aIter, &disconnectedReason) &&

          ReadParam(aMsg, aIter, &number) &&
          ReadParam(aMsg, aIter, &numberPresentation) &&
          ReadParam(aMsg, aIter, &name) &&
          ReadParam(aMsg, aIter, &namePresentation) &&

          ReadParam(aMsg, aIter, &isOutgoing) &&
          ReadParam(aMsg, aIter, &isEmergency) &&
          ReadParam(aMsg, aIter, &isConference) &&
          ReadParam(aMsg, aIter, &isSwitchable) &&
          ReadParam(aMsg, aIter, &isMergeable))) {
      return false;
    }

    nsCOMPtr<nsITelephonyCallInfo> info =
        new TelephonyCallInfo(clientId,
                              callIndex,
                              callState,
                              disconnectedReason,

                              number,
                              numberPresentation,
                              name,
                              namePresentation,

                              isOutgoing,
                              isEmergency,
                              isConference,
                              isSwitchable,
                              isMergeable);

    info.forget(aResult);

    return true;
  }
};


} 

#endif 
