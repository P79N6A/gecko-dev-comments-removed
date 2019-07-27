





#ifndef mozilla_dom_SpeechGrammarList_h
#define mozilla_dom_SpeechGrammarList_h

#include "mozilla/Attributes.h"
#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsWrapperCache.h"
#include "nsISpeechRecognitionService.h"

struct JSContext;

namespace mozilla {

class ErrorResult;

namespace dom {

class GlobalObject;
class SpeechGrammar;
template<typename> class Optional;

class SpeechGrammarList MOZ_FINAL : public nsISupports,
                                    public nsWrapperCache
{
public:
  explicit SpeechGrammarList(nsISupports* aParent, nsISpeechRecognitionService* aRecognitionService);

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(SpeechGrammarList)

  static already_AddRefed<SpeechGrammarList> Constructor(const GlobalObject& aGlobal, ErrorResult& aRv);

  nsISupports* GetParentObject() const;

  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) MOZ_OVERRIDE;

  uint32_t Length() const;

  already_AddRefed<SpeechGrammar> Item(uint32_t aIndex, ErrorResult& aRv);

  void AddFromURI(const nsAString& aSrc, const Optional<float>& aWeight, ErrorResult& aRv);

  void AddFromString(const nsAString& aString, const Optional<float>& aWeight, ErrorResult& aRv);

  already_AddRefed<SpeechGrammar> IndexedGetter(uint32_t aIndex, bool& aPresent, ErrorResult& aRv);

  nsCOMPtr<nsISpeechRecognitionService> mRecognitionService;

private:
  ~SpeechGrammarList();

  nsCOMPtr<nsISupports> mParent;
};

} 
} 

#endif
