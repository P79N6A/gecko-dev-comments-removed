





#ifndef OrientationObserver_h
#define OrientationObserver_h

#include "mozilla/Observer.h"
#include "mozilla/dom/ScreenOrientation.h"

namespace mozilla {
namespace hal {
class SensorData;
typedef mozilla::Observer<SensorData> ISensorObserver;
} 
} 

using mozilla::hal::ISensorObserver;
using mozilla::hal::SensorData;
using mozilla::dom::ScreenOrientation;

class OrientationObserver : public ISensorObserver {
public:
  OrientationObserver();
  ~OrientationObserver();

  
  void Notify(const SensorData& aSensorData);

  
  void EnableAutoOrientation();
  void DisableAutoOrientation();

  
  bool LockScreenOrientation(ScreenOrientation aOrientation);
  void UnlockScreenOrientation();

  static OrientationObserver* GetInstance();

private:
  bool mAutoOrientationEnabled;
  PRTime mLastUpdate;
  PRUint32 mAllowedOrientations;

  
  static const PRTime sMinUpdateInterval = 200 * PR_USEC_PER_MSEC;
  static const PRUint32 sDefaultOrientations =
      mozilla::dom::eScreenOrientation_Portrait |
      mozilla::dom::eScreenOrientation_Landscape;
};

#endif
