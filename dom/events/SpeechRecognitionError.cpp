




#include "SpeechRecognitionError.h"

namespace mozilla {
namespace dom {

SpeechRecognitionError::SpeechRecognitionError(
                          mozilla::dom::EventTarget* aOwner,
                          nsPresContext* aPresContext,
                          WidgetEvent* aEvent)
  : Event(aOwner, aPresContext, aEvent)
  , mError()
{
}

SpeechRecognitionError::~SpeechRecognitionError() {}

already_AddRefed<SpeechRecognitionError>
SpeechRecognitionError::Constructor(const GlobalObject& aGlobal,
                                    const nsAString& aType,
                                    const SpeechRecognitionErrorInit& aParam,
                                    ErrorResult& aRv)
{
  nsCOMPtr<mozilla::dom::EventTarget> t = do_QueryInterface(aGlobal.GetAsSupports());
  nsRefPtr<SpeechRecognitionError> e = new SpeechRecognitionError(t, nullptr, nullptr);
  bool trusted = e->Init(t);
  e->InitSpeechRecognitionError(aType, aParam.mBubbles, aParam.mCancelable, aParam.mError, aParam.mMessage, aRv);
  e->SetTrusted(trusted);
  return e.forget();
}

void
SpeechRecognitionError::InitSpeechRecognitionError(const nsAString& aType,
                                                   bool aCanBubble,
                                                   bool aCancelable,
                                                   SpeechRecognitionErrorCode aError,
                                                   const nsAString& aMessage,
                                                   ErrorResult& aRv)
{
  aRv = Event::InitEvent(aType, aCanBubble, aCancelable);
  NS_ENSURE_TRUE_VOID(!aRv.Failed());

  mError = aError;
  mMessage = aMessage;
  return;
}

} 
} 
