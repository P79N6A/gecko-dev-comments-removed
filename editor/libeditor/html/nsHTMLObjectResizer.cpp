





































#include "nsHTMLObjectResizer.h"

#include "nsIDOMEventTarget.h"
#include "nsIDOMNSHTMLElement.h"
#include "nsIDOMEventTarget.h"
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

#include "nsIServiceManager.h"
#include "mozilla/Preferences.h"

#include "nsILookAndFeel.h"
#include "nsWidgetsCID.h"

using namespace mozilla;

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

  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(*aReturn, NS_ERROR_FAILURE);

  
  nsCOMPtr<nsIDOMEventTarget> evtTarget(do_QueryInterface(*aReturn));
  evtTarget->AddEventListener(NS_LITERAL_STRING("mousedown"), mEventListener,
                              PR_TRUE);

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

  NS_ENSURE_TRUE(*aReturn, NS_ERROR_FAILURE);

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

  NS_ENSURE_TRUE(*aReturn, NS_ERROR_FAILURE);

  return res;
}

nsresult
nsHTMLEditor::SetAllResizersPosition()
{
  NS_ENSURE_TRUE(mTopLeftHandle, NS_ERROR_FAILURE);

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
  
  NS_ENSURE_TRUE(mResizedObject, NS_OK);

  nsresult res = GetPositionAndDimensions(mResizedObject,
                                          mResizedObjectX,
                                          mResizedObjectY,
                                          mResizedObjectWidth,
                                          mResizedObjectHeight,
                                          mResizedObjectBorderLeft,
                                          mResizedObjectBorderTop,
                                          mResizedObjectMarginLeft,
                                          mResizedObjectMarginTop);

  NS_ENSURE_SUCCESS(res, res);
  res = SetAllResizersPosition();
  NS_ENSURE_SUCCESS(res, res);
  return SetShadowPosition(mResizingShadow, mResizedObject,
                           mResizedObjectX, mResizedObjectY);
}

NS_IMETHODIMP 
nsHTMLEditor::ShowResizers(nsIDOMElement *aResizedElement)
{
  nsresult res = ShowResizersInner(aResizedElement);
  if (NS_FAILED(res))
    HideResizers();
  return res;
}

