





#include <stdlib.h>
#include <QObject>
#ifdef MOZ_ENABLE_QMSYSTEM2
#include "qmdevicemode.h"
#include "qmdisplaystate.h"
#include "qmactivity.h"
#endif
#include "nsNativeAppSupportBase.h"
#include "nsString.h"

class nsNativeAppSupportQt : public QObject, public nsNativeAppSupportBase
{
  Q_OBJECT
public:
  NS_IMETHOD Start(bool* aRetVal);
  NS_IMETHOD Stop(bool* aResult);

#ifdef MOZ_ENABLE_QMSYSTEM2
public Q_SLOTS:
  void activityChanged(MeeGo::QmActivity::Activity activity);
  void deviceModeChanged(MeeGo::QmDeviceMode::DeviceMode mode);
  void displayStateChanged(MeeGo::QmDisplayState::DisplayState state);
  void RefreshStates();

private:
  MeeGo::QmDeviceMode mDeviceMode;
  MeeGo::QmDisplayState mDisplayState;
  MeeGo::QmActivity mActivity;
#endif
};

