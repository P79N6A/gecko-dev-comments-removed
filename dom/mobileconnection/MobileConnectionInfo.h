





#ifndef mozilla_dom_MobileConnectionInfo_h
#define mozilla_dom_MobileConnectionInfo_h

#include "MobileCellInfo.h"
#include "MobileNetworkInfo.h"
#include "mozilla/dom/MozMobileConnectionInfoBinding.h"
#include "nsIMobileConnectionInfo.h"
#include "nsPIDOMWindow.h"
#include "nsWrapperCache.h"

namespace mozilla {
namespace dom {

class MobileConnectionInfo MOZ_FINAL : public nsIMobileConnectionInfo
                                     , public nsWrapperCache
{
public:
  NS_DECL_NSIMOBILECONNECTIONINFO
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(MobileConnectionInfo)

  explicit MobileConnectionInfo(nsPIDOMWindow* aWindow);

  MobileConnectionInfo(const nsAString& aState, bool aConnected,
                       bool aEmergencyCallsOnly, bool aRoaming,
                       nsIMobileNetworkInfo* aNetworkInfo,
                       const nsAString& aType,
                       const Nullable<int32_t>& aSignalStrength,
                       const Nullable<uint16_t>& aRelSignalStrength,
                       nsIMobileCellInfo* aCellInfo);

  void
  Update(nsIMobileConnectionInfo* aInfo);

  nsPIDOMWindow*
  GetParentObject() const
  {
    return mWindow;
  }

  virtual JSObject*
  WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  
  bool
  Connected() const
  {
    return mConnected;
  }

  bool
  EmergencyCallsOnly() const
  {
    return mEmergencyCallsOnly;
  }

  bool
  Roaming() const
  {
    return mRoaming;
  }

  Nullable<MobileConnectionState>
  GetState() const
  {
    return mState;
  }

  Nullable<MobileConnectionType>
  GetType() const
  {
    return mType;
  }

  MobileNetworkInfo*
  GetNetwork() const
  {
    return mNetworkInfo;
  }

  Nullable<int32_t>
  GetSignalStrength() const
  {
    return mSignalStrength;
  }

  Nullable<uint16_t>
  GetRelSignalStrength() const
  {
    return mRelSignalStrength;
  }

  MobileCellInfo*
  GetCell() const
  {
    return mCellInfo;
  }

private:
  ~MobileConnectionInfo() {}

private:
  bool mConnected;
  bool mEmergencyCallsOnly;
  bool mRoaming;
  nsCOMPtr<nsPIDOMWindow> mWindow;
  nsRefPtr<MobileNetworkInfo> mNetworkInfo;
  nsRefPtr<MobileCellInfo> mCellInfo;
  Nullable<MobileConnectionState> mState;
  Nullable<MobileConnectionType> mType;
  Nullable<int32_t> mSignalStrength;
  Nullable<uint16_t> mRelSignalStrength;
};

} 
} 

#endif 
