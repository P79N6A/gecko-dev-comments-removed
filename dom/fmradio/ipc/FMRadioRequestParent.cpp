




#include "FMRadioRequestParent.h"
#include "FMRadioService.h"
#include "mozilla/unused.h"
#include "mozilla/dom/PFMRadio.h"

BEGIN_FMRADIO_NAMESPACE

FMRadioRequestParent::FMRadioRequestParent()
  : mActorDestroyed(false)
{
  MOZ_COUNT_CTOR(FMRadioRequestParent);
}

FMRadioRequestParent::~FMRadioRequestParent()
{
  MOZ_COUNT_DTOR(FMRadioRequestParent);
}

void
FMRadioRequestParent::ActorDestroy(ActorDestroyReason aWhy)
{
  mActorDestroyed = true;
}

nsresult
FMRadioRequestParent::Run()
{
  MOZ_ASSERT(NS_IsMainThread(), "Wrong thread!");

  if (!mActorDestroyed) {
    unused << Send__delete__(this, mResponseType);
  }

  return NS_OK;
}

END_FMRADIO_NAMESPACE