nsresult 
nsHTMLEditor::ShowResizersInner(nsIDOMElement *aResizedElement)
{
  NS_ENSURE_ARG_POINTER(aResizedElement);
  nsresult res;

  nsCOMPtr<nsIDOMNode> parentNode;
  res = aResizedElement->GetParentNode(getter_AddRefs(parentNode));
  NS_ENSURE_SUCCESS(res, res);

  if (mResizedObject) {
    NS_ERROR("call HideResizers first");
    return NS_ERROR_UNEXPECTED;
  }
  mResizedObject = aResizedElement;

  
  res = CreateResizer(getter_AddRefs(mTopLeftHandle),
                      nsIHTMLObjectResizer::eTopLeft,     parentNode);
  NS_ENSURE_SUCCESS(res, res);
  res = CreateResizer(getter_AddRefs(mTopHandle),
                      nsIHTMLObjectResizer::eTop,         parentNode);
  NS_ENSURE_SUCCESS(res, res);
  res = CreateResizer(getter_AddRefs(mTopRightHandle),
                      nsIHTMLObjectResizer::eTopRight,    parentNode);
  NS_ENSURE_SUCCESS(res, res);

  res = CreateResizer(getter_AddRefs(mLeftHandle),
                      nsIHTMLObjectResizer::eLeft,        parentNode);
  NS_ENSURE_SUCCESS(res, res);
  res = CreateResizer(getter_AddRefs(mRightHandle),
                      nsIHTMLObjectResizer::eRight,       parentNode);
  NS_ENSURE_SUCCESS(res, res);

  res = CreateResizer(getter_AddRefs(mBottomLeftHandle),
                      nsIHTMLObjectResizer::eBottomLeft,  parentNode);
  NS_ENSURE_SUCCESS(res, res);
  res = CreateResizer(getter_AddRefs(mBottomHandle),
                      nsIHTMLObjectResizer::eBottom,      parentNode);
  NS_ENSURE_SUCCESS(res, res);
  res = CreateResizer(getter_AddRefs(mBottomRightHandle),
                      nsIHTMLObjectResizer::eBottomRight, parentNode);
  NS_ENSURE_SUCCESS(res, res);

  res = GetPositionAndDimensions(aResizedElement,
                                 mResizedObjectX,
                                 mResizedObjectY,
                                 mResizedObjectWidth,
                                 mResizedObjectHeight,
                                 mResizedObjectBorderLeft,
                                 mResizedObjectBorderTop,
                                 mResizedObjectMarginLeft,
                                 mResizedObjectMarginTop);
  NS_ENSURE_SUCCESS(res, res);

  
  res = SetAllResizersPosition();
  NS_ENSURE_SUCCESS(res, res);

  
  res = CreateShadow(getter_AddRefs(mResizingShadow), parentNode,
                     aResizedElement);
  NS_ENSURE_SUCCESS(res, res);
  
  res = SetShadowPosition(mResizingShadow, mResizedObject,
                          mResizedObjectX, mResizedObjectY);
  NS_ENSURE_SUCCESS(res, res);

  
  res = CreateResizingInfo(getter_AddRefs(mResizingInfo), parentNode);
  NS_ENSURE_SUCCESS(res, res);

  
  
  nsCOMPtr<nsIDOMDocument> domDoc;
  GetDocument(getter_AddRefs(domDoc));
  nsCOMPtr<nsIDocument> doc = do_QueryInterface(domDoc);
  NS_ENSURE_TRUE(doc, NS_ERROR_NULL_POINTER);

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
  NS_ENSURE_TRUE(mResizedObject, NS_OK);

  
  nsCOMPtr<nsIPresShell> ps = GetPresShell();
  
  
  

  nsresult res;
  nsCOMPtr<nsIDOMNode> parentNode;
  nsCOMPtr<nsIContent> parentContent;

  if (mTopLeftHandle) {
    res = mTopLeftHandle->GetParentNode(getter_AddRefs(parentNode));
    NS_ENSURE_SUCCESS(res, res);
    parentContent = do_QueryInterface(parentNode);
  }

  NS_NAMED_LITERAL_STRING(mousedown, "mousedown");

  RemoveListenerAndDeleteRef(mousedown, mEventListener, PR_TRUE,
                             mTopLeftHandle, parentContent, ps);
  mTopLeftHandle = nsnull;

  RemoveListenerAndDeleteRef(mousedown, mEventListener, PR_TRUE,
                             mTopHandle, parentContent, ps);
  mTopHandle = nsnull;

  RemoveListenerAndDeleteRef(mousedown, mEventListener, PR_TRUE,
                             mTopRightHandle, parentContent, ps);
  mTopRightHandle = nsnull;

  RemoveListenerAndDeleteRef(mousedown, mEventListener, PR_TRUE,
                             mLeftHandle, parentContent, ps);
  mLeftHandle = nsnull;

  RemoveListenerAndDeleteRef(mousedown, mEventListener, PR_TRUE,
                             mRightHandle, parentContent, ps);
  mRightHandle = nsnull;

  RemoveListenerAndDeleteRef(mousedown, mEventListener, PR_TRUE,
                             mBottomLeftHandle, parentContent, ps);
  mBottomLeftHandle = nsnull;

  RemoveListenerAndDeleteRef(mousedown, mEventListener, PR_TRUE,
                             mBottomHandle, parentContent, ps);
  mBottomHandle = nsnull;

  RemoveListenerAndDeleteRef(mousedown, mEventListener, PR_TRUE,
                             mBottomRightHandle, parentContent, ps);
  mBottomRightHandle = nsnull;

  RemoveListenerAndDeleteRef(mousedown, mEventListener, PR_TRUE,
                             mResizingShadow, parentContent, ps);
  mResizingShadow = nsnull;

  RemoveListenerAndDeleteRef(mousedown, mEventListener, PR_TRUE,
                             mResizingInfo, parentContent, ps);
  mResizingInfo = nsnull;

  if (mActivatedHandle) {
    mActivatedHandle->RemoveAttribute(NS_LITERAL_STRING("_moz_activated"));
    mActivatedHandle = nsnull;
  }

  

  nsCOMPtr<nsIDOMEventTarget> piTarget = GetPIDOMEventTarget();
  nsCOMPtr<nsIDOMEventTarget> target = do_QueryInterface(piTarget);

  if (target && mMouseMotionListenerP)
  {
    res = target->RemoveEventListener(NS_LITERAL_STRING("mousemove"),
                                      mMouseMotionListenerP, PR_TRUE);
    NS_ASSERTION(NS_SUCCEEDED(res), "failed to remove mouse motion listener");
  }
  mMouseMotionListenerP = nsnull;

  nsCOMPtr<nsIDOMDocument> domDoc;
  GetDocument(getter_AddRefs(domDoc));
  nsCOMPtr<nsIDocument> doc = do_QueryInterface(domDoc);
  if (!doc) { return NS_ERROR_NULL_POINTER; }
  target = do_QueryInterface(doc->GetWindow());
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

  
  PRBool preserveRatio = nsHTMLEditUtils::IsImage(mResizedObject) &&
    Preferences::GetBool("editor.resizing.preserve_ratio", PR_TRUE);

  
  
  nsAutoString locationStr;
  aHandle->GetAttribute(NS_LITERAL_STRING("anonlocation"), locationStr);
  if (locationStr.Equals(kTopLeft)) {
    SetResizeIncrements(1, 1, -1, -1, preserveRatio);
  }
  else if (locationStr.Equals(kTop)) {
    SetResizeIncrements(0, 1, 0, -1, PR_FALSE);
  }
  else if (locationStr.Equals(kTopRight)) {
    SetResizeIncrements(0, 1, 1, -1, preserveRatio);
  }
  else if (locationStr.Equals(kLeft)) {
    SetResizeIncrements(1, 0, -1, 0, PR_FALSE);
  }
  else if (locationStr.Equals(kRight)) {
    SetResizeIncrements(0, 0, 1, 0, PR_FALSE);
  }
  else if (locationStr.Equals(kBottomLeft)) {
    SetResizeIncrements(1, 0, -1, 1, preserveRatio);
  }
  else if (locationStr.Equals(kBottom)) {
    SetResizeIncrements(0, 0, 0, 1, PR_FALSE);
  }
  else if (locationStr.Equals(kBottomRight)) {
    SetResizeIncrements(0, 0, 1, 1, preserveRatio);
  }

  
  mResizingShadow->RemoveAttribute(NS_LITERAL_STRING("class"));

  
  mHTMLCSSUtils->SetCSSPropertyPixels(mResizingShadow,
                                      NS_LITERAL_STRING("width"),
                                      mResizedObjectWidth);
  mHTMLCSSUtils->SetCSSPropertyPixels(mResizingShadow,
                                      NS_LITERAL_STRING("height"),
                                      mResizedObjectHeight);

  
  nsresult result = NS_OK;
  if (!mMouseMotionListenerP) {
    mMouseMotionListenerP = new ResizerMouseMotionListener(this);
    if (!mMouseMotionListenerP) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    nsCOMPtr<nsIDOMEventTarget> piTarget = GetPIDOMEventTarget();
    nsCOMPtr<nsIDOMEventTarget> target = do_QueryInterface(piTarget);
    NS_ENSURE_TRUE(target, NS_ERROR_FAILURE);

    result = target->AddEventListener(NS_LITERAL_STRING("mousemove"),
                                      mMouseMotionListenerP, PR_TRUE);
    NS_ASSERTION(NS_SUCCEEDED(result),
                 "failed to register mouse motion listener");
  }
  return result;
}


