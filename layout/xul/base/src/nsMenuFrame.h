









































#ifndef nsMenuFrame_h__
#define nsMenuFrame_h__

#include "prtypes.h"
#include "nsIAtom.h"
#include "nsCOMPtr.h"

#include "nsBoxFrame.h"
#include "nsFrameList.h"
#include "nsIMenuParent.h"
#include "nsIMenuFrame.h"
#include "nsMenuDismissalListener.h"
#include "nsITimer.h"
#include "nsISupportsArray.h"
#include "nsIDOMText.h"
#include "nsIContent.h"
#include "nsIScrollableViewProvider.h"

nsIFrame* NS_NewMenuFrame(nsIPresShell* aPresShell, nsStyleContext* aContext, PRUint32 aFlags);

class nsMenuBarFrame;
class nsMenuPopupFrame;
class nsIScrollableView;

#define NS_STATE_ACCELTEXT_IS_DERIVED  NS_STATE_BOX_CHILD_RESERVED

class nsMenuFrame;








class nsMenuTimerMediator : public nsITimerCallback
{
public:
  nsMenuTimerMediator(nsMenuFrame* aFrame);
  ~nsMenuTimerMediator();

  NS_DECL_ISUPPORTS
  NS_DECL_NSITIMERCALLBACK

  void ClearFrame();

private:

  
  nsMenuFrame* mFrame;
};







class nsMenuFrame : public nsBoxFrame, 
                    public nsIMenuFrame,
                    public nsIScrollableViewProvider
{
public:
  nsMenuFrame(nsIPresShell* aShell, nsStyleContext* aContext);

  NS_DECL_ISUPPORTS

  
  NS_IMETHOD DoLayout(nsBoxLayoutState& aBoxLayoutState);
  virtual nsSize GetMinSize(nsBoxLayoutState& aBoxLayoutState);
  virtual nsSize GetPrefSize(nsBoxLayoutState& aBoxLayoutState);

  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);

#ifdef DEBUG_LAYOUT
  NS_IMETHOD SetDebug(nsBoxLayoutState& aState, PRBool aDebug);
#endif

  NS_IMETHOD IsActive(PRBool& aResult) { aResult = PR_TRUE; return NS_OK; }

  
  
  
  virtual nsIFrame* GetFirstChild(nsIAtom* aListName) const;
  NS_IMETHOD SetInitialChildList(nsIAtom*        aListName,
                                 nsIFrame*       aChildList);
  virtual nsIAtom* GetAdditionalChildListName(PRInt32 aIndex) const;
  virtual void Destroy(); 

  
  NS_IMETHOD BuildDisplayListForChildren(nsDisplayListBuilder*   aBuilder,
                                         const nsRect&           aDirtyRect,
                                         const nsDisplayListSet& aLists);
                                         
  NS_IMETHOD HandleEvent(nsPresContext* aPresContext, 
                         nsGUIEvent*     aEvent,
                         nsEventStatus*  aEventStatus); 

  NS_IMETHOD  AppendFrames(nsIAtom*        aListName,
                           nsIFrame*       aFrameList);

  NS_IMETHOD  InsertFrames(nsIAtom*        aListName,
                           nsIFrame*       aPrevFrame,
                           nsIFrame*       aFrameList);

  NS_IMETHOD  RemoveFrame(nsIAtom*        aListName,
                          nsIFrame*       aOldFrame);

  

  NS_IMETHOD ActivateMenu(PRBool aActivateFlag); 
  NS_IMETHOD SelectMenu(PRBool aActivateFlag); 
  NS_IMETHOD OpenMenu(PRBool aActivateFlag); 

  NS_IMETHOD MenuIsOpen(PRBool& aResult) { aResult = IsOpen(); return NS_OK; }
  NS_IMETHOD MenuIsContainer(PRBool& aResult) { aResult = IsMenu(); return NS_OK; }
  NS_IMETHOD MenuIsChecked(PRBool& aResult) { aResult = mChecked; return NS_OK; }
  NS_IMETHOD MenuIsDisabled(PRBool& aResult) { aResult = IsDisabled(); return NS_OK; }
  
  NS_IMETHOD GetActiveChild(nsIDOMElement** aResult);
  NS_IMETHOD SetActiveChild(nsIDOMElement* aChild); 

  NS_IMETHOD UngenerateMenu(); 

  NS_IMETHOD SelectFirstItem(); 

  NS_IMETHOD Escape(PRBool& aHandledFlag); 
  NS_IMETHOD Enter(); 
  NS_IMETHOD ShortcutNavigation(nsIDOMKeyEvent* aKeyEvent, PRBool& aHandledFlag); 
  NS_IMETHOD KeyboardNavigation(PRUint32 aKeyCode, PRBool& aHandledFlag); 

  NS_IMETHOD SetParent(const nsIFrame* aParent);

  virtual nsIMenuParent *GetMenuParent() { return mMenuParent; }
  virtual nsIFrame *GetMenuChild() { return mPopupFrames.FirstChild(); }
  NS_IMETHOD GetRadioGroupName(nsString &aName) { aName = mGroupName; return NS_OK; }
  NS_IMETHOD GetMenuType(nsMenuType &aType) { aType = mType; return NS_OK; }
  NS_IMETHOD MarkAsGenerated();

  

  virtual nsIScrollableView* GetScrollableView();

  

  nsresult DestroyPopupFrames(nsPresContext* aPresContext);

  PRBool IsOpen() { return mMenuOpen; }
  PRBool IsMenu();
  PRBool IsDisabled();
  PRBool IsGenerated();
  NS_IMETHOD ToggleMenuState(); 

  void SetIsMenu(PRBool aIsMenu) { mIsMenu = aIsMenu; }

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
      return MakeFrameName(NS_LITERAL_STRING("Menu"), aResult);
  }
