



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

  static void CheckVolumeSettings(const nsACString& aVolumeName);

  int32_t GetStatus() { return mStatus; }
  void SetStatus(int32_t aStatus);
  const char *StatusStr(int32_t aStatus);

private:
  int32_t mStatus;
};

}   
}   

#endif  

