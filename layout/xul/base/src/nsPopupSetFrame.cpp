







































#include "nsGkAtoms.h"
#include "nsPopupSetFrame.h"
#include "nsIMenuParent.h"
#include "nsMenuFrame.h"
#include "nsBoxFrame.h"
#include "nsIContent.h"
#include "prtypes.h"
#include "nsIAtom.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "nsCSSRendering.h"
#include "nsINameSpaceManager.h"
#include "nsMenuPopupFrame.h"
#include "nsMenuBarFrame.h"
#include "nsIView.h"
#include "nsIWidget.h"
#include "nsIDocument.h"
#include "nsIDOMNSDocument.h"
#include "nsIDOMDocument.h"
#include "nsIDOMXULDocument.h"
#include "nsIDOMElement.h"
#include "nsISupportsArray.h"
#include "nsIDOMText.h"
#include "nsBoxLayoutState.h"
#include "nsIScrollableFrame.h"
#include "nsCSSFrameConstructor.h"
#include "nsGUIEvent.h"
#include "nsIRootBox.h"
#include "nsIFocusController.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDocShell.h"
#include "nsPIDOMWindow.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIBaseWindow.h"
#include "nsIViewManager.h"

#define NS_MENU_POPUP_LIST_INDEX   0

nsPopupFrameList::nsPopupFrameList(nsIContent* aPopupContent, nsPopupFrameList* aNext)
:mNextPopup(aNext), 
 mPopupFrame(nsnull),
 mPopupContent(aPopupContent),
 mElementContent(nsnull), 
 mCreateHandlerSucceeded(PR_FALSE),
 mIsOpen(PR_FALSE),
 mLastPref(-1,-1)
{
}

nsPopupFrameList* nsPopupFrameList::GetEntry(nsIContent* aPopupContent) {
  if (aPopupContent == mPopupContent)
    return this;

  if (mNextPopup)
    return mNextPopup->GetEntry(aPopupContent);

  return nsnull;
}

nsPopupFrameList* nsPopupFrameList::GetEntryByFrame(nsIFrame* aPopupFrame) {
  if (aPopupFrame == mPopupFrame)
    return this;

  if (mNextPopup)
    return mNextPopup->GetEntryByFrame(aPopupFrame);

  return nsnull;
}






nsIFrame*
NS_NewPopupSetFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsPopupSetFrame (aPresShell, aContext);
}

NS_IMETHODIMP_(nsrefcnt) 
nsPopupSetFrame::AddRef(void)
{
  return NS_OK;
}

NS_IMETHODIMP_(nsrefcnt) 
nsPopupSetFrame::Release(void)
{
    return NS_OK;
}




NS_INTERFACE_MAP_BEGIN(nsPopupSetFrame)
  NS_INTERFACE_MAP_ENTRY(nsIPopupSetFrame)
NS_INTERFACE_MAP_END_INHERITING(nsBoxFrame)

NS_IMETHODIMP
nsPopupSetFrame::Init(nsIContent*      aContent,
                      nsIFrame*        aParent,
                      nsIFrame*        aPrevInFlow)
{
  nsresult  rv = nsBoxFrame::Init(aContent, aParent, aPrevInFlow);

  nsIRootBox *rootBox;
  nsresult res = CallQueryInterface(aParent->GetParent(), &rootBox);
  NS_ASSERTION(NS_SUCCEEDED(res), "grandparent should be root box");
  if (NS_SUCCEEDED(res)) {
    rootBox->SetPopupSetFrame(this);
  }

  return rv;
}

NS_IMETHODIMP
nsPopupSetFrame::AppendFrames(nsIAtom*        aListName,
                              nsIFrame*       aFrameList)
{
  if (aListName == nsGkAtoms::popupList) {
    return AddPopupFrameList(aFrameList);
  }
  return nsBoxFrame::AppendFrames(aListName, aFrameList);
}

