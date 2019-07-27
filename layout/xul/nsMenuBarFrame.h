








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

class nsMenuBarFrame final : public nsBoxFrame, public nsMenuParent
{
public:
  NS_DECL_QUERYFRAME_TARGET(nsMenuBarFrame)
  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  explicit nsMenuBarFrame(nsStyleContext* aContext);

  
  virtual nsMenuFrame* GetCurrentMenuItem() override;
  NS_IMETHOD SetCurrentMenuItem(nsMenuFrame* aMenuItem) override;
  virtual void CurrentMenuIsBeingDestroyed() override;
  NS_IMETHOD ChangeMenuItem(nsMenuFrame* aMenuItem, bool aSelectFirstItem) override;

  NS_IMETHOD SetActive(bool aActiveFlag) override; 

  virtual bool IsMenuBar() override { return true; }
  virtual bool IsContextMenu() override { return false; }
  virtual bool IsActive() override { return mIsActive; }
  virtual bool IsMenu() override { return false; }
  virtual bool IsOpen() override { return true; } 

  bool IsMenuOpen() { return mCurrentMenu && mCurrentMenu->IsOpen(); }

  void InstallKeyboardNavigator();
  void RemoveKeyboardNavigator();

  virtual void Init(nsIContent*       aContent,
                    nsContainerFrame* aParent,
                    nsIFrame*         aPrevInFlow) override;

  virtual void DestroyFrom(nsIFrame* aDestructRoot) override;

  virtual void LockMenuUntilClosed(bool aLock) override {}
  virtual bool IsMenuLocked() override { return false; }



  
  
  
  bool GetStayActive() { return mStayActive; }
  void SetStayActive(bool aStayActive) { mStayActive = aStayActive; }

  
  
  nsMenuFrame* ToggleMenuActiveState();

  bool IsActiveByKeyboard() { return mActiveByKeyboard; }
  void SetActiveByKeyboard() { mActiveByKeyboard = true; }

  
  
  virtual bool MenuClosed() override;

  
  
  nsMenuFrame* Enter(mozilla::WidgetGUIEvent* aEvent);

  
  nsMenuFrame* FindMenuWithShortcut(nsIDOMKeyEvent* aKeyEvent);

  virtual bool IsFrameOfType(uint32_t aFlags) const override
  {
    
    if (aFlags & (nsIFrame::eReplacedContainsBlock | nsIFrame::eReplaced))
      return false;
    return nsBoxFrame::IsFrameOfType(aFlags);
  }

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const override
  {
      return MakeFrameName(NS_LITERAL_STRING("MenuBar"), aResult);
  }
#endif

protected:
  nsRefPtr<nsMenuBarListener> mMenuBarListener; 

  
  
  bool mStayActive;

  bool mIsActive; 

  
  bool mActiveByKeyboard;

  
  
  nsMenuFrame* mCurrentMenu;

  mozilla::dom::EventTarget* mTarget;

}; 

#endif
