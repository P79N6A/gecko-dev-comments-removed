



#include "AlarmHalService.h"

namespace mozilla {
namespace dom {
namespace alarm {

using namespace hal;

NS_IMPL_ISUPPORTS1(AlarmHalService, nsIAlarmHalService)

void
AlarmHalService::Init()
{
  mAlarmEnabled = RegisterTheOneAlarmObserver(this);
}

 AlarmHalService::~AlarmHalService() 
{
  if (mAlarmEnabled) {
    UnregisterTheOneAlarmObserver();
  }
}

 nsRefPtr<AlarmHalService> AlarmHalService::sSingleton;

 already_AddRefed<nsIAlarmHalService>
AlarmHalService::GetInstance()
{
  if (!sSingleton) {
    sSingleton = new AlarmHalService();
    sSingleton->Init(); 
    ClearOnShutdown(&sSingleton);
  }

  nsCOMPtr<nsIAlarmHalService> service(do_QueryInterface(sSingleton));
  return service.forget();
}

NS_IMETHODIMP
AlarmHalService::SetAlarm(PRInt32 aSeconds, PRInt32 aNanoseconds, bool* aStatus)
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

void
AlarmHalService::Notify(const mozilla::void_t& aVoid)
{
  if (mAlarmFiredCb) {
    mAlarmFiredCb->OnAlarmFired();
  }
}

PRInt32
AlarmHalService::GetTimezoneOffset(bool aIgnoreDST)
{
  PRExplodedTime prTime;
  PR_ExplodeTime(PR_Now(), PR_LocalTimeParameters, &prTime);

  PRInt32 offset = prTime.tm_params.tp_gmt_offset;
  if (!aIgnoreDST) {
    offset += prTime.tm_params.tp_dst_offset;
  }

  return -(offset / 60);
}

} 
} 
} 
