






































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
    mIsActive(PR_FALSE),
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
                 NS_STATIC_CAST(nsMenuFrame *, currFrame) : nsnull;
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
  if (pm)
    pm->Rollup();

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

  nsWeakFrame weakFrame(this);
  if (mCurrentMenu)
    mCurrentMenu->SelectMenu(PR_FALSE);

  if (aMenuItem)
    aMenuItem->SelectMenu(PR_TRUE);

  NS_ENSURE_TRUE(weakFrame.IsAlive(), NS_OK);
  mCurrentMenu = aMenuItem;

  return NS_OK;
}

void
nsMenuBarFrame::CurrentMenuIsBeingDestroyed()
{
  mCurrentMenu->SelectMenu(PR_FALSE);
  mCurrentMenu = nsnull;
}

NS_IMETHODIMP
nsMenuBarFrame::ChangeMenuItem(nsMenuFrame* aMenuItem,
                               PRBool aSelectFirstItem)
{
  if (mCurrentMenu == aMenuItem)
    return NS_OK;

  
  nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
  if (pm && pm->HasContextMenu(nsnull))
    return NS_OK;

  
  PRBool wasOpen = PR_FALSE;
  if (mCurrentMenu) {
    wasOpen = mCurrentMenu->IsOpen();
    mCurrentMenu->SelectMenu(PR_FALSE);
    if (wasOpen) {
      nsMenuPopupFrame* popupFrame = mCurrentMenu->GetPopup();
      if (popupFrame)
        pm->HidePopup(popupFrame->GetContent(), PR_FALSE, PR_FALSE, PR_TRUE);
    }
  }

  
  mCurrentMenu = nsnull;

  
  if (aMenuItem) {
    nsCOMPtr<nsIContent> content = aMenuItem->GetContent();
    nsWeakFrame weakNewMenu(aMenuItem);
    aMenuItem->SelectMenu(PR_TRUE);
    NS_ENSURE_TRUE(weakNewMenu.IsAlive(), NS_OK);
    mCurrentMenu = aMenuItem;
    if (wasOpen && !aMenuItem->IsDisabled())
      pm->ShowMenu(content, aSelectFirstItem, PR_TRUE);
  }

  return NS_OK;
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
