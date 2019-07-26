





#pragma once

#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsWrapperCache.h"
#include "js/TypeDecls.h"

#include "mozilla/Attributes.h"
#include "mozilla/dom/BindingUtils.h"

#include "EnableWebSpeechRecognitionCheck.h"
#include "SpeechGrammar.h"

namespace mozilla {
namespace dom {

class GlobalObject;
class SpeechGrammarList MOZ_FINAL : public nsISupports,
                                    public nsWrapperCache,
                                    public EnableWebSpeechRecognitionCheck
{
public:
  SpeechGrammarList(nsISupports* aParent);
  ~SpeechGrammarList();

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(SpeechGrammarList)

  SpeechGrammarList* Constructor(const GlobalObject& aGlobal,
                                 ErrorResult& aRv);

  nsISupports* GetParentObject() const;

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  uint32_t Length() const;

  already_AddRefed<SpeechGrammar> Item(uint32_t aIndex, ErrorResult& aRv);

  void AddFromURI(const nsAString& aSrc, const Optional<float>& aWeight, ErrorResult& aRv);

  void AddFromString(const nsAString& aString, const Optional<float>& aWeight, ErrorResult& aRv);

  already_AddRefed<SpeechGrammar> IndexedGetter(uint32_t aIndex, bool& aPresent, ErrorResult& aRv);

private:
  nsCOMPtr<nsISupports> mParent;
};

} 
} 
