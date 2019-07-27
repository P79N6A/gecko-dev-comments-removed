



#include "mozilla/Attributes.h"
#include "mozilla/dom/Element.h"
#include "mozilla/mozalloc.h"
#include "nsAString.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsComputedDOMStyle.h"
#include "nsDebug.h"
#include "nsError.h"
#include "nsGkAtoms.h"
#include "nsHTMLCSSUtils.h"
#include "nsHTMLEditor.h"
#include "nsIAtom.h"
#include "nsIContent.h"
#include "nsID.h"
#include "nsIDOMCSSPrimitiveValue.h"
#include "nsIDOMCSSStyleDeclaration.h"
#include "nsIDOMCSSValue.h"
#include "nsIDOMElement.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMHTMLElement.h"
#include "nsIDOMNode.h"
#include "nsIDOMWindow.h"
#include "nsIDocument.h"
#include "nsIDocumentObserver.h"
#include "nsIHTMLAbsPosEditor.h"
#include "nsIHTMLEditor.h"
#include "nsIHTMLInlineTableEditor.h"
#include "nsIHTMLObjectResizer.h"
#include "nsIMutationObserver.h"
#include "nsINode.h"
#include "nsIPresShell.h"
#include "nsISupportsImpl.h"
#include "nsISupportsUtils.h"
#include "nsLiteralString.h"
#include "nsPresContext.h"
#include "nsReadableUtils.h"
#include "nsString.h"
#include "nsStringFwd.h"
#include "nsUnicharUtils.h"
#include "nscore.h"
#include "nsContentUtils.h" 

class nsIDOMEventListener;
class nsISelection;

using namespace mozilla;
using namespace mozilla::dom;


static int32_t GetCSSFloatValue(nsIDOMCSSStyleDeclaration * aDecl,
                                const nsAString & aProperty)
{
  MOZ_ASSERT(aDecl);

  nsCOMPtr<nsIDOMCSSValue> value;
  
  nsresult res = aDecl->GetPropertyCSSValue(aProperty, getter_AddRefs(value));
  if (NS_FAILED(res) || !value) return 0;

  
  
  nsCOMPtr<nsIDOMCSSPrimitiveValue> val = do_QueryInterface(value);
  uint16_t type;
  val->GetPrimitiveType(&type);

  float f = 0;
  switch (type) {
    case nsIDOMCSSPrimitiveValue::CSS_PX:
      
      res = val->GetFloatValue(nsIDOMCSSPrimitiveValue::CSS_PX, &f);
      NS_ENSURE_SUCCESS(res, 0);
      break;
    case nsIDOMCSSPrimitiveValue::CSS_IDENT: {
      
      
      nsAutoString str;
      res = val->GetStringValue(str);
      if (str.EqualsLiteral("thin"))
        f = 1;
      else if (str.EqualsLiteral("medium"))
        f = 3;
      else if (str.EqualsLiteral("thick"))
        f = 5;
      break;
    }
  }

  return (int32_t) f;
}

class nsElementDeletionObserver final : public nsIMutationObserver
{
public:
  nsElementDeletionObserver(nsINode* aNativeAnonNode, nsINode* aObservedNode)
  : mNativeAnonNode(aNativeAnonNode), mObservedNode(aObservedNode) {}
  NS_DECL_ISUPPORTS
  NS_DECL_NSIMUTATIONOBSERVER
protected:
  ~nsElementDeletionObserver() {}
  nsINode* mNativeAnonNode;
  nsINode* mObservedNode;
};

NS_IMPL_ISUPPORTS(nsElementDeletionObserver, nsIMutationObserver)
NS_IMPL_NSIMUTATIONOBSERVER_CONTENT(nsElementDeletionObserver)

void
nsElementDeletionObserver::NodeWillBeDestroyed(const nsINode* aNode)
{
  NS_ASSERTION(aNode == mNativeAnonNode || aNode == mObservedNode,
               "Wrong aNode!");
  if (aNode == mNativeAnonNode) {
    mObservedNode->RemoveMutationObserver(this);
  } else {
    mNativeAnonNode->RemoveMutationObserver(this);
    static_cast<nsIContent*>(mNativeAnonNode)->UnbindFromTree();
  }

  NS_RELEASE_THIS();
}





