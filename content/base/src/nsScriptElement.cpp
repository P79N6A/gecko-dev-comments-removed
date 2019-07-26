




#include "nsScriptElement.h"
#include "mozilla/dom/Element.h"
#include "nsContentUtils.h"
#include "nsGUIEvent.h"
#include "nsEventDispatcher.h"
#include "nsPresContext.h"
#include "nsScriptLoader.h"
#include "nsIParser.h"
#include "nsAutoPtr.h"
#include "nsGkAtoms.h"
#include "nsContentSink.h"

using namespace mozilla::dom;

NS_IMETHODIMP
nsScriptElement::ScriptAvailable(nsresult aResult,
                                 nsIScriptElement *aElement,
                                 bool aIsInline,
                                 nsIURI *aURI,
                                 int32_t aLineNo)
{
  if (!aIsInline && NS_FAILED(aResult)) {
    return FireErrorEvent();
  }
  return NS_OK;
}

 nsresult
nsScriptElement::FireErrorEvent()
{
  nsCOMPtr<nsIContent> cont =
    do_QueryInterface((nsIScriptElement*) this);

  return nsContentUtils::DispatchTrustedEvent(cont->OwnerDoc(),
                                              cont,
                                              NS_LITERAL_STRING("error"),
                                              false ,
                                              false );
}

NS_IMETHODIMP
nsScriptElement::ScriptEvaluated(nsresult aResult,
                                 nsIScriptElement *aElement,
                                 bool aIsInline)
{
  nsresult rv = NS_OK;
  if (!aIsInline) {
    nsCOMPtr<nsIContent> cont =
      do_QueryInterface((nsIScriptElement*) this);

    nsRefPtr<nsPresContext> presContext =
      nsContentUtils::GetContextForContent(cont);

    nsEventStatus status = nsEventStatus_eIgnore;
    uint32_t type = NS_SUCCEEDED(aResult) ? NS_LOAD : NS_LOAD_ERROR;
    nsEvent event(true, type);
    
    event.mFlags.mBubbles = (type != NS_LOAD);

    nsEventDispatcher::Dispatch(cont, presContext, &event, nullptr, &status);
  }

  return rv;
}

void
nsScriptElement::CharacterDataChanged(nsIDocument *aDocument,
                                      nsIContent* aContent,
                                      CharacterDataChangeInfo* aInfo)
{
  MaybeProcessScript();
}

void
nsScriptElement::AttributeChanged(nsIDocument* aDocument,
                                  Element* aElement,
                                  int32_t aNameSpaceID,
                                  nsIAtom* aAttribute,
                                  int32_t aModType)
{
  MaybeProcessScript();
}

void
nsScriptElement::ContentAppended(nsIDocument* aDocument,
                                 nsIContent* aContainer,
                                 nsIContent* aFirstNewContent,
                                 int32_t aNewIndexInContainer)
{
  MaybeProcessScript();
}

void
nsScriptElement::ContentInserted(nsIDocument *aDocument,
                                 nsIContent* aContainer,
                                 nsIContent* aChild,
                                 int32_t aIndexInContainer)
{
  MaybeProcessScript();
}

bool
nsScriptElement::MaybeProcessScript()
{
  nsCOMPtr<nsIContent> cont =
    do_QueryInterface((nsIScriptElement*) this);

  NS_ASSERTION(cont->DebugGetSlots()->mMutationObservers.Contains(this),
               "You forgot to add self as observer");

  if (mAlreadyStarted || !mDoneAddingChildren || !cont->IsInDoc() ||
      mMalformed || !HasScriptContent()) {
    return false;
  }

  FreezeUriAsyncDefer();

  mAlreadyStarted = true;

  nsIDocument* ownerDoc = cont->OwnerDoc();
  nsCOMPtr<nsIParser> parser = ((nsIScriptElement*) this)->GetCreatorParser();
  if (parser) {
    nsCOMPtr<nsIContentSink> sink = parser->GetContentSink();
    if (sink) {
      nsCOMPtr<nsIDocument> parserDoc = do_QueryInterface(sink->GetTarget());
      if (ownerDoc != parserDoc) {
        
        return false;
      }
    }
  }

  nsRefPtr<nsScriptLoader> loader = ownerDoc->ScriptLoader();
  return loader->ProcessScriptElement(this);
}
