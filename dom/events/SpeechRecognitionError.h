




#ifndef SpeechRecognitionError_h__
#define SpeechRecognitionError_h__

#include "nsDOMEvent.h"
#include "mozilla/dom/SpeechRecognitionErrorBinding.h"

namespace mozilla {
namespace dom {

class SpeechRecognitionError : public nsDOMEvent
{
public:
  SpeechRecognitionError(mozilla::dom::EventTarget* aOwner,
                         nsPresContext* aPresContext,
                         WidgetEvent* aEvent);
  virtual ~SpeechRecognitionError();

  static already_AddRefed<SpeechRecognitionError>
  Constructor(const GlobalObject& aGlobal,
              const nsAString& aType,
              const SpeechRecognitionErrorInit& aParam,
              ErrorResult& aRv);

  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope) MOZ_OVERRIDE
  {
    return mozilla::dom::SpeechRecognitionErrorBinding::Wrap(aCx, aScope, this);
  }

  void
  GetMessage(nsAString& aString)
  {
    aString = mMessage;
  }

  SpeechRecognitionErrorCode
  Error()
  {
    return mError;
  }

  void
  InitSpeechRecognitionError(const nsAString& aType,
                             bool aCanBubble,
                             bool aCancelable,
                             SpeechRecognitionErrorCode aError,
                             const nsAString& aMessage,
                             ErrorResult& aRv);

protected:
  SpeechRecognitionErrorCode mError;
  nsString mMessage;
};

}
}

#endif 
