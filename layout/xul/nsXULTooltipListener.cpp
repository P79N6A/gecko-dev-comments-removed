




#include "nsXULTooltipListener.h"

#include "nsIDOMMouseEvent.h"
#include "nsIDOMXULDocument.h"
#include "nsIDOMXULElement.h"
#include "nsIDocument.h"
#include "nsGkAtoms.h"
#include "nsIPopupBoxObject.h"
#include "nsMenuPopupFrame.h"
#include "nsIServiceManager.h"
#include "nsIDragService.h"
#include "nsIDragSession.h"
#ifdef MOZ_XUL
#include "nsITreeView.h"
#endif
#include "nsIScriptContext.h"
#include "nsPIDOMWindow.h"
#ifdef MOZ_XUL
#include "nsXULPopupManager.h"
#endif
#include "nsIRootBox.h"
#include "nsIBoxObject.h"
#include "mozilla/Preferences.h"
#include "mozilla/LookAndFeel.h"
#include "mozilla/dom/Element.h"
#include "mozilla/dom/Event.h" 

using namespace mozilla;
using namespace mozilla::dom;

nsXULTooltipListener* nsXULTooltipListener::mInstance = nullptr;




nsXULTooltipListener::nsXULTooltipListener()
  : mMouseScreenX(0)
  , mMouseScreenY(0)
  , mTooltipShownOnce(false)
#ifdef MOZ_XUL
  , mIsSourceTree(false)
  , mNeedTitletip(false)
  , mLastTreeRow(-1)
#endif
{
  if (sTooltipListenerCount++ == 0) {
    
    Preferences::RegisterCallback(ToolbarTipsPrefChanged,
                                  "browser.chrome.toolbar_tips");

    
    ToolbarTipsPrefChanged("browser.chrome.toolbar_tips", nullptr);
  }
}

nsXULTooltipListener::~nsXULTooltipListener()
{
  if (nsXULTooltipListener::mInstance == this) {
    ClearTooltipCache();
  }
  HideTooltip();

  if (--sTooltipListenerCount == 0) {
    
    Preferences::UnregisterCallback(ToolbarTipsPrefChanged,
                                    "browser.chrome.toolbar_tips");
  }
}

NS_IMPL_ISUPPORTS(nsXULTooltipListener, nsIDOMEventListener)

void
nsXULTooltipListener::MouseOut(nsIDOMEvent* aEvent)
{
  
  mTooltipShownOnce = false;

  
  
  
  nsCOMPtr<nsIContent> currentTooltip = do_QueryReferent(mCurrentTooltip);
  if (mTooltipTimer && !currentTooltip) {
    mTooltipTimer->Cancel();
    mTooltipTimer = nullptr;
    return;
  }

#ifdef DEBUG_crap
  if (mNeedTitletip)
    return;
#endif

#ifdef MOZ_XUL
  
  
  if (currentTooltip) {
    
    nsCOMPtr<nsIDOMNode> targetNode = do_QueryInterface(
      aEvent->InternalDOMEvent()->GetTarget());

    nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
    if (pm) {
      nsCOMPtr<nsIDOMNode> tooltipNode =
        pm->GetLastTriggerTooltipNode(currentTooltip->GetCurrentDoc());
      if (tooltipNode == targetNode) {
        
        
        HideTooltip();
        
        if (mIsSourceTree) {
          mLastTreeRow = -1;
          mLastTreeCol = nullptr;
        }
      }
    }
  }
#endif
}

