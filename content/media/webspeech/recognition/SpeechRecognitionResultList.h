





#ifndef mozilla_dom_SpeechRecognitionResultList_h
#define mozilla_dom_SpeechRecognitionResultList_h

#include "nsCycleCollectionParticipant.h"
#include "nsWrapperCache.h"
#include "nsAutoPtr.h"
#include "nsTArray.h"
#include "js/TypeDecls.h"

#include "mozilla/Attributes.h"

#include "SpeechRecognitionResult.h"

namespace mozilla {
namespace dom {

class SpeechRecognition;

class SpeechRecognitionResultList MOZ_FINAL : public nsISupports,
                                              public nsWrapperCache
{
public:
  explicit SpeechRecognitionResultList(SpeechRecognition* aParent);

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(SpeechRecognitionResultList)

  nsISupports* GetParentObject() const;

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  uint32_t Length() const;

  already_AddRefed<SpeechRecognitionResult> Item(uint32_t aIndex);

  already_AddRefed<SpeechRecognitionResult> IndexedGetter(uint32_t aIndex, bool& aPresent);

  nsTArray<nsRefPtr<SpeechRecognitionResult> > mItems;
private:
  ~SpeechRecognitionResultList();

  nsRefPtr<SpeechRecognition> mParent;
};

} 
} 

#endif
