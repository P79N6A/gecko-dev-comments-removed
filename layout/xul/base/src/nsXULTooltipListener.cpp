




































#include "nsXULTooltipListener.h"

#include "nsIDOMMouseEvent.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMDocument.h"
#include "nsIDOMXULDocument.h"
#include "nsIDOMXULElement.h"
#include "nsIDocument.h"
#include "nsGkAtoms.h"
#include "nsIPresShell.h"
#include "nsIFrame.h"
#include "nsIPopupBoxObject.h"
#include "nsIServiceManager.h"
#ifdef MOZ_XUL
#include "nsIDOMNSDocument.h"
#include "nsITreeView.h"
#endif
#include "nsGUIEvent.h"
#include "nsIPrivateDOMEvent.h"
#include "nsPresContext.h"
#include "nsIScriptContext.h"
#include "nsPIDOMWindow.h"
#include "nsContentUtils.h"
#ifdef MOZ_XUL
#include "nsXULPopupManager.h"
#endif
#include "nsIRootBox.h"
#include "nsEventDispatcher.h"

nsXULTooltipListener* nsXULTooltipListener::mInstance = nsnull;




nsXULTooltipListener::nsXULTooltipListener()
  : mMouseScreenX(0)
  , mMouseScreenY(0)
  , mTooltipShownOnce(PR_FALSE)
#ifdef MOZ_XUL
  , mIsSourceTree(PR_FALSE)
  , mNeedTitletip(PR_FALSE)
  , mLastTreeRow(-1)
#endif
{
  if (sTooltipListenerCount++ == 0) {
    
    nsContentUtils::RegisterPrefCallback("browser.chrome.toolbar_tips",
                                         ToolbarTipsPrefChanged, nsnull);

    
    ToolbarTipsPrefChanged("browser.chrome.toolbar_tips", nsnull);
  }
}

nsXULTooltipListener::~nsXULTooltipListener()
{
  if (nsXULTooltipListener::mInstance == this) {
    ClearTooltipCache();
  }
  HideTooltip();

  if (--sTooltipListenerCount == 0) {
    
    nsContentUtils::UnregisterPrefCallback("browser.chrome.toolbar_tips",
                                           ToolbarTipsPrefChanged, nsnull);
  }
}

NS_INTERFACE_MAP_BEGIN(nsXULTooltipListener)
  NS_INTERFACE_MAP_ENTRY(nsIDOMMouseListener)
  NS_INTERFACE_MAP_ENTRY(nsIDOMMouseMotionListener)
  NS_INTERFACE_MAP_ENTRY(nsIDOMKeyListener)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsIDOMEventListener, nsIDOMMouseListener)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMMouseMotionListener)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(nsXULTooltipListener)
NS_IMPL_RELEASE(nsXULTooltipListener)




NS_IMETHODIMP
nsXULTooltipListener::MouseDown(nsIDOMEvent* aMouseEvent)
{
  HideTooltip();

  return NS_OK;
}

NS_IMETHODIMP
nsXULTooltipListener::MouseUp(nsIDOMEvent* aMouseEvent)
{
  HideTooltip();

  return NS_OK;
}

NS_IMETHODIMP
nsXULTooltipListener::MouseOut(nsIDOMEvent* aMouseEvent)
{
  
  mTooltipShownOnce = PR_FALSE;
   
  
  
  mCachedMouseEvent = nsnull;

  
  
  
  nsCOMPtr<nsIContent> currentTooltip = do_QueryReferent(mCurrentTooltip);
  if (mTooltipTimer && !currentTooltip) {
    mTooltipTimer->Cancel();
    mTooltipTimer = nsnull;
    return NS_OK;
  }

#ifdef DEBUG_crap
  if (mNeedTitletip)
    return NS_OK;
#endif

  
  
  if (currentTooltip) {
    
    nsCOMPtr<nsIDOMEventTarget> eventTarget;
    aMouseEvent->GetTarget(getter_AddRefs(eventTarget));
    nsCOMPtr<nsIDOMNode> targetNode(do_QueryInterface(eventTarget));

    
    nsCOMPtr<nsIDOMXULDocument> xulDoc(do_QueryInterface(currentTooltip->GetDocument()));
    if (!xulDoc)     
      return NS_OK;  
    nsCOMPtr<nsIDOMNode> tooltipNode;
    xulDoc->TrustedGetTooltipNode (getter_AddRefs(tooltipNode));

    
    
    if (tooltipNode == targetNode) {
      HideTooltip();
#ifdef MOZ_XUL
      
      if (mIsSourceTree) {
        mLastTreeRow = -1;
        mLastTreeCol = nsnull;
      }
#endif
    }
  }

  return NS_OK;
}




