











































#ifndef nsMenuBarFrame_h__
#define nsMenuBarFrame_h__

#include "prtypes.h"
#include "nsIAtom.h"
#include "nsCOMPtr.h"
#include "nsBoxFrame.h"
#include "nsMenuBarListener.h"
#include "nsMenuListener.h"
#include "nsIMenuParent.h"
#include "nsIWidget.h"

class nsIContent;
class nsIMenuFrame;

nsIFrame* NS_NewMenuBarFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

class nsMenuBarFrame : public nsBoxFrame, public nsIMenuParent
{
public:
  nsMenuBarFrame(nsIPresShell* aShell, nsStyleContext* aContext);
  virtual ~nsMenuBarFrame();

  NS_DECL_ISUPPORTS

  
  virtual nsIMenuFrame* GetCurrentMenuItem();
  NS_IMETHOD SetCurrentMenuItem(nsIMenuFrame* aMenuItem);
  virtual nsIMenuFrame* GetNextMenuItem(nsIMenuFrame* aStart);
  virtual nsIMenuFrame* GetPreviousMenuItem(nsIMenuFrame* aStart);
  NS_IMETHOD SetActive(PRBool aActiveFlag); 
  NS_IMETHOD GetIsActive(PRBool& isActive) { isActive = IsActive(); return NS_OK; }
  NS_IMETHOD IsMenuBar(PRBool& isMenuBar) { isMenuBar = PR_TRUE; return NS_OK; }
  NS_IMETHOD ConsumeOutsideClicks(PRBool& aConsumeOutsideClicks) \
    {aConsumeOutsideClicks = PR_FALSE; return NS_OK;}
  NS_IMETHOD ClearRecentlyRolledUp();
  NS_IMETHOD RecentlyRolledUp(nsIMenuFrame *aMenuFrame, PRBool *aJustRolledUp);

  NS_IMETHOD SetIsContextMenu(PRBool aIsContextMenu) { return NS_OK; }
  NS_IMETHOD GetIsContextMenu(PRBool& aIsContextMenu) { aIsContextMenu = PR_FALSE; return NS_OK; }

  NS_IMETHOD GetParentPopup(nsIMenuParent** aResult) { *aResult = nsnull;
                                                       return NS_OK;}

  NS_IMETHOD IsActive() { return mIsActive; }

  NS_IMETHOD IsOpen();
  NS_IMETHOD KillPendingTimers();
  NS_IMETHOD CancelPendingTimers() { return NS_OK; }

  
  NS_IMETHOD DismissChain();

  
  NS_IMETHOD HideChain();

  NS_IMETHOD InstallKeyboardNavigator();
  NS_IMETHOD RemoveKeyboardNavigator();

  NS_IMETHOD GetWidget(nsIWidget **aWidget);
  
  NS_IMETHOD AttachedDismissalListener() { return NS_OK; }

  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);

  virtual void Destroy();



  
  void ToggleMenuActiveState();
  
  
  NS_IMETHOD KeyboardNavigation(PRUint32 aKeyCode, PRBool& aHandledFlag);
  NS_IMETHOD ShortcutNavigation(nsIDOMKeyEvent* aKeyEvent, PRBool& aHandledFlag);
  
  NS_IMETHOD Escape(PRBool& aHandledFlag);
  
  NS_IMETHOD Enter();

  
  nsIMenuFrame* FindMenuWithShortcut(nsIDOMKeyEvent* aKeyEvent);

  PRBool IsValidItem(nsIContent* aContent);
  PRBool IsDisabled(nsIContent* aContent);

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
  nsMenuListener* mKeyboardNavigator;

  PRBool mIsActive; 
  nsIMenuFrame* mCurrentMenu; 

  
  
  nsIMenuFrame* mRecentRollupMenu; 

  nsIDOMEventReceiver* mTarget;

private:
  PRBool mCaretWasVisible;

}; 

#endif