NS_IMETHODIMP
nsPopupSetFrame::RemoveFrame(nsIAtom*        aListName,
                             nsIFrame*       aOldFrame)
{
  if (aListName == nsGkAtoms::popupList) {
    return RemovePopupFrame(aOldFrame);
  }
  return nsBoxFrame::RemoveFrame(aListName, aOldFrame);
}

NS_IMETHODIMP
nsPopupSetFrame::InsertFrames(nsIAtom*        aListName,
                              nsIFrame*       aPrevFrame,
                              nsIFrame*       aFrameList)
{
  if (aListName == nsGkAtoms::popupList) {
    return AddPopupFrameList(aFrameList);
  }
  return nsBoxFrame::InsertFrames(aListName, aPrevFrame, aFrameList);
}

NS_IMETHODIMP
nsPopupSetFrame::SetInitialChildList(nsIAtom*        aListName,
                                     nsIFrame*       aChildList)
{
  if (aListName == nsGkAtoms::popupList) {
    return AddPopupFrameList(aChildList);
  }
  return nsBoxFrame::SetInitialChildList(aListName, aChildList);
}

void
nsPopupSetFrame::Destroy()
{
  
  if (mPopupList) {
    
    if (nsMenuDismissalListener::sInstance) {
      nsIMenuParent *menuParent =
        nsMenuDismissalListener::sInstance->GetCurrentMenuParent();
      nsIFrame* frame;
      CallQueryInterface(menuParent, &frame);
      
      if (frame && mPopupList->GetEntryByFrame(frame)) {
        nsMenuDismissalListener::sInstance->Rollup();
      }
    }

    
    
    while (mPopupList) {
      if (mPopupList->mPopupFrame) {
        mPopupList->mPopupFrame->Destroy();
      }

      nsPopupFrameList* temp = mPopupList;
      mPopupList = mPopupList->mNextPopup;
      delete temp;
    }
  }

  nsIRootBox *rootBox;
  nsresult res = CallQueryInterface(mParent->GetParent(), &rootBox);
  NS_ASSERTION(NS_SUCCEEDED(res), "grandparent should be root box");
  if (NS_SUCCEEDED(res)) {
    rootBox->SetPopupSetFrame(nsnull);
  }

  nsBoxFrame::Destroy();
}

NS_IMETHODIMP
nsPopupSetFrame::DoLayout(nsBoxLayoutState& aState)
{
  
  nsresult rv = nsBoxFrame::DoLayout(aState);

  
  nsPopupFrameList* currEntry = mPopupList;
  while (currEntry) {
    nsIFrame* popupChild = currEntry->mPopupFrame;
    if (popupChild) {
      NS_ASSERTION(popupChild->IsBoxFrame(), "popupChild is not box!!");

      
      nsSize prefSize = popupChild->GetPrefSize(aState);
      nsSize minSize = popupChild->GetMinSize(aState);
      nsSize maxSize = popupChild->GetMaxSize(aState);

      BoundsCheck(minSize, prefSize, maxSize);

      
      
     
        popupChild->SetBounds(aState, nsRect(0,0,prefSize.width, prefSize.height));
        RepositionPopup(currEntry, aState);
        currEntry->mLastPref = prefSize;
     

      
      nsIBox* child = popupChild->GetChildBox();

      nsRect bounds(popupChild->GetRect());

      nsCOMPtr<nsIScrollableFrame> scrollframe = do_QueryInterface(child);
      if (scrollframe &&
          scrollframe->GetScrollbarStyles().mVertical == NS_STYLE_OVERFLOW_AUTO) {
        
        if (bounds.height < prefSize.height) {
          
          popupChild->Layout(aState);

          nsMargin scrollbars = scrollframe->GetActualScrollbarSizes();
          if (bounds.width < prefSize.width + scrollbars.left + scrollbars.right)
          {
            bounds.width += scrollbars.left + scrollbars.right;
            
            popupChild->SetBounds(aState, bounds);
          }
        }
      }
    
      
      popupChild->Layout(aState);

      
      if (currEntry->mCreateHandlerSucceeded) {
        nsIView* view = popupChild->GetView();
        nsIViewManager* viewManager = view->GetViewManager();
        nsRect r(0, 0, bounds.width, bounds.height);
        viewManager->ResizeView(view, r);
        viewManager->SetViewVisibility(view, nsViewVisibility_kShow);
      }
    }

    currEntry = currEntry->mNextPopup;
  }

  SyncLayout(aState);

  return rv;
}


