





#ifndef mozilla_dom_fmradiorequestparent_h__
#define mozilla_dom_fmradiorequestparent_h__

#include "FMRadioCommon.h"
#include "mozilla/dom/PFMRadioRequestParent.h"
#include "FMRadioService.h"

BEGIN_FMRADIO_NAMESPACE

class FMRadioRequestParent MOZ_FINAL : public PFMRadioRequestParent
                                     , public FMRadioReplyRunnable
{
public:
  FMRadioRequestParent();
  ~FMRadioRequestParent();

  virtual void ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;

  NS_IMETHOD Run();

private:
  bool mActorDestroyed;
};

END_FMRADIO_NAMESPACE

#endif 

