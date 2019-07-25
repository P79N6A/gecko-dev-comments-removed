




































#include "nsHTMLEditor.h"

#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIEditor.h"
#include "nsIPresShell.h"
#include "nsPresContext.h"

#include "nsISelection.h"

#include "nsTextEditUtils.h"
#include "nsEditorUtils.h"
#include "nsHTMLEditUtils.h"
#include "nsTextEditRules.h"

#include "nsIDOMHTMLElement.h"
#include "nsIDOMNSHTMLElement.h"
#include "nsIDOMEventTarget.h"

#include "nsIDOMCSSValue.h"
#include "nsIDOMCSSPrimitiveValue.h"
#include "nsIDOMCSSStyleDeclaration.h"
#include "nsIMutationObserver.h"
#include "nsUnicharUtils.h"
#include "nsContentUtils.h"


static PRInt32 GetCSSFloatValue(nsIDOMCSSStyleDeclaration * aDecl,
                                const nsAString & aProperty)
{
  NS_ENSURE_ARG_POINTER(aDecl);

  nsCOMPtr<nsIDOMCSSValue> value;
  
  nsresult res = aDecl->GetPropertyCSSValue(aProperty, getter_AddRefs(value));
  if (NS_FAILED(res) || !value) return 0;

  
  
  nsCOMPtr<nsIDOMCSSPrimitiveValue> val = do_QueryInterface(value);
  PRUint16 type;
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

  return (PRInt32) f;
}

class nsElementDeletionObserver : public nsIMutationObserver
{
public:
  nsElementDeletionObserver(nsINode* aNativeAnonNode, nsINode* aObservedNode)
  : mNativeAnonNode(aNativeAnonNode), mObservedNode(aObservedNode) {}
  NS_DECL_ISUPPORTS
  NS_DECL_NSIMUTATIONOBSERVER
protected:
  nsINode* mNativeAnonNode;
  nsINode* mObservedNode;
};

NS_IMPL_ISUPPORTS1(nsElementDeletionObserver, nsIMutationObserver)
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
                                     const nsAString & aAnonClass, PRBool aIsCreatedHidden,
                                     nsIDOMElement ** aReturn)
{
  NS_ENSURE_ARG_POINTER(aParentNode);
  NS_ENSURE_ARG_POINTER(aReturn);
  *aReturn = nsnull;

  nsCOMPtr<nsIContent> parentContent( do_QueryInterface(aParentNode) );
  NS_ENSURE_TRUE(parentContent, NS_OK);

  
  nsCOMPtr<nsIDOMDocument> domDoc;
  GetDocument(getter_AddRefs(domDoc));
  nsCOMPtr<nsIDocument> doc = do_QueryInterface(domDoc);
  NS_ENSURE_TRUE(doc, NS_ERROR_NULL_POINTER);

  
  nsCOMPtr<nsIPresShell> ps;
  GetPresShell(getter_AddRefs(ps));
  NS_ENSURE_TRUE(ps, NS_ERROR_NOT_INITIALIZED);

  
  nsCOMPtr<nsIContent> newContent;
  nsresult res = CreateHTMLContent(aTag, getter_AddRefs(newContent));
  NS_ENSURE_SUCCESS(res, res);

  nsCOMPtr<nsIDOMElement> newElement = do_QueryInterface(newContent);
  NS_ENSURE_TRUE(newElement, NS_ERROR_FAILURE);

  
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

    
    newContent->SetNativeAnonymous();
    res = newContent->BindToTree(doc, parentContent, parentContent, PR_TRUE);
    if (NS_FAILED(res)) {
      newContent->UnbindFromTree();
      return res;
    }
  }

  nsElementDeletionObserver* observer =
    new nsElementDeletionObserver(newContent, parentContent);
  if (!observer) {
    newContent->UnbindFromTree();
    return NS_ERROR_OUT_OF_MEMORY;
  }
  NS_ADDREF(observer); 
  parentContent->AddMutationObserver(observer);
  newContent->AddMutationObserver(observer);

  
  ps->RecreateFramesFor(newContent);

  *aReturn = newElement;
  NS_IF_ADDREF(*aReturn);
  return NS_OK;
}


