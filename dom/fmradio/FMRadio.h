




#ifndef mozilla_dom_FMRadio_h
#define mozilla_dom_FMRadio_h

#include "FMRadioCommon.h"
#include "nsDOMEventTargetHelper.h"
#include "nsCycleCollectionParticipant.h"
#include "mozilla/HalTypes.h"
#include "nsWeakReference.h"
#include "AudioChannelAgent.h"

class nsPIDOMWindow;
class nsIScriptContext;

BEGIN_FMRADIO_NAMESPACE

class DOMRequest;

class FMRadio MOZ_FINAL : public nsDOMEventTargetHelper
                        , public hal::SwitchObserver
                        , public FMRadioEventObserver
                        , public nsSupportsWeakReference
                        , public nsIAudioChannelAgentCallback
                        , public nsIDOMEventListener

{
  friend class FMRadioRequest;

public:
  FMRadio();

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIAUDIOCHANNELAGENTCALLBACK

  NS_REALLY_FORWARD_NSIDOMEVENTTARGET(nsDOMEventTargetHelper)

  void Init(nsPIDOMWindow *aWindow);
  void Shutdown();

  
  virtual void Notify(const hal::SwitchEvent& aEvent) MOZ_OVERRIDE;
  
  virtual void Notify(const FMRadioEventType& aType) MOZ_OVERRIDE;

  nsPIDOMWindow* GetParentObject() const
  {
    return GetOwner();
  }

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  static bool Enabled();

  bool AntennaAvailable() const;

  Nullable<double> GetFrequency() const;

  double FrequencyUpperBound() const;

  double FrequencyLowerBound() const;

  double ChannelWidth() const;

  already_AddRefed<DOMRequest> Enable(double aFrequency);

  already_AddRefed<DOMRequest> Disable();

  already_AddRefed<DOMRequest> SetFrequency(double aFrequency);

  already_AddRefed<DOMRequest> SeekUp();

  already_AddRefed<DOMRequest> SeekDown();

  already_AddRefed<DOMRequest> CancelSeek();

  IMPL_EVENT_HANDLER(enabled);
  IMPL_EVENT_HANDLER(disabled);
  IMPL_EVENT_HANDLER(antennaavailablechange);
  IMPL_EVENT_HANDLER(frequencychange);

  
  NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent);

private:
  ~FMRadio();

  void SetCanPlay(bool aCanPlay);

  hal::SwitchState mHeadphoneState;
  bool mHasInternalAntenna;
  bool mIsShutdown;

  nsCOMPtr<nsIAudioChannelAgent> mAudioChannelAgent;
};

END_FMRADIO_NAMESPACE

#endif 

