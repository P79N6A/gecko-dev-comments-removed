





































#include <stdlib.h>
#include <QObject>
#ifdef MOZ_ENABLE_QMSYSTEM2
#include "qmdevicemode.h"
#include "qmdisplaystate.h"
#include "qmactivity.h"
#endif
#include "nsNativeAppSupportBase.h"
#include "nsString.h"

#ifdef MOZ_ENABLE_LIBCONIC
#include <glib-object.h>
#endif

#if (MOZ_PLATFORM_MAEMO == 5)
#include <libosso.h>
#endif

class nsNativeAppSupportQt : public QObject, public nsNativeAppSupportBase
{
  Q_OBJECT
public:
  NS_IMETHOD Start(bool* aRetVal);
  NS_IMETHOD Stop(bool* aResult);
#if (MOZ_PLATFORM_MAEMO == 5)
  
  osso_context_t *m_osso_context;
#endif
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

