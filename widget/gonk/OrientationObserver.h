
















#ifndef OrientationObserver_h
#define OrientationObserver_h

#include "mozilla/Observer.h"
#include "mozilla/dom/ScreenOrientation.h"

namespace mozilla {
class ProcessOrientation;
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
  uint32_t mAllowedOrientations;
  mozilla::ScopedDeletePtr<mozilla::ProcessOrientation> mOrientation;

  static const uint32_t sDefaultOrientations =
      mozilla::dom::eScreenOrientation_PortraitPrimary |
      mozilla::dom::eScreenOrientation_PortraitSecondary |
      mozilla::dom::eScreenOrientation_LandscapePrimary |
      mozilla::dom::eScreenOrientation_LandscapeSecondary;
};

#endif
