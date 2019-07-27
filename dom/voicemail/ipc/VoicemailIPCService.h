





#ifndef mozilla_dom_voicemail_VoicemailIPCService_h
#define mozilla_dom_voicemail_VoicemailIPCService_h

#include "mozilla/Attributes.h"
#include "mozilla/dom/voicemail/PVoicemailChild.h"
#include "nsAutoPtr.h"
#include "nsIVoicemailService.h"

namespace mozilla {
namespace dom {
namespace voicemail {

class VoicemailIPCService final : public PVoicemailChild
                                , public nsIVoicemailService
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIVOICEMAILSERVICE

  VoicemailIPCService();

  bool
  RecvNotifyInfoChanged(const uint32_t& aServiceId,
                        const nsString& aNumber,
                        const nsString& aDisplayName) override;

  bool
  RecvNotifyStatusChanged(const uint32_t& aServiceId,
                          const bool& aHasMessages,
                          const int32_t& aMessageCount,
                          const nsString& aNumber,
                          const nsString& aDisplayName) override;

  void
  ActorDestroy(ActorDestroyReason aWhy) override;

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
