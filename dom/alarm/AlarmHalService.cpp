





#include "AlarmHalService.h"

namespace mozilla {
namespace dom {
namespace alarm {

using namespace hal;

NS_IMPL_ISUPPORTS(AlarmHalService, nsIAlarmHalService)

void
AlarmHalService::Init()
{
  mAlarmEnabled = RegisterTheOneAlarmObserver(this);
  if (!mAlarmEnabled) {
    return;
  }
  RegisterSystemTimezoneChangeObserver(this);
  RegisterSystemClockChangeObserver(this);
}

 AlarmHalService::~AlarmHalService()
{
  if (mAlarmEnabled) {
    UnregisterTheOneAlarmObserver();
    UnregisterSystemTimezoneChangeObserver(this);
    UnregisterSystemClockChangeObserver(this);
  }
}

 StaticRefPtr<AlarmHalService> AlarmHalService::sSingleton;

 already_AddRefed<AlarmHalService>
AlarmHalService::GetInstance()
{
  if (!sSingleton) {
    sSingleton = new AlarmHalService();
    sSingleton->Init();
    ClearOnShutdown(&sSingleton);
  }

  nsRefPtr<AlarmHalService> service = sSingleton.get();
  return service.forget();
}

NS_IMETHODIMP
AlarmHalService::SetAlarm(int32_t aSeconds, int32_t aNanoseconds, bool* aStatus)
{
  if (!mAlarmEnabled) {
    return NS_ERROR_FAILURE;
  }

  bool status = hal::SetAlarm(aSeconds, aNanoseconds);
  if (status) {
    *aStatus = status;
    return NS_OK;
  } else {
    return NS_ERROR_FAILURE;
  }
}

NS_IMETHODIMP
AlarmHalService::SetAlarmFiredCb(nsIAlarmFiredCb* aAlarmFiredCb)
{
  mAlarmFiredCb = aAlarmFiredCb;
  return NS_OK;
}

NS_IMETHODIMP
AlarmHalService::SetTimezoneChangedCb(nsITimezoneChangedCb* aTimeZoneChangedCb)
{
  mTimezoneChangedCb = aTimeZoneChangedCb;
  return NS_OK;
}

NS_IMETHODIMP
AlarmHalService::SetSystemClockChangedCb(
    nsISystemClockChangedCb* aSystemClockChangedCb)
{
  mSystemClockChangedCb = aSystemClockChangedCb;
  return NS_OK;
}

void
AlarmHalService::Notify(const void_t& aVoid)
{
  if (!mAlarmFiredCb) {
    return;
  }
  mAlarmFiredCb->OnAlarmFired();
}

void
AlarmHalService::Notify(
  const SystemTimezoneChangeInformation& aSystemTimezoneChangeInfo)
{
  if (!mTimezoneChangedCb) {
    return;
  }
  mTimezoneChangedCb->OnTimezoneChanged(
    aSystemTimezoneChangeInfo.newTimezoneOffsetMinutes());
}

void
AlarmHalService::Notify(const int64_t& aClockDeltaMS)
{
  if (!mSystemClockChangedCb) {
    return;
  }
  mSystemClockChangedCb->OnSystemClockChanged(aClockDeltaMS);
}

} 
} 
} 
