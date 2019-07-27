




#include "inLayoutUtils.h"

#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsIContent.h"
#include "nsIContentViewer.h"
#include "nsPIDOMWindow.h"
#include "nsIDocShell.h"
#include "nsIPresShell.h"
#include "nsPresContext.h"
#include "mozilla/EventStateManager.h"
#include "mozilla/dom/Element.h"

using namespace mozilla;



EventStateManager*
inLayoutUtils::GetEventStateManagerFor(nsIDOMElement *aElement)
{
  NS_PRECONDITION(aElement, "Passing in a null element is bad");

  nsCOMPtr<nsIDOMDocument> domDoc;
  aElement->GetOwnerDocument(getter_AddRefs(domDoc));
  nsCOMPtr<nsIDocument> doc = do_QueryInterface(domDoc);

  if (!doc) {
    NS_WARNING("Could not get an nsIDocument!");
    return nullptr;
  }

  nsIPresShell *shell = doc->GetShell();
  if (!shell)
    return nullptr;

  return shell->GetPresContext()->EventStateManager();
}

nsIDOMDocument*
inLayoutUtils::GetSubDocumentFor(nsIDOMNode* aNode)
{
  nsCOMPtr<nsIContent> content = do_QueryInterface(aNode);
  if (content) {
    nsCOMPtr<nsIDocument> doc = content->GetComposedDoc();
    if (doc) {
      nsCOMPtr<nsIDOMDocument> domdoc(do_QueryInterface(doc->GetSubDocumentFor(content)));

      return domdoc;
    }
  }

  return nullptr;
}

nsIDOMNode*
inLayoutUtils::GetContainerFor(const nsIDocument& aDoc)
{
  nsPIDOMWindow* pwin = aDoc.GetWindow();
  if (!pwin) {
    return nullptr;
  }

  nsCOMPtr<nsIDOMNode> node = do_QueryInterface(pwin->GetFrameElementInternal());
  return node;
}

