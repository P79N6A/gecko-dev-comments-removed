




#include "mozilla/dom/PFMRadioRequestChild.h"
#include "FMRadioRequestChild.h"
#include "FMRadioService.h"

BEGIN_FMRADIO_NAMESPACE

FMRadioRequestChild::FMRadioRequestChild(ReplyRunnable* aReplyRunnable)
  : mReplyRunnable(aReplyRunnable)
{
  MOZ_COUNT_CTOR(FMRadioRequestChild);
}

FMRadioRequestChild::~FMRadioRequestChild()
{
  MOZ_COUNT_DTOR(FMRadioRequestChild);
}

bool
FMRadioRequestChild::Recv__delete__(const FMRadioResponseType& aType)
{
  mReplyRunnable->SetReply(aType);
  NS_DispatchToMainThread(mReplyRunnable);

  return true;
}

END_FMRADIO_NAMESPACE

