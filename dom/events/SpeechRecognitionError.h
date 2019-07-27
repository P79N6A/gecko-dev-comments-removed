





#ifndef SpeechRecognitionError_h__
#define SpeechRecognitionError_h__

#include "mozilla/dom/Event.h"
#include "mozilla/dom/SpeechRecognitionErrorBinding.h"

namespace mozilla {
namespace dom {

class SpeechRecognitionError : public Event
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

  virtual JSObject* WrapObjectInternal(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override
  {
    return mozilla::dom::SpeechRecognitionErrorBinding::Wrap(aCx, this, aGivenProto);
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
