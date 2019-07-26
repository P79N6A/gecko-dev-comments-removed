





#ifndef mozilla_dom_fmradiochild_h__
#define mozilla_dom_fmradiochild_h__

#include "FMRadioCommon.h"
#include "FMRadioService.h"
#include "mozilla/dom/PFMRadioChild.h"
#include "mozilla/StaticPtr.h"

BEGIN_FMRADIO_NAMESPACE









class FMRadioChild MOZ_FINAL : public IFMRadioService
                             , public PFMRadioChild
{
public:
  static FMRadioChild* Singleton();
  ~FMRadioChild();

  void SendRequest(FMRadioReplyRunnable* aReplyRunnable,
                   FMRadioRequestArgs aArgs);

  
  virtual bool IsEnabled() const MOZ_OVERRIDE;
  virtual double GetFrequency() const MOZ_OVERRIDE;
  virtual double GetFrequencyUpperBound() const MOZ_OVERRIDE;
  virtual double GetFrequencyLowerBound() const MOZ_OVERRIDE;
  virtual double GetChannelWidth() const MOZ_OVERRIDE;

  virtual void Enable(double aFrequency,
                      FMRadioReplyRunnable* aReplyRunnable) MOZ_OVERRIDE;
  virtual void Disable(FMRadioReplyRunnable* aReplyRunnable) MOZ_OVERRIDE;
  virtual void SetFrequency(double frequency,
                            FMRadioReplyRunnable* aReplyRunnable) MOZ_OVERRIDE;
  virtual void Seek(mozilla::hal::FMRadioSeekDirection aDirection,
                    FMRadioReplyRunnable* aReplyRunnable) MOZ_OVERRIDE;
  virtual void CancelSeek(FMRadioReplyRunnable* aReplyRunnable) MOZ_OVERRIDE;

  virtual void AddObserver(FMRadioEventObserver* aObserver) MOZ_OVERRIDE;
  virtual void RemoveObserver(FMRadioEventObserver* aObserver) MOZ_OVERRIDE;

  virtual void EnableAudio(bool aAudioEnabled) MOZ_OVERRIDE;

  
  virtual bool
  Recv__delete__() MOZ_OVERRIDE;

  virtual bool
  RecvNotifyFrequencyChanged(const double& aFrequency) MOZ_OVERRIDE;

  virtual bool
  RecvNotifyEnabledChanged(const bool& aEnabled,
                           const double& aFrequency) MOZ_OVERRIDE;

  virtual PFMRadioRequestChild*
  AllocPFMRadioRequestChild(const FMRadioRequestArgs& aArgs) MOZ_OVERRIDE;

  virtual bool
  DeallocPFMRadioRequestChild(PFMRadioRequestChild* aActor) MOZ_OVERRIDE;

private:
  FMRadioChild();

  void Init();

  inline void NotifyFMRadioEvent(FMRadioEventType aType);

  bool mEnabled;
  double mFrequency;
  double mUpperBound;
  double mLowerBound;
  double mChannelWidth;

  FMRadioEventObserverList mObserverList;

private:
  static StaticAutoPtr<FMRadioChild> sFMRadioChild;
};

END_FMRADIO_NAMESPACE

#endif 