#endif

  static PRBool IsSizedToPopup(nsIContent* aContent, PRBool aRequireAlways);

  static nsIMenuParent *GetContextMenu();

protected:
  friend class nsMenuTimerMediator;
  
  virtual void RePositionPopup(nsBoxLayoutState& aState);

  void
  ConvertPosition(nsIContent* aPopupElt, nsString& aAnchor, nsString& aAlign);

  friend class nsASyncMenuInitialization;
  void UpdateMenuType(nsPresContext* aPresContext); 
  void UpdateMenuSpecialState(nsPresContext* aPresContext); 

  void OpenMenuInternal(PRBool aActivateFlag); 
  void GetMenuChildrenElement(nsIContent** aResult);

  
  void BuildAcceleratorText();

  
  void Execute(nsGUIEvent *aEvent); 

  
  PRBool OnCreate(); 

  
  PRBool OnCreated(); 

  
  PRBool OnDestroy(); 

  
  PRBool OnDestroyed(); 

  NS_IMETHOD AttributeChanged(PRInt32 aNameSpaceID,
                              nsIAtom* aAttribute,
                              PRInt32 aModType); 
  virtual ~nsMenuFrame();

  PRBool SizeToPopup(nsBoxLayoutState& aState, nsSize& aSize);

protected:
#ifdef DEBUG_LAYOUT
  nsresult SetDebug(nsBoxLayoutState& aState, nsIFrame* aList, PRBool aDebug);
#endif
  NS_HIDDEN_(nsresult) Notify(nsITimer* aTimer);

  nsFrameList mPopupFrames;
  PRPackedBool mIsMenu; 
  PRPackedBool mMenuOpen;
  PRPackedBool mCreateHandlerSucceeded;  
  PRPackedBool mChecked;              
  nsMenuType mType;

  nsIMenuParent* mMenuParent; 

  
  nsRefPtr<nsMenuTimerMediator> mTimerMediator;

  nsCOMPtr<nsITimer> mOpenTimer;

  nsString mGroupName;
  nsSize mLastPref;
  
  
  static nsrefcnt gRefCnt; 
  static nsString *gShiftText;
  static nsString *gControlText;
  static nsString *gMetaText;
  static nsString *gAltText;
  static nsString *gModifierSeparator;

}; 

#endif