#ifdef DEBUG_LAYOUT
NS_IMETHODIMP
nsPopupSetFrame::SetDebug(nsBoxLayoutState& aState, PRBool aDebug)
{
  
  PRBool debugSet = mState & NS_STATE_CURRENTLY_IN_DEBUG;
  PRBool debugChanged = (!aDebug && debugSet) || (aDebug && !debugSet);

  
  if (debugChanged)
  {
    
  }

  return NS_OK;
}

nsresult
nsPopupSetFrame::SetDebug(nsBoxLayoutState& aState, nsIFrame* aList, PRBool aDebug)
{
      if (!aList)
          return NS_OK;

      while (aList) {
        if (aList->IsBoxFrame())
          aList->SetDebug(aState, aDebug);

        aList = aList->GetNextSibling();
      }

      return NS_OK;
}
#endif


void
nsPopupSetFrame::RepositionPopup(nsPopupFrameList* aEntry, nsBoxLayoutState& aState)
{
  
  if (aEntry && aEntry->mElementContent) {
    nsPresContext* presContext = aState.PresContext();
    nsIFrame* frameToSyncTo = presContext->PresShell()->
      GetPrimaryFrameFor(aEntry->mElementContent);
    ((nsMenuPopupFrame*)(aEntry->mPopupFrame))->SyncViewWithFrame(presContext, 
          aEntry->mPopupAnchor, aEntry->mPopupAlign, frameToSyncTo, aEntry->mXPos, aEntry->mYPos);
  }
}

NS_IMETHODIMP
nsPopupSetFrame::ShowPopup(nsIContent* aElementContent, nsIContent* aPopupContent, 
                           PRInt32 aXPos, PRInt32 aYPos, 
                           const nsString& aPopupType, const nsString& anAnchorAlignment,
                           const nsString& aPopupAlignment)
{
  if (!MayOpenPopup(this))
    return NS_OK;

  nsWeakFrame weakFrame(this);
  
  if (!OnCreate(aXPos, aYPos, aPopupContent) || !weakFrame.IsAlive())
    return NS_OK;
        
  
  nsPopupFrameList* entry = nsnull;
  if (mPopupList)
    entry = mPopupList->GetEntry(aPopupContent);
  if (!entry) {
    entry = new nsPopupFrameList(aPopupContent, mPopupList);
    if (!entry)
      return NS_ERROR_OUT_OF_MEMORY;
    mPopupList = entry;
  }

  
  entry->mPopupType = aPopupType;
  entry->mElementContent = aElementContent;
  entry->mPopupAlign = aPopupAlignment;
  entry->mPopupAnchor = anAnchorAlignment;
  entry->mXPos = aXPos;
  entry->mYPos = aYPos;

  
  entry->mPopupFrame = PresContext()->PresShell()
    ->GetPrimaryFrameFor(aPopupContent);

#ifdef DEBUG_PINK
  printf("X Pos: %d\n", mXPos);
  printf("Y Pos: %d\n", mYPos);
#endif

  
  entry->mCreateHandlerSucceeded = PR_TRUE;
  entry->mIsOpen = PR_TRUE;
  
  
  MarkAsGenerated(aPopupContent);

  if (!weakFrame.IsAlive()) {
    return NS_OK;
  }

  nsPopupFrameList* newEntry =
    mPopupList ? mPopupList->GetEntry(aPopupContent) : nsnull;
  if (!newEntry || newEntry != entry) {
    NS_WARNING("The popup entry for aPopupContent has changed!");
    return NS_OK;
  }

  
  nsIMenuParent* childPopup = nsnull;
  if (entry->mPopupFrame)
    CallQueryInterface(entry->mPopupFrame, &childPopup);
  if ( childPopup && aPopupType.EqualsLiteral("context") )
    childPopup->SetIsContextMenu(PR_TRUE);

  
  OpenPopup(entry, PR_TRUE);
  
  if (!weakFrame.IsAlive()) {
    return NS_OK;
  }

  
  OnCreated(aXPos, aYPos, aPopupContent);

  return NS_OK;
}

