






































#include "nsMenuListener.h"
#include "nsMenuBarFrame.h"
#include "nsIServiceManager.h"
#include "nsIContent.h"
#include "nsContentUtils.h"
#include "prtypes.h"
#include "nsIAtom.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "nsCSSRendering.h"
#include "nsINameSpaceManager.h"
#include "nsIDocument.h"
#include "nsIDOMEventTarget.h"
#include "nsGkAtoms.h"
#include "nsMenuFrame.h"
#include "nsMenuPopupFrame.h"
#include "nsGUIEvent.h"
#include "nsUnicharUtils.h"
#include "nsICaret.h"
#include "nsIFocusController.h"
#include "nsIDOMWindowInternal.h"
#include "nsIDOMDocument.h"
#include "nsPIDOMWindow.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsCSSFrameConstructor.h"
#ifdef XP_WIN
#include "nsISound.h"
#include "nsWidgetsCID.h"
#endif







nsIFrame*
NS_NewMenuBarFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsMenuBarFrame (aPresShell, aContext);
}

NS_IMETHODIMP_(nsrefcnt) 
nsMenuBarFrame::AddRef(void)
{
  return NS_OK;
}

NS_IMETHODIMP_(nsrefcnt) 
nsMenuBarFrame::Release(void)
{
    return NS_OK;
}





NS_INTERFACE_MAP_BEGIN(nsMenuBarFrame)
  NS_INTERFACE_MAP_ENTRY(nsIMenuParent)
NS_INTERFACE_MAP_END_INHERITING(nsBoxFrame)





nsMenuBarFrame::nsMenuBarFrame(nsIPresShell* aShell, nsStyleContext* aContext):
  nsBoxFrame(aShell, aContext),
    mMenuBarListener(nsnull),
    mKeyboardNavigator(nsnull),
    mIsActive(PR_FALSE),
    mTarget(nsnull),
    mCaretWasVisible(PR_FALSE)
{
} 

nsMenuBarFrame::~nsMenuBarFrame()
{
  





  SetActive(PR_FALSE);
}

NS_IMETHODIMP
nsMenuBarFrame::Init(nsIContent*      aContent,
                     nsIFrame*        aParent,
                     nsIFrame*        aPrevInFlow)
{
  nsresult  rv = nsBoxFrame::Init(aContent, aParent, aPrevInFlow);

  
  mMenuBarListener = new nsMenuBarListener(this);
  NS_IF_ADDREF(mMenuBarListener);
  if (! mMenuBarListener)
    return NS_ERROR_OUT_OF_MEMORY;

  
  
  nsCOMPtr<nsIDOMEventTarget> target = do_QueryInterface(aContent->GetDocument());
  
  mTarget = target;

  
  
  
  target->AddEventListener(NS_LITERAL_STRING("keypress"), (nsIDOMKeyListener*)mMenuBarListener, PR_FALSE); 
  target->AddEventListener(NS_LITERAL_STRING("keydown"), (nsIDOMKeyListener*)mMenuBarListener, PR_FALSE);  
  target->AddEventListener(NS_LITERAL_STRING("keyup"), (nsIDOMKeyListener*)mMenuBarListener, PR_FALSE);   

  target->AddEventListener(NS_LITERAL_STRING("mousedown"), (nsIDOMMouseListener*)mMenuBarListener, PR_FALSE);   
  target->AddEventListener(NS_LITERAL_STRING("blur"), (nsIDOMFocusListener*)mMenuBarListener, PR_TRUE);   

  return rv;
}

NS_IMETHODIMP
nsMenuBarFrame::IsOpen()
{
  PRBool isOpen = PR_FALSE;
  if(mCurrentMenu) {
    mCurrentMenu->MenuIsOpen(isOpen);
    if (isOpen) {
      return PR_TRUE;
    }
  }
  return PR_FALSE;
}