NS_IMETHODIMP
nsXULTooltipListener::MouseMove(nsIDOMEvent* aMouseEvent)
{
  if (!sShowTooltips)
    return NS_OK;

  
  
  
  
  nsCOMPtr<nsIDOMMouseEvent> mouseEvent(do_QueryInterface(aMouseEvent));
  PRInt32 newMouseX, newMouseY;
  mouseEvent->GetScreenX(&newMouseX);
  mouseEvent->GetScreenY(&newMouseY);

  
  if (mMouseScreenX == newMouseX && mMouseScreenY == newMouseY)
    return NS_OK;  

  
  
  nsCOMPtr<nsIContent> currentTooltip = do_QueryReferent(mCurrentTooltip);

  if ((currentTooltip) &&
      (abs(mMouseScreenX - newMouseX) <= kTooltipMouseMoveTolerance) &&
      (abs(mMouseScreenY - newMouseY) <= kTooltipMouseMoveTolerance))
    return NS_OK;
  mMouseScreenX = newMouseX;
  mMouseScreenY = newMouseY;
  mCachedMouseEvent = aMouseEvent;

  nsCOMPtr<nsIDOMEventTarget> currentTarget;
  aMouseEvent->GetCurrentTarget(getter_AddRefs(currentTarget));

  nsCOMPtr<nsIContent> sourceContent = do_QueryInterface(currentTarget);
  mSourceNode = do_GetWeakReference(sourceContent);
#ifdef MOZ_XUL
  mIsSourceTree = sourceContent->Tag() == nsGkAtoms::treechildren;
  if (mIsSourceTree)
    CheckTreeBodyMove(mouseEvent);
#endif

  
  
  
  KillTooltipTimer();

  
  
  
  if (!currentTooltip && !mTooltipShownOnce) {
    nsCOMPtr<nsIDOMEventTarget> eventTarget;
    aMouseEvent->GetTarget(getter_AddRefs(eventTarget));

    
    
    
    
    if (!sourceContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::popupsinherittooltip,
                                    nsGkAtoms::_true, eCaseMatters)) {
      nsCOMPtr<nsIContent> targetContent = do_QueryInterface(eventTarget);
      while (targetContent && targetContent != sourceContent) {
        nsIAtom* tag = targetContent->Tag();
        if (targetContent->GetNameSpaceID() == kNameSpaceID_XUL &&
            (tag == nsGkAtoms::menupopup ||
             tag == nsGkAtoms::panel ||
             tag == nsGkAtoms::tooltip)) {
          mSourceNode = nsnull;
          return NS_OK;
        }

        targetContent = targetContent->GetParent();
      }
    }

    mTooltipTimer = do_CreateInstance("@mozilla.org/timer;1");
    if (mTooltipTimer) {
      mTargetNode = do_GetWeakReference(eventTarget);
      if (mTargetNode) {
        nsresult rv = mTooltipTimer->InitWithFuncCallback(sTooltipCallback, this, 
                                                          kTooltipShowTime, nsITimer::TYPE_ONE_SHOT);
        if (NS_FAILED(rv)) {
          mTargetNode = nsnull;
          mSourceNode = nsnull;
        }
      }
    }
    return NS_OK;
  }

#ifdef MOZ_XUL
  if (mIsSourceTree)
    return NS_OK;
#endif

  HideTooltip();
  
  
  mTooltipShownOnce = PR_TRUE;

  return NS_OK;
}




NS_IMETHODIMP
nsXULTooltipListener::KeyDown(nsIDOMEvent* aKeyEvent)
{
  HideTooltip();
  return NS_OK;
}





NS_IMETHODIMP
nsXULTooltipListener::HandleEvent(nsIDOMEvent* aEvent)
{
  nsAutoString type;
  aEvent->GetType(type);
  if (type.EqualsLiteral("DOMMouseScroll") || type.EqualsLiteral("dragstart"))
    HideTooltip();
  else if (type.EqualsLiteral("popuphiding"))
    DestroyTooltip();

  return NS_OK;
}