void
nsXULTooltipListener::MouseMove(nsIDOMEvent* aEvent)
{
  if (!sShowTooltips)
    return;

  
  
  
  
  nsCOMPtr<nsIDOMMouseEvent> mouseEvent(do_QueryInterface(aEvent));
  if (!mouseEvent)
    return;
  int32_t newMouseX, newMouseY;
  mouseEvent->GetScreenX(&newMouseX);
  mouseEvent->GetScreenY(&newMouseY);

  
  if (mMouseScreenX == newMouseX && mMouseScreenY == newMouseY)
    return;

  
  
  nsCOMPtr<nsIContent> currentTooltip = do_QueryReferent(mCurrentTooltip);

  if ((currentTooltip) &&
      (abs(mMouseScreenX - newMouseX) <= kTooltipMouseMoveTolerance) &&
      (abs(mMouseScreenY - newMouseY) <= kTooltipMouseMoveTolerance))
    return;
  mMouseScreenX = newMouseX;
  mMouseScreenY = newMouseY;

  nsCOMPtr<nsIContent> sourceContent = do_QueryInterface(
    aEvent->InternalDOMEvent()->GetCurrentTarget());
  mSourceNode = do_GetWeakReference(sourceContent);
#ifdef MOZ_XUL
  mIsSourceTree = sourceContent->Tag() == nsGkAtoms::treechildren;
  if (mIsSourceTree)
    CheckTreeBodyMove(mouseEvent);
#endif

  
  
  
  KillTooltipTimer();

  
  
  
  if (!currentTooltip && !mTooltipShownOnce) {
    nsCOMPtr<EventTarget> eventTarget = aEvent->InternalDOMEvent()->GetTarget();

    
    
    
    
    if (!sourceContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::popupsinherittooltip,
                                    nsGkAtoms::_true, eCaseMatters)) {
      nsCOMPtr<nsIContent> targetContent = do_QueryInterface(eventTarget);
      while (targetContent && targetContent != sourceContent) {
        nsIAtom* tag = targetContent->Tag();
        if (targetContent->GetNameSpaceID() == kNameSpaceID_XUL &&
            (tag == nsGkAtoms::menupopup ||
             tag == nsGkAtoms::panel ||
             tag == nsGkAtoms::tooltip)) {
          mSourceNode = nullptr;
          return;
        }

        targetContent = targetContent->GetParent();
      }
    }

    mTooltipTimer = do_CreateInstance("@mozilla.org/timer;1");
    if (mTooltipTimer) {
      mTargetNode = do_GetWeakReference(eventTarget);
      if (mTargetNode) {
        nsresult rv =
          mTooltipTimer->InitWithFuncCallback(sTooltipCallback, this,
            LookAndFeel::GetInt(LookAndFeel::eIntID_TooltipDelay, 500),
            nsITimer::TYPE_ONE_SHOT);
        if (NS_FAILED(rv)) {
          mTargetNode = nullptr;
          mSourceNode = nullptr;
        }
      }
    }
    return;
  }

#ifdef MOZ_XUL
  if (mIsSourceTree)
    return;
#endif

  HideTooltip();
  
  
  mTooltipShownOnce = true;
}

NS_IMETHODIMP
nsXULTooltipListener::HandleEvent(nsIDOMEvent* aEvent)
{
  nsAutoString type;
  aEvent->GetType(type);
  if (type.EqualsLiteral("DOMMouseScroll") ||
      type.EqualsLiteral("keydown") ||
      type.EqualsLiteral("mousedown") ||
      type.EqualsLiteral("mouseup") ||
      type.EqualsLiteral("dragstart")) {
    HideTooltip();
    return NS_OK;
  }

  if (type.EqualsLiteral("popuphiding")) {
    DestroyTooltip();
    return NS_OK;
  }

  
  
  nsCOMPtr<nsIDragService> dragService =
    do_GetService("@mozilla.org/widget/dragservice;1");
  NS_ENSURE_TRUE(dragService, NS_OK);
  nsCOMPtr<nsIDragSession> dragSession;
  dragService->GetCurrentSession(getter_AddRefs(dragSession));
  if (dragSession) {
    return NS_OK;
  }

  

  if (type.EqualsLiteral("mousemove")) {
    MouseMove(aEvent);
    return NS_OK;
  }

  if (type.EqualsLiteral("mouseout")) {
    MouseOut(aEvent);
    return NS_OK;
  }

  return NS_OK;
}





void
nsXULTooltipListener::ToolbarTipsPrefChanged(const char *aPref,
                                             void *aClosure)
{
  sShowTooltips =
    Preferences::GetBool("browser.chrome.toolbar_tips", sShowTooltips);
}




bool nsXULTooltipListener::sShowTooltips = false;
uint32_t nsXULTooltipListener::sTooltipListenerCount = 0;

nsresult
nsXULTooltipListener::AddTooltipSupport(nsIContent* aNode)
{
  if (!aNode)
    return NS_ERROR_NULL_POINTER;

  aNode->AddSystemEventListener(NS_LITERAL_STRING("mouseout"), this,
                                false, false);
  aNode->AddSystemEventListener(NS_LITERAL_STRING("mousemove"), this,
                                false, false);
  aNode->AddSystemEventListener(NS_LITERAL_STRING("mousedown"), this,
                                false, false);
  aNode->AddSystemEventListener(NS_LITERAL_STRING("mouseup"), this,
                                false, false);
  aNode->AddSystemEventListener(NS_LITERAL_STRING("dragstart"), this,
                                true, false);

  return NS_OK;
}

