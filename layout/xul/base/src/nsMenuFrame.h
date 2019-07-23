









































#ifndef nsMenuFrame_h__
#define nsMenuFrame_h__

#include "prtypes.h"
#include "nsIAtom.h"
#include "nsCOMPtr.h"

#include "nsBoxFrame.h"
#include "nsFrameList.h"
#include "nsGkAtoms.h"
#include "nsMenuParent.h"
#include "nsIMenuFrame.h"
#include "nsXULPopupManager.h"
#include "nsITimer.h"
#include "nsIDOMText.h"
#include "nsIContent.h"
#include "nsIScrollableViewProvider.h"

nsIFrame* NS_NewMenuFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame* NS_NewMenuItemFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

class nsMenuBarFrame;
class nsIScrollableView;

#define NS_STATE_ACCELTEXT_IS_DERIVED  NS_STATE_BOX_CHILD_RESERVED


enum nsMenuType {
  
  eMenuType_Normal = 0,
  
  eMenuType_Checkbox = 1,
  
  
  eMenuType_Radio = 2
};

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

  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  
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

  
  
  
  virtual nsFrameList GetChildList(nsIAtom* aListName) const;
  NS_IMETHOD SetInitialChildList(nsIAtom*        aListName,
                                 nsFrameList&    aChildList);
  virtual nsIAtom* GetAdditionalChildListName(PRInt32 aIndex) const;
  virtual void Destroy();

  
  NS_IMETHOD BuildDisplayListForChildren(nsDisplayListBuilder*   aBuilder,
                                         const nsRect&           aDirtyRect,
                                         const nsDisplayListSet& aLists);
                                         
  
  NS_IMETHOD HandleEvent(nsPresContext* aPresContext, 
                         nsGUIEvent*     aEvent,
                         nsEventStatus*  aEventStatus);

  NS_IMETHOD  AppendFrames(nsIAtom*        aListName,
                           nsFrameList&    aFrameList);

  NS_IMETHOD  InsertFrames(nsIAtom*        aListName,
                           nsIFrame*       aPrevFrame,
                           nsFrameList&    aFrameList);

  NS_IMETHOD  RemoveFrame(nsIAtom*        aListName,
                          nsIFrame*       aOldFrame);

  virtual nsIAtom* GetType() const { return nsGkAtoms::menuFrame; }

  NS_IMETHOD SelectMenu(PRBool aActivateFlag);

  


  void OpenMenu(PRBool aSelectFirstItem);
  
  void CloseMenu(PRBool aDeselectMenu);

  PRBool IsChecked() { return mChecked; }

  NS_IMETHOD GetActiveChild(nsIDOMElement** aResult);
  NS_IMETHOD SetActiveChild(nsIDOMElement* aChild);

  
  
  
  
  nsMenuFrame* Enter();

  NS_IMETHOD SetParent(const nsIFrame* aParent);

  virtual nsMenuParent *GetMenuParent() { return mMenuParent; }
  const nsAString& GetRadioGroupName() { return mGroupName; }
  nsMenuType GetMenuType() { return mType; }
  nsMenuPopupFrame* GetPopup() { return mPopupFrame; }

  

  virtual nsIScrollableView* GetScrollableView();

  

  nsresult DestroyPopupFrames(nsPresContext* aPresContext);

  virtual PRBool IsOnMenuBar() { return mMenuParent && mMenuParent->IsMenuBar(); }
  virtual PRBool IsOnActiveMenuBar() { return IsOnMenuBar() && mMenuParent->IsActive(); }
  virtual PRBool IsOpen();
  virtual PRBool IsMenu();
  PRBool IsDisabled();
  PRBool IsGenerated();
  void ToggleMenuState();

  
  
  
  void PopupOpened();
  
  
  
  
  void PopupClosed(PRBool aDeselectMenu);

  
  
  PRBool IsOnMenu() { return mMenuParent && mMenuParent->IsMenu(); }
  void SetIsMenu(PRBool aIsMenu) { mIsMenu = aIsMenu; }

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
      return MakeFrameName(NS_LITERAL_STRING("Menu"), aResult);
  }
#endif

  static PRBool IsSizedToPopup(nsIContent* aContent, PRBool aRequireAlways);

protected:
  friend class nsMenuTimerMediator;
  friend class nsASyncMenuInitialization;

  
  
  void SetPopupFrame(nsFrameList& aChildList);

  
  
  
  void InitMenuParent(nsIFrame* aParent);

  
  
  void UpdateMenuType(nsPresContext* aPresContext);
  
  
  void UpdateMenuSpecialState(nsPresContext* aPresContext);

  
  void BuildAcceleratorText();

  
  void Execute(nsGUIEvent *aEvent);

  
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

  PRPackedBool mIsMenu; 
  PRPackedBool mChecked;              
  nsMenuType mType;

  nsMenuParent* mMenuParent; 

  
  nsMenuPopupFrame* mPopupFrame;

  
  nsRefPtr<nsMenuTimerMediator> mTimerMediator;

  nsCOMPtr<nsITimer> mOpenTimer;

  nsString mGroupName;
  
  
  static nsrefcnt gRefCnt; 
  static nsString *gShiftText;
  static nsString *gControlText;
  static nsString *gMetaText;
  static nsString *gAltText;
  static nsString *gModifierSeparator;

}; 

#endif
