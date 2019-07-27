





#include "mozilla/dom/MobileConnectionInfo.h"

#include "mozilla/dom/ScriptSettings.h"

#include "jsapi.h"

#ifdef CONVERT_STRING_TO_NULLABLE_ENUM
#undef CONVERT_STRING_TO_NULLABLE_ENUM
#endif
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

#define CONVERT_NULLABLE_ENUM_TO_STRING(_enumType, _enum, _string)      \
{                                                                       \
  if (_enum.IsNull()) {                                                 \
    _string.SetIsVoid(true);                                            \
  } else {                                                              \
    uint32_t index = uint32_t(_enum.Value());                           \
    _string.AssignASCII(_enumType##Values::strings[index].value,        \
                        _enumType##Values::strings[index].length);      \
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
  NS_INTERFACE_MAP_ENTRY(nsIMobileConnectionInfo)
NS_INTERFACE_MAP_END

MobileConnectionInfo::MobileConnectionInfo(nsPIDOMWindow* aWindow)
  : mConnected(false)
  , mEmergencyCallsOnly(false)
  , mRoaming(false)
  , mWindow(aWindow)
{
}

MobileConnectionInfo::MobileConnectionInfo(const nsAString& aState,
                                           bool aConnected,
                                           bool aEmergencyCallsOnly,
                                           bool aRoaming,
                                           nsIMobileNetworkInfo* aNetworkInfo,
                                           const nsAString& aType,
                                           const Nullable<int32_t>& aSignalStrength,
                                           const Nullable<uint16_t>& aRelSignalStrength,
                                           nsIMobileCellInfo* aCellInfo)
  : mConnected(aConnected)
  , mEmergencyCallsOnly(aEmergencyCallsOnly)
  , mRoaming(aRoaming)
  , mSignalStrength(aSignalStrength)
  , mRelSignalStrength(aRelSignalStrength)
{
  
  
  

  
  CONVERT_STRING_TO_NULLABLE_ENUM(aState, MobileConnectionState, mState);
  CONVERT_STRING_TO_NULLABLE_ENUM(aType, MobileConnectionType, mType);

  
  if (aNetworkInfo) {
    mNetworkInfo = new MobileNetworkInfo(mWindow);
    mNetworkInfo->Update(aNetworkInfo);
  }

  
  if (aCellInfo) {
    mCellInfo = new MobileCellInfo(mWindow);
    mCellInfo->Update(aCellInfo);
  }
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

  
  AutoJSContext cx;
  JS::Rooted<JS::Value> signalStrength(cx, JS::UndefinedValue());
  aInfo->GetSignalStrength(&signalStrength);
  if (signalStrength.isNumber()) {
    mSignalStrength.SetValue(signalStrength.toNumber());
  } else {
    mSignalStrength.SetNull();
  }

  
  JS::Rooted<JS::Value> relSignalStrength(cx, JS::UndefinedValue());
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
MobileConnectionInfo::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return MozMobileConnectionInfoBinding::Wrap(aCx, this, aGivenProto);
}



NS_IMETHODIMP
MobileConnectionInfo::GetState(nsAString& aState)
{
  CONVERT_NULLABLE_ENUM_TO_STRING(MobileConnectionState, mState, aState);
  return NS_OK;
}

NS_IMETHODIMP
MobileConnectionInfo::GetConnected(bool* aConnected)
{
  *aConnected = Connected();
  return NS_OK;
}

NS_IMETHODIMP
MobileConnectionInfo::GetEmergencyCallsOnly(bool* aEmergencyCallsOnly)
{
  *aEmergencyCallsOnly = EmergencyCallsOnly();
  return NS_OK;
}

NS_IMETHODIMP
MobileConnectionInfo::GetRoaming(bool* aRoaming)
{
  *aRoaming = Roaming();
  return NS_OK;
}

NS_IMETHODIMP
MobileConnectionInfo::GetNetwork(nsIMobileNetworkInfo** aInfo)
{
  NS_IF_ADDREF(*aInfo = GetNetwork());
  return NS_OK;
}

NS_IMETHODIMP
MobileConnectionInfo::GetType(nsAString& aType)
{
  CONVERT_NULLABLE_ENUM_TO_STRING(MobileConnectionType, mType, aType);
  return NS_OK;
}

NS_IMETHODIMP
MobileConnectionInfo::GetSignalStrength(JS::MutableHandle<JS::Value> aSignal)
{
  if (mSignalStrength.IsNull()) {
    aSignal.setNull();
  } else {
    aSignal.setInt32(mSignalStrength.Value());
  }
  return NS_OK;
}

NS_IMETHODIMP
MobileConnectionInfo::GetRelSignalStrength(JS::MutableHandle<JS::Value> aSignal)
{
  if (mRelSignalStrength.IsNull()) {
    aSignal.setNull();
  } else {
    aSignal.setNumber(uint32_t(mRelSignalStrength.Value()));
  }
  return NS_OK;
}

NS_IMETHODIMP
MobileConnectionInfo::GetCell(nsIMobileCellInfo** aInfo)
{
  NS_IF_ADDREF(*aInfo = GetCell());
  return NS_OK;
}
