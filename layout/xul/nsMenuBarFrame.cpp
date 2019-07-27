




#include "nsMenuBarFrame.h"
#include "nsIServiceManager.h"
#include "nsIContent.h"
#include "nsIAtom.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "nsCSSRendering.h"
#include "nsNameSpaceManager.h"
#include "nsIDocument.h"
#include "nsGkAtoms.h"
#include "nsMenuFrame.h"
#include "nsMenuPopupFrame.h"
#include "nsUnicharUtils.h"
#include "nsPIDOMWindow.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsCSSFrameConstructor.h"
#ifdef XP_WIN
#include "nsISound.h"
#include "nsWidgetsCID.h"
#endif
#include "nsContentUtils.h"
#include "nsUTF8Utils.h"
#include "mozilla/TextEvents.h"

using namespace mozilla;






nsIFrame*
NS_NewMenuBarFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsMenuBarFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsMenuBarFrame)

NS_QUERYFRAME_HEAD(nsMenuBarFrame)
  NS_QUERYFRAME_ENTRY(nsMenuBarFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsBoxFrame)




nsMenuBarFrame::nsMenuBarFrame(nsStyleContext* aContext):
  nsBoxFrame(aContext),
    mStayActive(false),
    mIsActive(false),
    mCurrentMenu(nullptr),
    mTarget(nullptr)
{
} 

void
nsMenuBarFrame::Init(nsIContent*       aContent,
                     nsContainerFrame* aParent,
                     nsIFrame*         aPrevInFlow)
{
  nsBoxFrame::Init(aContent, aParent, aPrevInFlow);

  
  mMenuBarListener = new nsMenuBarListener(this);

  
  
  mTarget = aContent->GetComposedDoc();

  
  

  mTarget->AddSystemEventListener(NS_LITERAL_STRING("keypress"), mMenuBarListener, false);
  mTarget->AddSystemEventListener(NS_LITERAL_STRING("keydown"), mMenuBarListener, false);
  mTarget->AddSystemEventListener(NS_LITERAL_STRING("keyup"), mMenuBarListener, false);

  
  mTarget->AddEventListener(NS_LITERAL_STRING("mousedown"), mMenuBarListener, true);
  mTarget->AddEventListener(NS_LITERAL_STRING("mousedown"), mMenuBarListener, false);
  mTarget->AddEventListener(NS_LITERAL_STRING("blur"), mMenuBarListener, true);
}

NS_IMETHODIMP
nsMenuBarFrame::SetActive(bool aActiveFlag)
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
    mActiveByKeyboard = false;
    RemoveKeyboardNavigator();
  }

  NS_NAMED_LITERAL_STRING(active, "DOMMenuBarActive");
  NS_NAMED_LITERAL_STRING(inactive, "DOMMenuBarInactive");
  
  FireDOMEvent(mIsActive ? active : inactive, mContent);

  return NS_OK;
}

nsMenuFrame*
nsMenuBarFrame::ToggleMenuActiveState()
{
  if (mIsActive) {
    
    SetActive(false);
    if (mCurrentMenu) {
      nsMenuFrame* closeframe = mCurrentMenu;
      closeframe->SelectMenu(false);
      mCurrentMenu = nullptr;
      return closeframe;
    }
  }
  else {
    
    if (mCurrentMenu)
      mCurrentMenu->SelectMenu(false);

    
    
    
    nsMenuFrame* firstFrame = nsXULPopupManager::GetNextMenuItem(this, nullptr, false);
    if (firstFrame) {
      
      SetActive(true);
      firstFrame->SelectMenu(true);
      
      
      mCurrentMenu = firstFrame;
    }
  }

  return nullptr;
}

nsMenuFrame*
nsMenuBarFrame::FindMenuWithShortcut(nsIDOMKeyEvent* aKeyEvent)
{
  uint32_t charCode;
  aKeyEvent->GetCharCode(&charCode);

  nsAutoTArray<uint32_t, 10> accessKeys;
  WidgetKeyboardEvent* nativeKeyEvent =
    aKeyEvent->GetInternalNSEvent()->AsKeyboardEvent();
  if (nativeKeyEvent)
    nsContentUtils::GetAccessKeyCandidates(nativeKeyEvent, accessKeys);
  if (accessKeys.IsEmpty() && charCode)
    accessKeys.AppendElement(charCode);

  if (accessKeys.IsEmpty())
    return nullptr; 

  
  auto insertion = PresContext()->PresShell()->FrameConstructor()->
    GetInsertionPoint(GetContent(), nullptr);
  nsContainerFrame* immediateParent = insertion.mParentFrame;
  if (!immediateParent)
    immediateParent = this;

  
  nsIFrame* foundMenu = nullptr;
  size_t foundIndex = accessKeys.NoIndex;
  nsIFrame* currFrame = immediateParent->GetFirstPrincipalChild();

  while (currFrame) {
    nsIContent* current = currFrame->GetContent();

    
    if (nsXULPopupManager::IsValidMenuItem(PresContext(), current, false)) {
      
      nsAutoString shortcutKey;
      current->GetAttr(kNameSpaceID_None, nsGkAtoms::accesskey, shortcutKey);
      if (!shortcutKey.IsEmpty()) {
        ToLowerCase(shortcutKey);
        const char16_t* start = shortcutKey.BeginReading();
        const char16_t* end = shortcutKey.EndReading();
        uint32_t ch = UTF16CharEnumerator::NextChar(&start, end);
        size_t index = accessKeys.IndexOf(ch);
        if (index != accessKeys.NoIndex &&
            (foundIndex == accessKeys.NoIndex || index < foundIndex)) {
          foundMenu = currFrame;
          foundIndex = index;
        }
      }
    }
    currFrame = currFrame->GetNextSibling();
  }
  if (foundMenu) {
    return do_QueryFrame(foundMenu);
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
      pm->HidePopup(popup->GetContent(), true, true, true, false);
  }

  SetCurrentMenuItem(nullptr);
  SetActive(false);

#endif  

  return nullptr;
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
    mCurrentMenu->SelectMenu(false);

  if (aMenuItem)
    aMenuItem->SelectMenu(true);

  mCurrentMenu = aMenuItem;

  return NS_OK;
}