nsresult
nsXULTooltipListener::RemoveTooltipSupport(nsIContent* aNode)
{
  if (!aNode)
    return NS_ERROR_NULL_POINTER;

  aNode->RemoveSystemEventListener(NS_LITERAL_STRING("mouseout"), this, false);
  aNode->RemoveSystemEventListener(NS_LITERAL_STRING("mousemove"), this, false);
  aNode->RemoveSystemEventListener(NS_LITERAL_STRING("mousedown"), this, false);
  aNode->RemoveSystemEventListener(NS_LITERAL_STRING("mouseup"), this, false);
  aNode->RemoveSystemEventListener(NS_LITERAL_STRING("dragstart"), this, true);

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
  nsIDocument* doc = sourceNode->GetComposedDoc();
  if (doc) {
    ErrorResult ignored;
    bx = doc->GetBoxObjectFor(doc->GetRootElement(), ignored);
  }

  nsCOMPtr<nsITreeBoxObject> obx;
  GetSourceTreeBoxObject(getter_AddRefs(obx));
  if (bx && obx) {
    int32_t x, y;
    aMouseEvent->GetScreenX(&x);
    aMouseEvent->GetScreenY(&y);

    int32_t row;
    nsCOMPtr<nsITreeColumn> col;
    nsAutoCString obj;

    
    int32_t boxX, boxY;
    bx->GetScreenX(&boxX);
    bx->GetScreenY(&boxY);
    x -= boxX;
    y -= boxY;

    obx->GetCellAt(x, y, &row, getter_AddRefs(col), obj);

    
    
    mNeedTitletip = false;
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

  
  nsCOMPtr<nsIDOMXULDocument> xulDoc =
    do_QueryInterface(tooltipNode->GetComposedDoc());
  if (xulDoc) {
    
    
    if (sourceNode->IsInComposedDoc()) {
#ifdef MOZ_XUL
      if (!mIsSourceTree) {
        mLastTreeRow = -1;
        mLastTreeCol = nullptr;
      }
#endif

      mCurrentTooltip = do_GetWeakReference(tooltipNode);
      LaunchTooltip();
      mTargetNode = nullptr;

      nsCOMPtr<nsIContent> currentTooltip = do_QueryReferent(mCurrentTooltip);
      if (!currentTooltip)
        return NS_OK;

      
      
      currentTooltip->AddSystemEventListener(NS_LITERAL_STRING("popuphiding"), 
                                             this, false, false);

      
      nsIDocument* doc = sourceNode->GetComposedDoc();
      if (doc) {
        
        
        
        
        
        doc->AddSystemEventListener(NS_LITERAL_STRING("DOMMouseScroll"),
                                    this, true);
        doc->AddSystemEventListener(NS_LITERAL_STRING("mousedown"),
                                    this, true);
        doc->AddSystemEventListener(NS_LITERAL_STRING("mouseup"),
                                    this, true);
        doc->AddSystemEventListener(NS_LITERAL_STRING("keydown"),
                                    this, true);
      }
      mSourceNode = nullptr;
    }
  }

  return NS_OK;
}

#ifdef MOZ_XUL


#ifdef DEBUG_crap
static void
GetTreeCellCoords(nsITreeBoxObject* aTreeBox, nsIContent* aSourceNode, 
                  int32_t aRow, nsITreeColumn* aCol, int32_t* aX, int32_t* aY)
{
  int32_t junk;
  aTreeBox->GetCoordsForCellItem(aRow, aCol, EmptyCString(), aX, aY, &junk, &junk);
  nsCOMPtr<nsIDOMXULElement> xulEl(do_QueryInterface(aSourceNode));
  nsCOMPtr<nsIBoxObject> bx;
  xulEl->GetBoxObject(getter_AddRefs(bx));
  int32_t myX, myY;
  bx->GetX(&myX);
  bx->GetY(&myY);
  *aX += myX;
  *aY += myY;
}
#endif

static void
SetTitletipLabel(nsITreeBoxObject* aTreeBox, nsIContent* aTooltip,
                 int32_t aRow, nsITreeColumn* aCol)
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
    aTooltip->SetAttr(kNameSpaceID_None, nsGkAtoms::label, label, true);
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
    currentTooltip->SetAttr(kNameSpaceID_None, nsGkAtoms::titletip, NS_LITERAL_STRING("true"), true);
  } else {
    currentTooltip->UnsetAttr(kNameSpaceID_None, nsGkAtoms::titletip, true);
  }
  if (!(currentTooltip = do_QueryReferent(mCurrentTooltip))) {
    
    return;
  }

  nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
  if (pm) {
    nsCOMPtr<nsIContent> target = do_QueryReferent(mTargetNode);
    pm->ShowTooltipAtScreen(currentTooltip, target, mMouseScreenX, mMouseScreenY);

    
    if (!pm->IsPopupOpen(currentTooltip))
      mCurrentTooltip = nullptr;
  }
#endif

}

nsresult
nsXULTooltipListener::HideTooltip()
{
#ifdef MOZ_XUL
  nsCOMPtr<nsIContent> currentTooltip = do_QueryReferent(mCurrentTooltip);
  if (currentTooltip) {
    nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
    if (pm)
      pm->HidePopup(currentTooltip, false, false, false, false);
  }
#endif

  DestroyTooltip();
  return NS_OK;
}

