




































#include <math.h>

#include "nsHTMLEditor.h"

#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIEditor.h"
#include "nsIPresShell.h"

#include "nsISelection.h"

#include "nsTextEditUtils.h"
#include "nsEditorUtils.h"
#include "nsHTMLEditUtils.h"
#include "nsTextEditRules.h"
#include "nsIHTMLEditRules.h"

#include "nsIDOMHTMLElement.h"
#include "nsIDOMNSHTMLElement.h"
#include "nsIDOMNodeList.h"

#include "nsIDOMEventTarget.h"

#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include "nsIServiceManager.h"

#include "nsIDOMCSSValue.h"
#include "nsIDOMCSSPrimitiveValue.h"
#include "nsIDOMRGBColor.h"

#define  BLACK_BG_RGB_TRIGGER 0xd0

NS_IMETHODIMP
nsHTMLEditor::AbsolutePositionSelection(PRBool aEnabled)
{
  nsAutoEditBatch beginBatching(this);
  nsAutoRules beginRulesSniffing(this,
                                 aEnabled ? kOpSetAbsolutePosition :
                                            kOpRemoveAbsolutePosition,
                                 nsIEditor::eNext);
  
  
  
  nsCOMPtr<nsISelection> selection;
  nsresult res = GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);

  nsTextRulesInfo ruleInfo(aEnabled ?
                           nsTextEditRules::kSetAbsolutePosition :
                           nsTextEditRules::kRemoveAbsolutePosition);
  PRBool cancel, handled;
  res = mRules->WillDoAction(selection, &ruleInfo, &cancel, &handled);
  if (NS_FAILED(res) || cancel)
    return res;
  
  return mRules->DidDoAction(selection, &ruleInfo, res);
}