int
nsXULTooltipListener::ToolbarTipsPrefChanged(const char *aPref,
                                             void *aClosure)
{
  sShowTooltips = nsContentUtils::GetBoolPref("browser.chrome.toolbar_tips",
                                              sShowTooltips);

  return 0;
}




PRBool nsXULTooltipListener::sShowTooltips = PR_FALSE;
PRUint32 nsXULTooltipListener::sTooltipListenerCount = 0;

nsresult
nsXULTooltipListener::AddTooltipSupport(nsIContent* aNode)
{
  if (!aNode)
    return NS_ERROR_NULL_POINTER;

  nsCOMPtr<nsIDOMEventTarget> evtTarget(do_QueryInterface(aNode));
  evtTarget->AddEventListener(NS_LITERAL_STRING("mouseout"), static_cast<nsIDOMMouseListener*>(this), PR_FALSE);
  evtTarget->AddEventListener(NS_LITERAL_STRING("mousemove"), static_cast<nsIDOMMouseListener*>(this), PR_FALSE);
  evtTarget->AddEventListener(NS_LITERAL_STRING("dragstart"), static_cast<nsIDOMMouseListener*>(this), PR_FALSE);
  
  return NS_OK;
}

nsresult
nsXULTooltipListener::RemoveTooltipSupport(nsIContent* aNode)
{
  if (!aNode)
    return NS_ERROR_NULL_POINTER;

  nsCOMPtr<nsIDOMEventTarget> evtTarget(do_QueryInterface(aNode));
  evtTarget->RemoveEventListener(NS_LITERAL_STRING("mouseout"), static_cast<nsIDOMMouseListener*>(this), PR_FALSE);
  evtTarget->RemoveEventListener(NS_LITERAL_STRING("mousemove"), static_cast<nsIDOMMouseListener*>(this), PR_FALSE);
  evtTarget->RemoveEventListener(NS_LITERAL_STRING("dragstart"), static_cast<nsIDOMMouseListener*>(this), PR_FALSE);

  return NS_OK;
}

#ifdef MOZ_XUL
void
nsXULTooltipListener::CheckTreeBodyMove(nsIDOMMouseEvent* aMouseEvent)
{
  nsCOMPtr<nsIContent> sourceNode = do_QueryReferent(mSourceNode);
  if (!sourceNode)
    return;

  
  nsCOMPtr<nsIBoxObject> bx;
  nsIDocument* doc = sourceNode->GetDocument();
  if (doc) {
    nsCOMPtr<nsIDOMElement> docElement = do_QueryInterface(doc->GetRootContent());
    if (docElement) {
      doc->GetBoxObjectFor(docElement, getter_AddRefs(bx));
    }
  }

  nsCOMPtr<nsITreeBoxObject> obx;
  GetSourceTreeBoxObject(getter_AddRefs(obx));
  if (bx && obx) {
    PRInt32 x, y;
    aMouseEvent->GetScreenX(&x);
    aMouseEvent->GetScreenY(&y);

    PRInt32 row;
    nsCOMPtr<nsITreeColumn> col;
    nsCAutoString obj;

    
    PRInt32 boxX, boxY;
    bx->GetScreenX(&boxX);
    bx->GetScreenY(&boxY);
    x -= boxX;
    y -= boxY;

    obx->GetCellAt(x, y, &row, getter_AddRefs(col), obj);

    
    
    mNeedTitletip = PR_FALSE;
    if (row >= 0 && obj.EqualsLiteral("text")) {
      obx->IsCellCropped(row, col, &mNeedTitletip);
    }

    nsCOMPtr<nsIContent> currentTooltip = do_QueryReferent(mCurrentTooltip);
    if (currentTooltip && (row != mLastTreeRow || col != mLastTreeCol)) {
      HideTooltip();
    } 

    mLastTreeRow = row;
    mLastTreeCol = col;
  }
}
#endif

