





#ifndef mozilla_dom_fm_radio_h__
#define mozilla_dom_fm_radio_h__

#include "nsCOMPtr.h"
#include "mozilla/HalTypes.h"
#include "nsDOMEventTargetHelper.h"
#include "nsIFMRadio.h"
#include "AudioChannelService.h"

#define NS_FMRADIO_CONTRACTID "@mozilla.org/fmradio;1"

#define NS_FMRADIO_CID {0x9cb91834, 0x78a9, 0x4029, \
      {0xb6, 0x44, 0x78, 0x06, 0x17, 0x3c, 0x5e, 0x2d}}

namespace mozilla {
namespace dom {
namespace fm {


class FMRadio : public nsDOMEventTargetHelper
              , public nsIFMRadio
              , public hal::FMRadioObserver
              , public hal::SwitchObserver
              , public nsIAudioChannelAgentCallback
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIFMRADIO
  NS_DECL_NSIAUDIOCHANNELAGENTCALLBACK

  NS_FORWARD_NSIDOMEVENTTARGET(nsDOMEventTargetHelper::)
  FMRadio();
  virtual void Notify(const hal::FMRadioOperationInformation& info);
  virtual void Notify(const hal::SwitchEvent& aEvent);

private:
  ~FMRadio();

  hal::SwitchState mHeadphoneState;
  bool mHasInternalAntenna;
  bool mHidden;
  nsCOMPtr<nsIAudioChannelAgent> mAudioChannelAgent;
};

} 
} 
} 
#endif

