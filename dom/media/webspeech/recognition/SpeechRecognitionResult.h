





#ifndef mozilla_dom_SpeechRecognitionResult_h
#define mozilla_dom_SpeechRecognitionResult_h

#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsWrapperCache.h"
#include "nsAutoPtr.h"
#include "nsTArray.h"
#include "js/TypeDecls.h"

#include "mozilla/Attributes.h"

#include "SpeechRecognitionAlternative.h"

namespace mozilla {
namespace dom {

class SpeechRecognitionResult MOZ_FINAL : public nsISupports,
                                          public nsWrapperCache
{
public:
  explicit SpeechRecognitionResult(SpeechRecognition* aParent);

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(SpeechRecognitionResult)

  nsISupports* GetParentObject() const;

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  uint32_t Length() const;

  already_AddRefed<SpeechRecognitionAlternative> Item(uint32_t aIndex);

  bool Final() const;

  already_AddRefed<SpeechRecognitionAlternative> IndexedGetter(uint32_t aIndex, bool& aPresent);

  nsTArray<nsRefPtr<SpeechRecognitionAlternative> > mItems;

private:
  ~SpeechRecognitionResult();

  nsRefPtr<SpeechRecognition> mParent;
};

} 
} 

#endif
