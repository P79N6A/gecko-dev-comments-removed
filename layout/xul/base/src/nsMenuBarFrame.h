











































#ifndef nsMenuBarFrame_h__
#define nsMenuBarFrame_h__

#include "prtypes.h"
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
  NS_DECL_FRAMEARENA_HELPERS

  nsMenuBarFrame(nsIPresShell* aShell, nsStyleContext* aContext);

  
  virtual nsMenuFrame* GetCurrentMenuItem();
  NS_IMETHOD SetCurrentMenuItem(nsMenuFrame* aMenuItem);
  virtual void CurrentMenuIsBeingDestroyed();
  NS_IMETHOD ChangeMenuItem(nsMenuFrame* aMenuItem, PRBool aSelectFirstItem);

  NS_IMETHOD SetActive(PRBool aActiveFlag); 

  virtual PRBool IsMenuBar() { return PR_TRUE; }
  virtual PRBool IsContextMenu() { return PR_FALSE; }
  virtual PRBool IsActive() { return mIsActive; }
  virtual PRBool IsMenu() { return PR_FALSE; }
  virtual PRBool IsOpen() { return PR_TRUE; } 

  PRBool IsMenuOpen() { return mCurrentMenu && mCurrentMenu->IsOpen(); }

  void InstallKeyboardNavigator();
  void RemoveKeyboardNavigator();

  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);

  virtual void Destroy();

  virtual nsIAtom* GetType() const { return nsGkAtoms::menuBarFrame; }



  void
  SetStayActive(PRBool aStayActive) { mStayActive = aStayActive; }

  
  
  nsMenuFrame* ToggleMenuActiveState();

  
  
  virtual PRBool MenuClosed();

  
  
  nsMenuFrame* Enter();

  
  nsMenuFrame* FindMenuWithShortcut(nsIDOMKeyEvent* aKeyEvent);

  virtual PRBool IsFrameOfType(PRUint32 aFlags) const
  {
    
    if (aFlags & (nsIFrame::eReplacedContainsBlock | nsIFrame::eReplaced))
      return PR_FALSE;
    return nsBoxFrame::IsFrameOfType(aFlags);
  }

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
      return MakeFrameName(NS_LITERAL_STRING("MenuBar"), aResult);
  }
#endif

protected:
  nsMenuBarListener* mMenuBarListener; 

  
  
  PRPackedBool mStayActive;

  PRPackedBool mIsActive; 
  
  
  nsMenuFrame* mCurrentMenu;

  nsIDOMEventTarget* mTarget;

}; 

#endif
