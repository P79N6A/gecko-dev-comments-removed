





#ifndef mozilla_dom_voicemail_VoicemailIPCService_h
#define mozilla_dom_voicemail_VoicemailIPCService_h

#include "mozilla/Attributes.h"
#include "mozilla/dom/voicemail/PVoicemailChild.h"
#include "nsAutoPtr.h"
#include "nsIVoicemailService.h"

namespace mozilla {
namespace dom {
namespace voicemail {

class VoicemailIPCService MOZ_FINAL : public PVoicemailChild
                                    , public nsIVoicemailService
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIVOICEMAILSERVICE

  VoicemailIPCService();

  bool
  RecvNotifyInfoChanged(const uint32_t& aServiceId,
                        const nsString& aNumber,
                        const nsString& aDisplayName) MOZ_OVERRIDE;

  bool
  RecvNotifyStatusChanged(const uint32_t& aServiceId,
                          const bool& aHasMessages,
                          const int32_t& aMessageCount,
                          const nsString& aNumber,
                          const nsString& aDisplayName) MOZ_OVERRIDE;

  void
  ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;

private:
  
  ~VoicemailIPCService();

private:
  bool mActorDestroyed;
  nsTArray<nsCOMPtr<nsIVoicemailListener>> mListeners;
  nsTArray<nsCOMPtr<nsIVoicemailProvider>> mProviders;
};

} 
} 
} 

#endif 