NS_IMETHODIMP
nsMenuBarFrame::SetActive(PRBool aActiveFlag)
{
  
  if (mIsActive == aActiveFlag)
    return NS_OK;

  mIsActive = aActiveFlag;
  if (mIsActive) {
    InstallKeyboardNavigator();
  }
  else {
    RemoveKeyboardNavigator();
  }
  
  
  
  
  do {
    nsIPresShell *presShell = PresContext()->GetPresShell();
    if (!presShell)
      break;

    nsIDocument *document = presShell->GetDocument();
    if (!document)
      break;

    nsCOMPtr<nsISupports> container = document->GetContainer();
    nsCOMPtr<nsPIDOMWindow> windowPrivate = do_GetInterface(container);
    if (!windowPrivate)
      break;

    nsIFocusController *focusController =
      windowPrivate->GetRootFocusController();
    if (!focusController)
      break;

    nsCOMPtr<nsIDOMWindowInternal> windowInternal;
    focusController->GetFocusedWindow(getter_AddRefs(windowInternal));
    if (!windowInternal)
      break;

    nsCOMPtr<nsIDOMDocument> domDoc;
    nsCOMPtr<nsIDocument> focusedDoc;
    windowInternal->GetDocument(getter_AddRefs(domDoc));
    focusedDoc = do_QueryInterface(domDoc);
    if (!focusedDoc)
      break;

    presShell = focusedDoc->GetPrimaryShell();
    nsCOMPtr<nsISelectionController> selCon(do_QueryInterface(presShell));
    
    if (!selCon)
      break;

    if (mIsActive) {
      PRBool isCaretVisible;
      selCon->GetCaretEnabled(&isCaretVisible);
      mCaretWasVisible |= isCaretVisible;
    }
    selCon->SetCaretEnabled(!mIsActive && mCaretWasVisible);
    if (!mIsActive) {
      mCaretWasVisible = PR_FALSE;
    }
  } while (0);

  NS_NAMED_LITERAL_STRING(active, "DOMMenuBarActive");
  NS_NAMED_LITERAL_STRING(inactive, "DOMMenuBarInactive");
  
  FireDOMEventSynch(mIsActive ? active : inactive);

  return NS_OK;
}

void
nsMenuBarFrame::ToggleMenuActiveState()
{
  if (mIsActive) {
    
    SetActive(PR_FALSE);
    if (mCurrentMenu) {
      
      mCurrentMenu->OpenMenu(PR_FALSE);
      mCurrentMenu->SelectMenu(PR_FALSE);
      mCurrentMenu = nsnull;
    }
  }
  else {
    
    if (mCurrentMenu)
      mCurrentMenu->SelectMenu(PR_FALSE);
    
    
    SetActive(PR_TRUE);

    
    
    
    nsIMenuFrame* firstFrame = GetNextMenuItem(nsnull);
    if (firstFrame) {
      firstFrame->SelectMenu(PR_TRUE);
      
      
      mCurrentMenu = firstFrame;
    }
  }
}

static void GetInsertionPoint(nsIPresShell* aShell, nsIFrame* aFrame, nsIFrame* aChild,
                              nsIFrame** aResult)
{
  nsIContent* child = nsnull;
  if (aChild)
    child = aChild->GetContent();
  aShell->FrameConstructor()->GetInsertionPoint(aFrame, child, aResult);
}

