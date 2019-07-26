





#ifndef mozilla_dom_SpeechSynthesisVoice_h
#define mozilla_dom_SpeechSynthesisVoice_h

#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsWrapperCache.h"
#include "js/TypeDecls.h"

#include "nsISpeechService.h"

namespace mozilla {
namespace dom {

class nsSynthVoiceRegistry;
class SpeechSynthesis;

class SpeechSynthesisVoice MOZ_FINAL : public nsISupports,
                                       public nsWrapperCache
{
  friend class nsSynthVoiceRegistry;
  friend class SpeechSynthesis;

public:
  SpeechSynthesisVoice(nsISupports* aParent, const nsAString& aUri);

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(SpeechSynthesisVoice)

  nsISupports* GetParentObject() const;

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  void GetVoiceURI(nsString& aRetval) const;

  void GetName(nsString& aRetval) const;

  void GetLang(nsString& aRetval) const;

  bool LocalService() const;

  bool Default() const;

private:
  virtual ~SpeechSynthesisVoice();

  nsCOMPtr<nsISupports> mParent;

  nsString mUri;
};

} 
} 

#endif
