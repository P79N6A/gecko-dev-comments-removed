





#include "MobileConnectionInfo.h"

#include "mozilla/dom/ScriptSettings.h"

#include "jsapi.h"

#define CONVERT_STRING_TO_NULLABLE_ENUM(_string, _enumType, _enum)      \
{                                                                       \
  _enum.SetNull();                                                      \
                                                                        \
  uint32_t i = 0;                                                       \
  for (const EnumEntry* entry = _enumType##Values::strings;             \
       entry->value;                                                    \
       ++entry, ++i) {                                                  \
    if (_string.EqualsASCII(entry->value)) {                            \
      _enum.SetValue(static_cast<_enumType>(i));                        \
    }                                                                   \
  }                                                                     \
}

using namespace mozilla::dom;

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(MobileConnectionInfo, mWindow,
                                      mNetworkInfo, mCellInfo)

NS_IMPL_CYCLE_COLLECTING_ADDREF(MobileConnectionInfo)
NS_IMPL_CYCLE_COLLECTING_RELEASE(MobileConnectionInfo)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(MobileConnectionInfo)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

MobileConnectionInfo::MobileConnectionInfo(nsPIDOMWindow* aWindow)
  : mConnected(false)
  , mEmergencyCallsOnly(false)
  , mRoaming(false)
  , mWindow(aWindow)
{
  SetIsDOMBinding();
}

void
MobileConnectionInfo::Update(nsIMobileConnectionInfo* aInfo)
{
  if (!aInfo) {
    return;
  }

  aInfo->GetConnected(&mConnected);
  aInfo->GetEmergencyCallsOnly(&mEmergencyCallsOnly);
  aInfo->GetRoaming(&mRoaming);

  
  nsAutoString state;
  aInfo->GetState(state);
  CONVERT_STRING_TO_NULLABLE_ENUM(state, MobileConnectionState, mState);

  
  nsAutoString type;
  aInfo->GetType(type);
  CONVERT_STRING_TO_NULLABLE_ENUM(type, MobileConnectionType, mType);

  
  AutoSafeJSContext cx;
  JS::Rooted<JS::Value> signalStrength(cx, JSVAL_VOID);
  aInfo->GetSignalStrength(&signalStrength);
  if (signalStrength.isNumber()) {
    mSignalStrength.SetValue(signalStrength.toNumber());
  } else {
    mSignalStrength.SetNull();
  }

  
  JS::Rooted<JS::Value> relSignalStrength(cx, JSVAL_VOID);
  aInfo->GetRelSignalStrength(&relSignalStrength);
  if (relSignalStrength.isNumber()) {
    mRelSignalStrength.SetValue(relSignalStrength.toNumber());
  } else {
    mRelSignalStrength.SetNull();
  }

  
  nsCOMPtr<nsIMobileNetworkInfo> networkInfo;
  aInfo->GetNetwork(getter_AddRefs(networkInfo));
  if (networkInfo) {
    if (!mNetworkInfo) {
      mNetworkInfo = new MobileNetworkInfo(mWindow);
    }
    mNetworkInfo->Update(networkInfo);
  } else {
    mNetworkInfo = nullptr;
  }

  
  nsCOMPtr<nsIMobileCellInfo> cellInfo;
  aInfo->GetCell(getter_AddRefs(cellInfo));
  if (cellInfo) {
    if (!mCellInfo) {
      mCellInfo = new MobileCellInfo(mWindow);
    }
    mCellInfo->Update(cellInfo);
  } else {
    mCellInfo = nullptr;
  }
}

JSObject*
MobileConnectionInfo::WrapObject(JSContext* aCx)
{
  return MozMobileConnectionInfoBinding::Wrap(aCx, this);
}
