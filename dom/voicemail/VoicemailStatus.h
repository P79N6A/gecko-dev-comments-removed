





#ifndef mozilla_dom_voicemail_VoicemailStatus_h__
#define mozilla_dom_voicemail_VoicemailStatus_h__

#include "mozilla/Attributes.h"
#include "nsCOMPtr.h"
#include "nsIVoicemailService.h" 
#include "nsString.h"
#include "nsWrapperCache.h"

namespace mozilla {
namespace dom {

class VoicemailStatus final : public nsISupports
                            , public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(VoicemailStatus)

  VoicemailStatus(nsISupports* aParent,
                  nsIVoicemailProvider* aProvider);

  nsISupports*
  GetParentObject() const { return mParent; }

  virtual JSObject*
  WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  

  uint32_t
  ServiceId() const;

  bool
  HasMessages() const;

  int32_t
  MessageCount() const;

  void
  GetReturnNumber(nsString& aReturnNumber) const;

  void
  GetReturnMessage(nsString& aReturnMessage) const;

private:
  
  ~VoicemailStatus() {}

private:
  nsCOMPtr<nsISupports> mParent;
  nsCOMPtr<nsIVoicemailProvider> mProvider;
};

} 
} 

#endif 
