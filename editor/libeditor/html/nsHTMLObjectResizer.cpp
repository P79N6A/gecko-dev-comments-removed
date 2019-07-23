





































#include "nsHTMLObjectResizer.h"

#include "nsIDOMEventTarget.h"
#include "nsIDOMNSHTMLElement.h"
#include "nsIDOMEventReceiver.h"
#include "nsIDOMText.h"

#include "nsIDOMCSSValue.h"
#include "nsIDOMCSSPrimitiveValue.h"

#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIDocumentObserver.h"
#include "nsIEditor.h"
#include "nsIPresShell.h"
#include "nsPIDOMWindow.h"

#include "nsHTMLEditor.h"
#include "nsEditor.h"
#include "nsEditorUtils.h"
#include "nsHTMLEditUtils.h"

#include "nsPoint.h"

#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include "nsIServiceManager.h"

#include "nsPresContext.h"
#include "nsILookAndFeel.h"
#include "nsWidgetsCID.h"

class nsHTMLEditUtils;

static NS_DEFINE_CID(kLookAndFeelCID, NS_LOOKANDFEEL_CID);




NS_IMPL_ISUPPORTS1(DocumentResizeEventListener, nsIDOMEventListener)

DocumentResizeEventListener::DocumentResizeEventListener(nsIHTMLEditor * aEditor) 
{
  mEditor = do_GetWeakReference(aEditor);
}

DocumentResizeEventListener::~DocumentResizeEventListener()
{
}

NS_IMETHODIMP
DocumentResizeEventListener::HandleEvent(nsIDOMEvent* aMouseEvent)
{
  nsCOMPtr<nsIHTMLObjectResizer> objectResizer = do_QueryReferent(mEditor);
  if (objectResizer)
    return objectResizer->RefreshResizers();
  return NS_OK;
}





NS_IMPL_ISUPPORTS1(ResizerSelectionListener, nsISelectionListener)

ResizerSelectionListener::ResizerSelectionListener(nsIHTMLEditor * aEditor)
{
  mEditor = do_GetWeakReference(aEditor);
}

ResizerSelectionListener::~ResizerSelectionListener()
{
}

NS_IMETHODIMP
ResizerSelectionListener::NotifySelectionChanged(nsIDOMDocument *, nsISelection *aSelection, PRInt16 aReason)
{
  if ((aReason & (nsISelectionListener::MOUSEDOWN_REASON |
                  nsISelectionListener::KEYPRESS_REASON |
                  nsISelectionListener::SELECTALL_REASON)) && aSelection) 
  {
    
    
    nsCOMPtr<nsIHTMLEditor> editor = do_QueryReferent(mEditor);
    if (editor)
      editor->CheckSelectionStateForAnonymousButtons(aSelection);
  }

  return NS_OK;
}





NS_IMPL_ISUPPORTS2(ResizerMouseMotionListener, nsIDOMEventListener, nsIDOMMouseMotionListener)

ResizerMouseMotionListener::ResizerMouseMotionListener(nsIHTMLEditor * aEditor)
{
  mEditor = do_GetWeakReference(aEditor);
}

ResizerMouseMotionListener::~ResizerMouseMotionListener() 
{
}


NS_IMETHODIMP
ResizerMouseMotionListener::MouseMove(nsIDOMEvent* aMouseEvent)
{
  nsCOMPtr<nsIDOMMouseEvent> mouseEvent ( do_QueryInterface(aMouseEvent) );
  if (!mouseEvent) {
    
    return NS_OK;
  }

  
  nsCOMPtr<nsIHTMLObjectResizer> objectResizer = do_QueryReferent(mEditor);
  if (objectResizer)
  {
    
    objectResizer->MouseMove(aMouseEvent);
  }

  return NS_OK;
}

NS_IMETHODIMP
ResizerMouseMotionListener::HandleEvent(nsIDOMEvent* aMouseEvent)
{
  return NS_OK;
}

NS_IMETHODIMP
ResizerMouseMotionListener::DragMove(nsIDOMEvent* aMouseEvent)
{
  return NS_OK;
}





nsresult
nsHTMLEditor::CreateResizer(nsIDOMElement ** aReturn, PRInt16 aLocation, nsIDOMNode * aParentNode)
{
  nsresult res = CreateAnonymousElement(NS_LITERAL_STRING("span"),
                                        aParentNode,
                                        NS_LITERAL_STRING("mozResizer"),
                                        PR_FALSE,
                                        aReturn);

  if (NS_FAILED(res)) return res;
  if (!*aReturn)
    return NS_ERROR_FAILURE;

  
  nsCOMPtr<nsIDOMEventTarget> evtTarget(do_QueryInterface(*aReturn));
  evtTarget->AddEventListener(NS_LITERAL_STRING("mousedown"), mMouseListenerP, PR_TRUE);

  nsAutoString locationStr;
  switch (aLocation) {
    case nsIHTMLObjectResizer::eTopLeft:
      locationStr = kTopLeft;
      break;
    case nsIHTMLObjectResizer::eTop:
      locationStr = kTop;
      break;
    case nsIHTMLObjectResizer::eTopRight:
      locationStr = kTopRight;
      break;

    case nsIHTMLObjectResizer::eLeft:
      locationStr = kLeft;
      break;
    case nsIHTMLObjectResizer::eRight:
      locationStr = kRight;
      break;

    case nsIHTMLObjectResizer::eBottomLeft:
      locationStr = kBottomLeft;
      break;
    case nsIHTMLObjectResizer::eBottom:
      locationStr = kBottom;
      break;
    case nsIHTMLObjectResizer::eBottomRight:
      locationStr = kBottomRight;
      break;
  }

  res = (*aReturn)->SetAttribute(NS_LITERAL_STRING("anonlocation"),
                                 locationStr);
  return res;
}