void
nsHTMLEditor::RemoveListenerAndDeleteRef(const nsAString& aEvent,
                                         nsIDOMEventListener* aListener,
                                         PRBool aUseCapture,
                                         nsIDOMElement* aElement,
                                         nsIContent * aParentContent,
                                         nsIPresShell* aShell)
{
  nsCOMPtr<nsIDOMEventTarget> evtTarget(do_QueryInterface(aElement));
  if (evtTarget) {
    evtTarget->RemoveEventListener(aEvent, aListener, aUseCapture);
  }
  DeleteRefToAnonymousNode(aElement, aParentContent, aShell);
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
          
          
          nsCOMPtr<nsIDOMDocument> domDocument;
          GetDocument(getter_AddRefs(domDocument));
          nsCOMPtr<nsIDocument> document = do_QueryInterface(domDocument);
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
                                      nsnull,
                                      getter_AddRefs(cellElement));
    NS_ENSURE_SUCCESS(res, res);
  }

  if (mIsObjectResizingEnabled && cellElement) {
    
    

    
    if (nsEditProperty::img != focusTagAtom) {
      
      
      nsCOMPtr<nsIDOMNode> tableNode = GetEnclosingTable(cellElement);
      focusElement = do_QueryInterface(tableNode);
      focusTagAtom = nsEditProperty::table;
    }
  }

  
  
  if (nsEditProperty::img != focusTagAtom &&
      nsEditProperty::table != focusTagAtom)
    focusElement = absPosElement;

  
  
  

  
  
  

  if (mIsAbsolutelyPositioningEnabled && mAbsolutelyPositionedObject &&
      absPosElement != mAbsolutelyPositionedObject) {
    res = HideGrabber();
    NS_ENSURE_SUCCESS(res, res);
    NS_ASSERTION(!mAbsolutelyPositionedObject, "HideGrabber failed");
  }

  if (mIsObjectResizingEnabled && mResizedObject &&
      mResizedObject != focusElement) {
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

  

  if (mIsObjectResizingEnabled && focusElement &&
      IsModifiableNode(focusElement)) {
    if (nsEditProperty::img == focusTagAtom)
      mResizedObjectIsAnImage = PR_TRUE;
    if (mResizedObject)
      res = RefreshResizers();
    else
      res = ShowResizers(focusElement);
    NS_ENSURE_SUCCESS(res, res);
  }

  if (mIsAbsolutelyPositioningEnabled && absPosElement &&
      IsModifiableNode(absPosElement)) {
    if (mAbsolutelyPositionedObject)
      res = RefreshGrabber();
    else
      res = ShowGrabberOnElement(absPosElement);
    NS_ENSURE_SUCCESS(res, res);
  }

  if (mIsInlineTableEditingEnabled && cellElement &&
      IsModifiableNode(cellElement)) {
    if (mInlineEditedCell)
      res = RefreshInlineTableEditingUI();
    else
      res = ShowInlineTableEditingUI(cellElement);
  }

  return res;
}



nsresult
nsHTMLEditor::GetPositionAndDimensions(nsIDOMElement * aElement,
                                       PRInt32 & aX, PRInt32 & aY,
                                       PRInt32 & aW, PRInt32 & aH,
                                       PRInt32 & aBorderLeft,
                                       PRInt32 & aBorderTop,
                                       PRInt32 & aMarginLeft,
                                       PRInt32 & aMarginTop)
{
  NS_ENSURE_ARG_POINTER(aElement);

  
  PRBool isPositioned = PR_FALSE;
  nsresult res = aElement->HasAttribute(NS_LITERAL_STRING("_moz_abspos"), &isPositioned);
  NS_ENSURE_SUCCESS(res, res);
  if (!isPositioned) {
    
    nsAutoString positionStr;
    mHTMLCSSUtils->GetComputedProperty(aElement, nsEditProperty::cssPosition,
                                       positionStr);
    isPositioned = positionStr.EqualsLiteral("absolute");
  }

  if (isPositioned) {
    
    mResizedObjectIsAbsolutelyPositioned = PR_TRUE;

    nsCOMPtr<nsIDOMWindow> window;
    res = mHTMLCSSUtils->GetDefaultViewCSS(aElement, getter_AddRefs(window));
    NS_ENSURE_TRUE(window, NS_ERROR_FAILURE);

    nsCOMPtr<nsIDOMCSSStyleDeclaration> cssDecl;
    
    res = window->GetComputedStyle(aElement, EmptyString(), getter_AddRefs(cssDecl));
    NS_ENSURE_SUCCESS(res, res);

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
    mResizedObjectIsAbsolutelyPositioned = PR_FALSE;
    nsCOMPtr<nsIDOMNSHTMLElement> nsElement = do_QueryInterface(aElement);
    if (!nsElement) {return NS_ERROR_NULL_POINTER; }

    GetElementOrigin(aElement, aX, aY);

    res = nsElement->GetOffsetWidth(&aW);
    NS_ENSURE_SUCCESS(res, res);
    res = nsElement->GetOffsetHeight(&aH);

    aBorderLeft = 0;
    aBorderTop  = 0;
    aMarginLeft = 0;
    aMarginTop = 0;
  }
  return res;
}


void
nsHTMLEditor::SetAnonymousElementPosition(PRInt32 aX, PRInt32 aY, nsIDOMElement *aElement)
{
  mHTMLCSSUtils->SetCSSPropertyPixels(aElement, NS_LITERAL_STRING("left"), aX);
  mHTMLCSSUtils->SetCSSPropertyPixels(aElement, NS_LITERAL_STRING("top"), aY);
}
