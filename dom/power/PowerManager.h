



































#ifndef mozilla_dom_power_PowerManager_h
#define mozilla_dom_power_PowerManager_h

#include "nsCOMPtr.h"
#include "nsTArray.h"
#include "nsIDOMPowerManager.h"
#include "nsIDOMWakeLockListener.h"
#include "nsIDOMWindow.h"
#include "nsWeakReference.h"

namespace mozilla {
namespace dom {
namespace power {

class PowerManager
  : public nsIDOMMozPowerManager
  , public nsIDOMMozWakeLockListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMMOZPOWERMANAGER
  NS_DECL_NSIDOMMOZWAKELOCKLISTENER

  PowerManager() {};
  virtual ~PowerManager() {};

  nsresult Init(nsIDOMWindow *aWindow);
  nsresult Shutdown();

private:
  nsresult CheckPermission();

  nsWeakPtr mWindow;
  nsTArray<nsCOMPtr<nsIDOMMozWakeLockListener> > mListeners;
};

} 
} 
} 

#endif 