NS_IMETHODIMP
nsPopupSetFrame::HidePopup(nsIFrame* aPopup)
{
  if (!mPopupList)
    return NS_OK; 

  nsPopupFrameList* entry = mPopupList->GetEntryByFrame(aPopup);
  if (!entry)
    return NS_OK;

  if (entry->mCreateHandlerSucceeded)
    ActivatePopup(entry, PR_FALSE);

  if (entry->mElementContent && entry->mPopupType.EqualsLiteral("context")) {
    
    
    
    if (entry->mElementContent->Tag() == nsGkAtoms::menupopup) {
      nsIFrame* popupFrame = PresContext()->PresShell()
        ->GetPrimaryFrameFor(entry->mElementContent);
      if (popupFrame) {
        nsIMenuParent *menuParent;
        if (NS_SUCCEEDED(CallQueryInterface(popupFrame, &menuParent))) {
          menuParent->HideChain();
        }
      }
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsPopupSetFrame::DestroyPopup(nsIFrame* aPopup, PRBool aDestroyEntireChain)
{
  if (!mPopupList)
    return NS_OK; 

  nsPopupFrameList* entry = mPopupList->GetEntryByFrame(aPopup);

  if (entry && entry->mCreateHandlerSucceeded) {    
    nsWeakFrame weakFrame(this);
    OpenPopup(entry, PR_FALSE);
    nsCOMPtr<nsIContent> popupContent = entry->mPopupContent;
    if (weakFrame.IsAlive()) {
      if (aDestroyEntireChain && entry->mElementContent && entry->mPopupType.EqualsLiteral("context")) {
        
        
        
        if (entry->mElementContent->Tag() == nsGkAtoms::menupopup) {
          nsIFrame* popupFrame = PresContext()->PresShell()
            ->GetPrimaryFrameFor(entry->mElementContent);
          if (popupFrame) {
            nsIMenuParent *menuParent;
            if (NS_SUCCEEDED(CallQueryInterface(popupFrame, &menuParent))) {
              menuParent->DismissChain();
            }
          }
        }
      }
  
      
      entry->mPopupType.Truncate();
      entry->mCreateHandlerSucceeded = PR_FALSE;
      entry->mElementContent = nsnull;
      entry->mXPos = entry->mYPos = 0;
      entry->mLastPref.width = -1;
      entry->mLastPref.height = -1;
    }
    
    popupContent->UnsetAttr(kNameSpaceID_None, nsGkAtoms::menugenerated, PR_TRUE);
  }

  return NS_OK;
}

void
nsPopupSetFrame::MarkAsGenerated(nsIContent* aPopupContent)
{
  
  
  if (!aPopupContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::menugenerated,
                                  nsGkAtoms::_true, eCaseMatters)) {
    
    aPopupContent->SetAttr(kNameSpaceID_None, nsGkAtoms::menugenerated, NS_LITERAL_STRING("true"),
                           PR_TRUE);
  }
}

void
nsPopupSetFrame::OpenPopup(nsPopupFrameList* aEntry, PRBool aActivateFlag)
{
  nsWeakFrame weakFrame(this);
  nsIFrame* activeChild = aEntry->mPopupFrame;
  nsWeakFrame weakPopupFrame(activeChild);
  nsRefPtr<nsPresContext> presContext = PresContext();
  nsCOMPtr<nsIContent> popupContent = aEntry->mPopupContent;
  PRBool createHandlerSucceeded = aEntry->mCreateHandlerSucceeded;
  nsAutoString popupType = aEntry->mPopupType;
  if (aActivateFlag) {
    ActivatePopup(aEntry, PR_TRUE);

    
    if (!popupType.EqualsLiteral("tooltip")) {
      nsIMenuParent* childPopup = nsnull;
      if (weakPopupFrame.IsAlive())
        CallQueryInterface(activeChild, &childPopup);

      
      if (childPopup && !nsMenuDismissalListener::sInstance) {
        
        if (!popupContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::ignorekeys,
                                       nsGkAtoms::_true, eCaseMatters))
          childPopup->InstallKeyboardNavigator();
      }

      nsMenuDismissalListener* listener = nsMenuDismissalListener::GetInstance();
      if (listener)
        listener->SetCurrentMenuParent(childPopup);
    }
  }
  else {
    if (createHandlerSucceeded && !OnDestroy(popupContent))
      return;

    
    if (!popupType.EqualsLiteral("tooltip") ) {
      nsMenuDismissalListener::Shutdown();
    }
    
    
    nsIMenuParent* childPopup = nsnull;
    if (weakPopupFrame.IsAlive())
      CallQueryInterface(activeChild, &childPopup);
    if (childPopup)
      childPopup->RemoveKeyboardNavigator();

    if (weakPopupFrame.IsAlive())
      ActivatePopup(aEntry, PR_FALSE);

    OnDestroyed(presContext, popupContent);
  }

  if (weakFrame.IsAlive()) {
    AddStateBits(NS_FRAME_HAS_DIRTY_CHILDREN);
    PresContext()->PresShell()->FrameNeedsReflow(this, nsIPresShell::eTreeChange);
  }
}

void
nsPopupSetFrame::ActivatePopup(nsPopupFrameList* aEntry, PRBool aActivateFlag)
{
  if (aEntry->mPopupContent) {
    
    
    
    if (aActivateFlag)
      
      aEntry->mPopupContent->SetAttr(kNameSpaceID_None, nsGkAtoms::menutobedisplayed, NS_LITERAL_STRING("true"), PR_TRUE);
    else {
      nsWeakFrame weakFrame(this);
      nsWeakFrame weakActiveChild(aEntry->mPopupFrame);
      nsCOMPtr<nsIContent> content = aEntry->mPopupContent;
      content->UnsetAttr(kNameSpaceID_None, nsGkAtoms::menuactive, PR_TRUE);
      content->UnsetAttr(kNameSpaceID_None, nsGkAtoms::menutobedisplayed, PR_TRUE);

      
      
      
      
      nsIDocument* doc = content->GetDocument();
      if (doc)
        doc->FlushPendingNotifications(Flush_OnlyReflow);

      
      
      
      if (weakFrame.IsAlive() && weakActiveChild.IsAlive()) {
        nsIView* view = weakActiveChild.GetFrame()->GetView();
        NS_ASSERTION(view, "View is gone, looks like someone forgot to roll up the popup!");
        if (view) {
          nsIViewManager* viewManager = view->GetViewManager();
          viewManager->SetViewVisibility(view, nsViewVisibility_kHide);
          nsRect r(0, 0, 0, 0);
          viewManager->ResizeView(view, r);
          if (aEntry->mIsOpen) {
            aEntry->mIsOpen = PR_FALSE;
            FireDOMEventSynch(NS_LITERAL_STRING("DOMMenuInactive"), content);
          }
        }
      }
    }
  }
}

PRBool
nsPopupSetFrame::OnCreate(PRInt32 aX, PRInt32 aY, nsIContent* aPopupContent)
{
  nsEventStatus status = nsEventStatus_eIgnore;
  nsMouseEvent event(PR_TRUE, NS_XUL_POPUP_SHOWING, nsnull,
                     nsMouseEvent::eReal);
  
  nsPoint dummy;
  event.widget = GetClosestView()->GetNearestWidget(&dummy);
  event.refPoint.x = aX;
  event.refPoint.y = aY;

  if (aPopupContent) {
    nsCOMPtr<nsIContent> kungFuDeathGrip(aPopupContent);
    nsCOMPtr<nsIPresShell> shell = PresContext()->GetPresShell();
    if (shell) {
      nsresult rv = shell->HandleDOMEventWithTarget(aPopupContent, &event,
                                                    &status);
      if ( NS_FAILED(rv) || status == nsEventStatus_eConsumeNoDefault )
        return PR_FALSE;
    }

    nsCOMPtr<nsIDOMDocument> domDoc(do_QueryInterface(aPopupContent->GetDocument()));
    if (!domDoc) return PR_FALSE;

    
    
    
    

    PRUint32 count = aPopupContent->GetChildCount();
    for (PRUint32 i = 0; i < count; i++) {
      nsCOMPtr<nsIContent> grandChild = aPopupContent->GetChildAt(i);

      if (grandChild->Tag() == nsGkAtoms::menuitem) {
        
        nsAutoString command;
        grandChild->GetAttr(kNameSpaceID_None, nsGkAtoms::command, command);
        if (!command.IsEmpty()) {
          
          nsCOMPtr<nsIDOMElement> commandElt;
          domDoc->GetElementById(command, getter_AddRefs(commandElt));
          nsCOMPtr<nsIContent> commandContent(do_QueryInterface(commandElt));
          if ( commandContent ) {
            nsAutoString commandValue;
            
            if (commandContent->GetAttr(kNameSpaceID_None, nsGkAtoms::disabled, commandValue))
              grandChild->SetAttr(kNameSpaceID_None, nsGkAtoms::disabled, commandValue, PR_TRUE);
            else
              grandChild->UnsetAttr(kNameSpaceID_None, nsGkAtoms::disabled, PR_TRUE);

            
            
            
            if (commandContent->GetAttr(kNameSpaceID_None, nsGkAtoms::label, commandValue))
              grandChild->SetAttr(kNameSpaceID_None, nsGkAtoms::label, commandValue, PR_TRUE);

            if (commandContent->GetAttr(kNameSpaceID_None, nsGkAtoms::accesskey, commandValue))
              grandChild->SetAttr(kNameSpaceID_None, nsGkAtoms::accesskey, commandValue, PR_TRUE);

            if (commandContent->GetAttr(kNameSpaceID_None, nsGkAtoms::checked, commandValue))
              grandChild->SetAttr(kNameSpaceID_None, nsGkAtoms::checked, commandValue, PR_TRUE);
          }
        }
      }
    }
  }

  return PR_TRUE;
}

PRBool
nsPopupSetFrame::OnCreated(PRInt32 aX, PRInt32 aY, nsIContent* aPopupContent)
{
  nsEventStatus status = nsEventStatus_eIgnore;
  nsMouseEvent event(PR_TRUE, NS_XUL_POPUP_SHOWN, nsnull,
                     nsMouseEvent::eReal);
  
  
  

  if (aPopupContent) {
    nsCOMPtr<nsIPresShell> shell = PresContext()->GetPresShell();
    if (shell) {
      nsresult rv = shell->HandleDOMEventWithTarget(aPopupContent, &event,
                                                    &status);
      if ( NS_FAILED(rv) || status == nsEventStatus_eConsumeNoDefault )
        return PR_FALSE;
    }
  }

  return PR_TRUE;
}

PRBool
nsPopupSetFrame::OnDestroy(nsIContent* aPopupContent)
{
  nsEventStatus status = nsEventStatus_eIgnore;
  nsMouseEvent event(PR_TRUE, NS_XUL_POPUP_HIDING, nsnull,
                     nsMouseEvent::eReal);

  if (aPopupContent) {
    nsCOMPtr<nsIPresShell> shell = PresContext()->GetPresShell();
    if (shell) {
      nsresult rv = shell->HandleDOMEventWithTarget(aPopupContent, &event,
                                                    &status);
      if ( NS_FAILED(rv) || status == nsEventStatus_eConsumeNoDefault )
        return PR_FALSE;
    }
  }
  return PR_TRUE;
}

PRBool
nsPopupSetFrame::OnDestroyed(nsPresContext* aPresContext,
                             nsIContent* aPopupContent)
{
  nsEventStatus status = nsEventStatus_eIgnore;
  nsMouseEvent event(PR_TRUE, NS_XUL_POPUP_HIDDEN, nsnull,
                     nsMouseEvent::eReal);

  if (aPopupContent && aPresContext) {
    nsCOMPtr<nsIPresShell> shell = aPresContext->GetPresShell();
    if (shell) {
      nsresult rv = shell->HandleDOMEventWithTarget(aPopupContent, &event,
                                                    &status);
      if ( NS_FAILED(rv) || status == nsEventStatus_eConsumeNoDefault )
        return PR_FALSE;
    }
  }
  return PR_TRUE;
}

nsresult
nsPopupSetFrame::RemovePopupFrame(nsIFrame* aPopup)
{
  
  
  nsPopupFrameList* currEntry = mPopupList;
  nsPopupFrameList* temp = nsnull;
  while (currEntry) {
    if (currEntry->mPopupFrame == aPopup) {
      
      if (temp)
        temp->mNextPopup = currEntry->mNextPopup;
      else
        mPopupList = currEntry->mNextPopup;
      
      
      currEntry->mPopupFrame->Destroy();

      
      currEntry->mNextPopup = nsnull;
      delete currEntry;

      
      break;
    }

    temp = currEntry;
    currEntry = currEntry->mNextPopup;
  }

  return NS_OK;
}

nsresult
nsPopupSetFrame::AddPopupFrameList(nsIFrame* aPopupFrameList)
{
  for (nsIFrame* kid = aPopupFrameList; kid; kid = kid->GetNextSibling()) {
    nsresult rv = AddPopupFrame(kid);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}

nsresult
nsPopupSetFrame::AddPopupFrame(nsIFrame* aPopup)
{
  
  

  
  nsIContent* content = aPopup->GetContent();
  nsPopupFrameList* entry = nsnull;
  if (mPopupList)
    entry = mPopupList->GetEntry(content);
  if (!entry) {
    entry = new nsPopupFrameList(content, mPopupList);
    if (!entry)
      return NS_ERROR_OUT_OF_MEMORY;
    mPopupList = entry;
  }
  else {
    NS_ASSERTION(!entry->mPopupFrame, "Leaking a popup frame");
  }

  
  entry->mPopupFrame = aPopup;
  
  
  
  return NS_OK;
}


PRBool
nsPopupSetFrame::MayOpenPopup(nsIFrame* aFrame)
{
  nsCOMPtr<nsISupports> cont = aFrame->PresContext()->GetContainer();
  nsCOMPtr<nsIDocShellTreeItem> dsti = do_QueryInterface(cont);
  if (!dsti)
    return PR_FALSE;

  
  PRInt32 type = -1;
  if (NS_SUCCEEDED(dsti->GetItemType(&type)) && type == nsIDocShellTreeItem::typeChrome)
    return PR_TRUE;

  nsCOMPtr<nsIDocShell> shell = do_QueryInterface(dsti);
  if (!shell)
    return PR_FALSE;

  nsCOMPtr<nsPIDOMWindow> win = do_GetInterface(shell);
  if (!win)
    return PR_FALSE;

  
  PRBool active;
  nsIFocusController* focusController = win->GetRootFocusController();
  focusController->GetActive(&active);
  if (!active)
    return PR_FALSE;

  nsCOMPtr<nsIBaseWindow> baseWin = do_QueryInterface(shell);
  if (!baseWin)
    return PR_FALSE;

  
  PRBool visible;
  baseWin->GetVisibility(&visible);
  return visible;
}

