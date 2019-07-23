




































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

nsXULTooltipListener* nsXULTooltipListener::mInstance = nsnull;




nsXULTooltipListener::nsXULTooltipListener()
  : mMouseClientX(0)
  , mMouseClientY(0)
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

NS_IMPL_CYCLE_COLLECTION_CLASS(nsXULTooltipListener)

NS_INTERFACE_MAP_BEGIN(nsXULTooltipListener)
  NS_INTERFACE_MAP_ENTRY(nsIDOMMouseListener)
  NS_INTERFACE_MAP_ENTRY(nsIDOMMouseMotionListener)
  NS_INTERFACE_MAP_ENTRY(nsIDOMKeyListener)
  NS_INTERFACE_MAP_ENTRY(nsIDOMXULListener)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsIDOMEventListener, nsIDOMMouseListener)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMMouseMotionListener)
  NS_INTERFACE_MAP_ENTRIES_CYCLE_COLLECTION(nsXULTooltipListener)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsXULTooltipListener)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsXULTooltipListener)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsXULTooltipListener)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mSourceNode)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mTargetNode)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mCurrentTooltip)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsXULTooltipListener)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mSourceNode)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mTargetNode)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mCurrentTooltip)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END




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
  
  
  
  if (mTooltipTimer && !mCurrentTooltip) {
    mTooltipTimer->Cancel();
    mTooltipTimer = nsnull;
    return NS_OK;
  }

#ifdef DEBUG_crap
  if (mNeedTitletip)
    return NS_OK;
