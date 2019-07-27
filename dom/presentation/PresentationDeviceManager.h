




#ifndef mozilla_dom_PresentationDeviceManager_h__
#define mozilla_dom_PresentationDeviceManager_h__

#include "nsIObserver.h"
#include "nsIPresentationDevice.h"
#include "nsIPresentationDeviceManager.h"
#include "nsIPresentationDeviceProvider.h"
#include "nsCOMArray.h"

namespace mozilla {
namespace dom {

class PresentationDeviceManager final : public nsIPresentationDeviceManager
                                          , public nsIPresentationDeviceListener
                                          , public nsIPresentationDeviceEventListener
                                          , public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPRESENTATIONDEVICEMANAGER
  NS_DECL_NSIPRESENTATIONDEVICELISTENER
  NS_DECL_NSIPRESENTATIONDEVICEEVENTLISTENER
  NS_DECL_NSIOBSERVER

  PresentationDeviceManager();

private:
  virtual ~PresentationDeviceManager();

  void LoadDeviceProviders();

  void UnloadDeviceProviders();

  void NotifyDeviceChange(nsIPresentationDevice* aDevice,
                          const char16_t* aType);

  nsCOMArray<nsIPresentationDeviceProvider> mProviders;
  nsCOMArray<nsIPresentationDevice> mDevices;
};

} 
} 

#endif 