nsresult
nsXULTooltipListener::ShowTooltip()
{
  nsCOMPtr<nsIContent> sourceNode = do_QueryReferent(mSourceNode);

  
  nsCOMPtr<nsIContent> tooltipNode;
  GetTooltipFor(sourceNode, getter_AddRefs(tooltipNode));
  if (!tooltipNode || sourceNode == tooltipNode)
    return NS_ERROR_FAILURE; 

  
  nsCOMPtr<nsIDOMXULDocument> xulDoc(do_QueryInterface(tooltipNode->GetDocument()));
  if (xulDoc) {
    
    
    if (sourceNode->GetDocument()) {
#ifdef MOZ_XUL
      if (!mIsSourceTree) {
        mLastTreeRow = -1;
        mLastTreeCol = nsnull;
      }
#endif

      nsCOMPtr<nsIDOMNode> targetNode = do_QueryReferent(mTargetNode);
      xulDoc->SetTooltipNode(targetNode);
      mCurrentTooltip = do_GetWeakReference(tooltipNode);
      LaunchTooltip();
      mTargetNode = nsnull;

      nsCOMPtr<nsIContent> currentTooltip = do_QueryReferent(mCurrentTooltip);
      if (!currentTooltip)
        return NS_OK;

      
      
      nsCOMPtr<nsIDOMEventTarget> evtTarget(do_QueryInterface(currentTooltip));
      evtTarget->AddEventListener(NS_LITERAL_STRING("popuphiding"), 
                                  static_cast<nsIDOMMouseListener*>(this), PR_FALSE);

      
      nsIDocument* doc = sourceNode->GetDocument();
      if (doc) {
        evtTarget = do_QueryInterface(doc);
        evtTarget->AddEventListener(NS_LITERAL_STRING("DOMMouseScroll"), 
                                    static_cast<nsIDOMMouseListener*>(this), PR_TRUE);
        evtTarget->AddEventListener(NS_LITERAL_STRING("mousedown"), 
                                    static_cast<nsIDOMMouseListener*>(this), PR_TRUE);
        evtTarget->AddEventListener(NS_LITERAL_STRING("mouseup"), 
                                    static_cast<nsIDOMMouseListener*>(this), PR_TRUE);                                    
        evtTarget->AddEventListener(NS_LITERAL_STRING("keydown"), 
                                    static_cast<nsIDOMMouseListener*>(this), PR_TRUE);
      }
      mSourceNode = nsnull;
    }
  }

  return NS_OK;
}

#ifdef MOZ_XUL


#ifdef DEBUG_crap
static void
GetTreeCellCoords(nsITreeBoxObject* aTreeBox, nsIContent* aSourceNode, 
                  PRInt32 aRow, nsITreeColumn* aCol, PRInt32* aX, PRInt32* aY)
{
  PRInt32 junk;
  aTreeBox->GetCoordsForCellItem(aRow, aCol, EmptyCString(), aX, aY, &junk, &junk);
  nsCOMPtr<nsIDOMXULElement> xulEl(do_QueryInterface(aSourceNode));
  nsCOMPtr<nsIBoxObject> bx;
  xulEl->GetBoxObject(getter_AddRefs(bx));
  PRInt32 myX, myY;
  bx->GetX(&myX);
  bx->GetY(&myY);
  *aX += myX;
  *aY += myY;
}
#endif

static void
SetTitletipLabel(nsITreeBoxObject* aTreeBox, nsIContent* aTooltip,
                 PRInt32 aRow, nsITreeColumn* aCol)
{
  nsCOMPtr<nsITreeView> view;
  aTreeBox->GetView(getter_AddRefs(view));
  if (view) {
    nsAutoString label;
#ifdef DEBUG
    nsresult rv = 
#endif
      view->GetCellText(aRow, aCol, label);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Couldn't get the cell text!");
    aTooltip->SetAttr(kNameSpaceID_None, nsGkAtoms::label, label, PR_TRUE);
  }
}
#endif

