
















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

  
  
  static void ShutDown();

  
  void Notify(const SensorData& aSensorData);

  
  void EnableAutoOrientation();
  void DisableAutoOrientation();

  
  bool LockScreenOrientation(ScreenOrientation aOrientation);
  void UnlockScreenOrientation();

  static OrientationObserver* GetInstance();

private:
  bool mAutoOrientationEnabled;
  PRTime mLastUpdate;
  uint32_t mAllowedOrientations;

  
  static const PRTime sMinUpdateInterval = 200 * PR_USEC_PER_MSEC;
  static const uint32_t sDefaultOrientations =
      mozilla::dom::eScreenOrientation_Portrait |
      mozilla::dom::eScreenOrientation_Landscape;
};

#endif