nsresult
nsHTMLEditor::CreateAnonymousElement(const nsAString & aTag, nsIDOMNode *  aParentNode,
                                     const nsAString & aAnonClass, bool aIsCreatedHidden,
                                     nsIDOMElement ** aReturn)
{
  NS_ENSURE_ARG_POINTER(aParentNode);
  NS_ENSURE_ARG_POINTER(aReturn);
  *aReturn = nullptr;

  nsCOMPtr<nsIContent> parentContent( do_QueryInterface(aParentNode) );
  NS_ENSURE_TRUE(parentContent, NS_OK);

  nsCOMPtr<nsIDocument> doc = GetDocument();
  NS_ENSURE_TRUE(doc, NS_ERROR_NULL_POINTER);

  
  nsCOMPtr<nsIPresShell> ps = GetPresShell();
  NS_ENSURE_TRUE(ps, NS_ERROR_NOT_INITIALIZED);

  
  nsCOMPtr<Element> newContent =
    CreateHTMLContent(nsCOMPtr<nsIAtom>(do_GetAtom(aTag)));
  NS_ENSURE_STATE(newContent);

  nsCOMPtr<nsIDOMElement> newElement = do_QueryInterface(newContent);
  NS_ENSURE_TRUE(newElement, NS_ERROR_FAILURE);

  
  nsresult res;
  if (aIsCreatedHidden) {
    res = newElement->SetAttribute(NS_LITERAL_STRING("class"),
                                   NS_LITERAL_STRING("hidden"));
    NS_ENSURE_SUCCESS(res, res);
  }

  
  if (!aAnonClass.IsEmpty()) {
    res = newElement->SetAttribute(NS_LITERAL_STRING("_moz_anonclass"),
                                   aAnonClass);
    NS_ENSURE_SUCCESS(res, res);
  }

  {
    nsAutoScriptBlocker scriptBlocker;

    
    newContent->SetIsNativeAnonymousRoot();
    res = newContent->BindToTree(doc, parentContent, parentContent, true);
    if (NS_FAILED(res)) {
      newContent->UnbindFromTree();
      return res;
    }
  }

  nsElementDeletionObserver* observer =
    new nsElementDeletionObserver(newContent, parentContent);
  NS_ADDREF(observer); 
  parentContent->AddMutationObserver(observer);
  newContent->AddMutationObserver(observer);

  
  ps->RecreateFramesFor(newContent);

  newElement.forget(aReturn);
  return NS_OK;
}


void
nsHTMLEditor::RemoveListenerAndDeleteRef(const nsAString& aEvent,
                                         nsIDOMEventListener* aListener,
                                         bool aUseCapture,
                                         Element* aElement,
                                         nsIContent * aParentContent,
                                         nsIPresShell* aShell)
{
  nsCOMPtr<nsIDOMEventTarget> evtTarget(do_QueryInterface(aElement));
  if (evtTarget) {
    evtTarget->RemoveEventListener(aEvent, aListener, aUseCapture);
  }
  DeleteRefToAnonymousNode(static_cast<nsIDOMElement*>(GetAsDOMNode(aElement)), aParentContent, aShell);
}


void
nsHTMLEditor::DeleteRefToAnonymousNode(nsIDOMElement* aElement,
                                       nsIContent* aParentContent,
                                       nsIPresShell* aShell)
{
  
  
  

  if (aElement) {
    nsCOMPtr<nsIContent> content = do_QueryInterface(aElement);
    if (content) {
      nsAutoScriptBlocker scriptBlocker;
      
      
      
      if (aShell && aShell->GetPresContext() &&
          aShell->GetPresContext()->GetPresShell() == aShell) {
        nsCOMPtr<nsIDocumentObserver> docObserver = do_QueryInterface(aShell);
        if (docObserver) {
          
          
          nsCOMPtr<nsIDocument> document = GetDocument();
          if (document)
            docObserver->BeginUpdate(document, UPDATE_CONTENT_MODEL);

          
          
          
          docObserver->ContentRemoved(content->GetCurrentDoc(),
                                      aParentContent, content, -1,
                                      content->GetPreviousSibling());
          if (document)
            docObserver->EndUpdate(document, UPDATE_CONTENT_MODEL);
        }
      }
      content->UnbindFromTree();
    }
  }
}