nsIMenuFrame*
nsMenuBarFrame::FindMenuWithShortcut(nsIDOMKeyEvent* aKeyEvent)
{
  PRUint32 charCode;
  aKeyEvent->GetCharCode(&charCode);

  
  nsIFrame* immediateParent = nsnull;
  GetInsertionPoint(PresContext()->PresShell(), this, nsnull, &immediateParent);
  if (!immediateParent)
    immediateParent = this;

  nsIFrame* currFrame = immediateParent->GetFirstChild(nsnull);

  while (currFrame) {
    nsIContent* current = currFrame->GetContent();
    
    
    if (IsValidItem(current)) {
      
      nsAutoString shortcutKey;
      current->GetAttr(kNameSpaceID_None, nsGkAtoms::accesskey, shortcutKey);
      if (!shortcutKey.IsEmpty()) {
        
        PRUnichar letter = PRUnichar(charCode); 
        if ( shortcutKey.Equals(Substring(&letter, &letter+1),
                                nsCaseInsensitiveStringComparator()) )  {
          
          nsIMenuFrame *menuFrame;
          if (NS_FAILED(CallQueryInterface(currFrame, &menuFrame))) {
            menuFrame = nsnull;
          }
          return menuFrame;
        }
      }
    }
    currFrame = currFrame->GetNextSibling();
  }

  
#ifdef XP_WIN
  
  if (mIsActive) {
    nsCOMPtr<nsISound> soundInterface = do_CreateInstance("@mozilla.org/sound;1");
    if (soundInterface)
      soundInterface->Beep();
  }

  DismissChain();
#endif  

  return nsnull;
}

