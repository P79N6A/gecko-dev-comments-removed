








#ifndef nsMenuFrame_h__
#define nsMenuFrame_h__

#include "nsIAtom.h"
#include "nsCOMPtr.h"

#include "nsBoxFrame.h"
#include "nsFrameList.h"
#include "nsGkAtoms.h"
#include "nsMenuParent.h"
#include "nsXULPopupManager.h"
#include "nsITimer.h"
#include "mozilla/Attributes.h"

nsIFrame* NS_NewMenuFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
nsIFrame* NS_NewMenuItemFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

class nsIContent;
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








class nsMenuTimerMediator MOZ_FINAL : public nsITimerCallback
{
public:
  explicit nsMenuTimerMediator(nsMenuFrame* aFrame);

  NS_DECL_ISUPPORTS
  NS_DECL_NSITIMERCALLBACK

  void ClearFrame();

private:
  ~nsMenuTimerMediator();

  
  nsMenuFrame* mFrame;
};

class nsMenuFrame MOZ_FINAL : public nsBoxFrame
{
public:
  nsMenuFrame(nsIPresShell* aShell, nsStyleContext* aContext);

  NS_DECL_QUERYFRAME_TARGET(nsMenuFrame)
  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  NS_IMETHOD DoLayout(nsBoxLayoutState& aBoxLayoutState) MOZ_OVERRIDE;
  virtual nsSize GetMinSize(nsBoxLayoutState& aBoxLayoutState) MOZ_OVERRIDE;
  virtual nsSize GetPrefSize(nsBoxLayoutState& aBoxLayoutState) MOZ_OVERRIDE;

  virtual void Init(nsIContent*       aContent,
                    nsContainerFrame* aParent,
                    nsIFrame*         aPrevInFlow) MOZ_OVERRIDE;

#ifdef DEBUG_LAYOUT
  virtual nsresult SetDebug(nsBoxLayoutState& aState, bool aDebug) MOZ_OVERRIDE;
#endif

  
  
  
  virtual const nsFrameList& GetChildList(ChildListID aList) const MOZ_OVERRIDE;
  virtual void GetChildLists(nsTArray<ChildList>* aLists) const MOZ_OVERRIDE;
  virtual void DestroyFrom(nsIFrame* aDestructRoot) MOZ_OVERRIDE;

  
  virtual void BuildDisplayListForChildren(nsDisplayListBuilder*   aBuilder,
                                           const nsRect&           aDirtyRect,
                                           const nsDisplayListSet& aLists) MOZ_OVERRIDE;
                                         
  
  virtual nsresult HandleEvent(nsPresContext* aPresContext,
                               mozilla::WidgetGUIEvent* aEvent,
                               nsEventStatus* aEventStatus) MOZ_OVERRIDE;

  virtual void SetInitialChildList(ChildListID  aListID,
                                   nsFrameList& aChildList) MOZ_OVERRIDE;
  virtual void AppendFrames(ChildListID     aListID,
                            nsFrameList&    aFrameList) MOZ_OVERRIDE;
  virtual void InsertFrames(ChildListID     aListID,
                            nsIFrame*       aPrevFrame,
                            nsFrameList&    aFrameList) MOZ_OVERRIDE;
  virtual void RemoveFrame(ChildListID     aListID,
                           nsIFrame*       aOldFrame) MOZ_OVERRIDE;

  virtual nsIAtom* GetType() const MOZ_OVERRIDE { return nsGkAtoms::menuFrame; }

  NS_IMETHOD SelectMenu(bool aActivateFlag);

  virtual nsIScrollableFrame* GetScrollTargetFrame() MOZ_OVERRIDE;

  
  
  
  
  nsIContent* GetAnchor();

  


  void OpenMenu(bool aSelectFirstItem);
  
  void CloseMenu(bool aDeselectMenu);

  bool IsChecked() { return mChecked; }

  NS_IMETHOD GetActiveChild(nsIDOMElement** aResult);
  NS_IMETHOD SetActiveChild(nsIDOMElement* aChild);

  
  
  
  
  nsMenuFrame* Enter(mozilla::WidgetGUIEvent* aEvent);

  
  nsMenuParent* GetMenuParent() const;

  const nsAString& GetRadioGroupName() { return mGroupName; }
  nsMenuType GetMenuType() { return mType; }
  nsMenuPopupFrame* GetPopup();

  


  bool HasPopup() const
  {
    return (GetStateBits() & NS_STATE_MENU_HAS_POPUP_LIST) != 0;
  }


  

  bool IsOnMenuBar() const
  {
    nsMenuParent* menuParent = GetMenuParent();
    return menuParent && menuParent->IsMenuBar();
  }
  bool IsOnActiveMenuBar() const
  {
    nsMenuParent* menuParent = GetMenuParent();
    return menuParent && menuParent->IsMenuBar() && menuParent->IsActive();
  }
  virtual bool IsOpen();
  virtual bool IsMenu();
  nsMenuListType GetParentMenuListType();
  bool IsDisabled();
  void ToggleMenuState();

  
  
  
  void PopupOpened();
  
  
  
  
  void PopupClosed(bool aDeselectMenu);

  
  
  bool IsOnMenu() const
  {
    nsMenuParent* menuParent = GetMenuParent();
    return menuParent && menuParent->IsMenu();
  }
  void SetIsMenu(bool aIsMenu) { mIsMenu = aIsMenu; }

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const MOZ_OVERRIDE
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

  



  nsFrameList* GetPopupList() const;

  


  void DestroyPopupList();

  
  
  void UpdateMenuType(nsPresContext* aPresContext);
  
  
  void UpdateMenuSpecialState(nsPresContext* aPresContext);

  
  void BuildAcceleratorText(bool aNotify);

  
  void Execute(mozilla::WidgetGUIEvent *aEvent);

  
  virtual nsresult AttributeChanged(int32_t aNameSpaceID,
                                    nsIAtom* aAttribute,
                                    int32_t aModType) MOZ_OVERRIDE;
  virtual ~nsMenuFrame() { }

  bool SizeToPopup(nsBoxLayoutState& aState, nsSize& aSize);

  bool ShouldBlink();
  void StartBlinking(mozilla::WidgetGUIEvent* aEvent, bool aFlipChecked);
  void StopBlinking();
  void CreateMenuCommandEvent(mozilla::WidgetGUIEvent* aEvent,
                              bool aFlipChecked);
  void PassMenuCommandEventToPopupManager();

protected:
#ifdef DEBUG_LAYOUT
  nsresult SetDebug(nsBoxLayoutState& aState, nsIFrame* aList, bool aDebug);
#endif
  nsresult Notify(nsITimer* aTimer);

  bool mIsMenu; 
  bool mChecked;              
  bool mIgnoreAccelTextChange; 
  nsMenuType mType;

  
  nsRefPtr<nsMenuTimerMediator> mTimerMediator;

  nsCOMPtr<nsITimer> mOpenTimer;
  nsCOMPtr<nsITimer> mBlinkTimer;

  uint8_t mBlinkState; 
  nsRefPtr<nsXULMenuCommandEvent> mDelayedMenuCommandEvent;

  nsString mGroupName;

}; 

#endif
