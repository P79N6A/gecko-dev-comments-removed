





#pragma once

#include "nsCycleCollectionParticipant.h"
#include "nsString.h"
#include "nsWrapperCache.h"
#include "nsAutoPtr.h"

#include "mozilla/Attributes.h"

#include "EnableWebSpeechRecognitionCheck.h"

struct JSContext;

namespace mozilla {
namespace dom {

class SpeechRecognition;

class SpeechRecognitionAlternative MOZ_FINAL : public nsISupports,
                                               public nsWrapperCache,
                                               public EnableWebSpeechRecognitionCheck
{
public:
  SpeechRecognitionAlternative(SpeechRecognition* aParent);
  ~SpeechRecognitionAlternative();

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(SpeechRecognitionAlternative)

  nsISupports* GetParentObject() const;

  virtual JSObject* WrapObject(JSContext* aCx, JSObject* aScope);

  void GetTranscript(nsString& aRetVal) const;

  float Confidence() const;

  nsString mTranscript;
  float mConfidence;
private:
  nsRefPtr<SpeechRecognition> mParent;
};

} 
} 