void
nsXULTooltipListener::LaunchTooltip()
{
  nsCOMPtr<nsIContent> currentTooltip = do_QueryReferent(mCurrentTooltip);
  if (!currentTooltip)
    return;

#ifdef MOZ_XUL
  if (mIsSourceTree && mNeedTitletip) {
    nsCOMPtr<nsITreeBoxObject> obx;
    GetSourceTreeBoxObject(getter_AddRefs(obx));

    SetTitletipLabel(obx, currentTooltip, mLastTreeRow, mLastTreeCol);
    if (!(currentTooltip = do_QueryReferent(mCurrentTooltip))) {
      
      return;
    }
    currentTooltip->SetAttr(nsnull, nsGkAtoms::titletip, NS_LITERAL_STRING("true"), PR_TRUE);
  } else {
    currentTooltip->UnsetAttr(nsnull, nsGkAtoms::titletip, PR_TRUE);
  }
  if (!(currentTooltip = do_QueryReferent(mCurrentTooltip))) {
    
    return;
  }

  nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
  if (pm) {
    pm->ShowPopupAtScreen(currentTooltip, mMouseScreenX, mMouseScreenY,
                          PR_FALSE, mCachedMouseEvent);

    mCachedMouseEvent = nsnull;

    
    if (!pm->IsPopupOpen(currentTooltip))
      mCurrentTooltip = nsnull;
  }
#endif

}

nsresult
nsXULTooltipListener::HideTooltip()
{
  mCachedMouseEvent = nsnull;

#ifdef MOZ_XUL
  nsCOMPtr<nsIContent> currentTooltip = do_QueryReferent(mCurrentTooltip);
  if (currentTooltip) {
    nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
    if (pm)
      pm->HidePopup(currentTooltip, PR_FALSE, PR_FALSE, PR_FALSE);
  }
#endif

  DestroyTooltip();
  return NS_OK;
}

static void
GetImmediateChild(nsIContent* aContent, nsIAtom *aTag, nsIContent** aResult) 
{
  *aResult = nsnull;
  PRUint32 childCount = aContent->GetChildCount();
  for (PRUint32 i = 0; i < childCount; i++) {
    nsIContent *child = aContent->GetChildAt(i);

    if (child->Tag() == aTag) {
      *aResult = child;
      NS_ADDREF(*aResult);
      return;
    }
  }

  return;
}

nsresult
nsXULTooltipListener::FindTooltip(nsIContent* aTarget, nsIContent** aTooltip)
{
  if (!aTarget)
    return NS_ERROR_NULL_POINTER;

  
  nsCOMPtr<nsIDocument> document = aTarget->GetDocument();
  if (!document) {
    NS_WARNING("Unable to retrieve the tooltip node document.");
    return NS_ERROR_FAILURE;
  }
  nsCOMPtr<nsPIDOMWindow> window = document->GetWindow();
  if (!window) {
    return NS_OK;
  }

  PRBool closed;
  window->GetClosed(&closed);

  if (closed) {
    return NS_OK;
  }

  nsAutoString tooltipText;
  aTarget->GetAttr(kNameSpaceID_None, nsGkAtoms::tooltiptext, tooltipText);
  if (!tooltipText.IsEmpty()) {
    
    nsIRootBox* rootBox = nsIRootBox::GetRootBox(document->GetPrimaryShell());
    NS_ENSURE_STATE(rootBox);
    *aTooltip = rootBox->GetDefaultTooltip();
    if (*aTooltip) {
      NS_ADDREF(*aTooltip);
      (*aTooltip)->SetAttr(kNameSpaceID_None, nsGkAtoms::label, tooltipText, PR_TRUE);
    }
    return NS_OK;
  }

  nsAutoString tooltipId;
  aTarget->GetAttr(kNameSpaceID_None, nsGkAtoms::tooltip, tooltipId);

  
  if (tooltipId.EqualsLiteral("_child")) {
    GetImmediateChild(aTarget, nsGkAtoms::tooltip, aTooltip);
    return NS_OK;
  }

  if (!tooltipId.IsEmpty()) {
    
    nsCOMPtr<nsIDOMDocument> domDocument =
      do_QueryInterface(document);
    if (!domDocument) {
      return NS_ERROR_FAILURE;
    }

    nsCOMPtr<nsIDOMElement> tooltipEl;
    domDocument->GetElementById(tooltipId, getter_AddRefs(tooltipEl));

    if (tooltipEl) {
#ifdef MOZ_XUL
      mNeedTitletip = PR_FALSE;
#endif
      CallQueryInterface(tooltipEl, aTooltip);
      return NS_OK;
    }
  }

#ifdef MOZ_XUL
  
  if (mIsSourceTree && mNeedTitletip) {
    nsIRootBox* rootBox = nsIRootBox::GetRootBox(document->GetPrimaryShell());
    NS_ENSURE_STATE(rootBox);
    NS_IF_ADDREF(*aTooltip = rootBox->GetDefaultTooltip());
  }
#endif

  return NS_OK;
}


