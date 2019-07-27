





#ifndef mozilla_dom_fmradioparent_h__
#define mozilla_dom_fmradioparent_h__

#include "FMRadioCommon.h"
#include "mozilla/dom/PFMRadioParent.h"
#include "mozilla/HalTypes.h"

BEGIN_FMRADIO_NAMESPACE

class PFMRadioRequestParent;

class FMRadioParent final : public PFMRadioParent
                          , public FMRadioEventObserver
{
public:
  FMRadioParent();
  ~FMRadioParent();

  virtual void
  ActorDestroy(ActorDestroyReason aWhy) override;

  virtual bool
  RecvGetStatusInfo(StatusInfo* aStatusInfo) override;

  virtual PFMRadioRequestParent*
  AllocPFMRadioRequestParent(const FMRadioRequestArgs& aArgs) override;

  virtual bool
  DeallocPFMRadioRequestParent(PFMRadioRequestParent* aActor) override;

  
  virtual void Notify(const FMRadioEventType& aType) override;

  virtual bool
  RecvEnableAudio(const bool& aAudioEnabled) override;

  virtual bool
  RecvSetRDSGroupMask(const uint32_t& aRDSGroupMask) override;
};

END_FMRADIO_NAMESPACE

#endif 