NS_IMETHODIMP
nsHTMLEditor::GetAbsolutelyPositionedSelectionContainer(nsIDOMElement **_retval)
{
  nsCOMPtr<nsIDOMElement> element;
  nsresult res = GetSelectionContainer(getter_AddRefs(element));
  NS_ENSURE_SUCCESS(res, res);

  nsAutoString positionStr;
  nsCOMPtr<nsIDOMNode> node = do_QueryInterface(element);
  nsCOMPtr<nsIDOMNode> resultNode;

  while (!resultNode && node && !nsEditor::NodeIsType(node, nsEditProperty::html)) {
    res = mHTMLCSSUtils->GetComputedProperty(node, nsEditProperty::cssPosition,
                                             positionStr);
    NS_ENSURE_SUCCESS(res, res);
    if (positionStr.EqualsLiteral("absolute"))
      resultNode = node;
    else {
      nsCOMPtr<nsIDOMNode> parentNode;
      res = node->GetParentNode(getter_AddRefs(parentNode));
      NS_ENSURE_SUCCESS(res, res);
      node.swap(parentNode);
    }
  }

  element = do_QueryInterface(resultNode ); 
  *_retval = element;
  NS_IF_ADDREF(*_retval);
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLEditor::GetSelectionContainerAbsolutelyPositioned(PRBool *aIsSelectionContainerAbsolutelyPositioned)
{
  *aIsSelectionContainerAbsolutelyPositioned = (mAbsolutelyPositionedObject != nsnull);
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLEditor::GetAbsolutePositioningEnabled(PRBool * aIsEnabled)
{
  *aIsEnabled = mIsAbsolutelyPositioningEnabled;
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLEditor::SetAbsolutePositioningEnabled(PRBool aIsEnabled)
{
  mIsAbsolutelyPositioningEnabled = aIsEnabled;
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLEditor::RelativeChangeElementZIndex(nsIDOMElement * aElement,
                                          PRInt32 aChange,
                                          PRInt32 * aReturn)
{
  NS_ENSURE_ARG_POINTER(aElement);
  NS_ENSURE_ARG_POINTER(aReturn);
  if (!aChange) 
    return NS_OK;

  PRInt32 zIndex;
  nsresult res = GetElementZIndex(aElement, &zIndex);
  NS_ENSURE_SUCCESS(res, res);

  zIndex = NS_MAX(zIndex + aChange, 0);
  SetElementZIndex(aElement, zIndex);
  *aReturn = zIndex;

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLEditor::SetElementZIndex(nsIDOMElement * aElement,
                               PRInt32 aZindex)
{
  NS_ENSURE_ARG_POINTER(aElement);
  
  nsAutoString zIndexStr;
  zIndexStr.AppendInt(aZindex);

  mHTMLCSSUtils->SetCSSProperty(aElement,
                                nsEditProperty::cssZIndex,
                                zIndexStr,
                                PR_FALSE);
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLEditor::RelativeChangeZIndex(PRInt32 aChange)
{
  nsAutoEditBatch beginBatching(this);
  nsAutoRules beginRulesSniffing(this,
                                 (aChange < 0) ? kOpDecreaseZIndex :
                                                 kOpIncreaseZIndex,
                                 nsIEditor::eNext);
  
  
  
  nsCOMPtr<nsISelection> selection;
  nsresult res = GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);
  nsTextRulesInfo ruleInfo((aChange < 0) ? nsTextEditRules::kDecreaseZIndex:
                                           nsTextEditRules::kIncreaseZIndex);
  PRBool cancel, handled;
  res = mRules->WillDoAction(selection, &ruleInfo, &cancel, &handled);
  if (cancel || NS_FAILED(res))
    return res;
  
  return mRules->DidDoAction(selection, &ruleInfo, res);
}

NS_IMETHODIMP
nsHTMLEditor::GetElementZIndex(nsIDOMElement * aElement,
                               PRInt32 * aZindex)
{
  nsAutoString zIndexStr;
  *aZindex = 0;

  nsresult res = mHTMLCSSUtils->GetSpecifiedProperty(aElement,
                                                     nsEditProperty::cssZIndex,
                                                     zIndexStr);
  NS_ENSURE_SUCCESS(res, res);
  if (zIndexStr.EqualsLiteral("auto")) {
    
    
    nsCOMPtr<nsIDOMNode> parentNode;
    res = aElement->GetParentNode(getter_AddRefs(parentNode));
    NS_ENSURE_SUCCESS(res, res);
    nsCOMPtr<nsIDOMNode> node = parentNode;
    nsAutoString positionStr;
    while (node && 
           zIndexStr.EqualsLiteral("auto") &&
           !nsTextEditUtils::IsBody(node)) {
      res = mHTMLCSSUtils->GetComputedProperty(node,
                                               nsEditProperty::cssPosition,
                                               positionStr);
      NS_ENSURE_SUCCESS(res, res);
      if (positionStr.EqualsLiteral("absolute")) {
        
        
        res = mHTMLCSSUtils->GetComputedProperty(node,
                                                 nsEditProperty::cssZIndex,
                                                 zIndexStr);
        NS_ENSURE_SUCCESS(res, res);
      }
      res = node->GetParentNode(getter_AddRefs(parentNode));
      NS_ENSURE_SUCCESS(res, res);
      node = parentNode;
    }
  }

  if (!zIndexStr.EqualsLiteral("auto")) {
    PRInt32 errorCode;
    *aZindex = zIndexStr.ToInteger(&errorCode);
  }

  return NS_OK;
}

nsresult
nsHTMLEditor::CreateGrabber(nsIDOMNode * aParentNode, nsIDOMElement ** aReturn)
{
  
  nsresult res = CreateAnonymousElement(NS_LITERAL_STRING("span"),
                                        aParentNode,
                                        NS_LITERAL_STRING("mozGrabber"),
                                        PR_FALSE,
                                        aReturn);

  NS_ENSURE_TRUE(*aReturn, NS_ERROR_FAILURE);

  
  nsCOMPtr<nsIDOMEventTarget> evtTarget(do_QueryInterface(*aReturn));
  evtTarget->AddEventListener(NS_LITERAL_STRING("mousedown"),
                              mEventListener, PR_FALSE);

  return res;
}

NS_IMETHODIMP
nsHTMLEditor::RefreshGrabber()
{
  NS_ENSURE_TRUE(mAbsolutelyPositionedObject, NS_ERROR_NULL_POINTER);

  nsresult res = GetPositionAndDimensions(mAbsolutelyPositionedObject,
                                         mPositionedObjectX,
                                         mPositionedObjectY,
                                         mPositionedObjectWidth,
                                         mPositionedObjectHeight,
                                         mPositionedObjectBorderLeft,
                                         mPositionedObjectBorderTop,
                                         mPositionedObjectMarginLeft,
                                         mPositionedObjectMarginTop);

  NS_ENSURE_SUCCESS(res, res);

  SetAnonymousElementPosition(mPositionedObjectX+12,
                              mPositionedObjectY-14,
                              mGrabber);
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLEditor::HideGrabber()
{
  nsresult res =
    mAbsolutelyPositionedObject->RemoveAttribute(NS_LITERAL_STRING("_moz_abspos"));
  NS_ENSURE_SUCCESS(res, res);

  mAbsolutelyPositionedObject = nsnull;
  NS_ENSURE_TRUE(mGrabber, NS_ERROR_NULL_POINTER);

  
  nsCOMPtr<nsIPresShell> ps;
  GetPresShell(getter_AddRefs(ps));
  
  
  

  nsCOMPtr<nsIDOMNode> parentNode;
  res = mGrabber->GetParentNode(getter_AddRefs(parentNode));
  NS_ENSURE_SUCCESS(res, res);

  nsCOMPtr<nsIContent> parentContent = do_QueryInterface(parentNode);
  NS_ENSURE_TRUE(parentContent, NS_ERROR_NULL_POINTER);

  DeleteRefToAnonymousNode(mGrabber, parentContent, ps);
  mGrabber = nsnull;
  DeleteRefToAnonymousNode(mPositioningShadow, parentContent, ps);
  mPositioningShadow = nsnull;

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLEditor::ShowGrabberOnElement(nsIDOMElement * aElement)
{
  NS_ENSURE_ARG_POINTER(aElement);

  if (mGrabber) {
    NS_ERROR("call HideGrabber first");
    return NS_ERROR_UNEXPECTED;
  }

  nsAutoString classValue;
  nsresult res = CheckPositionedElementBGandFG(aElement, classValue);
  NS_ENSURE_SUCCESS(res, res);

  res = aElement->SetAttribute(NS_LITERAL_STRING("_moz_abspos"),
                               classValue);
  NS_ENSURE_SUCCESS(res, res);

  
  mAbsolutelyPositionedObject = aElement;

  nsCOMPtr<nsIDOMNode> parentNode;
  res = aElement->GetParentNode(getter_AddRefs(parentNode));
  NS_ENSURE_SUCCESS(res, res);

  res = CreateGrabber(parentNode, getter_AddRefs(mGrabber));
  NS_ENSURE_SUCCESS(res, res);

  
  return RefreshGrabber();
}

nsresult
nsHTMLEditor::StartMoving(nsIDOMElement *aHandle)
{
  nsCOMPtr<nsIDOMNode> parentNode;
  nsresult res = mGrabber->GetParentNode(getter_AddRefs(parentNode));
  NS_ENSURE_SUCCESS(res, res);

  
  res = CreateShadow(getter_AddRefs(mPositioningShadow),
                                 parentNode, mAbsolutelyPositionedObject);
  NS_ENSURE_SUCCESS(res,res);
  res = SetShadowPosition(mPositioningShadow, mAbsolutelyPositionedObject,
                             mPositionedObjectX, mPositionedObjectY);
  NS_ENSURE_SUCCESS(res,res);

  
  mPositioningShadow->RemoveAttribute(NS_LITERAL_STRING("class"));

  
  mHTMLCSSUtils->SetCSSPropertyPixels(mPositioningShadow,
                                      NS_LITERAL_STRING("width"),
                                      mPositionedObjectWidth);
  mHTMLCSSUtils->SetCSSPropertyPixels(mPositioningShadow,
                                      NS_LITERAL_STRING("height"),
                                      mPositionedObjectHeight);

  mIsMoving = PR_TRUE;
  return res;
}

void
nsHTMLEditor::SnapToGrid(PRInt32 & newX, PRInt32 & newY)
{
  if (mSnapToGridEnabled && mGridSize) {
    newX = (PRInt32) floor( ((float)newX / (float)mGridSize) + 0.5f ) * mGridSize;
    newY = (PRInt32) floor( ((float)newY / (float)mGridSize) + 0.5f ) * mGridSize;
  }
}

nsresult
nsHTMLEditor::GrabberClicked()
{
  
  nsresult res = NS_OK;
  if (!mMouseMotionListenerP) {
    mMouseMotionListenerP = new ResizerMouseMotionListener(this);
    if (!mMouseMotionListenerP) {return NS_ERROR_NULL_POINTER;}

    nsCOMPtr<nsPIDOMEventTarget> piTarget = GetPIDOMEventTarget();
    NS_ENSURE_TRUE(piTarget, NS_ERROR_FAILURE);

    res = piTarget->AddEventListenerByIID(mMouseMotionListenerP,
                                          NS_GET_IID(nsIDOMMouseMotionListener));
    NS_ASSERTION(NS_SUCCEEDED(res),
                 "failed to register mouse motion listener");
  }
  mGrabberClicked = PR_TRUE;
  return res;
}

nsresult
nsHTMLEditor::EndMoving()
{
  if (mPositioningShadow) {
    nsCOMPtr<nsIPresShell> ps;
    GetPresShell(getter_AddRefs(ps));
    NS_ENSURE_TRUE(ps, NS_ERROR_NOT_INITIALIZED);

    nsCOMPtr<nsIDOMNode> parentNode;
    nsresult res = mGrabber->GetParentNode(getter_AddRefs(parentNode));
    NS_ENSURE_SUCCESS(res, res);

    nsCOMPtr<nsIContent> parentContent( do_QueryInterface(parentNode) );
    NS_ENSURE_TRUE(parentContent, NS_ERROR_FAILURE);

    DeleteRefToAnonymousNode(mPositioningShadow, parentContent, ps);

    mPositioningShadow = nsnull;
  }
  nsCOMPtr<nsPIDOMEventTarget> piTarget = GetPIDOMEventTarget();

  if (piTarget && mMouseMotionListenerP) {
#ifdef DEBUG
    nsresult res =
#endif
    piTarget->RemoveEventListenerByIID(mMouseMotionListenerP,
                                       NS_GET_IID(nsIDOMMouseMotionListener));
    NS_ASSERTION(NS_SUCCEEDED(res), "failed to remove mouse motion listener");
  }
  mMouseMotionListenerP = nsnull;

  mGrabberClicked = PR_FALSE;
  mIsMoving = PR_FALSE;
  nsCOMPtr<nsISelection> selection;
  GetSelection(getter_AddRefs(selection));
  if (!selection) {
    return NS_ERROR_NOT_INITIALIZED;
  }
  return CheckSelectionStateForAnonymousButtons(selection);
}
nsresult
nsHTMLEditor::SetFinalPosition(PRInt32 aX, PRInt32 aY)
{
  nsresult res = EndMoving();
  NS_ENSURE_SUCCESS(res, res);

  
  
  
  PRInt32 newX = mPositionedObjectX + aX - mOriginalX - (mPositionedObjectBorderLeft+mPositionedObjectMarginLeft);
  PRInt32 newY = mPositionedObjectY + aY - mOriginalY - (mPositionedObjectBorderTop+mPositionedObjectMarginTop);

  SnapToGrid(newX, newY);

  nsAutoString x, y;
  x.AppendInt(newX);
  y.AppendInt(newY);

  
  nsAutoEditBatch batchIt(this);

  mHTMLCSSUtils->SetCSSPropertyPixels(mAbsolutelyPositionedObject,
                                      nsEditProperty::cssTop,
                                      newY,
                                      PR_FALSE);
  mHTMLCSSUtils->SetCSSPropertyPixels(mAbsolutelyPositionedObject,
                                      nsEditProperty::cssLeft,
                                      newX,
                                      PR_FALSE);
  
  mPositionedObjectX  = newX;
  mPositionedObjectY  = newY;

  return RefreshResizers();
}

void
nsHTMLEditor::AddPositioningOffset(PRInt32 & aX, PRInt32 & aY)
{
  
  nsresult res;
  nsCOMPtr<nsIPrefBranch> prefBranch =
    do_GetService(NS_PREFSERVICE_CONTRACTID, &res);
  PRInt32 positioningOffset = 0;
  if (NS_SUCCEEDED(res) && prefBranch) {
    res = prefBranch->GetIntPref("editor.positioning.offset", &positioningOffset);
    if (NS_FAILED(res)) 
      positioningOffset = 0;
  }

  aX += positioningOffset;
  aY += positioningOffset;
}

NS_IMETHODIMP
nsHTMLEditor::AbsolutelyPositionElement(nsIDOMElement * aElement,
                                        PRBool aEnabled)
{
  NS_ENSURE_ARG_POINTER(aElement);

  nsAutoString positionStr;
  mHTMLCSSUtils->GetComputedProperty(aElement, nsEditProperty::cssPosition,
                                     positionStr);
  PRBool isPositioned = (positionStr.EqualsLiteral("absolute"));

  
  if (isPositioned == aEnabled)
    return NS_OK;

  nsAutoEditBatch batchIt(this);
  nsresult res;

  if (aEnabled) {
    PRInt32 x, y;
    GetElementOrigin(aElement, x, y);

    mHTMLCSSUtils->SetCSSProperty(aElement,
                                  nsEditProperty::cssPosition,
                                  NS_LITERAL_STRING("absolute"),
                                  PR_FALSE);

    AddPositioningOffset(x, y);
    SnapToGrid(x, y);
    SetElementPosition(aElement, x, y);

    
    
    nsCOMPtr<nsIDOMNode> parentNode;
    res = aElement->GetParentNode(getter_AddRefs(parentNode));
    NS_ENSURE_SUCCESS(res, res);

    nsCOMPtr<nsIDOMNodeList> childNodes;
    res = parentNode->GetChildNodes(getter_AddRefs(childNodes));
    NS_ENSURE_SUCCESS(res, res);
    NS_ENSURE_TRUE(childNodes, NS_ERROR_NULL_POINTER);
    PRUint32 childCount;
    res = childNodes->GetLength(&childCount);
    NS_ENSURE_SUCCESS(res, res);

    if (childCount == 1) {
      nsCOMPtr<nsIDOMNode> brNode;
      res = CreateBR(parentNode, 0, address_of(brNode));
    }
  }
  else {
    mHTMLCSSUtils->RemoveCSSProperty(aElement,
                                     nsEditProperty::cssPosition,
                                     EmptyString(), PR_FALSE);
    mHTMLCSSUtils->RemoveCSSProperty(aElement,
                                     nsEditProperty::cssTop,
                                     EmptyString(), PR_FALSE);
    mHTMLCSSUtils->RemoveCSSProperty(aElement,
                                     nsEditProperty::cssLeft,
                                     EmptyString(), PR_FALSE);
    mHTMLCSSUtils->RemoveCSSProperty(aElement,
                                     nsEditProperty::cssZIndex,
                                     EmptyString(), PR_FALSE);

    if (!nsHTMLEditUtils::IsImage(aElement)) {
      mHTMLCSSUtils->RemoveCSSProperty(aElement,
                                       nsEditProperty::cssWidth,
                                       EmptyString(), PR_FALSE);
      mHTMLCSSUtils->RemoveCSSProperty(aElement,
                                       nsEditProperty::cssHeight,
                                       EmptyString(), PR_FALSE);
    }

    PRBool hasStyleOrIdOrClass;
    res = HasStyleOrIdOrClass(aElement, &hasStyleOrIdOrClass);
    NS_ENSURE_SUCCESS(res, res);
    if (!hasStyleOrIdOrClass && nsHTMLEditUtils::IsDiv(aElement)) {
      nsCOMPtr<nsIHTMLEditRules> htmlRules = do_QueryInterface(mRules);
      NS_ENSURE_TRUE(htmlRules, NS_ERROR_FAILURE);
      res = htmlRules->MakeSureElemStartsOrEndsOnCR(aElement);
      NS_ENSURE_SUCCESS(res, res);
      res = RemoveContainer(aElement);
    }
  }
  return res;
}

NS_IMETHODIMP
nsHTMLEditor::SetSnapToGridEnabled(PRBool aEnabled)
{
  mSnapToGridEnabled = aEnabled;
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLEditor::GetSnapToGridEnabled(PRBool * aIsEnabled)
{
  *aIsEnabled = mSnapToGridEnabled;
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLEditor::SetGridSize(PRUint32 aSize)
{
  mGridSize = aSize;
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLEditor::GetGridSize(PRUint32 * aSize)
{
  *aSize = mGridSize;
  return NS_OK;
}


NS_IMETHODIMP
nsHTMLEditor::SetElementPosition(nsIDOMElement *aElement, PRInt32 aX, PRInt32 aY)
{
  nsAutoEditBatch batchIt(this);

  mHTMLCSSUtils->SetCSSPropertyPixels(aElement,
                                      nsEditProperty::cssLeft,
                                      aX,
                                      PR_FALSE);
  mHTMLCSSUtils->SetCSSPropertyPixels(aElement,
                                      nsEditProperty::cssTop,
                                      aY,
                                      PR_FALSE);
  return NS_OK;
}


NS_IMETHODIMP
nsHTMLEditor::GetPositionedElement(nsIDOMElement ** aReturn)
{
  *aReturn = mAbsolutelyPositionedObject;
  NS_IF_ADDREF(*aReturn);
  return NS_OK;
}

nsresult
nsHTMLEditor::CheckPositionedElementBGandFG(nsIDOMElement * aElement,
                                            nsAString & aReturn)
{
  
  
  
  
  
  
  
  
  
  

  aReturn.Truncate();
  
  nsAutoString bgImageStr;
  nsresult res =
    mHTMLCSSUtils->GetComputedProperty(aElement,
                                       nsEditProperty::cssBackgroundImage,
                                       bgImageStr);
  NS_ENSURE_SUCCESS(res, res);
  if (bgImageStr.EqualsLiteral("none")) {
    nsAutoString bgColorStr;
    res =
      mHTMLCSSUtils->GetComputedProperty(aElement,
                                         nsEditProperty::cssBackgroundColor,
                                         bgColorStr);
    NS_ENSURE_SUCCESS(res, res);
    if (bgColorStr.EqualsLiteral("transparent")) {
      nsCOMPtr<nsIDOMWindow> window;
      res = mHTMLCSSUtils->GetDefaultViewCSS(aElement, getter_AddRefs(window));
      NS_ENSURE_SUCCESS(res, res);

      nsCOMPtr<nsIDOMCSSStyleDeclaration> cssDecl;
      res = window->GetComputedStyle(aElement, EmptyString(), getter_AddRefs(cssDecl));
      NS_ENSURE_SUCCESS(res, res);

      
      nsCOMPtr<nsIDOMCSSValue> colorCssValue;
      res = cssDecl->GetPropertyCSSValue(NS_LITERAL_STRING("color"), getter_AddRefs(colorCssValue));
      NS_ENSURE_SUCCESS(res, res);

      PRUint16 type;
      res = colorCssValue->GetCssValueType(&type);
      NS_ENSURE_SUCCESS(res, res);
      if (nsIDOMCSSValue::CSS_PRIMITIVE_VALUE == type) {
        nsCOMPtr<nsIDOMCSSPrimitiveValue> val = do_QueryInterface(colorCssValue);
        res = val->GetPrimitiveType(&type);
        NS_ENSURE_SUCCESS(res, res);
        if (nsIDOMCSSPrimitiveValue::CSS_RGBCOLOR == type) {
          nsCOMPtr<nsIDOMRGBColor> rgbColor;
          res = val->GetRGBColorValue(getter_AddRefs(rgbColor));
          NS_ENSURE_SUCCESS(res, res);
          nsCOMPtr<nsIDOMCSSPrimitiveValue> red, green, blue;
          float r, g, b;
          res = rgbColor->GetRed(getter_AddRefs(red));
          NS_ENSURE_SUCCESS(res, res);
          res = rgbColor->GetGreen(getter_AddRefs(green));
          NS_ENSURE_SUCCESS(res, res);
          res = rgbColor->GetBlue(getter_AddRefs(blue));
          NS_ENSURE_SUCCESS(res, res);
          res = red->GetFloatValue(nsIDOMCSSPrimitiveValue::CSS_NUMBER, &r);
          NS_ENSURE_SUCCESS(res, res);
          res = green->GetFloatValue(nsIDOMCSSPrimitiveValue::CSS_NUMBER, &g);
          NS_ENSURE_SUCCESS(res, res);
          res = blue->GetFloatValue(nsIDOMCSSPrimitiveValue::CSS_NUMBER, &b);
          NS_ENSURE_SUCCESS(res, res);
          if (r >= BLACK_BG_RGB_TRIGGER &&
              g >= BLACK_BG_RGB_TRIGGER &&
              b >= BLACK_BG_RGB_TRIGGER)
            aReturn.AssignLiteral("black");
          else
            aReturn.AssignLiteral("white");
          return NS_OK;
        }
      }
    }
  }

  return NS_OK;
}
