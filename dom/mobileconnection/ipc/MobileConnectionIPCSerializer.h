





#ifndef mozilla_dom_mobileconnection_MobileConnectionIPCSerialiser_h
#define mozilla_dom_mobileconnection_MobileConnectionIPCSerialiser_h

#include "ipc/IPCMessageUtils.h"
#include "mozilla/dom/mobileconnection/MobileCallForwardingOptions.h"
#include "mozilla/dom/MobileCellInfo.h"
#include "mozilla/dom/MobileConnectionInfo.h"
#include "mozilla/dom/MobileNetworkInfo.h"
#include "mozilla/dom/MozMobileConnectionBinding.h"

using mozilla::AutoJSContext;
using mozilla::dom::mobileconnection::MobileCallForwardingOptions;
using mozilla::dom::MobileNetworkInfo;
using mozilla::dom::MobileCellInfo;
using mozilla::dom::MobileConnectionInfo;

typedef nsIMobileCellInfo* nsMobileCellInfo;
typedef nsIMobileConnectionInfo* nsMobileConnectionInfo;
typedef nsIMobileNetworkInfo* nsMobileNetworkInfo;
typedef nsIMobileCallForwardingOptions* nsMobileCallForwardingOptions;

namespace IPC {

struct MozCallForwardingOptions : public mozilla::dom::MozCallForwardingOptions
{
  bool operator==(const MozCallForwardingOptions& aOther) const
  {
    return 
           ((!mActive.WasPassed() && !aOther.mActive.WasPassed()) ||
            (mActive.WasPassed() && aOther.mActive.WasPassed() &&
             mActive.Value() == aOther.mActive.Value())) &&
           
           ((!mAction.WasPassed() && !aOther.mAction.WasPassed()) ||
            (mAction.WasPassed() && aOther.mAction.WasPassed() &&
             mAction.Value() == aOther.mAction.Value())) &&
           
           ((!mReason.WasPassed() && !aOther.mReason.WasPassed()) ||
            (mReason.WasPassed() && aOther.mReason.WasPassed() &&
             mReason.Value() == aOther.mReason.Value())) &&
           
           ((!mNumber.WasPassed() && !aOther.mNumber.WasPassed()) ||
            (mNumber.WasPassed() && aOther.mNumber.WasPassed() &&
             mNumber.Value() == aOther.mNumber.Value())) &&
           
           ((!mTimeSeconds.WasPassed() && !aOther.mTimeSeconds.WasPassed()) ||
            (mTimeSeconds.WasPassed() && aOther.mTimeSeconds.WasPassed() &&
             mTimeSeconds.Value() == aOther.mTimeSeconds.Value())) &&
           
           ((!mServiceClass.WasPassed() && !aOther.mServiceClass.WasPassed()) ||
            (mServiceClass.WasPassed() && aOther.mServiceClass.WasPassed() &&
             mServiceClass.Value() == aOther.mServiceClass.Value()));
  };
};

template <>
struct ParamTraits<nsIMobileCallForwardingOptions*>
{
  typedef nsIMobileCallForwardingOptions* paramType;

  
  static void Write(Message *aMsg, const paramType& aParam)
  {
    bool isNull = !aParam;
    WriteParam(aMsg, isNull);
    
    if (isNull) {
      return;
    }

    int16_t pShort;
    nsString pString;
    bool pBool;

    aParam->GetActive(&pBool);
    WriteParam(aMsg, pBool);

    aParam->GetAction(&pShort);
    WriteParam(aMsg, pShort);

    aParam->GetReason(&pShort);
    WriteParam(aMsg, pShort);

    aParam->GetNumber(pString);
    WriteParam(aMsg, pString);

    aParam->GetTimeSeconds(&pShort);
    WriteParam(aMsg, pShort);

    aParam->GetServiceClass(&pShort);
    WriteParam(aMsg, pShort);
  }

  
  static bool Read(const Message *aMsg, void **aIter, paramType* aResult)
  {
    
    bool isNull;
    if (!ReadParam(aMsg, aIter, &isNull)) {
      return false;
    }

    if (isNull) {
      *aResult = nullptr;
      return true;
    }

    bool active;
    int16_t action;
    int16_t reason;
    nsString number;
    int16_t timeSeconds;
    int16_t serviceClass;

    
    if (!(ReadParam(aMsg, aIter, &active) &&
          ReadParam(aMsg, aIter, &action) &&
          ReadParam(aMsg, aIter, &reason) &&
          ReadParam(aMsg, aIter, &number) &&
          ReadParam(aMsg, aIter, &timeSeconds) &&
          ReadParam(aMsg, aIter, &serviceClass))) {
      return false;
    }

    *aResult = new MobileCallForwardingOptions(active, action, reason,
                                               number, timeSeconds, serviceClass);

    
    NS_ADDREF(*aResult);

    return true;
  }
};




template <>
struct ParamTraits<nsIMobileNetworkInfo*>
{
  typedef nsIMobileNetworkInfo* paramType;

  
  static void Write(Message *aMsg, const paramType& aParam)
  {
    bool isNull = !aParam;
    WriteParam(aMsg, isNull);
    
    if (isNull) {
      return;
    }

    nsString pString;
    aParam->GetShortName(pString);
    WriteParam(aMsg, pString);

    aParam->GetLongName(pString);
    WriteParam(aMsg, pString);

    aParam->GetMcc(pString);
    WriteParam(aMsg, pString);

    aParam->GetMnc(pString);
    WriteParam(aMsg, pString);

    aParam->GetState(pString);
    WriteParam(aMsg, pString);

    
    aParam->Release();
  }

  
  static bool Read(const Message *aMsg, void **aIter, paramType* aResult)
  {
    
    bool isNull;
    if (!ReadParam(aMsg, aIter, &isNull)) {
      return false;
    }

    if (isNull) {
      *aResult = nullptr;
      return true;
    }

    nsString shortName;
    nsString longName;
    nsString mcc;
    nsString mnc;
    nsString state;

    
    if (!(ReadParam(aMsg, aIter, &shortName) &&
          ReadParam(aMsg, aIter, &longName) &&
          ReadParam(aMsg, aIter, &mcc) &&
          ReadParam(aMsg, aIter, &mnc) &&
          ReadParam(aMsg, aIter, &state))) {
      return false;
    }

    *aResult = new MobileNetworkInfo(shortName,
                                     longName,
                                     mcc,
                                     mnc,
                                     state);
    
    NS_ADDREF(*aResult);

    return true;
  }
};




template <>
struct ParamTraits<nsIMobileCellInfo*>
{
  typedef nsIMobileCellInfo* paramType;

  
  static void Write(Message *aMsg, const paramType& aParam)
  {
    bool isNull = !aParam;
    WriteParam(aMsg, isNull);
    
    if (isNull) {
      return;
    }

    int32_t pLong;
    int64_t pLongLong;

    aParam->GetGsmLocationAreaCode(&pLong);
    WriteParam(aMsg, pLong);

    aParam->GetGsmCellId(&pLongLong);
    WriteParam(aMsg, pLongLong);

    aParam->GetCdmaBaseStationId(&pLong);
    WriteParam(aMsg, pLong);

    aParam->GetCdmaBaseStationLatitude(&pLong);
    WriteParam(aMsg, pLong);

    aParam->GetCdmaBaseStationLongitude(&pLong);
    WriteParam(aMsg, pLong);

    aParam->GetCdmaSystemId(&pLong);
    WriteParam(aMsg, pLong);

    aParam->GetCdmaNetworkId(&pLong);
    WriteParam(aMsg, pLong);

    
    aParam->Release();
  }

  
  static bool Read(const Message *aMsg, void **aIter, paramType* aResult)
  {
    
    bool isNull;
    if (!ReadParam(aMsg, aIter, &isNull)) {
      return false;
    }

    if (isNull) {
      *aResult = nullptr;
      return true;
    }

    int32_t gsmLac;
    int64_t gsmCellId;
    int32_t cdmaBsId;
    int32_t cdmaBsLat;
    int32_t cdmaBsLong;
    int32_t cdmaSystemId;
    int32_t cdmaNetworkId;

    
    if (!(ReadParam(aMsg, aIter, &gsmLac) &&
          ReadParam(aMsg, aIter, &gsmCellId) &&
          ReadParam(aMsg, aIter, &cdmaBsId) &&
          ReadParam(aMsg, aIter, &cdmaBsLat) &&
          ReadParam(aMsg, aIter, &cdmaBsLong) &&
          ReadParam(aMsg, aIter, &cdmaSystemId) &&
          ReadParam(aMsg, aIter, &cdmaNetworkId))) {
      return false;
    }

    *aResult = new MobileCellInfo(gsmLac, gsmCellId, cdmaBsId, cdmaBsLat,
                                  cdmaBsLong, cdmaSystemId, cdmaNetworkId);
    
    NS_ADDREF(*aResult);

    return true;
  }
};




template <>
struct ParamTraits<nsIMobileConnectionInfo*>
{
  typedef nsIMobileConnectionInfo* paramType;

  
  static void Write(Message *aMsg, const paramType& aParam)
  {
    bool isNull = !aParam;
    WriteParam(aMsg, isNull);
    
    if (isNull) {
      return;
    }

    AutoJSContext cx;
    nsString pString;
    bool pBool;
    nsCOMPtr<nsIMobileNetworkInfo> pNetworkInfo;
    nsCOMPtr<nsIMobileCellInfo> pCellInfo;
    JS::Rooted<JS::Value> pJsval(cx);
    int32_t pInt32;

    aParam->GetState(pString);
    WriteParam(aMsg, pString);

    aParam->GetConnected(&pBool);
    WriteParam(aMsg, pBool);

    aParam->GetEmergencyCallsOnly(&pBool);
    WriteParam(aMsg, pBool);

    aParam->GetRoaming(&pBool);
    WriteParam(aMsg, pBool);

    aParam->GetType(pString);
    WriteParam(aMsg, pString);

    aParam->GetNetwork(getter_AddRefs(pNetworkInfo));
    
    WriteParam(aMsg, pNetworkInfo.forget().take());

    aParam->GetCell(getter_AddRefs(pCellInfo));
    
    WriteParam(aMsg, pCellInfo.forget().take());

    
    aParam->GetSignalStrength(&pJsval);
    isNull = !pJsval.isInt32();
    WriteParam(aMsg, isNull);

    if (!isNull) {
      pInt32 = pJsval.toInt32();
      WriteParam(aMsg, pInt32);
    }

    
    aParam->GetRelSignalStrength(&pJsval);
    isNull = !pJsval.isInt32();
    WriteParam(aMsg, isNull);

    if (!isNull) {
      pInt32 = pJsval.toInt32();
      WriteParam(aMsg, pInt32);
    }

    
    aParam->Release();
  }

  
  static bool Read(const Message* aMsg, void **aIter, paramType* aResult)
  {
    
    bool isNull;
    if (!ReadParam(aMsg, aIter, &isNull)) {
      return false;
    }

    if (isNull) {
      *aResult = nullptr;
      return true;
    }

    AutoJSContext cx;
    nsString state;
    bool connected;
    bool emergencyOnly;
    bool roaming;
    nsString type;
    nsIMobileNetworkInfo* networkInfo = nullptr;
    nsIMobileCellInfo* cellInfo = nullptr;
    Nullable<int32_t> signalStrength;
    Nullable<uint16_t> relSignalStrength;

    
    if (!(ReadParam(aMsg, aIter, &state) &&
          ReadParam(aMsg, aIter, &connected) &&
          ReadParam(aMsg, aIter, &emergencyOnly) &&
          ReadParam(aMsg, aIter, &roaming) &&
          ReadParam(aMsg, aIter, &type) &&
          ReadParam(aMsg, aIter, &networkInfo) &&
          ReadParam(aMsg, aIter, &cellInfo))) {
      return false;
    }

    
    if (!ReadParam(aMsg, aIter, &isNull)) {
      return false;
    }

    if (!isNull) {
      int32_t value;

      if (!ReadParam(aMsg, aIter, &value)) {
        return false;
      }

      signalStrength.SetValue(value);
    }

    
    if (!ReadParam(aMsg, aIter, &isNull)) {
      return false;
    }

    if (!isNull) {
      int32_t value;

      if (!ReadParam(aMsg, aIter, &value)) {
        return false;
      }

      relSignalStrength.SetValue(uint16_t(value));
    }

    *aResult = new MobileConnectionInfo(state,
                                        connected,
                                        emergencyOnly,
                                        roaming,
                                        networkInfo,
                                        type,
                                        signalStrength,
                                        relSignalStrength,
                                        cellInfo);
    
    NS_ADDREF(*aResult);
    
    
    NS_IF_RELEASE(networkInfo);
    NS_IF_RELEASE(cellInfo);

    return true;
  }
};




template <>
struct ParamTraits<MozCallForwardingOptions>
{
  typedef MozCallForwardingOptions paramType;

  
  static void Write(Message *aMsg, const paramType& aParam)
  {
    bool wasPassed = false;
    bool isNull = false;

    
    wasPassed = aParam.mActive.WasPassed();
    WriteParam(aMsg, wasPassed);
    if (wasPassed) {
      isNull = aParam.mActive.Value().IsNull();
      WriteParam(aMsg, isNull);
      if (!isNull) {
        WriteParam(aMsg, aParam.mActive.Value().Value());
      }
    }

    
    wasPassed = aParam.mAction.WasPassed();
    WriteParam(aMsg, wasPassed);
    if (wasPassed) {
      isNull = aParam.mAction.Value().IsNull();
      WriteParam(aMsg, isNull);
      if (!isNull) {
        WriteParam(aMsg, aParam.mAction.Value().Value());
      }
    }

    
    wasPassed = aParam.mReason.WasPassed();
    WriteParam(aMsg, wasPassed);
    if (wasPassed) {
      isNull = aParam.mReason.Value().IsNull();
      WriteParam(aMsg, isNull);
      if (!isNull) {
        WriteParam(aMsg, aParam.mReason.Value().Value());
      }
    }

    
    wasPassed = aParam.mNumber.WasPassed();
    WriteParam(aMsg, wasPassed);
    if (wasPassed) {
      WriteParam(aMsg, aParam.mNumber.Value());
    }

    
    wasPassed = aParam.mTimeSeconds.WasPassed();
    WriteParam(aMsg, wasPassed);
    if (wasPassed) {
      isNull = aParam.mTimeSeconds.Value().IsNull();
      WriteParam(aMsg, isNull);
      if (!isNull) {
        WriteParam(aMsg, aParam.mTimeSeconds.Value().Value());
      }
    }

    
    wasPassed = aParam.mServiceClass.WasPassed();
    WriteParam(aMsg, wasPassed);
    if (wasPassed) {
      isNull = aParam.mServiceClass.Value().IsNull();
      WriteParam(aMsg, isNull);
      if (!isNull) {
        WriteParam(aMsg, aParam.mServiceClass.Value().Value());
      }
    }
  }

  
  static bool Read(const Message *aMsg, void **aIter, paramType* aResult)
  {
    bool wasPassed = false;
    bool isNull = false;

    
    if (!ReadParam(aMsg, aIter, &wasPassed)) {
      return false;
    }
    if (wasPassed) {
      aResult->mActive.Construct();
      if (!ReadParam(aMsg, aIter, &isNull)) {
        return false;
      }

      if (!isNull) {
        if (!ReadParam(aMsg, aIter, &aResult->mActive.Value().SetValue())) {
          return false;
        }
      }
    }

    
    if (!ReadParam(aMsg, aIter, &wasPassed)) {
      return false;
    }
    if (wasPassed) {
      aResult->mAction.Construct();
      if (!ReadParam(aMsg, aIter, &isNull)) {
        return false;
      }

      if (!isNull) {
        if (!ReadParam(aMsg, aIter, &aResult->mAction.Value().SetValue())) {
          return false;
        }
      }
    }

    
    if (!ReadParam(aMsg, aIter, &wasPassed)) {
      return false;
    }
    if (wasPassed) {
      aResult->mReason.Construct();
      if (!ReadParam(aMsg, aIter, &isNull)) {
        return false;
      }

      if (!isNull) {
        if (!ReadParam(aMsg, aIter, &aResult->mReason.Value().SetValue())) {
          return false;
        }
      }
    }

    
    if (!ReadParam(aMsg, aIter, &wasPassed)) {
      return false;
    }
    if (wasPassed) {
      if (!ReadParam(aMsg, aIter, &aResult->mNumber.Construct())) {
        return false;
      }
    }

    
    if (!ReadParam(aMsg, aIter, &wasPassed)) {
      return false;
    }
    if (wasPassed) {
      aResult->mTimeSeconds.Construct();
      if (!ReadParam(aMsg, aIter, &isNull)) {
        return false;
      }

      if (!isNull) {
        if (!ReadParam(aMsg, aIter, &aResult->mTimeSeconds.Value().SetValue())) {
          return false;
        }
      }
    }

    
    if (!ReadParam(aMsg, aIter, &wasPassed)) {
      return false;
    }
    if (wasPassed) {
      aResult->mServiceClass.Construct();
      if (!ReadParam(aMsg, aIter, &isNull)) {
        return false;
      }

      if (!isNull) {
        if (!ReadParam(aMsg, aIter, &aResult->mServiceClass.Value().SetValue())) {
          return false;
        }
      }
    }

    return true;
  }
};

} 

#endif 
