





#include "SpeechGrammar.h"

#include "nsContentUtils.h"

#include "mozilla/Preferences.h"
#include "mozilla/dom/SpeechGrammarBinding.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_1(SpeechGrammar, mParent)
NS_IMPL_CYCLE_COLLECTING_ADDREF(SpeechGrammar)
NS_IMPL_CYCLE_COLLECTING_RELEASE(SpeechGrammar)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(SpeechGrammar)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

SpeechGrammar::SpeechGrammar(nsISupports* aParent)
  : mParent(aParent)
{
  SetIsDOMBinding();
}

SpeechGrammar::~SpeechGrammar()
{
}

SpeechGrammar*
SpeechGrammar::Constructor(const GlobalObject& aGlobal, ErrorResult& aRv)
{
  return new SpeechGrammar(aGlobal.Get());
}

nsISupports*
SpeechGrammar::GetParentObject() const
{
  return mParent;
}

JSObject*
SpeechGrammar::WrapObject(JSContext* aCx, JSObject* aScope)
{
  return SpeechGrammarBinding::Wrap(aCx, aScope, this);
}

void
SpeechGrammar::GetSrc(nsString& aRetVal, ErrorResult& aRv) const
{
  aRv.Throw(NS_ERROR_NOT_IMPLEMENTED);
  return;
}

void
SpeechGrammar::SetSrc(const nsAString& aArg, ErrorResult& aRv)
{
  aRv.Throw(NS_ERROR_NOT_IMPLEMENTED);
  return;
}

float
SpeechGrammar::GetWeight(ErrorResult& aRv) const
{
  aRv.Throw(NS_ERROR_NOT_IMPLEMENTED);
  return 0;
}

void
SpeechGrammar::SetWeight(float aArg, ErrorResult& aRv)
{
  aRv.Throw(NS_ERROR_NOT_IMPLEMENTED);
  return;
}

} 
} 