nsresult
nsHTMLEditor::CreateShadow(nsIDOMElement ** aReturn, nsIDOMNode * aParentNode,
                           nsIDOMElement * aOriginalObject)
{
  
  nsAutoString name;
  if (nsHTMLEditUtils::IsImage(aOriginalObject))
    name.AssignLiteral("img");
  else
    name.AssignLiteral("span");
  nsresult res = CreateAnonymousElement(name,
                                        aParentNode,
                                        NS_LITERAL_STRING("mozResizingShadow"),
                                        PR_TRUE,
                                        aReturn);

  if (!*aReturn)
    return NS_ERROR_FAILURE;

  return res;
}

nsresult
nsHTMLEditor::CreateResizingInfo(nsIDOMElement ** aReturn, nsIDOMNode * aParentNode)
{
  
  nsresult res = CreateAnonymousElement(NS_LITERAL_STRING("span"),
                                        aParentNode,
                                        NS_LITERAL_STRING("mozResizingInfo"),
                                        PR_TRUE,
                                        aReturn);

  if (!*aReturn)
    return NS_ERROR_FAILURE;

  return res;
}

nsresult
nsHTMLEditor::SetAllResizersPosition()
{
  if (!mTopLeftHandle)
    return NS_ERROR_FAILURE;

  PRInt32 x = mResizedObjectX;
  PRInt32 y = mResizedObjectY;
  PRInt32 w = mResizedObjectWidth;
  PRInt32 h = mResizedObjectHeight;

  

  
  nsAutoString value;
  float resizerWidth, resizerHeight;
  nsCOMPtr<nsIAtom> dummyUnit;
  mHTMLCSSUtils->GetComputedProperty(mTopLeftHandle, nsEditProperty::cssWidth, value);
  mHTMLCSSUtils->ParseLength(value, &resizerWidth, getter_AddRefs(dummyUnit));
  mHTMLCSSUtils->GetComputedProperty(mTopLeftHandle, nsEditProperty::cssHeight, value);
  mHTMLCSSUtils->ParseLength(value, &resizerHeight, getter_AddRefs(dummyUnit));

  PRInt32 rw  = (PRInt32)((resizerWidth + 1) / 2);
  PRInt32 rh =  (PRInt32)((resizerHeight+ 1) / 2);

  SetAnonymousElementPosition(x-rw,     y-rh, mTopLeftHandle);
  SetAnonymousElementPosition(x+w/2-rw, y-rh, mTopHandle);
  SetAnonymousElementPosition(x+w-rw-1, y-rh, mTopRightHandle);

  SetAnonymousElementPosition(x-rw,     y+h/2-rh, mLeftHandle);
  SetAnonymousElementPosition(x+w-rw-1, y+h/2-rh, mRightHandle);

  SetAnonymousElementPosition(x-rw,     y+h-rh-1, mBottomLeftHandle);
  SetAnonymousElementPosition(x+w/2-rw, y+h-rh-1, mBottomHandle);
  SetAnonymousElementPosition(x+w-rw-1, y+h-rh-1, mBottomRightHandle);

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLEditor::RefreshResizers()
{
  
  if (!mResizedObject)
    return NS_OK;

  nsresult res = GetPositionAndDimensions(mResizedObject,
                                          mResizedObjectX,
                                          mResizedObjectY,
                                          mResizedObjectWidth,
                                          mResizedObjectHeight,
                                          mResizedObjectBorderLeft,
                                          mResizedObjectBorderTop,
                                          mResizedObjectMarginLeft,
                                          mResizedObjectMarginTop);

  if (NS_FAILED(res)) return res;
  res = SetAllResizersPosition();
  if (NS_FAILED(res)) return res;
  return SetShadowPosition(mResizingShadow, mResizedObject,
                           mResizedObjectX, mResizedObjectY);
}

NS_IMETHODIMP 
nsHTMLEditor::ShowResizers(nsIDOMElement *aResizedElement)
{
  NS_ENSURE_ARG_POINTER(aResizedElement);
  mResizedObject = aResizedElement;

  
  nsIDOMElement *bodyElement = GetRoot();
  if (!bodyElement)   return NS_ERROR_NULL_POINTER;

  
  nsresult res;
  res = CreateResizer(getter_AddRefs(mTopLeftHandle),
                      nsIHTMLObjectResizer::eTopLeft,     bodyElement);
  if (NS_FAILED(res)) return res;
  res = CreateResizer(getter_AddRefs(mTopHandle),
                      nsIHTMLObjectResizer::eTop,         bodyElement);
  if (NS_FAILED(res)) return res;
  res = CreateResizer(getter_AddRefs(mTopRightHandle),
                      nsIHTMLObjectResizer::eTopRight,    bodyElement);
  if (NS_FAILED(res)) return res;

  res = CreateResizer(getter_AddRefs(mLeftHandle),
                      nsIHTMLObjectResizer::eLeft,        bodyElement);
  if (NS_FAILED(res)) return res;
  res = CreateResizer(getter_AddRefs(mRightHandle),
                      nsIHTMLObjectResizer::eRight,       bodyElement);
  if (NS_FAILED(res)) return res;

  res = CreateResizer(getter_AddRefs(mBottomLeftHandle),
                      nsIHTMLObjectResizer::eBottomLeft,  bodyElement);
  if (NS_FAILED(res)) return res;
  res = CreateResizer(getter_AddRefs(mBottomHandle),
                      nsIHTMLObjectResizer::eBottom,      bodyElement);
  if (NS_FAILED(res)) return res;
  res = CreateResizer(getter_AddRefs(mBottomRightHandle),
                      nsIHTMLObjectResizer::eBottomRight, bodyElement);
  if (NS_FAILED(res)) return res;

  res = GetPositionAndDimensions(aResizedElement,
                                 mResizedObjectX,
                                 mResizedObjectY,
                                 mResizedObjectWidth,
                                 mResizedObjectHeight,
                                 mResizedObjectBorderLeft,
                                 mResizedObjectBorderTop,
                                 mResizedObjectMarginLeft,
                                 mResizedObjectMarginTop);
  if (NS_FAILED(res)) return res;

  
  res = SetAllResizersPosition();
  if (NS_FAILED(res)) return res;

  
  res = CreateShadow(getter_AddRefs(mResizingShadow), bodyElement,
                     aResizedElement);
  if (NS_FAILED(res)) return res;
  
  res = SetShadowPosition(mResizingShadow, mResizedObject,
                          mResizedObjectX, mResizedObjectY);
  if (NS_FAILED(res)) return res;

  
  res = CreateResizingInfo(getter_AddRefs(mResizingInfo), bodyElement);
  if (NS_FAILED(res)) return res;


  
  
  nsCOMPtr<nsIDOMDocument> domDoc;
  GetDocument(getter_AddRefs(domDoc));
  nsCOMPtr<nsIDocument> doc = do_QueryInterface(domDoc);
  if (!doc) return NS_ERROR_NULL_POINTER;

  nsCOMPtr<nsIDOMEventTarget> target = do_QueryInterface(doc->GetWindow());
  if (!target) { return NS_ERROR_NULL_POINTER; }

  mResizeEventListenerP = new DocumentResizeEventListener(this);
  if (!mResizeEventListenerP) { return NS_ERROR_OUT_OF_MEMORY; }
  res = target->AddEventListener(NS_LITERAL_STRING("resize"), mResizeEventListenerP, PR_FALSE);

  aResizedElement->SetAttribute(NS_LITERAL_STRING("_moz_resizing"), NS_LITERAL_STRING("true"));
  return res;
}

NS_IMETHODIMP 
nsHTMLEditor::HideResizers(void)
{
  if (!mResizedObject)
    return NS_OK;

  
  nsCOMPtr<nsIPresShell> ps = do_QueryReferent(mPresShellWeak);
  if (!ps) return NS_ERROR_NOT_INITIALIZED;

  

  nsIDOMElement *bodyElement = GetRoot();

  nsCOMPtr<nsIContent> bodyContent( do_QueryInterface(bodyElement) );
  if (!bodyContent) return NS_ERROR_FAILURE;

  NS_NAMED_LITERAL_STRING(mousedown, "mousedown");
  
  RemoveListenerAndDeleteRef(mousedown, mMouseListenerP, PR_TRUE,
                             mTopLeftHandle, bodyContent, ps);
  mTopLeftHandle = nsnull;

  RemoveListenerAndDeleteRef(mousedown, mMouseListenerP, PR_TRUE,
                             mTopHandle, bodyContent, ps);
  mTopHandle = nsnull;

  RemoveListenerAndDeleteRef(mousedown, mMouseListenerP, PR_TRUE,
                             mTopRightHandle, bodyContent, ps);
  mTopRightHandle = nsnull;

  RemoveListenerAndDeleteRef(mousedown, mMouseListenerP, PR_TRUE,
                             mLeftHandle, bodyContent, ps);
  mLeftHandle = nsnull;

  RemoveListenerAndDeleteRef(mousedown, mMouseListenerP, PR_TRUE,
                             mRightHandle, bodyContent, ps);
  mRightHandle = nsnull;

  RemoveListenerAndDeleteRef(mousedown, mMouseListenerP, PR_TRUE,
                             mBottomLeftHandle, bodyContent, ps);
  mBottomLeftHandle = nsnull;

  RemoveListenerAndDeleteRef(mousedown, mMouseListenerP, PR_TRUE,
                             mBottomHandle, bodyContent, ps);
  mBottomHandle = nsnull;

  RemoveListenerAndDeleteRef(mousedown, mMouseListenerP, PR_TRUE,
                             mBottomRightHandle, bodyContent, ps);
  mBottomRightHandle = nsnull;

  RemoveListenerAndDeleteRef(mousedown, mMouseListenerP, PR_TRUE,
                             mResizingShadow, bodyContent, ps);
  mResizingShadow = nsnull;

  RemoveListenerAndDeleteRef(mousedown, mMouseListenerP, PR_TRUE,
                             mResizingInfo, bodyContent, ps);
  mResizingInfo = nsnull;

  

  nsCOMPtr<nsIDOMEventReceiver> erP = GetDOMEventReceiver();
  nsresult res;

  if (erP && mMouseMotionListenerP)
  {
    res = erP->RemoveEventListener(NS_LITERAL_STRING("mousemove"), mMouseMotionListenerP, PR_TRUE);
    NS_ASSERTION(NS_SUCCEEDED(res), "failed to remove mouse motion listener");
  }
  mMouseMotionListenerP = nsnull;

  nsCOMPtr<nsIDOMDocument> domDoc;
  GetDocument(getter_AddRefs(domDoc));
  nsCOMPtr<nsIDocument> doc = do_QueryInterface(domDoc);
  if (!doc) { return NS_ERROR_NULL_POINTER; }
  nsCOMPtr<nsIDOMEventTarget> target = do_QueryInterface(doc->GetWindow());
  if (!target) { return NS_ERROR_NULL_POINTER; }

  if (mResizeEventListenerP) {
    res = target->RemoveEventListener(NS_LITERAL_STRING("resize"), mResizeEventListenerP, PR_FALSE);
    NS_ASSERTION(NS_SUCCEEDED(res), "failed to remove resize event listener");
  }
  mResizeEventListenerP = nsnull;

  mResizedObject->RemoveAttribute(NS_LITERAL_STRING("_moz_resizing"));
  mResizedObject = nsnull;

  return NS_OK;
}

void
nsHTMLEditor::HideShadowAndInfo()
{
  if (mResizingShadow)
    mResizingShadow->SetAttribute(NS_LITERAL_STRING("class"), NS_LITERAL_STRING("hidden"));
  if (mResizingInfo)
    mResizingInfo->SetAttribute(NS_LITERAL_STRING("class"), NS_LITERAL_STRING("hidden"));
}

void
nsHTMLEditor::SetInfoIncrements(PRInt8 aX, PRInt8 aY)
{
  mInfoXIncrement = aX;
  mInfoYIncrement = aY;
}

nsresult
nsHTMLEditor::StartResizing(nsIDOMElement *aHandle)
{
  
  PRInt32 listenersCount = objectResizeEventListeners.Count();
  if (listenersCount) {
    nsCOMPtr<nsIHTMLObjectResizeListener> listener;
    PRInt32 index;
    for (index = 0; index < listenersCount; index++) {
      listener = objectResizeEventListeners[index];
      listener->OnStartResizing(mResizedObject);
    }
  }

  mIsResizing = PR_TRUE;
  mActivatedHandle = aHandle;
  mActivatedHandle->SetAttribute(NS_LITERAL_STRING("_moz_activated"), NS_LITERAL_STRING("true"));

  
  PRBool preserveRatio = nsHTMLEditUtils::IsImage(mResizedObject);
  nsresult result;
  nsCOMPtr<nsIPrefBranch> prefBranch =
    do_GetService(NS_PREFSERVICE_CONTRACTID, &result);
  if (NS_SUCCEEDED(result) && prefBranch && preserveRatio) {
    result = prefBranch->GetBoolPref("editor.resizing.preserve_ratio", &preserveRatio);
    if (NS_FAILED(result)) {
      
      
      
      preserveRatio = PR_TRUE;
    }
  }

  
  
  nsAutoString locationStr;
  aHandle->GetAttribute(NS_LITERAL_STRING("anonlocation"), locationStr);
  if (locationStr.Equals(kTopLeft)) {
    SetResizeIncrements(1, 1, -1, -1, preserveRatio);
    SetInfoIncrements(20, 20);
  }
  else if (locationStr.Equals(kTop)) {
    SetResizeIncrements(0, 1, 0, -1, PR_FALSE);
    SetInfoIncrements(0, 20);
  }
  else if (locationStr.Equals(kTopRight)) {
    SetResizeIncrements(0, 1, 1, -1, preserveRatio);
    SetInfoIncrements(-20, 20);
  }
  else if (locationStr.Equals(kLeft)) {
    SetResizeIncrements(1, 0, -1, 0, PR_FALSE);
    SetInfoIncrements(20, 20);
  }
  else if (locationStr.Equals(kRight)) {
    SetResizeIncrements(0, 0, 1, 0, PR_FALSE);
    SetInfoIncrements(-20, 0);
  }
  else if (locationStr.Equals(kBottomLeft)) {
    SetResizeIncrements(1, 0, -1, 1, preserveRatio);
    SetInfoIncrements(20, -20);
  }
  else if (locationStr.Equals(kBottom)) {
    SetResizeIncrements(0, 0, 0, 1, PR_FALSE);
    SetInfoIncrements(0, -20);
  }
  else if (locationStr.Equals(kBottomRight)) {
    SetResizeIncrements(0, 0, 1, 1, preserveRatio);
    SetInfoIncrements(-20, -20);
  }

  
  mResizingShadow->RemoveAttribute(NS_LITERAL_STRING("class"));

  
  mHTMLCSSUtils->SetCSSPropertyPixels(mResizingShadow,
                                      NS_LITERAL_STRING("width"),
                                      mResizedObjectWidth);
  mHTMLCSSUtils->SetCSSPropertyPixels(mResizingShadow,
                                      NS_LITERAL_STRING("height"),
                                      mResizedObjectHeight);

  
  if (!mMouseMotionListenerP) {
    mMouseMotionListenerP = new ResizerMouseMotionListener(this);
    if (!mMouseMotionListenerP) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    nsCOMPtr<nsIDOMEventReceiver> erP = GetDOMEventReceiver();
    NS_ENSURE_TRUE(erP, NS_ERROR_FAILURE);

    result = erP->AddEventListener(NS_LITERAL_STRING("mousemove"),
                                   mMouseMotionListenerP, PR_TRUE);
    NS_ASSERTION(NS_SUCCEEDED(result),
                 "failed to register mouse motion listener");
  }
  return result;
}


NS_IMETHODIMP 
nsHTMLEditor::MouseDown(PRInt32 aClientX, PRInt32 aClientY,
                        nsIDOMElement *aTarget)
{
  PRBool anonElement = PR_FALSE;
  if (aTarget && NS_SUCCEEDED(aTarget->HasAttribute(NS_LITERAL_STRING("_moz_anonclass"), &anonElement)))
    
    if (anonElement) {
      nsAutoString anonclass;
      nsresult res = aTarget->GetAttribute(NS_LITERAL_STRING("_moz_anonclass"), anonclass);
      if (NS_FAILED(res)) return res;
      if (anonclass.EqualsLiteral("mozResizer")) {
        
        mOriginalX = aClientX;
        mOriginalY = aClientY;
        return StartResizing(aTarget);
      }
      if (anonclass.EqualsLiteral("mozGrabber")) {
        
        mOriginalX = aClientX;
        mOriginalY = aClientY;
        return GrabberClicked();
      }
    }
  return NS_OK;
}

NS_IMETHODIMP 
nsHTMLEditor::MouseUp(PRInt32 aClientX, PRInt32 aClientY,
                      nsIDOMElement *aTarget)
{
  if (mIsResizing) {
    
    
    mIsResizing = PR_FALSE;
    HideShadowAndInfo();
    SetFinalSize(aClientX, aClientY);
  }
  else if (mIsMoving || mGrabberClicked) {
    if (mIsMoving) {
      mPositioningShadow->SetAttribute(NS_LITERAL_STRING("class"), NS_LITERAL_STRING("hidden"));
      SetFinalPosition(aClientX, aClientY);
    }
    if (mGrabberClicked) {
      EndMoving();
      mGrabberClicked = PR_FALSE;
      mIsMoving = PR_FALSE;
    }
  }
  return NS_OK;
}


void
nsHTMLEditor::SetResizeIncrements(PRInt32 aX, PRInt32 aY,
                                  PRInt32 aW, PRInt32 aH,
                                  PRBool aPreserveRatio)
{
  mXIncrementFactor = aX;
  mYIncrementFactor = aY;
  mWidthIncrementFactor = aW;
  mHeightIncrementFactor = aH;
  mPreserveRatio = aPreserveRatio;
}

nsresult
nsHTMLEditor::SetResizingInfoPosition(PRInt32 aX, PRInt32 aY, PRInt32 aW, PRInt32 aH)
{
  nsCOMPtr<nsIDOMDocument> domdoc;
  nsEditor::GetDocument(getter_AddRefs(domdoc));

  nsCOMPtr<nsIDocument> doc (do_QueryInterface(domdoc));
  if (!doc)
    return NS_ERROR_UNEXPECTED;

  
  nsCOMPtr<nsIDOMNSHTMLElement> nsElement = do_QueryInterface(doc->GetRootContent());
  if (!nsElement) {return NS_ERROR_NULL_POINTER; }

  
  PRInt32 w, h;
  nsElement->GetOffsetWidth(&w);
  nsElement->GetOffsetHeight(&h);
  
  if (mInfoXIncrement < 0)
    aX = w - aX ;
  if (mInfoYIncrement < 0)
    aY = h - aY;

  NS_NAMED_LITERAL_STRING(rightStr, "right");
  NS_NAMED_LITERAL_STRING(leftStr, "left");
  NS_NAMED_LITERAL_STRING(topStr, "top");
  NS_NAMED_LITERAL_STRING(bottomStr, "bottom");
  mHTMLCSSUtils->SetCSSPropertyPixels(mResizingInfo,
                                      (mInfoXIncrement < 0) ? rightStr : leftStr,
                                      aX + PR_ABS(mInfoXIncrement));
  mHTMLCSSUtils->SetCSSPropertyPixels(mResizingInfo,
                                      (mInfoYIncrement < 0) ? bottomStr : topStr,
                                      aY + PR_ABS(mInfoYIncrement));

  mHTMLCSSUtils->RemoveCSSProperty(mResizingInfo,
                                   (mInfoXIncrement >= 0) ? rightStr  : leftStr);
  mHTMLCSSUtils->RemoveCSSProperty(mResizingInfo,
                                   (mInfoYIncrement >= 0) ? bottomStr  : topStr);

  
  nsAutoString value;
  float f;
  nsCOMPtr<nsIAtom> unit;
  if (mInfoXIncrement < 0) {
    mHTMLCSSUtils->GetComputedProperty(mResizingInfo, nsEditProperty::cssLeft, value);
    mHTMLCSSUtils->ParseLength(value, &f, getter_AddRefs(unit));
    if (f <= 0) {
      mHTMLCSSUtils->SetCSSPropertyPixels(mResizingInfo, leftStr, 0);
      mHTMLCSSUtils->RemoveCSSProperty(mResizingInfo,
                                       rightStr);
    }
  }
  if (mInfoYIncrement < 0) {
    mHTMLCSSUtils->GetComputedProperty(mResizingInfo, nsEditProperty::cssTop, value);
    mHTMLCSSUtils->ParseLength(value, &f, getter_AddRefs(unit));
    if (f <= 0) {
      mHTMLCSSUtils->SetCSSPropertyPixels(mResizingInfo, topStr, 0);
      mHTMLCSSUtils->RemoveCSSProperty(mResizingInfo,
                                       bottomStr);
    }
  }

  nsCOMPtr<nsIDOMNode> textInfo;
  nsresult res = mResizingInfo->GetFirstChild(getter_AddRefs(textInfo));
  if (NS_FAILED(res)) return res;
  nsCOMPtr<nsIDOMNode> junk;
  if (textInfo) {
    res = mResizingInfo->RemoveChild(textInfo, getter_AddRefs(junk));
    if (NS_FAILED(res)) return res;
    textInfo = nsnull;
    junk = nsnull;
  }

  nsAutoString widthStr, heightStr, diffWidthStr, diffHeightStr;
  widthStr.AppendInt(aW);
  heightStr.AppendInt(aH);
  PRInt32 diffWidth  = aW - mResizedObjectWidth;
  PRInt32 diffHeight = aH - mResizedObjectHeight;
  if (diffWidth > 0)
    diffWidthStr.AssignLiteral("+");
  if (diffHeight > 0)
    diffHeightStr.AssignLiteral("+");
  diffWidthStr.AppendInt(diffWidth);
  diffHeightStr.AppendInt(diffHeight);

  nsAutoString info(widthStr + NS_LITERAL_STRING(" x ") + heightStr +
                    NS_LITERAL_STRING(" (") + diffWidthStr +
                    NS_LITERAL_STRING(", ") + diffHeightStr +
                    NS_LITERAL_STRING(")"));

  nsCOMPtr<nsIDOMText> nodeAsText;
  res = domdoc->CreateTextNode(info, getter_AddRefs(nodeAsText));
  if (NS_FAILED(res)) return res;
  textInfo = do_QueryInterface(nodeAsText);
  res =  mResizingInfo->AppendChild(textInfo, getter_AddRefs(junk));
  if (NS_FAILED(res)) return res;

  PRBool hasClass = PR_FALSE;
  if (NS_SUCCEEDED(mResizingInfo->HasAttribute(NS_LITERAL_STRING("class"), &hasClass )) && hasClass)
    res = mResizingInfo->RemoveAttribute(NS_LITERAL_STRING("class"));

  return res;
}

nsresult
nsHTMLEditor::SetShadowPosition(nsIDOMElement * aShadow,
                                nsIDOMElement * aOriginalObject,
                                PRInt32 aOriginalObjectX,
                                PRInt32 aOriginalObjectY)
{
  SetAnonymousElementPosition(aOriginalObjectX, aOriginalObjectY, aShadow);

  if (nsHTMLEditUtils::IsImage(aOriginalObject)) {
    nsAutoString imageSource;
    nsresult res = aOriginalObject->GetAttribute(NS_LITERAL_STRING("src"),
                                                imageSource);
    if (NS_FAILED(res)) return res;
    res = aShadow->SetAttribute(NS_LITERAL_STRING("src"), imageSource);
    if (NS_FAILED(res)) return res;
  }
  return NS_OK;
}

PRInt32
nsHTMLEditor::GetNewResizingIncrement(PRInt32 aX, PRInt32 aY, PRInt32 aID)
{
  PRInt32 result = 0;
  if (!mPreserveRatio) {
    switch (aID) {
      case kX:
      case kWidth:
        result = aX - mOriginalX;
        break;
      case kY:
      case kHeight:
        result = aY - mOriginalY;
        break;
    }
    return result;
  }

  PRInt32 xi = (aX - mOriginalX) * mWidthIncrementFactor;
  PRInt32 yi = (aY - mOriginalY) * mHeightIncrementFactor;
  float objectSizeRatio = 
              ((float)mResizedObjectWidth) / ((float)mResizedObjectHeight);
  result = (xi > yi) ? xi : yi;
  switch (aID) {
    case kX:
    case kWidth:
      if (result == yi)
        result = (PRInt32) (((float) result) * objectSizeRatio);
      result = (PRInt32) (((float) result) * mWidthIncrementFactor);
      break;
    case kY:
    case kHeight:
      if (result == xi)
        result =  (PRInt32) (((float) result) / objectSizeRatio);
      result = (PRInt32) (((float) result) * mHeightIncrementFactor);
      break;
  }
  return result;
}

PRInt32
nsHTMLEditor::GetNewResizingX(PRInt32 aX, PRInt32 aY)
{
  PRInt32 resized = mResizedObjectX +
                    GetNewResizingIncrement(aX, aY, kX) * mXIncrementFactor;
  PRInt32 max =   mResizedObjectX + mResizedObjectWidth;
  return PR_MIN(resized, max);
}

PRInt32
nsHTMLEditor::GetNewResizingY(PRInt32 aX, PRInt32 aY)
{
  PRInt32 resized = mResizedObjectY +
                    GetNewResizingIncrement(aX, aY, kY) * mYIncrementFactor;
  PRInt32 max =   mResizedObjectY + mResizedObjectHeight;
  return PR_MIN(resized, max);
}

PRInt32
nsHTMLEditor::GetNewResizingWidth(PRInt32 aX, PRInt32 aY)
{
  PRInt32 resized = mResizedObjectWidth +
                     GetNewResizingIncrement(aX, aY, kWidth) *
                         mWidthIncrementFactor;
  return PR_MAX(resized, 1);
}

PRInt32
nsHTMLEditor::GetNewResizingHeight(PRInt32 aX, PRInt32 aY)
{
  PRInt32 resized = mResizedObjectHeight +
                     GetNewResizingIncrement(aX, aY, kHeight) *
                         mHeightIncrementFactor;
  return PR_MAX(resized, 1);
}


NS_IMETHODIMP
nsHTMLEditor::MouseMove(nsIDOMEvent* aMouseEvent)
{
  NS_NAMED_LITERAL_STRING(leftStr, "left");
  NS_NAMED_LITERAL_STRING(topStr, "top");

  if (mIsResizing) {
    
    
    nsCOMPtr<nsIDOMMouseEvent> mouseEvent ( do_QueryInterface(aMouseEvent) );
    PRInt32 clientX, clientY;
    mouseEvent->GetClientX(&clientX);
    mouseEvent->GetClientY(&clientY);

    PRInt32 newWidth  = GetNewResizingWidth(clientX, clientY);
    PRInt32 newHeight = GetNewResizingHeight(clientX, clientY);

    mHTMLCSSUtils->SetCSSPropertyPixels(mResizingShadow,
                                        leftStr,
                                        GetNewResizingX(clientX, clientY));
    mHTMLCSSUtils->SetCSSPropertyPixels(mResizingShadow,
                                        topStr,
                                        GetNewResizingY(clientX, clientY));
    mHTMLCSSUtils->SetCSSPropertyPixels(mResizingShadow,
                                        NS_LITERAL_STRING("width"),
                                        newWidth);
    mHTMLCSSUtils->SetCSSPropertyPixels(mResizingShadow,
                                        NS_LITERAL_STRING("height"),
                                        newHeight);

    return SetResizingInfoPosition(clientX, clientY, newWidth, newHeight);
  }

  if (mGrabberClicked) {
    nsCOMPtr<nsIDOMMouseEvent> mouseEvent ( do_QueryInterface(aMouseEvent) );
    PRInt32 clientX, clientY;
    mouseEvent->GetClientX(&clientX);
    mouseEvent->GetClientY(&clientY);

    nsCOMPtr<nsILookAndFeel> look = do_GetService(kLookAndFeelCID);
    NS_ASSERTION(look, "Look and feel service must be implemented for this toolkit");

    PRInt32 xThreshold=1, yThreshold=1;
    look->GetMetric(nsILookAndFeel::eMetric_DragThresholdX, xThreshold);
    look->GetMetric(nsILookAndFeel::eMetric_DragThresholdY, yThreshold);

    if (PR_ABS(clientX - mOriginalX ) * 2 >= xThreshold ||
        PR_ABS(clientY - mOriginalY ) * 2 >= yThreshold) {
      mGrabberClicked = PR_FALSE;
      StartMoving(nsnull);
    }
  }
  if (mIsMoving) {
    nsCOMPtr<nsIDOMMouseEvent> mouseEvent ( do_QueryInterface(aMouseEvent) );
    PRInt32 clientX, clientY;
    mouseEvent->GetClientX(&clientX);
    mouseEvent->GetClientY(&clientY);

    PRInt32 newX = mPositionedObjectX + clientX - mOriginalX;
    PRInt32 newY = mPositionedObjectY + clientY - mOriginalY;

    SnapToGrid(newX, newY);

    mHTMLCSSUtils->SetCSSPropertyPixels(mPositioningShadow, leftStr, newX);
    mHTMLCSSUtils->SetCSSPropertyPixels(mPositioningShadow, topStr, newY);
  }
  return NS_OK;
}

void
nsHTMLEditor::SetFinalSize(PRInt32 aX, PRInt32 aY)
{
  if (!mResizedObject) {
    
    return;
  }

  if (mActivatedHandle) {
    mActivatedHandle->RemoveAttribute(NS_LITERAL_STRING("_moz_activated"));
    mActivatedHandle = nsnull;
  }

  
  
  
  PRInt32 left   = GetNewResizingX(aX, aY);
  PRInt32 top    = GetNewResizingY(aX, aY);
  PRInt32 width  = GetNewResizingWidth(aX, aY);
  PRInt32 height = GetNewResizingHeight(aX, aY);
  PRBool setWidth  = !mResizedObjectIsAbsolutelyPositioned || (width != mResizedObjectWidth);
  PRBool setHeight = !mResizedObjectIsAbsolutelyPositioned || (height != mResizedObjectHeight);
  
  PRInt32 x, y;
  x = left - ((mResizedObjectIsAbsolutelyPositioned) ? mResizedObjectBorderLeft+mResizedObjectMarginLeft : 0);
  y = top - ((mResizedObjectIsAbsolutelyPositioned) ? mResizedObjectBorderTop+mResizedObjectMarginTop : 0);

  
  PRBool useCSS;
  GetIsCSSEnabled(&useCSS);

  
  nsAutoEditBatch batchIt(this);

  NS_NAMED_LITERAL_STRING(widthStr,  "width");
  NS_NAMED_LITERAL_STRING(heightStr, "height");
  
  PRBool hasAttr = PR_FALSE;
  if (mResizedObjectIsAbsolutelyPositioned) {
    if (setHeight)
      mHTMLCSSUtils->SetCSSPropertyPixels(mResizedObject,
                                          nsEditProperty::cssTop,
                                          y,
                                          PR_FALSE);
    if (setWidth)
      mHTMLCSSUtils->SetCSSPropertyPixels(mResizedObject,
                                          nsEditProperty::cssLeft,
                                          x,
                                          PR_FALSE);
  }
  if (useCSS || mResizedObjectIsAbsolutelyPositioned) {
    if (setWidth && NS_SUCCEEDED(mResizedObject->HasAttribute(widthStr, &hasAttr)) && hasAttr)
      RemoveAttribute(mResizedObject, widthStr);

    hasAttr = PR_FALSE;
    if (setHeight && NS_SUCCEEDED(mResizedObject->HasAttribute(heightStr, &hasAttr)) && hasAttr)
      RemoveAttribute(mResizedObject, heightStr);

    if (setWidth)
      mHTMLCSSUtils->SetCSSPropertyPixels(mResizedObject,
                                          nsEditProperty::cssWidth,
                                          width,
                                          PR_FALSE);
    if (setHeight)
      mHTMLCSSUtils->SetCSSPropertyPixels(mResizedObject,
                                    nsEditProperty::cssHeight,
                                    height,
                                    PR_FALSE);
  }
  else {
    

    
    
    
    if (setWidth)
      mHTMLCSSUtils->SetCSSPropertyPixels(mResizedObject,
                                          nsEditProperty::cssWidth,
                                          width,
                                          PR_FALSE);
    if (setHeight)
      mHTMLCSSUtils->SetCSSPropertyPixels(mResizedObject,
                                          nsEditProperty::cssHeight,
                                          height,
                                          PR_FALSE);

    if (setWidth) {
      nsAutoString w;
      w.AppendInt(width);
      SetAttribute(mResizedObject, widthStr, w);
    }
    if (setHeight) {
      nsAutoString h;
      h.AppendInt(height);
      SetAttribute(mResizedObject, heightStr, h);
    }

    if (setWidth)
      mHTMLCSSUtils->RemoveCSSProperty(mResizedObject,
                                       nsEditProperty::cssWidth,
                                       EmptyString(),
                                       PR_FALSE);
    if (setHeight)
      mHTMLCSSUtils->RemoveCSSProperty(mResizedObject,
                                      nsEditProperty::cssHeight,
                                      EmptyString(),
                                      PR_FALSE);
  }
  
  PRInt32 listenersCount = objectResizeEventListeners.Count();
  if (listenersCount) {
    nsCOMPtr<nsIHTMLObjectResizeListener> listener;
    PRInt32 index;
    for (index = 0; index < listenersCount; index++) {
      listener = objectResizeEventListeners[index];
      listener->OnEndResizing(mResizedObject,
                              mResizedObjectWidth, mResizedObjectHeight,
                              width, height);
    }
  }

  
  mResizedObjectWidth  = width;
  mResizedObjectHeight = height;

  RefreshResizers();
}

NS_IMETHODIMP
nsHTMLEditor::GetResizedObject(nsIDOMElement * *aResizedObject)
{
  *aResizedObject = mResizedObject;
  NS_IF_ADDREF(*aResizedObject);
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLEditor::GetObjectResizingEnabled(PRBool *aIsObjectResizingEnabled)
{
  *aIsObjectResizingEnabled = mIsObjectResizingEnabled;
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLEditor::SetObjectResizingEnabled(PRBool aObjectResizingEnabled)
{
  mIsObjectResizingEnabled = aObjectResizingEnabled;
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLEditor::AddObjectResizeEventListener(nsIHTMLObjectResizeListener * aListener)
{
  NS_ENSURE_ARG_POINTER(aListener);
  if (objectResizeEventListeners.Count() &&
      objectResizeEventListeners.IndexOf(aListener) != -1) {
    
    NS_ASSERTION(PR_FALSE,
                 "trying to register an already registered object resize event listener");
    return NS_OK;
  }
  objectResizeEventListeners.AppendObject(aListener);
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLEditor::RemoveObjectResizeEventListener(nsIHTMLObjectResizeListener * aListener)
{
  NS_ENSURE_ARG_POINTER(aListener);
  if (!objectResizeEventListeners.Count() ||
      objectResizeEventListeners.IndexOf(aListener) == -1) {
    
    NS_ASSERTION(PR_FALSE,
                 "trying to remove an object resize event listener that was not already registered");
    return NS_OK;
  }
  objectResizeEventListeners.RemoveObject(aListener);
  return NS_OK;
}

