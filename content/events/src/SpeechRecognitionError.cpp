




#include "nsDOMClassInfoID.h"
#include "SpeechRecognitionError.h"

namespace mozilla {
namespace dom {

SpeechRecognitionError::SpeechRecognitionError(mozilla::dom::EventTarget* aOwner, nsPresContext* aPresContext, nsEvent* aEvent)
: nsDOMEvent(aOwner, aPresContext, aEvent),
  mError()
{}

SpeechRecognitionError::~SpeechRecognitionError() {}

already_AddRefed<SpeechRecognitionError>
SpeechRecognitionError::Constructor(const GlobalObject& aGlobal,
                                    const nsAString& aType,
                                    const SpeechRecognitionErrorInit& aParam,
                                    ErrorResult& aRv)
{
  nsCOMPtr<mozilla::dom::EventTarget> t = do_QueryInterface(aGlobal.Get());
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
  aRv = nsDOMEvent::InitEvent(aType, aCanBubble, aCancelable);
  NS_ENSURE_SUCCESS_VOID(aRv.ErrorCode());

  mError = aError;
  mMessage = aMessage;
  return;
}

} 
} 
