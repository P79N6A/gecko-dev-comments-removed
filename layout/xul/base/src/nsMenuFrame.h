









































#ifndef nsMenuFrame_h__
#define nsMenuFrame_h__

#include "prtypes.h"
#include "nsIAtom.h"
#include "nsCOMPtr.h"

#include "nsBoxFrame.h"
#include "nsFrameList.h"
#include "nsGkAtoms.h"
#include "nsMenuParent.h"
#include "nsXULPopupManager.h"
#include "nsITimer.h"
#include "nsIContent.h"

nsIFrame* NS_NewMenuFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame* NS_NewMenuItemFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

class nsMenuBarFrame;

#define NS_STATE_ACCELTEXT_IS_DERIVED  NS_STATE_BOX_CHILD_RESERVED


enum nsMenuType {
  
  eMenuType_Normal = 0,
  
  eMenuType_Checkbox = 1,
  
  
  eMenuType_Radio = 2
};

enum nsMenuListType {
  eNotMenuList,      
  eReadonlyMenuList, 
  eEditableMenuList  
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

class nsMenuFrame : public nsBoxFrame
{
public:
  nsMenuFrame(nsIPresShell* aShell, nsStyleContext* aContext);

  NS_DECL_QUERYFRAME_TARGET(nsMenuFrame)
  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  
  NS_IMETHOD DoLayout(nsBoxLayoutState& aBoxLayoutState);
  virtual nsSize GetMinSize(nsBoxLayoutState& aBoxLayoutState);
  virtual nsSize GetPrefSize(nsBoxLayoutState& aBoxLayoutState);

  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);

#ifdef DEBUG_LAYOUT
  NS_IMETHOD SetDebug(nsBoxLayoutState& aState, bool aDebug);
#endif

  
  
  
  virtual nsFrameList GetChildList(ChildListID aList) const;
  virtual void GetChildLists(nsTArray<ChildList>* aLists) const;
  NS_IMETHOD SetInitialChildList(ChildListID     aListID,
                                 nsFrameList&    aChildList);
  virtual void DestroyFrom(nsIFrame* aDestructRoot);

  
  NS_IMETHOD BuildDisplayListForChildren(nsDisplayListBuilder*   aBuilder,
                                         const nsRect&           aDirtyRect,
                                         const nsDisplayListSet& aLists);
                                         
  
  NS_IMETHOD HandleEvent(nsPresContext* aPresContext, 
                         nsGUIEvent*     aEvent,
                         nsEventStatus*  aEventStatus);

  NS_IMETHOD  AppendFrames(ChildListID     aListID,
                           nsFrameList&    aFrameList);

  NS_IMETHOD  InsertFrames(ChildListID     aListID,
                           nsIFrame*       aPrevFrame,
                           nsFrameList&    aFrameList);

  NS_IMETHOD  RemoveFrame(ChildListID     aListID,
                          nsIFrame*       aOldFrame);

  virtual nsIAtom* GetType() const { return nsGkAtoms::menuFrame; }

  NS_IMETHOD SelectMenu(bool aActivateFlag);

  virtual nsIScrollableFrame* GetScrollTargetFrame();

  


  void OpenMenu(bool aSelectFirstItem);
  
  void CloseMenu(bool aDeselectMenu);

  bool IsChecked() { return mChecked; }

  NS_IMETHOD GetActiveChild(nsIDOMElement** aResult);
  NS_IMETHOD SetActiveChild(nsIDOMElement* aChild);

  
  
  
  
  nsMenuFrame* Enter(nsGUIEvent* aEvent);

  virtual void SetParent(nsIFrame* aParent);

  virtual nsMenuParent *GetMenuParent() { return mMenuParent; }
  const nsAString& GetRadioGroupName() { return mGroupName; }
  nsMenuType GetMenuType() { return mType; }
  nsMenuPopupFrame* GetPopup() { return mPopupFrame; }

  

  bool IsOnMenuBar() { return mMenuParent && mMenuParent->IsMenuBar(); }
  bool IsOnActiveMenuBar() { return IsOnMenuBar() && mMenuParent->IsActive(); }
  virtual bool IsOpen();
  virtual bool IsMenu();
  nsMenuListType GetParentMenuListType();
  bool IsDisabled();
  void ToggleMenuState();

  
  
  
  void PopupOpened();
  
  
  
  
  void PopupClosed(bool aDeselectMenu);

  
  
  bool IsOnMenu() { return mMenuParent && mMenuParent->IsMenu(); }
  void SetIsMenu(bool aIsMenu) { mIsMenu = aIsMenu; }

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
      return MakeFrameName(NS_LITERAL_STRING("Menu"), aResult);
  }
#endif

  static bool IsSizedToPopup(nsIContent* aContent, bool aRequireAlways);

protected:
  friend class nsMenuTimerMediator;
  friend class nsASyncMenuInitialization;
  friend class nsMenuAttributeChangedEvent;

  
  
  void SetPopupFrame(nsFrameList& aChildList);

  
  
  
  void InitMenuParent(nsIFrame* aParent);

  
  
  void UpdateMenuType(nsPresContext* aPresContext);
  
  
  void UpdateMenuSpecialState(nsPresContext* aPresContext);

  
  void BuildAcceleratorText(bool aNotify);

  
  void Execute(nsGUIEvent *aEvent);

  
  NS_IMETHOD AttributeChanged(PRInt32 aNameSpaceID,
                              nsIAtom* aAttribute,
                              PRInt32 aModType);
  virtual ~nsMenuFrame() { };

  bool SizeToPopup(nsBoxLayoutState& aState, nsSize& aSize);

  bool ShouldBlink();
  void StartBlinking(nsGUIEvent *aEvent, bool aFlipChecked);
  void StopBlinking();
  void CreateMenuCommandEvent(nsGUIEvent *aEvent, bool aFlipChecked);
  void PassMenuCommandEventToPopupManager();

protected:
#ifdef DEBUG_LAYOUT
  nsresult SetDebug(nsBoxLayoutState& aState, nsIFrame* aList, bool aDebug);
#endif
  NS_HIDDEN_(nsresult) Notify(nsITimer* aTimer);

  bool mIsMenu; 
  bool mChecked;              
  bool mIgnoreAccelTextChange; 
  nsMenuType mType;

  nsMenuParent* mMenuParent; 

  
  nsMenuPopupFrame* mPopupFrame;

  
  nsRefPtr<nsMenuTimerMediator> mTimerMediator;

  nsCOMPtr<nsITimer> mOpenTimer;
  nsCOMPtr<nsITimer> mBlinkTimer;

  PRUint8 mBlinkState; 
  nsRefPtr<nsXULMenuCommandEvent> mDelayedMenuCommandEvent;

  nsString mGroupName;

}; 

#endif
