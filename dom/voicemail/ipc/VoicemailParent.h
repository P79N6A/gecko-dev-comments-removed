





#ifndef mozilla_dom_voicemail_VoicemailParent_h
#define mozilla_dom_voicemail_VoicemailParent_h

#include "mozilla/Attributes.h"
#include "mozilla/dom/voicemail/PVoicemailParent.h"
#include "mozilla/dom/voicemail/VoicemailParent.h"
#include "nsAutoPtr.h"
#include "nsIVoicemailService.h"
#include "nsString.h"

namespace mozilla {
namespace dom {
namespace voicemail {

class VoicemailParent final : public PVoicemailParent
                            , public nsIVoicemailListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIVOICEMAILLISTENER

  VoicemailParent() { MOZ_COUNT_CTOR(VoicemailParent); }

  bool
  Init();

  bool
  RecvGetAttributes(const uint32_t& aServiceId,
                    nsString* aNumber,
                    nsString* aDisplayName,
                    bool* aHasMessages,
                    int32_t* aMessageCount,
                    nsString* aReturnNumber,
                    nsString* aReturnMessage) override;

  void
  ActorDestroy(ActorDestroyReason aWhy) override;

private:
  
  ~VoicemailParent() { MOZ_COUNT_DTOR(VoicemailParent); }

private:
  nsCOMPtr<nsIVoicemailService> mService;
};

} 
} 
} 

#endif 