void
nsMenuBarFrame::CurrentMenuIsBeingDestroyed()
{
  mCurrentMenu->SelectMenu(false);
  mCurrentMenu = nullptr;
}

class nsMenuBarSwitchMenu : public nsRunnable
{
public:
  nsMenuBarSwitchMenu(nsIContent* aMenuBar,
                      nsIContent *aOldMenu,
                      nsIContent *aNewMenu,
                      bool aSelectFirstItem)
    : mMenuBar(aMenuBar), mOldMenu(aOldMenu), mNewMenu(aNewMenu),
      mSelectFirstItem(aSelectFirstItem)
  {
  }

  NS_IMETHOD Run() override
  {
    nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
    if (!pm)
      return NS_ERROR_UNEXPECTED;

    
    
    nsMenuBarFrame* menubar = nullptr;
    if (mOldMenu && mNewMenu) {
      menubar = do_QueryFrame(mMenuBar->GetPrimaryFrame());
      if (menubar)
        menubar->SetStayActive(true);
    }

    if (mOldMenu) {
      nsWeakFrame weakMenuBar(menubar);
      pm->HidePopup(mOldMenu, false, false, false, false);
      
      if (mNewMenu && weakMenuBar.IsAlive())
        menubar->SetStayActive(false);
    }

    if (mNewMenu)
      pm->ShowMenu(mNewMenu, mSelectFirstItem, false);

    return NS_OK;
  }

private:
  nsCOMPtr<nsIContent> mMenuBar;
  nsCOMPtr<nsIContent> mOldMenu;
  nsCOMPtr<nsIContent> mNewMenu;
  bool mSelectFirstItem;
};

NS_IMETHODIMP
nsMenuBarFrame::ChangeMenuItem(nsMenuFrame* aMenuItem,
                               bool aSelectFirstItem)
{
  if (mCurrentMenu == aMenuItem)
    return NS_OK;

  
  nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
  if (pm && pm->HasContextMenu(nullptr))
    return NS_OK;

  nsIContent* aOldMenu = nullptr;
  nsIContent* aNewMenu = nullptr;
  
  
  bool wasOpen = false;
  if (mCurrentMenu) {
    wasOpen = mCurrentMenu->IsOpen();
    mCurrentMenu->SelectMenu(false);
    if (wasOpen) {
      nsMenuPopupFrame* popupFrame = mCurrentMenu->GetPopup();
      if (popupFrame)
        aOldMenu = popupFrame->GetContent();
    }
  }

  
  mCurrentMenu = nullptr;

  
  if (aMenuItem) {
    nsCOMPtr<nsIContent> content = aMenuItem->GetContent();
    aMenuItem->SelectMenu(true);
    mCurrentMenu = aMenuItem;
    if (wasOpen && !aMenuItem->IsDisabled())
      aNewMenu = content;
  }

  
  
  nsCOMPtr<nsIRunnable> event =
    new nsMenuBarSwitchMenu(GetContent(), aOldMenu, aNewMenu, aSelectFirstItem);
  return NS_DispatchToCurrentThread(event);
}

nsMenuFrame*
nsMenuBarFrame::Enter(WidgetGUIEvent* aEvent)
{
  if (!mCurrentMenu)
    return nullptr;

  if (mCurrentMenu->IsOpen())
    return mCurrentMenu->Enter(aEvent);

  return mCurrentMenu;
}

bool
nsMenuBarFrame::MenuClosed()
{
  SetActive(false);
  if (!mIsActive && mCurrentMenu) {
    mCurrentMenu->SelectMenu(false);
    mCurrentMenu = nullptr;
    return true;
  }
  return false;
}

void
nsMenuBarFrame::InstallKeyboardNavigator()
{
  nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
  if (pm)
    pm->SetActiveMenuBar(this, true);
}

void
nsMenuBarFrame::RemoveKeyboardNavigator()
{
  if (!mIsActive) {
    nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
    if (pm)
      pm->SetActiveMenuBar(this, false);
  }
}

void
nsMenuBarFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
  if (pm)
    pm->SetActiveMenuBar(this, false);

  mTarget->RemoveSystemEventListener(NS_LITERAL_STRING("keypress"), mMenuBarListener, false);
  mTarget->RemoveSystemEventListener(NS_LITERAL_STRING("keydown"), mMenuBarListener, false);
  mTarget->RemoveSystemEventListener(NS_LITERAL_STRING("keyup"), mMenuBarListener, false);

  mTarget->RemoveEventListener(NS_LITERAL_STRING("mousedown"), mMenuBarListener, true);
  mTarget->RemoveEventListener(NS_LITERAL_STRING("mousedown"), mMenuBarListener, false);
  mTarget->RemoveEventListener(NS_LITERAL_STRING("blur"), mMenuBarListener, true);

  mMenuBarListener = nullptr;

  nsBoxFrame::DestroyFrom(aDestructRoot);
}