NS_IMETHODIMP
nsHTMLEditor::CheckSelectionStateForAnonymousButtons(nsISelection * aSelection)
{
  NS_ENSURE_ARG_POINTER(aSelection);

  
  NS_ENSURE_TRUE(mIsObjectResizingEnabled ||
      mIsAbsolutelyPositioningEnabled ||
      mIsInlineTableEditingEnabled, NS_OK);

  
  if (mIsMoving) {
    return NS_OK;
  }

  nsCOMPtr<nsIDOMElement> focusElement;
  
  nsresult res  = GetSelectionContainer(getter_AddRefs(focusElement));
  NS_ENSURE_TRUE(focusElement, NS_OK);
  NS_ENSURE_SUCCESS(res, res);

  
  nsCOMPtr<dom::Element> focusElementNode = do_QueryInterface(focusElement);
  NS_ENSURE_STATE(focusElementNode);
  if (!focusElementNode->IsInDoc()) {
    return NS_OK;
  }

  
  nsAutoString focusTagName;
  res = focusElement->GetTagName(focusTagName);
  NS_ENSURE_SUCCESS(res, res);
  ToLowerCase(focusTagName);
  nsCOMPtr<nsIAtom> focusTagAtom = do_GetAtom(focusTagName);

  nsCOMPtr<nsIDOMElement> absPosElement;
  if (mIsAbsolutelyPositioningEnabled) {
    
    
    res = GetAbsolutelyPositionedSelectionContainer(getter_AddRefs(absPosElement));
    NS_ENSURE_SUCCESS(res, res);
  }

  nsCOMPtr<nsIDOMElement> cellElement;
  if (mIsObjectResizingEnabled || mIsInlineTableEditingEnabled) {
    
    
    res = GetElementOrParentByTagName(NS_LITERAL_STRING("td"),
                                      nullptr,
                                      getter_AddRefs(cellElement));
    NS_ENSURE_SUCCESS(res, res);
  }

  if (mIsObjectResizingEnabled && cellElement) {
    
    

    
    if (nsGkAtoms::img != focusTagAtom) {
      
      
      nsCOMPtr<nsIDOMNode> tableNode = GetEnclosingTable(cellElement);
      focusElement = do_QueryInterface(tableNode);
      focusTagAtom = nsGkAtoms::table;
    }
  }

  
  
  if (nsGkAtoms::img != focusTagAtom && nsGkAtoms::table != focusTagAtom) {
    focusElement = absPosElement;
  }

  
  
  

  
  
  

  if (mIsAbsolutelyPositioningEnabled && mAbsolutelyPositionedObject &&
      absPosElement != GetAsDOMNode(mAbsolutelyPositionedObject)) {
    res = HideGrabber();
    NS_ENSURE_SUCCESS(res, res);
    NS_ASSERTION(!mAbsolutelyPositionedObject, "HideGrabber failed");
  }

  if (mIsObjectResizingEnabled && mResizedObject &&
      GetAsDOMNode(mResizedObject) != focusElement) {
    res = HideResizers();
    NS_ENSURE_SUCCESS(res, res);
    NS_ASSERTION(!mResizedObject, "HideResizers failed");
  }

  if (mIsInlineTableEditingEnabled && mInlineEditedCell &&
      mInlineEditedCell != cellElement) {
    res = HideInlineTableEditingUI();
    NS_ENSURE_SUCCESS(res, res);
    NS_ASSERTION(!mInlineEditedCell, "HideInlineTableEditingUI failed");
  }

  
  nsIContent* hostContent = GetActiveEditingHost();
  nsCOMPtr<nsIDOMNode> hostNode = do_QueryInterface(hostContent);

  if (mIsObjectResizingEnabled && focusElement &&
      IsModifiableNode(focusElement) && focusElement != hostNode) {
    if (nsGkAtoms::img == focusTagAtom) {
      mResizedObjectIsAnImage = true;
    }
    if (mResizedObject)
      res = RefreshResizers();
    else
      res = ShowResizers(focusElement);
    NS_ENSURE_SUCCESS(res, res);
  }

  if (mIsAbsolutelyPositioningEnabled && absPosElement &&
      IsModifiableNode(absPosElement) && absPosElement != hostNode) {
    if (mAbsolutelyPositionedObject)
      res = RefreshGrabber();
    else
      res = ShowGrabberOnElement(absPosElement);
    NS_ENSURE_SUCCESS(res, res);
  }

  if (mIsInlineTableEditingEnabled && cellElement &&
      IsModifiableNode(cellElement) && cellElement != hostNode) {
    if (mInlineEditedCell)
      res = RefreshInlineTableEditingUI();
    else
      res = ShowInlineTableEditingUI(cellElement);
  }

  return res;
}



