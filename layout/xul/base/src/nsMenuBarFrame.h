








#ifndef nsMenuBarFrame_h__
#define nsMenuBarFrame_h__

#include "mozilla/Attributes.h"
#include "nsIAtom.h"
#include "nsCOMPtr.h"
#include "nsBoxFrame.h"
#include "nsMenuFrame.h"
#include "nsMenuBarListener.h"
#include "nsMenuParent.h"

class nsIContent;

nsIFrame* NS_NewMenuBarFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

class nsMenuBarFrame : public nsBoxFrame, public nsMenuParent
{
public:
  NS_DECL_QUERYFRAME_TARGET(nsMenuBarFrame)
  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  nsMenuBarFrame(nsIPresShell* aShell, nsStyleContext* aContext);

  
  virtual nsMenuFrame* GetCurrentMenuItem() MOZ_OVERRIDE;
  NS_IMETHOD SetCurrentMenuItem(nsMenuFrame* aMenuItem) MOZ_OVERRIDE;
  virtual void CurrentMenuIsBeingDestroyed() MOZ_OVERRIDE;
  NS_IMETHOD ChangeMenuItem(nsMenuFrame* aMenuItem, bool aSelectFirstItem) MOZ_OVERRIDE;

  NS_IMETHOD SetActive(bool aActiveFlag) MOZ_OVERRIDE; 

  virtual bool IsMenuBar() MOZ_OVERRIDE { return true; }
  virtual bool IsContextMenu() { return false; }
  virtual bool IsActive() MOZ_OVERRIDE { return mIsActive; }
  virtual bool IsMenu() { return false; }
  virtual bool IsOpen() MOZ_OVERRIDE { return true; } 

  bool IsMenuOpen() { return mCurrentMenu && mCurrentMenu->IsOpen(); }

  void InstallKeyboardNavigator();
  void RemoveKeyboardNavigator();

  virtual void Init(nsIContent*      aContent,
                    nsIFrame*        aParent,
                    nsIFrame*        aPrevInFlow) MOZ_OVERRIDE;

  virtual void DestroyFrom(nsIFrame* aDestructRoot) MOZ_OVERRIDE;

  virtual void LockMenuUntilClosed(bool aLock) MOZ_OVERRIDE {}
  virtual bool IsMenuLocked() MOZ_OVERRIDE { return false; }



  
  
  
  bool GetStayActive() { return mStayActive; }
  void SetStayActive(bool aStayActive) { mStayActive = aStayActive; }

  
  
  nsMenuFrame* ToggleMenuActiveState();

  bool IsActiveByKeyboard() { return mActiveByKeyboard; }
  void SetActiveByKeyboard() { mActiveByKeyboard = true; }

  
  
  virtual bool MenuClosed() MOZ_OVERRIDE;

  
  
  nsMenuFrame* Enter(nsGUIEvent* aEvent);

  
  nsMenuFrame* FindMenuWithShortcut(nsIDOMKeyEvent* aKeyEvent);

  virtual bool IsFrameOfType(uint32_t aFlags) const MOZ_OVERRIDE
  {
    
    if (aFlags & (nsIFrame::eReplacedContainsBlock | nsIFrame::eReplaced))
      return false;
    return nsBoxFrame::IsFrameOfType(aFlags);
  }

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const MOZ_OVERRIDE
  {
      return MakeFrameName(NS_LITERAL_STRING("MenuBar"), aResult);
  }
#endif

protected:
  nsMenuBarListener* mMenuBarListener; 

  
  
  bool mStayActive;

  bool mIsActive; 

  
  bool mActiveByKeyboard;

  
  
  nsMenuFrame* mCurrentMenu;

  nsIDOMEventTarget* mTarget;

}; 

#endif
