



#ifndef mozilla_system_automountersetting_h__
#define mozilla_system_automountersetting_h__

#include "nsIObserver.h"

namespace mozilla {
namespace system {

class ResultListener;

class AutoMounterSetting : public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  AutoMounterSetting();
  virtual ~AutoMounterSetting();
};

}   
}   

#endif  

