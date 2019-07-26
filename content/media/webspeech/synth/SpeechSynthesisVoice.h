





#pragma once

#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsWrapperCache.h"

#include "EnableSpeechSynthesisCheck.h"
#include "nsISpeechService.h"

struct JSContext;

namespace mozilla {
namespace dom {

class nsSynthVoiceRegistry;
class SpeechSynthesis;

class SpeechSynthesisVoice MOZ_FINAL : public nsISupports,
                                       public nsWrapperCache,
                                       public EnableSpeechSynthesisCheck
{
  friend class nsSynthVoiceRegistry;
  friend class SpeechSynthesis;

public:
  SpeechSynthesisVoice(nsISupports* aParent, const nsAString& aUri);

  virtual ~SpeechSynthesisVoice();

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(SpeechSynthesisVoice)

  nsISupports* GetParentObject() const;

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  void GetVoiceURI(nsString& aRetval) const;

  void GetName(nsString& aRetval) const;

  void GetLang(nsString& aRetval) const;

  bool LocalService() const;

  bool Default() const;

private:

  nsCOMPtr<nsISupports> mParent;

  nsString mUri;
};

} 
} 
