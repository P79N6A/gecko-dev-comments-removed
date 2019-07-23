






































#include "nsMenuBarFrame.h"
#include "nsIServiceManager.h"
#include "nsIContent.h"
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




nsMenuBarFrame::nsMenuBarFrame(nsIPresShell* aShell, nsStyleContext* aContext):
  nsBoxFrame(aShell, aContext),
    mMenuBarListener(nsnull),
    mStayActive(PR_FALSE),
    mIsActive(PR_FALSE),
    mCurrentMenu(nsnull),
    mTarget(nsnull),
    mCaretWasVisible(PR_FALSE)
{
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
nsMenuBarFrame::SetActive(PRBool aActiveFlag)
{
  
  if (mIsActive == aActiveFlag)
    return NS_OK;

  if (!aActiveFlag) {
    
    if (mStayActive)
      return NS_OK;

    
    
    
    nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
    if (pm && pm->IsPopupOpenForMenuParent(this))
      return NS_OK;
  }

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
  
  FireDOMEvent(mIsActive ? active : inactive, mContent);

  return NS_OK;
}

nsMenuFrame*
nsMenuBarFrame::ToggleMenuActiveState()
{
  if (mIsActive) {
    
    SetActive(PR_FALSE);
    if (mCurrentMenu) {
      nsMenuFrame* closeframe = mCurrentMenu;
      closeframe->SelectMenu(PR_FALSE);
      mCurrentMenu = nsnull;
      return closeframe;
    }
  }
  else {
    
    if (mCurrentMenu)
      mCurrentMenu->SelectMenu(PR_FALSE);
    
    
    SetActive(PR_TRUE);

    
    
    
    nsMenuFrame* firstFrame = nsXULPopupManager::GetNextMenuItem(this, nsnull, PR_FALSE);
    if (firstFrame) {
      firstFrame->SelectMenu(PR_TRUE);
      
      
      mCurrentMenu = firstFrame;
    }
  }

  return nsnull;
}

static void
GetInsertionPoint(nsIPresShell* aShell, nsIFrame* aFrame, nsIFrame* aChild,
                  nsIFrame** aResult)
{
  nsIContent* child = nsnull;
  if (aChild)
    child = aChild->GetContent();
  aShell->FrameConstructor()->GetInsertionPoint(aFrame, child, aResult);
}

nsMenuFrame*
nsMenuBarFrame::FindMenuWithShortcut(nsIDOMKeyEvent* aKeyEvent)
{
  PRUint32 charCode;
  aKeyEvent->GetCharCode(&charCode);
  if (!charCode) 
    return nsnull;

  
  nsIFrame* immediateParent = nsnull;
  GetInsertionPoint(PresContext()->PresShell(), this, nsnull, &immediateParent);
  if (!immediateParent)
    immediateParent = this;

  nsIFrame* currFrame = immediateParent->GetFirstChild(nsnull);

  while (currFrame) {
    nsIContent* current = currFrame->GetContent();
    
    
    if (nsXULPopupManager::IsValidMenuItem(PresContext(), current, PR_FALSE)) {
      
      nsAutoString shortcutKey;
      current->GetAttr(kNameSpaceID_None, nsGkAtoms::accesskey, shortcutKey);
      if (!shortcutKey.IsEmpty()) {
        
        PRUnichar letter = PRUnichar(charCode); 
        if ( shortcutKey.Equals(Substring(&letter, &letter+1),
                                nsCaseInsensitiveStringComparator()) )  {
          
          return (currFrame->GetType() == nsGkAtoms::menuFrame) ?
                 static_cast<nsMenuFrame *>(currFrame) : nsnull;
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

  nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
  if (pm) {
    nsIFrame* popup = pm->GetTopPopup(ePopupTypeAny);
    if (popup)
      pm->HidePopup(popup->GetContent(), PR_TRUE, PR_TRUE, PR_TRUE);
  }

  SetCurrentMenuItem(nsnull);
  SetActive(PR_FALSE);

#endif  

  return nsnull;
}

 nsMenuFrame*
nsMenuBarFrame::GetCurrentMenuItem()
{
  return mCurrentMenu;
}

NS_IMETHODIMP
nsMenuBarFrame::SetCurrentMenuItem(nsMenuFrame* aMenuItem)
{
  if (mCurrentMenu == aMenuItem)
    return NS_OK;

  if (mCurrentMenu)
    mCurrentMenu->SelectMenu(PR_FALSE);

  if (aMenuItem)
    aMenuItem->SelectMenu(PR_TRUE);

  mCurrentMenu = aMenuItem;

  return NS_OK;
}

void
nsMenuBarFrame::CurrentMenuIsBeingDestroyed()
{
  mCurrentMenu->SelectMenu(PR_FALSE);
  mCurrentMenu = nsnull;
}

class nsMenuBarSwitchMenu : public nsRunnable
{
public:
  nsMenuBarSwitchMenu(nsIContent* aMenuBar,
                      nsIContent *aOldMenu,
                      nsIContent *aNewMenu,
                      PRBool aSelectFirstItem)
    : mMenuBar(aMenuBar), mOldMenu(aOldMenu), mNewMenu(aNewMenu),
      mSelectFirstItem(aSelectFirstItem)
  {
  }

  NS_IMETHOD Run()
  {
    nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
    if (!pm)
      return NS_ERROR_UNEXPECTED;

    
    
    nsMenuBarFrame* menubar = nsnull;
    if (mOldMenu && mNewMenu) {
      menubar = static_cast<nsMenuBarFrame *>
        (pm->GetFrameOfTypeForContent(mMenuBar, nsGkAtoms::menuBarFrame, PR_FALSE));
      menubar->SetStayActive(PR_TRUE);
    }

    if (mOldMenu) {
      nsWeakFrame weakMenuBar(menubar);
      pm->HidePopup(mOldMenu, PR_FALSE, PR_FALSE, PR_FALSE);
      
      if (mNewMenu && weakMenuBar.IsAlive())
        menubar->SetStayActive(PR_FALSE);
    }

    if (mNewMenu)
      pm->ShowMenu(mNewMenu, mSelectFirstItem, PR_FALSE);

    return NS_OK;
  }

private:
  nsCOMPtr<nsIContent> mMenuBar;
  nsCOMPtr<nsIContent> mOldMenu;
  nsCOMPtr<nsIContent> mNewMenu;
  PRBool mSelectFirstItem;
};

NS_IMETHODIMP
nsMenuBarFrame::ChangeMenuItem(nsMenuFrame* aMenuItem,
                               PRBool aSelectFirstItem)
{
  if (mCurrentMenu == aMenuItem)
    return NS_OK;

  
  nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
  if (pm && pm->HasContextMenu(nsnull))
    return NS_OK;

  nsIContent* aOldMenu = nsnull, *aNewMenu = nsnull;
  
  
  PRBool wasOpen = PR_FALSE;
  if (mCurrentMenu) {
    wasOpen = mCurrentMenu->IsOpen();
    mCurrentMenu->SelectMenu(PR_FALSE);
    if (wasOpen) {
      nsMenuPopupFrame* popupFrame = mCurrentMenu->GetPopup();
      if (popupFrame)
        aOldMenu = popupFrame->GetContent();
    }
  }

  
  mCurrentMenu = nsnull;

  
  if (aMenuItem) {
    nsCOMPtr<nsIContent> content = aMenuItem->GetContent();
    aMenuItem->SelectMenu(PR_TRUE);
    mCurrentMenu = aMenuItem;
    if (wasOpen && !aMenuItem->IsDisabled())
      aNewMenu = content;
  }

  
  
  nsCOMPtr<nsIRunnable> event =
    new nsMenuBarSwitchMenu(GetContent(), aOldMenu, aNewMenu, aSelectFirstItem);
  return NS_DispatchToCurrentThread(event);
}

nsMenuFrame*
nsMenuBarFrame::Enter()
{
  if (!mCurrentMenu)
    return nsnull;

  if (mCurrentMenu->IsOpen())
    return mCurrentMenu->Enter();

  return mCurrentMenu;
}

PRBool
nsMenuBarFrame::MenuClosed()
{
  SetActive(PR_FALSE);
  if (!mIsActive && mCurrentMenu) {
    mCurrentMenu->SelectMenu(PR_FALSE);
    mCurrentMenu = nsnull;
    return PR_TRUE;
  }
  return PR_FALSE;
}

void
nsMenuBarFrame::InstallKeyboardNavigator()
{
  nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
  if (pm)
    pm->SetActiveMenuBar(this, PR_TRUE);
}

void
nsMenuBarFrame::RemoveKeyboardNavigator()
{
  if (!mIsActive) {
    nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
    if (pm)
      pm->SetActiveMenuBar(this, PR_FALSE);
  }
}

void
nsMenuBarFrame::Destroy()
{
  nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
  if (pm)
    pm->SetActiveMenuBar(this, PR_FALSE);

  mTarget->RemoveEventListener(NS_LITERAL_STRING("keypress"), (nsIDOMKeyListener*)mMenuBarListener, PR_FALSE); 
  mTarget->RemoveEventListener(NS_LITERAL_STRING("keydown"), (nsIDOMKeyListener*)mMenuBarListener, PR_FALSE);  
  mTarget->RemoveEventListener(NS_LITERAL_STRING("keyup"), (nsIDOMKeyListener*)mMenuBarListener, PR_FALSE);

  mTarget->RemoveEventListener(NS_LITERAL_STRING("mousedown"), (nsIDOMMouseListener*)mMenuBarListener, PR_FALSE);
  mTarget->RemoveEventListener(NS_LITERAL_STRING("blur"), (nsIDOMFocusListener*)mMenuBarListener, PR_TRUE);

  NS_IF_RELEASE(mMenuBarListener);

  nsBoxFrame::Destroy();
}
