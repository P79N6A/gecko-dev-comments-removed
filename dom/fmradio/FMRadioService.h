





#ifndef mozilla_dom_fmradioservice_h__
#define mozilla_dom_fmradioservice_h__

#include "mozilla/dom/PFMRadioRequest.h"
#include "FMRadioCommon.h"
#include "mozilla/Hal.h"
#include "mozilla/StaticPtr.h"
#include "mozilla/Services.h"
#include "nsThreadUtils.h"
#include "nsIObserver.h"
#include "nsXULAppAPI.h"

BEGIN_FMRADIO_NAMESPACE

class ReplyRunnable : public nsRunnable
{
public:
  ReplyRunnable() : mResponseType(SuccessResponse()) {}
  virtual ~ReplyRunnable() {}

  void
  SetReply(const FMRadioResponseType& aResponseType)
  {
    mResponseType = aResponseType;
  }

protected:
  FMRadioResponseType mResponseType;
};























































class IFMRadioService
{
protected:
  virtual ~IFMRadioService() { }

public:
  virtual bool IsEnabled() const = 0;
  virtual double GetFrequency() const = 0;
  virtual double GetFrequencyUpperBound() const = 0;
  virtual double GetFrequencyLowerBound() const = 0;
  virtual double GetChannelWidth() const = 0;

  virtual void Enable(double aFrequency, ReplyRunnable* aReplyRunnable) = 0;
  virtual void Disable(ReplyRunnable* aReplyRunnable) = 0;
  virtual void SetFrequency(double aFrequency, ReplyRunnable* aReplyRunnable) = 0;
  virtual void Seek(mozilla::hal::FMRadioSeekDirection aDirection,
                    ReplyRunnable* aReplyRunnable) = 0;
  virtual void CancelSeek(ReplyRunnable* aReplyRunnable) = 0;

  






  virtual void AddObserver(FMRadioEventObserver* aObserver) = 0;
  virtual void RemoveObserver(FMRadioEventObserver* aObserver) = 0;

  



  static IFMRadioService* Singleton();
};

enum FMRadioState
{
  Disabled,
  Disabling,
  Enabling,
  Enabled,
  Seeking
};

class FMRadioService MOZ_FINAL : public IFMRadioService
                               , public hal::FMRadioObserver
                               , public nsIObserver
{
  friend class ReadRilSettingTask;
  friend class SetFrequencyRunnable;

public:
  static FMRadioService* Singleton();
  virtual ~FMRadioService();

  NS_DECL_ISUPPORTS

  virtual bool IsEnabled() const MOZ_OVERRIDE;
  virtual double GetFrequency() const MOZ_OVERRIDE;
  virtual double GetFrequencyUpperBound() const MOZ_OVERRIDE;
  virtual double GetFrequencyLowerBound() const MOZ_OVERRIDE;
  virtual double GetChannelWidth() const MOZ_OVERRIDE;

  virtual void Enable(double aFrequency, ReplyRunnable* aReplyRunnable) MOZ_OVERRIDE;
  virtual void Disable(ReplyRunnable* aReplyRunnable) MOZ_OVERRIDE;
  virtual void SetFrequency(double aFrequency, ReplyRunnable* aReplyRunnable) MOZ_OVERRIDE;
  virtual void Seek(mozilla::hal::FMRadioSeekDirection aDirection,
                    ReplyRunnable* aReplyRunnable) MOZ_OVERRIDE;
  virtual void CancelSeek(ReplyRunnable* aReplyRunnable) MOZ_OVERRIDE;

  virtual void AddObserver(FMRadioEventObserver* aObserver) MOZ_OVERRIDE;
  virtual void RemoveObserver(FMRadioEventObserver* aObserver) MOZ_OVERRIDE;

  
  void Notify(const hal::FMRadioOperationInformation& aInfo) MOZ_OVERRIDE;

  NS_DECL_NSIOBSERVER

protected:
  FMRadioService();

private:
  int32_t RoundFrequency(double aFrequencyInMHz);

  void NotifyFMRadioEvent(FMRadioEventType aType);
  void DoDisable();
  void TransitionState(const FMRadioResponseType& aResponse, FMRadioState aState);
  void SetState(FMRadioState aState);
  void UpdatePowerState();
  void UpdateFrequency();

private:
  bool mEnabled;

  int32_t mPendingFrequencyInKHz;

  FMRadioState mState;

  bool mHasReadRilSetting;
  bool mRilDisabled;

  double mUpperBoundInKHz;
  double mLowerBoundInKHz;
  double mChannelWidthInKHz;

  nsRefPtr<ReplyRunnable> mPendingRequest;

  FMRadioEventObserverList mObserverList;

  static StaticRefPtr<FMRadioService> sFMRadioService;
};

END_FMRADIO_NAMESPACE

#endif 