static void
GetImmediateChild(nsIContent* aContent, nsIAtom *aTag, nsIContent** aResult) 
{
  *aResult = nullptr;
  uint32_t childCount = aContent->GetChildCount();
  for (uint32_t i = 0; i < childCount; i++) {
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

  
  nsIDocument *document = aTarget->GetComposedDoc();
  if (!document) {
    NS_WARNING("Unable to retrieve the tooltip node document.");
    return NS_ERROR_FAILURE;
  }
  nsPIDOMWindow *window = document->GetWindow();
  if (!window) {
    return NS_OK;
  }

  bool closed;
  window->GetClosed(&closed);

  if (closed) {
    return NS_OK;
  }

  nsAutoString tooltipText;
  aTarget->GetAttr(kNameSpaceID_None, nsGkAtoms::tooltiptext, tooltipText);
  if (!tooltipText.IsEmpty()) {
    
    nsIRootBox* rootBox = nsIRootBox::GetRootBox(document->GetShell());
    NS_ENSURE_STATE(rootBox);
    *aTooltip = rootBox->GetDefaultTooltip();
    if (*aTooltip) {
      NS_ADDREF(*aTooltip);
      (*aTooltip)->SetAttr(kNameSpaceID_None, nsGkAtoms::label, tooltipText, true);
    }
    return NS_OK;
  }

  nsAutoString tooltipId;
  aTarget->GetAttr(kNameSpaceID_None, nsGkAtoms::tooltip, tooltipId);

  
  if (tooltipId.EqualsLiteral("_child")) {
    GetImmediateChild(aTarget, nsGkAtoms::tooltip, aTooltip);
    return NS_OK;
  }

  if (!tooltipId.IsEmpty() && aTarget->IsInUncomposedDoc()) {
    
    
    
    nsCOMPtr<nsIContent> tooltipEl = document->GetElementById(tooltipId);

    if (tooltipEl) {
#ifdef MOZ_XUL
      mNeedTitletip = false;
#endif
      tooltipEl.forget(aTooltip);
      return NS_OK;
    }
  }

#ifdef MOZ_XUL
  
  if (mIsSourceTree && mNeedTitletip) {
    nsIRootBox* rootBox = nsIRootBox::GetRootBox(document->GetShell());
    NS_ENSURE_STATE(rootBox);
    NS_IF_ADDREF(*aTooltip = rootBox->GetDefaultTooltip());
  }
#endif

  return NS_OK;
}


nsresult
nsXULTooltipListener::GetTooltipFor(nsIContent* aTarget, nsIContent** aTooltip)
{
  *aTooltip = nullptr;
  nsCOMPtr<nsIContent> tooltip;
  nsresult rv = FindTooltip(aTarget, getter_AddRefs(tooltip));
  if (NS_FAILED(rv) || !tooltip) {
    return rv;
  }

#ifdef MOZ_XUL
  
  nsIContent* parent = tooltip->GetParent();
  if (parent) {
    nsMenuFrame* menu = do_QueryFrame(parent->GetPrimaryFrame());
    if (menu) {
      NS_WARNING("Menu cannot be used as a tooltip");
      return NS_ERROR_FAILURE;
    }
  }
#endif

  tooltip.swap(*aTooltip);
  return rv;
}

nsresult
nsXULTooltipListener::DestroyTooltip()
{
  nsCOMPtr<nsIDOMEventListener> kungFuDeathGrip(this);
  nsCOMPtr<nsIContent> currentTooltip = do_QueryReferent(mCurrentTooltip);
  if (currentTooltip) {
    
    
    mCurrentTooltip = nullptr;

    
    nsCOMPtr<nsIDocument> doc = currentTooltip->GetComposedDoc();
    if (doc) {
      
      doc->RemoveSystemEventListener(NS_LITERAL_STRING("DOMMouseScroll"), this,
                                     true);
      doc->RemoveSystemEventListener(NS_LITERAL_STRING("mousedown"), this,
                                     true);
      doc->RemoveSystemEventListener(NS_LITERAL_STRING("mouseup"), this, true);
      doc->RemoveSystemEventListener(NS_LITERAL_STRING("keydown"), this, true);
    }

    
    currentTooltip->RemoveSystemEventListener(NS_LITERAL_STRING("popuphiding"), this, false);
  }

  
  KillTooltipTimer();
  mSourceNode = nullptr;
#ifdef MOZ_XUL
  mLastTreeCol = nullptr;
#endif

  return NS_OK;
}

void
nsXULTooltipListener::KillTooltipTimer()
{
  if (mTooltipTimer) {
    mTooltipTimer->Cancel();
    mTooltipTimer = nullptr;
    mTargetNode = nullptr;
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
  *aBoxObject = nullptr;

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
