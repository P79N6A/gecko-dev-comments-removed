





#ifndef mozilla_dom_fmradioservice_h__
#define mozilla_dom_fmradioservice_h__

#include "mozilla/dom/Nullable.h"
#include "mozilla/dom/PFMRadioRequest.h"
#include "FMRadioCommon.h"
#include "mozilla/Hal.h"
#include "mozilla/Mutex.h"
#include "mozilla/StaticPtr.h"
#include "mozilla/Services.h"
#include "nsThreadUtils.h"
#include "nsIObserver.h"
#include "nsXULAppAPI.h"

BEGIN_FMRADIO_NAMESPACE

class FMRadioReplyRunnable : public nsRunnable
{
public:
  FMRadioReplyRunnable() : mResponseType(SuccessResponse()) {}
  virtual ~FMRadioReplyRunnable() {}

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
  virtual bool IsRDSEnabled() const = 0;
  virtual double GetFrequency() const = 0;
  virtual double GetFrequencyUpperBound() const = 0;
  virtual double GetFrequencyLowerBound() const = 0;
  virtual double GetChannelWidth() const = 0;
  virtual Nullable<unsigned short> GetPi() const = 0;
  virtual Nullable<uint8_t> GetPty() const = 0;
  virtual bool GetPs(nsString& aPsname) = 0;
  virtual bool GetRt(nsString& aRadiotext) = 0;
  virtual bool GetRdsgroup(uint64_t& aRDSGroup) = 0;

  virtual void Enable(double aFrequency, FMRadioReplyRunnable* aReplyRunnable) = 0;
  virtual void Disable(FMRadioReplyRunnable* aReplyRunnable) = 0;
  virtual void SetFrequency(double aFrequency, FMRadioReplyRunnable* aReplyRunnable) = 0;
  virtual void Seek(mozilla::hal::FMRadioSeekDirection aDirection,
                    FMRadioReplyRunnable* aReplyRunnable) = 0;
  virtual void CancelSeek(FMRadioReplyRunnable* aReplyRunnable) = 0;
  virtual void SetRDSGroupMask(uint32_t aRDSGroupMask) = 0;
  virtual void EnableRDS(FMRadioReplyRunnable* aReplyRunnable) = 0;
  virtual void DisableRDS(FMRadioReplyRunnable* aReplyRunnable) = 0;

  






  virtual void AddObserver(FMRadioEventObserver* aObserver) = 0;
  virtual void RemoveObserver(FMRadioEventObserver* aObserver) = 0;

  
  virtual void EnableAudio(bool aAudioEnabled) = 0;

  



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
                               , public hal::FMRadioRDSObserver
                               , public nsIObserver
{
  friend class ReadAirplaneModeSettingTask;
  friend class EnableRunnable;
  friend class DisableRunnable;
  friend class NotifyRunnable;

public:
  static FMRadioService* Singleton();

  NS_DECL_ISUPPORTS

  virtual bool IsEnabled() const MOZ_OVERRIDE;
  virtual bool IsRDSEnabled() const MOZ_OVERRIDE;
  virtual double GetFrequency() const MOZ_OVERRIDE;
  virtual double GetFrequencyUpperBound() const MOZ_OVERRIDE;
  virtual double GetFrequencyLowerBound() const MOZ_OVERRIDE;
  virtual double GetChannelWidth() const MOZ_OVERRIDE;
  virtual Nullable<unsigned short> GetPi() const MOZ_OVERRIDE;
  virtual Nullable<uint8_t> GetPty() const MOZ_OVERRIDE;
  virtual bool GetPs(nsString& aPsname) MOZ_OVERRIDE;
  virtual bool GetRt(nsString& aRadiotext) MOZ_OVERRIDE;
  virtual bool GetRdsgroup(uint64_t& aRDSGroup) MOZ_OVERRIDE;

  virtual void Enable(double aFrequency,
                      FMRadioReplyRunnable* aReplyRunnable) MOZ_OVERRIDE;
  virtual void Disable(FMRadioReplyRunnable* aReplyRunnable) MOZ_OVERRIDE;
  virtual void SetFrequency(double aFrequency,
                            FMRadioReplyRunnable* aReplyRunnable) MOZ_OVERRIDE;
  virtual void Seek(mozilla::hal::FMRadioSeekDirection aDirection,
                    FMRadioReplyRunnable* aReplyRunnable) MOZ_OVERRIDE;
  virtual void CancelSeek(FMRadioReplyRunnable* aReplyRunnable) MOZ_OVERRIDE;
  virtual void SetRDSGroupMask(uint32_t aRDSGroupMask) MOZ_OVERRIDE;
  virtual void EnableRDS(FMRadioReplyRunnable* aReplyRunnable) MOZ_OVERRIDE;
  virtual void DisableRDS(FMRadioReplyRunnable* aReplyRunnable) MOZ_OVERRIDE;

  virtual void AddObserver(FMRadioEventObserver* aObserver) MOZ_OVERRIDE;
  virtual void RemoveObserver(FMRadioEventObserver* aObserver) MOZ_OVERRIDE;

  virtual void EnableAudio(bool aAudioEnabled) MOZ_OVERRIDE;

  
  void Notify(const hal::FMRadioOperationInformation& aInfo) MOZ_OVERRIDE;
  
  void Notify(const hal::FMRadioRDSGroup& aRDSGroup) MOZ_OVERRIDE;

  NS_DECL_NSIOBSERVER

protected:
  FMRadioService();
  virtual ~FMRadioService();

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

  bool mHasReadAirplaneModeSetting;
  bool mAirplaneModeEnabled;
  bool mRDSEnabled;

  uint32_t mUpperBoundInKHz;
  uint32_t mLowerBoundInKHz;
  uint32_t mChannelWidthInKHz;
  uint32_t mPreemphasis;

  nsCOMPtr<nsIThread> mTuneThread;
  nsRefPtr<FMRadioReplyRunnable> mPendingRequest;

  FMRadioEventObserverList mObserverList;

  static StaticRefPtr<FMRadioService> sFMRadioService;

  uint32_t mRDSGroupMask;

  uint16_t mLastPI;
  uint16_t mLastPTY;
  Atomic<uint32_t> mPI;
  Atomic<uint32_t> mPTY;
  Atomic<bool> mPISet;
  Atomic<bool> mPTYSet;

  
  Mutex mRDSLock;
  char16_t mPSName[9];
  char16_t mRadiotext[65];
  uint64_t mRDSGroup;

  uint8_t mPSNameState;
  uint16_t mRadiotextState;
  uint16_t mTempPSName[8];
  uint16_t mTempRadiotext[64];
  bool mRadiotextAB;
  bool mRDSGroupSet;
  bool mPSNameSet;
  bool mRadiotextSet;
};

END_FMRADIO_NAMESPACE

#endif 

