





#ifndef mozilla_dom_SpeechRecognitionAlternative_h
#define mozilla_dom_SpeechRecognitionAlternative_h

#include "nsCycleCollectionParticipant.h"
#include "nsString.h"
#include "nsWrapperCache.h"
#include "nsAutoPtr.h"
#include "js/TypeDecls.h"

#include "mozilla/Attributes.h"

namespace mozilla {
namespace dom {

class SpeechRecognition;

class SpeechRecognitionAlternative MOZ_FINAL : public nsISupports,
                                               public nsWrapperCache
{
public:
  explicit SpeechRecognitionAlternative(SpeechRecognition* aParent);

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(SpeechRecognitionAlternative)

  nsISupports* GetParentObject() const;

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  void GetTranscript(nsString& aRetVal) const;

  float Confidence() const;

  nsString mTranscript;
  float mConfidence;
private:
  ~SpeechRecognitionAlternative();

  nsRefPtr<SpeechRecognition> mParent;
};

} 
} 

#endif
