





































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

using namespace mozilla::dom;

NS_IMETHODIMP
nsScriptElement::ScriptAvailable(nsresult aResult,
                                 nsIScriptElement *aElement,
                                 PRBool aIsInline,
                                 nsIURI *aURI,
                                 PRInt32 aLineNo)
{
  if (!aIsInline && NS_FAILED(aResult)) {
    nsCOMPtr<nsIContent> cont =
      do_QueryInterface((nsIScriptElement*) this);

    nsRefPtr<nsPresContext> presContext =
      nsContentUtils::GetContextForContent(cont);

    nsEventStatus status = nsEventStatus_eIgnore;
    nsScriptErrorEvent event(PR_TRUE, NS_LOAD_ERROR);

    event.lineNr = aLineNo;

    NS_NAMED_LITERAL_STRING(errorString, "Error loading script");
    event.errorMsg = errorString.get();

    nsCAutoString spec;
    aURI->GetSpec(spec);

    NS_ConvertUTF8toUTF16 fileName(spec);
    event.fileName = fileName.get();

    nsEventDispatcher::Dispatch(cont, presContext, &event, nsnull, &status);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsScriptElement::ScriptEvaluated(nsresult aResult,
                                 nsIScriptElement *aElement,
                                 PRBool aIsInline)
{
  nsresult rv = NS_OK;
  if (!aIsInline) {
    nsCOMPtr<nsIContent> cont =
      do_QueryInterface((nsIScriptElement*) this);

    nsRefPtr<nsPresContext> presContext =
      nsContentUtils::GetContextForContent(cont);

    nsEventStatus status = nsEventStatus_eIgnore;
    PRUint32 type = NS_SUCCEEDED(aResult) ? NS_LOAD : NS_LOAD_ERROR;
    nsEvent event(PR_TRUE, type);
    if (type == NS_LOAD) {
      
      event.flags |= NS_EVENT_FLAG_CANT_BUBBLE;
    }

    nsEventDispatcher::Dispatch(cont, presContext, &event, nsnull, &status);
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
                                  PRInt32 aNameSpaceID,
                                  nsIAtom* aAttribute,
                                  PRInt32 aModType)
{
  MaybeProcessScript();
}

void
nsScriptElement::ContentAppended(nsIDocument* aDocument,
                                 nsIContent* aContainer,
                                 nsIContent* aFirstNewContent,
                                 PRInt32 aNewIndexInContainer)
{
  MaybeProcessScript();
}

void
nsScriptElement::ContentInserted(nsIDocument *aDocument,
                                 nsIContent* aContainer,
                                 nsIContent* aChild,
                                 PRInt32 aIndexInContainer)
{
  MaybeProcessScript();
}

static PRBool
InNonScriptingContainer(nsIContent* aNode)
{
  aNode = aNode->GetParent();
  while (aNode) {
    
    
    
    
    if (aNode->IsHTML()) {
      nsIAtom *localName = aNode->Tag();
      if (localName == nsGkAtoms::iframe ||
          localName == nsGkAtoms::noframes ||
          localName == nsGkAtoms::noembed) {
        return PR_TRUE;
      }
    }
    aNode = aNode->GetParent();
  }

  return PR_FALSE;
}

nsresult
nsScriptElement::MaybeProcessScript()
{
  nsCOMPtr<nsIContent> cont =
    do_QueryInterface((nsIScriptElement*) this);

  NS_ASSERTION(cont->DebugGetSlots()->mMutationObservers.Contains(this),
               "You forgot to add self as observer");

  if (mIsEvaluated || !mDoneAddingChildren || !cont->IsInDoc() ||
      mMalformed || !HasScriptContent()) {
    return NS_OK;
  }

  FreezeUriAsyncDefer();

  if (InNonScriptingContainer(cont)) {
    
    mIsEvaluated = PR_TRUE;
    return NS_OK;
  }

  nsresult scriptresult = NS_OK;
  nsRefPtr<nsScriptLoader> loader = cont->GetOwnerDoc()->ScriptLoader();
  mIsEvaluated = PR_TRUE;
  scriptresult = loader->ProcessScriptElement(this);

  
  
  
  if (NS_FAILED(scriptresult) &&
      scriptresult != NS_ERROR_HTMLPARSER_BLOCK) {
    scriptresult = NS_OK;
  }

  return scriptresult;
}
