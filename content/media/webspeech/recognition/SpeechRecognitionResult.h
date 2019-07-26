





#pragma once

#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsWrapperCache.h"
#include "nsAutoPtr.h"
#include "nsTArray.h"

#include "mozilla/Attributes.h"

#include "EnableWebSpeechRecognitionCheck.h"
#include "SpeechRecognitionAlternative.h"

struct JSContext;

namespace mozilla {
namespace dom {

class SpeechRecognitionResult MOZ_FINAL : public nsISupports,
                                          public nsWrapperCache,
                                          public EnableWebSpeechRecognitionCheck
{
public:
  SpeechRecognitionResult(SpeechRecognition* aParent);
  ~SpeechRecognitionResult();

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(SpeechRecognitionResult)

  nsISupports* GetParentObject() const;

  virtual JSObject* WrapObject(JSContext* aCx, JSObject* aScope);

  uint32_t Length() const;

  already_AddRefed<SpeechRecognitionAlternative> Item(uint32_t aIndex);

  bool Final() const;

  already_AddRefed<SpeechRecognitionAlternative> IndexedGetter(uint32_t aIndex, bool& aPresent);

  nsTArray<nsRefPtr<SpeechRecognitionAlternative> > mItems;
private:
  nsRefPtr<SpeechRecognition> mParent;
};

} 
} 