nsresult
nsXULTooltipListener::GetTooltipFor(nsIContent* aTarget, nsIContent** aTooltip)
{
  *aTooltip = nsnull;
  nsCOMPtr<nsIContent> tooltip;
  nsresult rv = FindTooltip(aTarget, getter_AddRefs(tooltip));
  if (NS_FAILED(rv) || !tooltip) {
    return rv;
  }

  
  nsIContent* parent = tooltip->GetParent();
  if (parent) {
    nsIDocument* doc = parent->GetCurrentDoc();
    nsIPresShell* presShell = doc ? doc->GetPrimaryShell() : nsnull;
    nsIFrame* frame = presShell ? presShell->GetPrimaryFrameFor(parent) : nsnull;
    if (frame && frame->GetType() == nsGkAtoms::menuFrame) {
      NS_WARNING("Menu cannot be used as a tooltip");
      return NS_ERROR_FAILURE;
    }
  }

  tooltip.swap(*aTooltip);
  return rv;
}

nsresult
nsXULTooltipListener::DestroyTooltip()
{
  nsCOMPtr<nsIDOMMouseListener> kungFuDeathGrip(this);
  nsCOMPtr<nsIContent> currentTooltip = do_QueryReferent(mCurrentTooltip);
  if (currentTooltip) {
    
    nsCOMPtr<nsIDocument> doc = currentTooltip->GetDocument();
    if (doc) {
      nsCOMPtr<nsIDOMXULDocument> xulDoc(do_QueryInterface(doc));
      if (xulDoc)
        xulDoc->SetTooltipNode(nsnull);

      
      nsCOMPtr<nsIDOMEventTarget> evtTarget(do_QueryInterface(doc));
      evtTarget->RemoveEventListener(NS_LITERAL_STRING("DOMMouseScroll"), static_cast<nsIDOMMouseListener*>(this), PR_TRUE);
      evtTarget->RemoveEventListener(NS_LITERAL_STRING("mousedown"), static_cast<nsIDOMMouseListener*>(this), PR_TRUE);
      evtTarget->RemoveEventListener(NS_LITERAL_STRING("mouseup"), static_cast<nsIDOMMouseListener*>(this), PR_TRUE);
      evtTarget->RemoveEventListener(NS_LITERAL_STRING("keydown"), static_cast<nsIDOMMouseListener*>(this), PR_TRUE);
    }

    
    nsCOMPtr<nsIDOMEventTarget> evtTarget(do_QueryInterface(currentTooltip));

    
    
    mCurrentTooltip = nsnull;

    evtTarget->RemoveEventListener(NS_LITERAL_STRING("popuphiding"), static_cast<nsIDOMMouseListener*>(this), PR_FALSE);
  }
  
  
  KillTooltipTimer();
  mSourceNode = nsnull;
#ifdef MOZ_XUL
  mLastTreeCol = nsnull;
#endif

  return NS_OK;
}

void
nsXULTooltipListener::KillTooltipTimer()
{
  if (mTooltipTimer) {
    mTooltipTimer->Cancel();
    mTooltipTimer = nsnull;
    mTargetNode = nsnull;
  }
}

void
nsXULTooltipListener::sTooltipCallback(nsITimer *aTimer, void *aListener)
{
  nsRefPtr<nsXULTooltipListener> instance = mInstance;
  if (instance)
    instance->ShowTooltip();
}

#ifdef MOZ_XUL
nsresult
nsXULTooltipListener::GetSourceTreeBoxObject(nsITreeBoxObject** aBoxObject)
{
  *aBoxObject = nsnull;

  nsCOMPtr<nsIContent> sourceNode = do_QueryReferent(mSourceNode);
  if (mIsSourceTree && sourceNode) {
    nsCOMPtr<nsIDOMXULElement> xulEl(do_QueryInterface(sourceNode->GetParent()));
    if (xulEl) {
      nsCOMPtr<nsIBoxObject> bx;
      xulEl->GetBoxObject(getter_AddRefs(bx));
      nsCOMPtr<nsITreeBoxObject> obx(do_QueryInterface(bx));
      if (obx) {
        *aBoxObject = obx;
        NS_ADDREF(*aBoxObject);
        return NS_OK;
      }
    }
  }
  return NS_ERROR_FAILURE;
}
#endif
