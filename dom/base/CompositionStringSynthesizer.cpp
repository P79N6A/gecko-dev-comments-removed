




#include "CompositionStringSynthesizer.h"
#include "nsContentUtils.h"
#include "nsIDocShell.h"
#include "nsIFrame.h"
#include "nsIPresShell.h"
#include "nsIWidget.h"
#include "nsPIDOMWindow.h"
#include "nsView.h"
#include "mozilla/TextEvents.h"

namespace mozilla {
namespace dom {

NS_IMPL_ISUPPORTS(CompositionStringSynthesizer,
                  nsICompositionStringSynthesizer)

CompositionStringSynthesizer::CompositionStringSynthesizer(
                                TextEventDispatcher* aDispatcher)
  : mDispatcher(aDispatcher)
{
}

CompositionStringSynthesizer::~CompositionStringSynthesizer()
{
}

NS_IMETHODIMP
CompositionStringSynthesizer::SetString(const nsAString& aString)
{
  MOZ_RELEASE_ASSERT(nsContentUtils::IsCallerChrome());
  return mDispatcher->SetPendingCompositionString(aString);
}

NS_IMETHODIMP
CompositionStringSynthesizer::AppendClause(uint32_t aLength,
                                           uint32_t aAttribute)
{
  MOZ_RELEASE_ASSERT(nsContentUtils::IsCallerChrome());
  return mDispatcher->AppendClauseToPendingComposition(aLength, aAttribute);
}

NS_IMETHODIMP
CompositionStringSynthesizer::SetCaret(uint32_t aOffset, uint32_t aLength)
{
  MOZ_RELEASE_ASSERT(nsContentUtils::IsCallerChrome());
  return mDispatcher->SetCaretInPendingComposition(aOffset, aLength);
}

NS_IMETHODIMP
CompositionStringSynthesizer::DispatchEvent(bool* aDefaultPrevented)
{
  MOZ_RELEASE_ASSERT(nsContentUtils::IsCallerChrome());
  NS_ENSURE_ARG_POINTER(aDefaultPrevented);
  nsEventStatus status = nsEventStatus_eIgnore;
  nsresult rv = mDispatcher->FlushPendingComposition(status);
  *aDefaultPrevented = (status == nsEventStatus_eConsumeNoDefault);
  return rv;
}

} 
} 