NS_IMETHODIMP 
nsHTMLEditor::MouseDown(PRInt32 aClientX, PRInt32 aClientY,
                        nsIDOMElement *aTarget, nsIDOMEvent* aEvent)
{
  PRBool anonElement = PR_FALSE;
  if (aTarget && NS_SUCCEEDED(aTarget->HasAttribute(NS_LITERAL_STRING("_moz_anonclass"), &anonElement)))
    
    if (anonElement) {
      nsAutoString anonclass;
      nsresult res = aTarget->GetAttribute(NS_LITERAL_STRING("_moz_anonclass"), anonclass);
      NS_ENSURE_SUCCESS(res, res);
      if (anonclass.EqualsLiteral("mozResizer")) {
        
        aEvent->PreventDefault();

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

  NS_NAMED_LITERAL_STRING(leftStr, "left");
  NS_NAMED_LITERAL_STRING(topStr, "top");

  
  
  
  
  
  PRInt32 infoXPosition;
  PRInt32 infoYPosition;

  if (mActivatedHandle == mTopLeftHandle ||
      mActivatedHandle == mLeftHandle ||
      mActivatedHandle == mBottomLeftHandle)
    infoXPosition = aX;
  else if (mActivatedHandle == mTopHandle ||
             mActivatedHandle == mBottomHandle)
    infoXPosition = aX + (aW / 2);
  else
    
    
    infoXPosition = aX + aW;

  if (mActivatedHandle == mTopLeftHandle ||
      mActivatedHandle == mTopHandle ||
      mActivatedHandle == mTopRightHandle)
    infoYPosition = aY;
  else if (mActivatedHandle == mLeftHandle ||
           mActivatedHandle == mRightHandle)
    infoYPosition = aY + (aH / 2);
  else
    
    
    infoYPosition = aY + aH;

  
  const int mouseCursorOffset = 20;
  mHTMLCSSUtils->SetCSSPropertyPixels(mResizingInfo, leftStr,
                                      infoXPosition + mouseCursorOffset);
  mHTMLCSSUtils->SetCSSPropertyPixels(mResizingInfo, topStr,
                                      infoYPosition + mouseCursorOffset);

  nsCOMPtr<nsIDOMNode> textInfo;
  nsresult res = mResizingInfo->GetFirstChild(getter_AddRefs(textInfo));
  NS_ENSURE_SUCCESS(res, res);
  nsCOMPtr<nsIDOMNode> junk;
  if (textInfo) {
    res = mResizingInfo->RemoveChild(textInfo, getter_AddRefs(junk));
    NS_ENSURE_SUCCESS(res, res);
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
  NS_ENSURE_SUCCESS(res, res);
  textInfo = do_QueryInterface(nodeAsText);
  res =  mResizingInfo->AppendChild(textInfo, getter_AddRefs(junk));
  NS_ENSURE_SUCCESS(res, res);

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
    NS_ENSURE_SUCCESS(res, res);
    res = aShadow->SetAttribute(NS_LITERAL_STRING("src"), imageSource);
    NS_ENSURE_SUCCESS(res, res);
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
  return NS_MIN(resized, max);
}

PRInt32
nsHTMLEditor::GetNewResizingY(PRInt32 aX, PRInt32 aY)
{
  PRInt32 resized = mResizedObjectY +
                    GetNewResizingIncrement(aX, aY, kY) * mYIncrementFactor;
  PRInt32 max =   mResizedObjectY + mResizedObjectHeight;
  return NS_MIN(resized, max);
}

PRInt32
nsHTMLEditor::GetNewResizingWidth(PRInt32 aX, PRInt32 aY)
{
  PRInt32 resized = mResizedObjectWidth +
                     GetNewResizingIncrement(aX, aY, kWidth) *
                         mWidthIncrementFactor;
  return NS_MAX(resized, 1);
}

PRInt32
nsHTMLEditor::GetNewResizingHeight(PRInt32 aX, PRInt32 aY)
{
  PRInt32 resized = mResizedObjectHeight +
                     GetNewResizingIncrement(aX, aY, kHeight) *
                         mHeightIncrementFactor;
  return NS_MAX(resized, 1);
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

    PRInt32 newX = GetNewResizingX(clientX, clientY);
    PRInt32 newY = GetNewResizingY(clientX, clientY);
    PRInt32 newWidth  = GetNewResizingWidth(clientX, clientY);
    PRInt32 newHeight = GetNewResizingHeight(clientX, clientY);

    mHTMLCSSUtils->SetCSSPropertyPixels(mResizingShadow,
                                        leftStr,
                                        newX);
    mHTMLCSSUtils->SetCSSPropertyPixels(mResizingShadow,
                                        topStr,
                                        newY);
    mHTMLCSSUtils->SetCSSPropertyPixels(mResizingShadow,
                                        NS_LITERAL_STRING("width"),
                                        newWidth);
    mHTMLCSSUtils->SetCSSPropertyPixels(mResizingShadow,
                                        NS_LITERAL_STRING("height"),
                                        newHeight);

    return SetResizingInfoPosition(newX, newY, newWidth, newHeight);
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

    if (NS_ABS(clientX - mOriginalX ) * 2 >= xThreshold ||
        NS_ABS(clientY - mOriginalY ) * 2 >= yThreshold) {
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

