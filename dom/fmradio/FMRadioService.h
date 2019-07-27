





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

class FMRadioService final : public IFMRadioService
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

  virtual bool IsEnabled() const override;
  virtual bool IsRDSEnabled() const override;
  virtual double GetFrequency() const override;
  virtual double GetFrequencyUpperBound() const override;
  virtual double GetFrequencyLowerBound() const override;
  virtual double GetChannelWidth() const override;
  virtual Nullable<unsigned short> GetPi() const override;
  virtual Nullable<uint8_t> GetPty() const override;
  virtual bool GetPs(nsString& aPsname) override;
  virtual bool GetRt(nsString& aRadiotext) override;
  virtual bool GetRdsgroup(uint64_t& aRDSGroup) override;

  virtual void Enable(double aFrequency,
                      FMRadioReplyRunnable* aReplyRunnable) override;
  virtual void Disable(FMRadioReplyRunnable* aReplyRunnable) override;
  virtual void SetFrequency(double aFrequency,
                            FMRadioReplyRunnable* aReplyRunnable) override;
  virtual void Seek(mozilla::hal::FMRadioSeekDirection aDirection,
                    FMRadioReplyRunnable* aReplyRunnable) override;
  virtual void CancelSeek(FMRadioReplyRunnable* aReplyRunnable) override;
  virtual void SetRDSGroupMask(uint32_t aRDSGroupMask) override;
  virtual void EnableRDS(FMRadioReplyRunnable* aReplyRunnable) override;
  virtual void DisableRDS(FMRadioReplyRunnable* aReplyRunnable) override;

  virtual void AddObserver(FMRadioEventObserver* aObserver) override;
  virtual void RemoveObserver(FMRadioEventObserver* aObserver) override;

  virtual void EnableAudio(bool aAudioEnabled) override;

  
  void Notify(const hal::FMRadioOperationInformation& aInfo) override;
  
  void Notify(const hal::FMRadioRDSGroup& aRDSGroup) override;

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