NS_IMETHODIMP 
nsMenuBarFrame::ShortcutNavigation(nsIDOMKeyEvent* aKeyEvent, PRBool& aHandledFlag)
{
  if (mCurrentMenu) {
    PRBool isOpen = PR_FALSE;
    mCurrentMenu->MenuIsOpen(isOpen);
    if (isOpen) {
      
      mCurrentMenu->ShortcutNavigation(aKeyEvent, aHandledFlag);
      return NS_OK;
    }
  }

  
  nsIMenuFrame* result = FindMenuWithShortcut(aKeyEvent);
  if (result) {
    
    nsWeakFrame weakFrame(this);
    nsIFrame* frame = nsnull;
    CallQueryInterface(result, &frame);
    nsWeakFrame weakResult(frame);
    aHandledFlag = PR_TRUE;
    SetActive(PR_TRUE);
    if (weakFrame.IsAlive()) {
      SetCurrentMenuItem(result);
    }
    if (weakResult.IsAlive()) {
      result->OpenMenu(PR_TRUE);
      if (weakResult.IsAlive()) {
        result->SelectFirstItem();
      }
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsMenuBarFrame::KeyboardNavigation(PRUint32 aKeyCode, PRBool& aHandledFlag)
{
  nsNavigationDirection theDirection;
  NS_DIRECTION_FROM_KEY_CODE(theDirection, aKeyCode);
  if (!mCurrentMenu)
    return NS_OK;

  nsWeakFrame weakFrame(this);
  PRBool isContainer = PR_FALSE;
  PRBool isOpen = PR_FALSE;
  mCurrentMenu->MenuIsContainer(isContainer);
  mCurrentMenu->MenuIsOpen(isOpen);

  aHandledFlag = PR_FALSE;
  
  if (isOpen) {
    
    mCurrentMenu->KeyboardNavigation(aKeyCode, aHandledFlag);
  }

  if (aHandledFlag)
    return NS_OK;

  if NS_DIRECTION_IS_INLINE(theDirection) {
    
    nsIMenuFrame* nextItem = (theDirection == eNavigationDirection_End) ?
                             GetNextMenuItem(mCurrentMenu) : 
                             GetPreviousMenuItem(mCurrentMenu);

    nsIFrame* nextFrame = nsnull;
    if (nextItem) {
      CallQueryInterface(nextItem, &nextFrame);
    }
    nsWeakFrame weakNext(nextFrame);
    SetCurrentMenuItem(nextItem);
    if (weakNext.IsAlive()) {
      PRBool nextIsOpen;
      nextItem->MenuIsOpen(nextIsOpen);
      if (nextIsOpen) {
        
        nextItem->SelectFirstItem();
      }
    }
  }
  else if NS_DIRECTION_IS_BLOCK(theDirection) {
    NS_ENSURE_TRUE(weakFrame.IsAlive(), NS_OK);
    nsIFrame* frame = nsnull;
    CallQueryInterface(mCurrentMenu, &frame);
    nsWeakFrame weakCurrentMenu(frame);
    nsIMenuFrame* currentMenu = mCurrentMenu;
     
    currentMenu->OpenMenu(PR_TRUE);
    if (weakCurrentMenu.IsAlive()) {
      currentMenu->SelectFirstItem();
    }
  }

  return NS_OK;
}

 nsIMenuFrame*
nsMenuBarFrame::GetNextMenuItem(nsIMenuFrame* aStart)
{
  nsIFrame* immediateParent = nsnull;
  GetInsertionPoint(PresContext()->PresShell(), this, nsnull, &immediateParent);
  if (!immediateParent)
    immediateParent = this;

  nsIFrame* currFrame = nsnull;
  nsIFrame* startFrame = nsnull;
  if (aStart) {
    aStart->QueryInterface(NS_GET_IID(nsIFrame), (void**)&currFrame); 
    if (currFrame) {
      startFrame = currFrame;
      currFrame = currFrame->GetNextSibling();
    }
  }
  else 
    currFrame = immediateParent->GetFirstChild(nsnull);

  while (currFrame) {
    
    if (IsValidItem(currFrame->GetContent())) {
      nsIMenuFrame *menuFrame;
      if (NS_FAILED(CallQueryInterface(currFrame, &menuFrame)))
        menuFrame = nsnull;
      return menuFrame;
    }
    currFrame = currFrame->GetNextSibling();
  }

  currFrame = immediateParent->GetFirstChild(nsnull);

  
  while (currFrame && currFrame != startFrame) {
    
    if (IsValidItem(currFrame->GetContent())) {
      nsIMenuFrame *menuFrame;
      if (NS_FAILED(CallQueryInterface(currFrame, &menuFrame)))
        menuFrame = nsnull;
      return menuFrame;
    }

    currFrame = currFrame->GetNextSibling();
  }

  
  return aStart;
}

 nsIMenuFrame*
nsMenuBarFrame::GetPreviousMenuItem(nsIMenuFrame* aStart)
{
  nsIFrame* immediateParent = nsnull;
  GetInsertionPoint(PresContext()->PresShell(), this, nsnull, &immediateParent);
  if (!immediateParent)
    immediateParent = this;

  nsFrameList frames(immediateParent->GetFirstChild(nsnull));
                              
  nsIFrame* currFrame = nsnull;
  nsIFrame* startFrame = nsnull;
  if (aStart) {
    aStart->QueryInterface(NS_GET_IID(nsIFrame), (void**)&currFrame);
    if (currFrame) {
      startFrame = currFrame;
      currFrame = frames.GetPrevSiblingFor(currFrame);
    }
  }
  else currFrame = frames.LastChild();

  while (currFrame) {
    
    if (IsValidItem(currFrame->GetContent())) {
      nsIMenuFrame *menuFrame;
      if (NS_FAILED(CallQueryInterface(currFrame, &menuFrame)))
        menuFrame = nsnull;
      return menuFrame;
    }
    currFrame = frames.GetPrevSiblingFor(currFrame);
  }

  currFrame = frames.LastChild();

  
  while (currFrame && currFrame != startFrame) {
    
    if (IsValidItem(currFrame->GetContent())) {
      nsIMenuFrame *menuFrame;
      if (NS_FAILED(CallQueryInterface(currFrame, &menuFrame)))
        menuFrame = nsnull;
      return menuFrame;
    }

    currFrame = frames.GetPrevSiblingFor(currFrame);
  }

  
  return aStart;
}

 nsIMenuFrame*
nsMenuBarFrame::GetCurrentMenuItem()
{
  return mCurrentMenu;
}


NS_IMETHODIMP nsMenuBarFrame::SetCurrentMenuItem(nsIMenuFrame* aMenuItem)
{
  if (mCurrentMenu == aMenuItem)
    return NS_OK;

  PRBool wasOpen = PR_FALSE;
  
  
  if (nsMenuFrame::GetContextMenu())
    return NS_OK;

  nsWeakFrame weakFrame(this);

  
  if (mCurrentMenu) {
    nsIFrame* frame = nsnull;
    CallQueryInterface(mCurrentMenu, &frame);
    nsWeakFrame weakCurrentMenu(frame);
    nsIMenuFrame* currentMenu = mCurrentMenu;
    currentMenu->MenuIsOpen(wasOpen);
    currentMenu->SelectMenu(PR_FALSE);
    if (wasOpen && weakCurrentMenu.IsAlive()) {
      currentMenu->OpenMenu(PR_FALSE);
    }
  }

  NS_ENSURE_TRUE(weakFrame.IsAlive(), NS_OK);


  
  if (aMenuItem) {
    nsIFrame* newMenu = nsnull;
    CallQueryInterface(aMenuItem, &newMenu);
    nsWeakFrame weakNewMenu(newMenu);
    aMenuItem->SelectMenu(PR_TRUE);
    NS_ENSURE_TRUE(weakNewMenu.IsAlive(), NS_OK);
    aMenuItem->MarkAsGenerated(); 
    NS_ENSURE_TRUE(weakNewMenu.IsAlive(), NS_OK);

    PRBool isDisabled = PR_FALSE;
    aMenuItem->MenuIsDisabled(isDisabled);
    if (wasOpen&&!isDisabled)
      aMenuItem->OpenMenu(PR_TRUE);
    ClearRecentlyRolledUp();
  }

  NS_ENSURE_TRUE(weakFrame.IsAlive(), NS_OK);
  mCurrentMenu = aMenuItem;

  return NS_OK;
}


NS_IMETHODIMP 
nsMenuBarFrame::Escape(PRBool& aHandledFlag)
{
  if (!mCurrentMenu)
    return NS_OK;

  nsWeakFrame weakFrame(this);
  
  PRBool isOpen = PR_FALSE;
  mCurrentMenu->MenuIsOpen(isOpen);
  if (isOpen) {
    
    aHandledFlag = PR_FALSE;
    mCurrentMenu->Escape(aHandledFlag);
    NS_ENSURE_TRUE(weakFrame.IsAlive(), NS_OK);
    if (!aHandledFlag) {
      
      
      mCurrentMenu->OpenMenu(PR_FALSE);
      NS_ENSURE_TRUE(weakFrame.IsAlive(), NS_OK);
    }
    nsMenuDismissalListener::Shutdown();
    return NS_OK;
  }

  
  SetCurrentMenuItem(nsnull);
  NS_ENSURE_TRUE(weakFrame.IsAlive(), NS_OK);

  SetActive(PR_FALSE);

  
  nsMenuDismissalListener::Shutdown();
  return NS_OK;
}

NS_IMETHODIMP 
nsMenuBarFrame::Enter()
{
  if (!mCurrentMenu)
    return NS_OK;

  ClearRecentlyRolledUp();

  
  PRBool isOpen = PR_FALSE;
  mCurrentMenu->MenuIsOpen(isOpen);
  if (isOpen) {
    
    mCurrentMenu->Enter();
    return NS_OK;
  }

  
  mCurrentMenu->OpenMenu(PR_TRUE);
  mCurrentMenu->SelectFirstItem();

  return NS_OK;
}

NS_IMETHODIMP
nsMenuBarFrame::ClearRecentlyRolledUp()
{
  
  
  mRecentRollupMenu = nsnull;

  return NS_OK;
}

NS_IMETHODIMP
nsMenuBarFrame::RecentlyRolledUp(nsIMenuFrame *aMenuFrame, PRBool *aJustRolledUp)
{
  
  
  
  *aJustRolledUp = (mRecentRollupMenu == aMenuFrame);

  return NS_OK;
}

NS_IMETHODIMP
nsMenuBarFrame::HideChain()
{
  
  
  
  
  if (nsMenuFrame::GetContextMenu()) {
    PRBool dummy;
    mCurrentMenu->Escape(dummy);
  }

  
  
  
  nsMenuDismissalListener::Shutdown();

  ClearRecentlyRolledUp();
  if (mCurrentMenu) {
    mCurrentMenu->ActivateMenu(PR_FALSE);
    mCurrentMenu->SelectMenu(PR_FALSE);
    mRecentRollupMenu = mCurrentMenu;
  }

  if (mIsActive) {
    ToggleMenuActiveState();
  }

  return NS_OK;
}

NS_IMETHODIMP
nsMenuBarFrame::DismissChain()
{
  
  nsMenuDismissalListener::Shutdown();
  nsWeakFrame weakFrame(this);
  SetCurrentMenuItem(nsnull);
  if (weakFrame.IsAlive()) {
    SetActive(PR_FALSE);
  }
  return NS_OK;
}


NS_IMETHODIMP
nsMenuBarFrame::KillPendingTimers ( )
{
  return NS_OK;

} 


NS_IMETHODIMP
nsMenuBarFrame::GetWidget(nsIWidget **aWidget)
{
  
  
  
  
  
  
  
  *aWidget = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsMenuBarFrame::InstallKeyboardNavigator()
{
  if (mKeyboardNavigator)
    return NS_OK;

  mKeyboardNavigator = new nsMenuListener(this);
  NS_IF_ADDREF(mKeyboardNavigator);

  mTarget->AddEventListener(NS_LITERAL_STRING("keypress"), (nsIDOMKeyListener*)mKeyboardNavigator, PR_TRUE); 
  mTarget->AddEventListener(NS_LITERAL_STRING("keydown"), (nsIDOMKeyListener*)mKeyboardNavigator, PR_TRUE);  
  mTarget->AddEventListener(NS_LITERAL_STRING("keyup"), (nsIDOMKeyListener*)mKeyboardNavigator, PR_TRUE);   

  nsContentUtils::NotifyInstalledMenuKeyboardListener(PR_TRUE);

  return NS_OK;
}

NS_IMETHODIMP
nsMenuBarFrame::RemoveKeyboardNavigator()
{
  if (!mKeyboardNavigator || mIsActive)
    return NS_OK;

  mTarget->RemoveEventListener(NS_LITERAL_STRING("keypress"), (nsIDOMKeyListener*)mKeyboardNavigator, PR_TRUE);
  mTarget->RemoveEventListener(NS_LITERAL_STRING("keydown"), (nsIDOMKeyListener*)mKeyboardNavigator, PR_TRUE);
  mTarget->RemoveEventListener(NS_LITERAL_STRING("keyup"), (nsIDOMKeyListener*)mKeyboardNavigator, PR_TRUE);

  NS_IF_RELEASE(mKeyboardNavigator);

  nsContentUtils::NotifyInstalledMenuKeyboardListener(PR_FALSE);

  return NS_OK;
}



PRBool 
nsMenuBarFrame::IsValidItem(nsIContent* aContent)
{
  nsIAtom *tag = aContent->Tag();

  return ((tag == nsGkAtoms::menu ||
           tag == nsGkAtoms::menuitem) &&
          !IsDisabled(aContent));
}

PRBool 
nsMenuBarFrame::IsDisabled(nsIContent* aContent)
{
  return aContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::disabled,
                               nsGkAtoms::_true, eCaseMatters);
}

void
nsMenuBarFrame::Destroy()
{
  mTarget->RemoveEventListener(NS_LITERAL_STRING("keypress"), (nsIDOMKeyListener*)mMenuBarListener, PR_FALSE); 
  mTarget->RemoveEventListener(NS_LITERAL_STRING("keydown"), (nsIDOMKeyListener*)mMenuBarListener, PR_FALSE);  
  mTarget->RemoveEventListener(NS_LITERAL_STRING("keyup"), (nsIDOMKeyListener*)mMenuBarListener, PR_FALSE);

  mTarget->RemoveEventListener(NS_LITERAL_STRING("mousedown"), (nsIDOMMouseListener*)mMenuBarListener, PR_FALSE);
  mTarget->RemoveEventListener(NS_LITERAL_STRING("blur"), (nsIDOMFocusListener*)mMenuBarListener, PR_TRUE);

  NS_IF_RELEASE(mMenuBarListener);

  nsBoxFrame::Destroy();
}

