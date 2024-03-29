





#include "SpeechRecognitionAlternative.h"

#include "mozilla/dom/SpeechRecognitionAlternativeBinding.h"

#include "SpeechRecognition.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(SpeechRecognitionAlternative, mParent)
NS_IMPL_CYCLE_COLLECTING_ADDREF(SpeechRecognitionAlternative)
NS_IMPL_CYCLE_COLLECTING_RELEASE(SpeechRecognitionAlternative)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(SpeechRecognitionAlternative)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

SpeechRecognitionAlternative::SpeechRecognitionAlternative(SpeechRecognition* aParent)
  : mConfidence(0)
  , mParent(aParent)
{
}

SpeechRecognitionAlternative::~SpeechRecognitionAlternative()
{
}

JSObject*
SpeechRecognitionAlternative::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return SpeechRecognitionAlternativeBinding::Wrap(aCx, this, aGivenProto);
}

nsISupports*
SpeechRecognitionAlternative::GetParentObject() const
{
  return static_cast<DOMEventTargetHelper*>(mParent.get());
}

void
SpeechRecognitionAlternative::GetTranscript(nsString& aRetVal) const
{
  aRetVal = mTranscript;
}

float
SpeechRecognitionAlternative::Confidence() const
{
  return mConfidence;
}

} 
} 
