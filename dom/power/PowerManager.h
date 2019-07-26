



#ifndef mozilla_dom_power_PowerManager_h
#define mozilla_dom_power_PowerManager_h

#include "nsCOMPtr.h"
#include "nsTArray.h"
#include "nsIDOMPowerManager.h"
#include "nsIDOMWakeLockListener.h"
#include "nsIDOMWindow.h"
#include "nsWeakReference.h"

class nsPIDOMWindow;

namespace mozilla {
namespace dom {
namespace power {

class PowerManager
  : public nsIDOMMozPowerManager
  , public nsIDOMMozWakeLockListener
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(PowerManager, nsIDOMMozPowerManager)
  NS_DECL_NSIDOMMOZPOWERMANAGER
  NS_DECL_NSIDOMMOZWAKELOCKLISTENER

  PowerManager() {};
  virtual ~PowerManager() {};

  nsresult Init(nsIDOMWindow *aWindow);
  nsresult Shutdown();

  static bool CheckPermission(nsPIDOMWindow*);

  static already_AddRefed<PowerManager> CreateInstance(nsPIDOMWindow*);

private:

  nsWeakPtr mWindow;
  nsTArray<nsCOMPtr<nsIDOMMozWakeLockListener> > mListeners;
};

} 
} 
} 

#endif 