#endif

  
  
  if (mCurrentTooltip) {
    
    nsCOMPtr<nsIDOMEventTarget> eventTarget;
    aMouseEvent->GetTarget(getter_AddRefs(eventTarget));
    nsCOMPtr<nsIDOMNode> targetNode(do_QueryInterface(eventTarget));

    
    nsCOMPtr<nsIDOMXULDocument> xulDoc(do_QueryInterface(mCurrentTooltip->GetDocument()));
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
  if (mMouseClientX == newMouseX && mMouseClientY == newMouseY)
    return NS_OK;
  mMouseClientX = newMouseX;
  mMouseClientY = newMouseY;

  nsCOMPtr<nsIDOMEventTarget> eventTarget;
  aMouseEvent->GetCurrentTarget(getter_AddRefs(eventTarget));
  mSourceNode = do_QueryInterface(eventTarget);
#ifdef MOZ_XUL
  mIsSourceTree = mSourceNode->Tag() == nsGkAtoms::treechildren;
  if (mIsSourceTree)
    CheckTreeBodyMove(mouseEvent);
#endif

  
  
  
  KillTooltipTimer();
    
  
  
  
  if (!mCurrentTooltip) {
    mTooltipTimer = do_CreateInstance("@mozilla.org/timer;1");
    if (mTooltipTimer) {
      aMouseEvent->GetTarget(getter_AddRefs(eventTarget));
      mTargetNode = do_QueryInterface(eventTarget);
      if (mTargetNode) {
        nsresult rv = mTooltipTimer->InitWithFuncCallback(sTooltipCallback, this, 
                                                          kTooltipShowTime, nsITimer::TYPE_ONE_SHOT);
        if (NS_FAILED(rv)) {
          mTargetNode = nsnull;
          mSourceNode = nsnull;
        }
      }
    }
  }

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
  if (type.EqualsLiteral("DOMMouseScroll"))
    HideTooltip();
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

NS_IMETHODIMP
nsXULTooltipListener::PopupHiding(nsIDOMEvent* aEvent)
{
  DestroyTooltip();
  return NS_OK;
}




PRBool nsXULTooltipListener::sShowTooltips = PR_FALSE;
PRUint32 nsXULTooltipListener::sTooltipListenerCount = 0;

nsresult
nsXULTooltipListener::AddTooltipSupport(nsIContent* aNode)
{
  if (!aNode)
    return NS_ERROR_NULL_POINTER;

  nsCOMPtr<nsIDOMEventTarget> evtTarget(do_QueryInterface(aNode));
  evtTarget->AddEventListener(NS_LITERAL_STRING("mouseout"), (nsIDOMMouseListener*)this, PR_FALSE);
  evtTarget->AddEventListener(NS_LITERAL_STRING("mousemove"), (nsIDOMMouseListener*)this, PR_FALSE);
  
  return NS_OK;
}

nsresult
nsXULTooltipListener::RemoveTooltipSupport(nsIContent* aNode)
{
  if (!aNode)
    return NS_ERROR_NULL_POINTER;

  nsCOMPtr<nsIDOMEventTarget> evtTarget(do_QueryInterface(aNode));
  evtTarget->RemoveEventListener(NS_LITERAL_STRING("mouseout"), (nsIDOMMouseListener*)this, PR_FALSE);
  evtTarget->RemoveEventListener(NS_LITERAL_STRING("mousemove"), (nsIDOMMouseListener*)this, PR_FALSE);

  return NS_OK;
}

#ifdef MOZ_XUL
void
nsXULTooltipListener::CheckTreeBodyMove(nsIDOMMouseEvent* aMouseEvent)
{
  if (!mSourceNode)
    return;

  
  nsCOMPtr<nsIBoxObject> bx;
  nsCOMPtr<nsIDOMDocument> doc(do_QueryInterface(mSourceNode->GetDocument()));
  if (doc) {
    nsCOMPtr<nsIDOMNSDocument> nsDoc(do_QueryInterface(doc));
    nsCOMPtr<nsIDOMElement> docElement;
    doc->GetDocumentElement(getter_AddRefs(docElement));
    if (nsDoc && docElement) {
      nsDoc->GetBoxObjectFor(docElement, getter_AddRefs(bx));
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

    if (mCurrentTooltip && (row != mLastTreeRow || col != mLastTreeCol)) {
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
  
  nsCOMPtr<nsIContent> tooltipNode;
  GetTooltipFor(mSourceNode, getter_AddRefs(tooltipNode));
  if (!tooltipNode || mSourceNode == tooltipNode)
    return NS_ERROR_FAILURE; 

  
  nsCOMPtr<nsIDOMXULDocument> xulDoc(do_QueryInterface(tooltipNode->GetDocument()));
  if (xulDoc) {
    
    
    if (mSourceNode->GetDocument()) {
#ifdef MOZ_XUL
      if (!mIsSourceTree) {
        mLastTreeRow = -1;
        mLastTreeCol = nsnull;
      }
#endif

      xulDoc->SetTooltipNode(mTargetNode);
      mCurrentTooltip = tooltipNode;
      LaunchTooltip();
      mTargetNode = nsnull;
      if (!mCurrentTooltip)
        return NS_OK;

      
      
      
      if (mCurrentTooltip->AttrValueIs(kNameSpaceID_None, nsGkAtoms::noautohide,
                                       nsGkAtoms::_true, eCaseMatters))
        CreateAutoHideTimer();

      
      
      nsCOMPtr<nsIDOMEventTarget> evtTarget(do_QueryInterface(mCurrentTooltip));
      evtTarget->AddEventListener(NS_LITERAL_STRING("popuphiding"), 
                                  (nsIDOMMouseListener*)this, PR_FALSE);

      
      nsIDocument* doc = mSourceNode->GetDocument();
      if (doc) {
        evtTarget = do_QueryInterface(doc);
        evtTarget->AddEventListener(NS_LITERAL_STRING("DOMMouseScroll"), 
                                    (nsIDOMMouseListener*)this, PR_TRUE);
        evtTarget->AddEventListener(NS_LITERAL_STRING("mousedown"), 
                                    (nsIDOMMouseListener*)this, PR_TRUE);
        evtTarget->AddEventListener(NS_LITERAL_STRING("mouseup"), 
                                    (nsIDOMMouseListener*)this, PR_TRUE);                                    
        evtTarget->AddEventListener(NS_LITERAL_STRING("keydown"), 
                                    (nsIDOMMouseListener*)this, PR_TRUE);
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
    nsresult rv = view->GetCellText(aRow, aCol, label);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Couldn't get the cell text!");
    aTooltip->SetAttr(kNameSpaceID_None, nsGkAtoms::label, label, PR_TRUE);
  }
}
#endif

void
nsXULTooltipListener::LaunchTooltip()
{
  if (!mCurrentTooltip)
    return;

#ifdef MOZ_XUL
  if (mIsSourceTree && mNeedTitletip) {
    nsCOMPtr<nsITreeBoxObject> obx;
    GetSourceTreeBoxObject(getter_AddRefs(obx));

    SetTitletipLabel(obx, mCurrentTooltip, mLastTreeRow, mLastTreeCol);
    if (!mCurrentTooltip) {
      
      return;
    }
    mCurrentTooltip->SetAttr(nsnull, nsGkAtoms::titletip, NS_LITERAL_STRING("true"), PR_TRUE);
  } else {
    mCurrentTooltip->UnsetAttr(nsnull, nsGkAtoms::titletip, PR_TRUE);
  }
  if (!mCurrentTooltip) {
    
    return;
  }

  nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
  if (pm) {
    pm->ShowPopupAtScreen(mCurrentTooltip, mMouseClientX, mMouseClientY, PR_FALSE);
    
    if (!pm->IsPopupOpen(mCurrentTooltip))
      mCurrentTooltip = nsnull;
  }
#endif

}

nsresult
nsXULTooltipListener::HideTooltip()
{
#ifdef MOZ_XUL
  if (mCurrentTooltip) {
    nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
    if (pm)
      pm->HidePopup(mCurrentTooltip, PR_FALSE, PR_FALSE, PR_FALSE);
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
  if (mCurrentTooltip) {
    
    nsCOMPtr<nsIDocument> doc = mCurrentTooltip->GetDocument();
    if (doc) {
      nsCOMPtr<nsIDOMXULDocument> xulDoc(do_QueryInterface(doc));
      if (xulDoc)
        xulDoc->SetTooltipNode(nsnull);

      
      nsCOMPtr<nsIDOMEventTarget> evtTarget(do_QueryInterface(doc));
      evtTarget->RemoveEventListener(NS_LITERAL_STRING("DOMMouseScroll"), (nsIDOMMouseListener*)this, PR_TRUE);
      evtTarget->RemoveEventListener(NS_LITERAL_STRING("mousedown"), (nsIDOMMouseListener*)this, PR_TRUE);
      evtTarget->RemoveEventListener(NS_LITERAL_STRING("mouseup"), (nsIDOMMouseListener*)this, PR_TRUE);
      evtTarget->RemoveEventListener(NS_LITERAL_STRING("keydown"), (nsIDOMMouseListener*)this, PR_TRUE);
    }

    
    nsCOMPtr<nsIDOMEventTarget> evtTarget(do_QueryInterface(mCurrentTooltip));

    
    
    mCurrentTooltip = nsnull;

    evtTarget->RemoveEventListener(NS_LITERAL_STRING("popuphiding"), (nsIDOMMouseListener*)this, PR_FALSE);
  }
  
  
  KillTooltipTimer();
  mSourceNode = nsnull;
#ifdef MOZ_XUL
  mLastTreeCol = nsnull;
#endif
  if (mAutoHideTimer) {
    mAutoHideTimer->Cancel();
    mAutoHideTimer = nsnull;
  }

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
nsXULTooltipListener::CreateAutoHideTimer()
{
  if (mAutoHideTimer) {
    mAutoHideTimer->Cancel();
    mAutoHideTimer = nsnull;
  }

  mAutoHideTimer = do_CreateInstance("@mozilla.org/timer;1");
  if ( mAutoHideTimer )
    mAutoHideTimer->InitWithFuncCallback(sAutoHideCallback, this, kTooltipAutoHideTime, 
                                         nsITimer::TYPE_ONE_SHOT);
}

void
nsXULTooltipListener::sTooltipCallback(nsITimer *aTimer, void *aListener)
{
  nsRefPtr<nsXULTooltipListener> instance = mInstance;
  if (instance)
    instance->ShowTooltip();
}

void
nsXULTooltipListener::sAutoHideCallback(nsITimer *aTimer, void* aListener)
{
  nsRefPtr<nsXULTooltipListener> instance = mInstance;
  if (instance)
    instance->HideTooltip();
}

#ifdef MOZ_XUL
nsresult
nsXULTooltipListener::GetSourceTreeBoxObject(nsITreeBoxObject** aBoxObject)
{
  *aBoxObject = nsnull;

  if (mIsSourceTree && mSourceNode) {
    nsCOMPtr<nsIDOMXULElement> xulEl(do_QueryInterface(mSourceNode->GetParent()));
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