nsresult
nsHTMLEditor::GetPositionAndDimensions(nsIDOMElement * aElement,
                                       int32_t & aX, int32_t & aY,
                                       int32_t & aW, int32_t & aH,
                                       int32_t & aBorderLeft,
                                       int32_t & aBorderTop,
                                       int32_t & aMarginLeft,
                                       int32_t & aMarginTop)
{
  nsCOMPtr<Element> element = do_QueryInterface(aElement);
  NS_ENSURE_ARG_POINTER(element);

  
  bool isPositioned = false;
  nsresult res = aElement->HasAttribute(NS_LITERAL_STRING("_moz_abspos"), &isPositioned);
  NS_ENSURE_SUCCESS(res, res);
  if (!isPositioned) {
    
    nsAutoString positionStr;
    mHTMLCSSUtils->GetComputedProperty(*element, *nsGkAtoms::position,
                                       positionStr);
    isPositioned = positionStr.EqualsLiteral("absolute");
  }

  if (isPositioned) {
    
    mResizedObjectIsAbsolutelyPositioned = true;

    
    nsRefPtr<nsComputedDOMStyle> cssDecl =
      mHTMLCSSUtils->GetComputedStyle(element);
    NS_ENSURE_STATE(cssDecl);

    aBorderLeft = GetCSSFloatValue(cssDecl, NS_LITERAL_STRING("border-left-width"));
    aBorderTop  = GetCSSFloatValue(cssDecl, NS_LITERAL_STRING("border-top-width"));
    aMarginLeft = GetCSSFloatValue(cssDecl, NS_LITERAL_STRING("margin-left"));
    aMarginTop  = GetCSSFloatValue(cssDecl, NS_LITERAL_STRING("margin-top"));

    aX = GetCSSFloatValue(cssDecl, NS_LITERAL_STRING("left")) +
         aMarginLeft + aBorderLeft;
    aY = GetCSSFloatValue(cssDecl, NS_LITERAL_STRING("top")) +
         aMarginTop + aBorderTop;
    aW = GetCSSFloatValue(cssDecl, NS_LITERAL_STRING("width"));
    aH = GetCSSFloatValue(cssDecl, NS_LITERAL_STRING("height"));
  }
  else {
    mResizedObjectIsAbsolutelyPositioned = false;
    nsCOMPtr<nsIDOMHTMLElement> htmlElement = do_QueryInterface(aElement);
    if (!htmlElement) {
      return NS_ERROR_NULL_POINTER;
    }
    GetElementOrigin(aElement, aX, aY);

    res = htmlElement->GetOffsetWidth(&aW);
    NS_ENSURE_SUCCESS(res, res);
    res = htmlElement->GetOffsetHeight(&aH);

    aBorderLeft = 0;
    aBorderTop  = 0;
    aMarginLeft = 0;
    aMarginTop = 0;
  }
  return res;
}


void
nsHTMLEditor::SetAnonymousElementPosition(int32_t aX, int32_t aY, nsIDOMElement *aElement)
{
  mHTMLCSSUtils->SetCSSPropertyPixels(aElement, NS_LITERAL_STRING("left"), aX);
  mHTMLCSSUtils->SetCSSPropertyPixels(aElement, NS_LITERAL_STRING("top"), aY);
}
