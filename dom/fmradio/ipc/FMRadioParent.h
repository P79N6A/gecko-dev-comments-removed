





#ifndef mozilla_dom_fmradioparent_h__
#define mozilla_dom_fmradioparent_h__

#include "FMRadioCommon.h"
#include "mozilla/dom/PFMRadioParent.h"
#include "mozilla/HalTypes.h"

BEGIN_FMRADIO_NAMESPACE

class PFMRadioRequestParent;

class FMRadioParent MOZ_FINAL : public PFMRadioParent
                              , public FMRadioEventObserver
{
public:
  FMRadioParent();
  ~FMRadioParent();

  virtual bool
  RecvGetStatusInfo(StatusInfo* aStatusInfo) MOZ_OVERRIDE;

  virtual PFMRadioRequestParent*
  AllocPFMRadioRequestParent(const FMRadioRequestArgs& aArgs) MOZ_OVERRIDE;

  virtual bool
  DeallocPFMRadioRequestParent(PFMRadioRequestParent* aActor) MOZ_OVERRIDE;

  
  virtual void Notify(const FMRadioEventType& aType) MOZ_OVERRIDE;

  virtual bool
  RecvEnableAudio(const bool& aAudioEnabled) MOZ_OVERRIDE;
};

END_FMRADIO_NAMESPACE

#endif 

