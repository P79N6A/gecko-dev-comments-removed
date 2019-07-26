





#pragma once

#include "nsCycleCollectionParticipant.h"
#include "nsWrapperCache.h"
#include "nsAutoPtr.h"
#include "nsTArray.h"

#include "mozilla/Attributes.h"

#include "EnableWebSpeechRecognitionCheck.h"
#include "SpeechRecognitionResult.h"

struct JSContext;

namespace mozilla {
namespace dom {

class SpeechRecognition;

class SpeechRecognitionResultList MOZ_FINAL : public nsISupports,
                                              public nsWrapperCache,
                                              public EnableWebSpeechRecognitionCheck
{
public:
  SpeechRecognitionResultList(SpeechRecognition* aParent);
  ~SpeechRecognitionResultList();

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(SpeechRecognitionResultList)

  nsISupports* GetParentObject() const;

  virtual JSObject* WrapObject(JSContext* aCx, JSObject* aScope);

  uint32_t Length() const;

  already_AddRefed<SpeechRecognitionResult> Item(uint32_t aIndex);

  already_AddRefed<SpeechRecognitionResult> IndexedGetter(uint32_t aIndex, bool& aPresent);

  nsTArray<nsRefPtr<SpeechRecognitionResult> > mItems;
private:
  nsRefPtr<SpeechRecognition> mParent;
};

} 
} 
