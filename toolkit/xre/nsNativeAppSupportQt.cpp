





































#include <stdlib.h>
#include <QTimer>
#include "mozilla/ipc/GeckoChildProcessHost.h"
#include "nsNativeAppSupportQt.h"
#include "nsCOMPtr.h"
#include "nsIObserverService.h"
#include "mozilla/Services.h"

#ifdef MOZ_ENABLE_QMSYSTEM2
void
nsNativeAppSupportQt::activityChanged(MeeGo::QmActivity::Activity activity)
{
    nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
    if (!os)
        return;

    if (MeeGo::QmActivity::Inactive == activity) {
        os->NotifyObservers(nsnull, "system-idle", nsnull);
    } else {
        os->NotifyObservers(nsnull, "system-active", nsnull);
    }
}

void
nsNativeAppSupportQt::displayStateChanged(MeeGo::QmDisplayState::DisplayState state)
{
    nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
    if (!os)
        return;

    switch (state) {
    case MeeGo::QmDisplayState::On:
        os->NotifyObservers(nsnull, "system-display-on", nsnull);
        break;
    case MeeGo::QmDisplayState::Off:
        os->NotifyObservers(nsnull, "system-display-dimmed", nsnull);
        break;
    case MeeGo::QmDisplayState::Dimmed:
        os->NotifyObservers(nsnull, "system-display-off", nsnull);
        break;
    default:
        NS_WARNING("Unknown display state");
        break;
    }
}

void nsNativeAppSupportQt::deviceModeChanged(MeeGo::QmDeviceMode::DeviceMode mode)
{
    nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
    if (!os)
        return;

    switch (mode) {
    case MeeGo::QmDeviceMode::DeviceMode::Normal:
        os->NotifyObservers(nsnull, "profile-change-net-restore", nsnull);
        break;
    case MeeGo::QmDeviceMode::DeviceMode::Flight:
        os->NotifyObservers(nsnull, "profile-change-net-teardown", nsnull);
        break;
    case MeeGo::QmDeviceMode::DeviceMode::Error:
    default:
        NS_WARNING("Unknown DeviceMode");
        break;
    }
}

void nsNativeAppSupportQt::RefreshStates()
{
  activityChanged(mActivity.get());
  displayStateChanged(mDisplayState.get());
  deviceModeChanged(mDeviceMode.getMode());
}
#endif

NS_IMETHODIMP
nsNativeAppSupportQt::Start(bool* aRetVal)
{
  NS_ASSERTION(gAppData, "gAppData must not be null.");

#ifdef MOZ_ENABLE_QMSYSTEM2
  connect(&mActivity, SIGNAL(activityChanged(MeeGo::QmActivity::Activity)), this, SLOT(activityChanged(MeeGo::QmActivity::Activity)));
  connect(&mDeviceMode, SIGNAL(deviceModeChanged(MeeGo::QmDeviceMode::DeviceMode)), this, SLOT(deviceModeChanged(MeeGo::QmDeviceMode::DeviceMode)));
  connect(&mDisplayState, SIGNAL(displayStateChanged(MeeGo::QmDisplayState::DisplayState)), this, SLOT(displayStateChanged(MeeGo::QmDisplayState::DisplayState)));
  
  QTimer::singleShot(0, this, SLOT(RefreshStates()));
#endif

  *aRetVal = PR_TRUE;
#ifdef MOZ_ENABLE_LIBCONIC
  g_type_init();
#endif

#if (MOZ_PLATFORM_MAEMO == 5)
  








  nsCAutoString applicationName;
  if (gAppData->vendor) {
      applicationName.Append(gAppData->vendor);
      applicationName.Append(".");
  }
  applicationName.Append(gAppData->name);
  ToLowerCase(applicationName);

  m_osso_context = osso_initialize(applicationName.get(),
                                   gAppData->version ? gAppData->version : "1.0",
                                   PR_TRUE,
                                   nsnull);

  
  if (m_osso_context == nsnull) {
      return NS_ERROR_FAILURE;
  }
#endif

  return NS_OK;
}

NS_IMETHODIMP
nsNativeAppSupportQt::Stop(bool* aResult)
{
  NS_ENSURE_ARG(aResult);
  *aResult = PR_TRUE;

#if (MOZ_PLATFORM_MAEMO == 5)
  if (m_osso_context) {
    osso_deinitialize(m_osso_context);
    m_osso_context = nsnull;
  }
#endif

  return NS_OK;
}

nsresult
NS_CreateNativeAppSupport(nsINativeAppSupport** aResult)
{
  nsNativeAppSupportBase* native = new nsNativeAppSupportQt();
  if (!native)
    return NS_ERROR_OUT_OF_MEMORY;

  *aResult = native;
  NS_ADDREF(*aResult);

  return NS_OK;
}
