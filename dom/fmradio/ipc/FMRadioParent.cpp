




#include "FMRadioParent.h"
#include "mozilla/unused.h"
#include "mozilla/dom/ContentParent.h"
#include "FMRadioRequestParent.h"
#include "FMRadioService.h"

BEGIN_FMRADIO_NAMESPACE

FMRadioParent::FMRadioParent()
{
  MOZ_COUNT_CTOR(FMRadioParent);

  IFMRadioService::Singleton()->AddObserver(this);
}

FMRadioParent::~FMRadioParent()
{
  MOZ_COUNT_DTOR(FMRadioParent);

  IFMRadioService::Singleton()->RemoveObserver(this);
}

bool
FMRadioParent::RecvGetStatusInfo(StatusInfo* aStatusInfo)
{
  aStatusInfo->enabled() = IFMRadioService::Singleton()->IsEnabled();
  aStatusInfo->frequency() = IFMRadioService::Singleton()->GetFrequency();
  aStatusInfo->upperBound() =
    IFMRadioService::Singleton()->GetFrequencyUpperBound();
  aStatusInfo->lowerBound() =
    IFMRadioService::Singleton()->GetFrequencyLowerBound();
  aStatusInfo->channelWidth() =
    IFMRadioService::Singleton()->GetChannelWidth();
  return true;
}

PFMRadioRequestParent*
FMRadioParent::AllocPFMRadioRequestParent(const FMRadioRequestArgs& aArgs)
{
  nsRefPtr<FMRadioRequestParent> requestParent = new FMRadioRequestParent();

  switch (aArgs.type()) {
    case FMRadioRequestArgs::TEnableRequestArgs:
      IFMRadioService::Singleton()->Enable(
        aArgs.get_EnableRequestArgs().frequency(), requestParent);
      break;
    case FMRadioRequestArgs::TDisableRequestArgs:
      IFMRadioService::Singleton()->Disable(requestParent);
      break;
    case FMRadioRequestArgs::TSetFrequencyRequestArgs:
      IFMRadioService::Singleton()->SetFrequency(
        aArgs.get_SetFrequencyRequestArgs().frequency(), requestParent);
      break;
    case FMRadioRequestArgs::TSeekRequestArgs:
      IFMRadioService::Singleton()->Seek(
        aArgs.get_SeekRequestArgs().direction(), requestParent);
      break;
    case FMRadioRequestArgs::TCancelSeekRequestArgs:
      IFMRadioService::Singleton()->CancelSeek(requestParent);
      break;
    default:
      MOZ_CRASH();
  }

  return requestParent.forget().get();
}

bool
FMRadioParent::DeallocPFMRadioRequestParent(PFMRadioRequestParent* aActor)
{
  FMRadioRequestParent* parent = static_cast<FMRadioRequestParent*>(aActor);
  NS_RELEASE(parent);
  return true;
}

void
FMRadioParent::Notify(const FMRadioEventType& aType)
{
  switch (aType) {
    case FrequencyChanged:
      unused << SendNotifyFrequencyChanged(
        IFMRadioService::Singleton()->GetFrequency());
      break;
    case EnabledChanged:
      unused << SendNotifyEnabledChanged(
        IFMRadioService::Singleton()->IsEnabled(),
        IFMRadioService::Singleton()->GetFrequency());
      break;
    default:
      NS_RUNTIMEABORT("not reached");
      break;
  }
}

END_FMRADIO_NAMESPACE

