



#ifndef mozilla_system_automountersetting_h__
#define mozilla_system_automountersetting_h__

#include "nsIObserver.h"

namespace mozilla {
namespace system {

class AutoMounterSetting : public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  AutoMounterSetting();

  static void CheckVolumeSettings(const nsACString& aVolumeName);

  int32_t GetStatus() { return mStatus; }
  void SetStatus(int32_t aStatus);
  const char *StatusStr(int32_t aStatus);

protected:
  virtual ~AutoMounterSetting();

private:
  int32_t mStatus;
};

}   
}   

#endif  

