





#ifndef mozilla_dom_SpeechSynthesis_h
#define mozilla_dom_SpeechSynthesis_h

#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsWrapperCache.h"
#include "nsRefPtrHashtable.h"
#include "js/TypeDecls.h"

#include "SpeechSynthesisUtterance.h"
#include "SpeechSynthesisVoice.h"

class nsIDOMWindow;

namespace mozilla {
namespace dom {

class nsSpeechTask;

class SpeechSynthesis MOZ_FINAL : public nsISupports,
                                  public nsWrapperCache
{
public:
  explicit SpeechSynthesis(nsPIDOMWindow* aParent);

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(SpeechSynthesis)

  nsIDOMWindow* GetParentObject() const;

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  bool Pending() const;

  bool Speaking() const;

  bool Paused() const;

  void Speak(SpeechSynthesisUtterance& aUtterance);

  void Cancel();

  void Pause();

  void Resume();

  void OnEnd(const nsSpeechTask* aTask);

  void GetVoices(nsTArray< nsRefPtr<SpeechSynthesisVoice> >& aResult);

private:
  virtual ~SpeechSynthesis();

  void AdvanceQueue();

  nsCOMPtr<nsPIDOMWindow> mParent;

  nsTArray<nsRefPtr<SpeechSynthesisUtterance> > mSpeechQueue;

  nsRefPtr<nsSpeechTask> mCurrentTask;

  nsRefPtrHashtable<nsStringHashKey, SpeechSynthesisVoice> mVoiceCache;
};

} 
} 

#endif
