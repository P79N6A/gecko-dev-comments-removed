





#ifndef mozilla_dom_SpeechGrammar_h
#define mozilla_dom_SpeechGrammar_h

#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsString.h"
#include "nsWrapperCache.h"
#include "js/TypeDecls.h"

#include "mozilla/Attributes.h"
#include "mozilla/ErrorResult.h"

namespace mozilla {
namespace dom {

class GlobalObject;

class SpeechGrammar MOZ_FINAL : public nsISupports,
                                public nsWrapperCache
{
public:
  SpeechGrammar(nsISupports* aParent);
  ~SpeechGrammar();

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(SpeechGrammar)

  nsISupports* GetParentObject() const;

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  static SpeechGrammar* Constructor(const GlobalObject& aGlobal,
                                    ErrorResult& aRv);

  void GetSrc(nsString& aRetVal, ErrorResult& aRv) const;

  void SetSrc(const nsAString& aArg, ErrorResult& aRv);

  float GetWeight(ErrorResult& aRv) const;

  void SetWeight(float aArg, ErrorResult& aRv);

private:
  nsCOMPtr<nsISupports> mParent;
};

} 
} 

#endif
