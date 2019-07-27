





#ifndef mozilla_dom_MobileCellInfo_h
#define mozilla_dom_MobileCellInfo_h

#include "nsIMobileCellInfo.h"
#include "nsPIDOMWindow.h"
#include "nsWrapperCache.h"

namespace mozilla {
namespace dom {

class MobileCellInfo MOZ_FINAL : public nsISupports
                               , public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(MobileCellInfo)

  MobileCellInfo(nsPIDOMWindow* aWindow);

  void
  Update(nsIMobileCellInfo* aInfo);

  nsPIDOMWindow*
  GetParentObject() const
  {
    return mWindow;
  }

  virtual JSObject*
  WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  
  int32_t
  GsmLocationAreaCode() const
  {
    return mGsmLocationAreaCode;
  }

  int64_t
  GsmCellId() const
  {
    return mGsmCellId;
  }

  int32_t
  CdmaBaseStationId() const
  {
    return mCdmaBaseStationId;
  }

  int32_t
  CdmaBaseStationLatitude() const
  {
    return mCdmaBaseStationLatitude;
  }

  int32_t
  CdmaBaseStationLongitude() const
  {
    return mCdmaBaseStationLongitude;
  }

  int32_t
  CdmaSystemId() const
  {
    return mCdmaSystemId;
  }

  int32_t
  CdmaNetworkId() const
  {
    return mCdmaNetworkId;
  }

private:
  ~MobileCellInfo() {}

private:
  nsCOMPtr<nsPIDOMWindow> mWindow;
  int32_t mGsmLocationAreaCode;
  int64_t mGsmCellId;
  int32_t mCdmaBaseStationId;
  int32_t mCdmaBaseStationLatitude;
  int32_t mCdmaBaseStationLongitude;
  int32_t mCdmaSystemId;
  int32_t mCdmaNetworkId;
};

} 
} 

#endif 
