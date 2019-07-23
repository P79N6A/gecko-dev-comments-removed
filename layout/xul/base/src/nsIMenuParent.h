





































#ifndef nsIMenuParent_h___
#define nsIMenuParent_h___



#define NS_IMENUPARENT_IID \
{ 0x33f700c8, 0x976a, 0x4cdb, { 0x8f, 0x6c, 0xd9, 0xf4, 0xcf, 0xee, 0x83, 0x66 } }

class nsIMenuFrame;
class nsIDOMKeyEvent;
































enum nsNavigationDirection {
  eNavigationDirection_Last,
  eNavigationDirection_First,
  eNavigationDirection_Start,
  eNavigationDirection_Before,
  eNavigationDirection_End,
  eNavigationDirection_After
};

#define NS_DIRECTION_IS_INLINE(dir) (dir == eNavigationDirection_Start ||     \
                                     dir == eNavigationDirection_End)
#define NS_DIRECTION_IS_BLOCK(dir) (dir == eNavigationDirection_Before || \
                                    dir == eNavigationDirection_After)
#define NS_DIRECTION_IS_BLOCK_TO_EDGE(dir) (dir == eNavigationDirection_First ||    \
                                            dir == eNavigationDirection_Last)





static nsNavigationDirection DirectionFromKeyCode_lr_tb [6] = {
  eNavigationDirection_Last,   
  eNavigationDirection_First,  
  eNavigationDirection_Start,  
  eNavigationDirection_Before, 
  eNavigationDirection_End,    
  eNavigationDirection_After   
};





static nsNavigationDirection DirectionFromKeyCode_rl_tb [6] = {
  eNavigationDirection_Last,   
  eNavigationDirection_First,  
  eNavigationDirection_End,    
  eNavigationDirection_Before, 
  eNavigationDirection_Start,  
  eNavigationDirection_After   
};

#ifdef IBMBIDI
#define NS_DIRECTION_FROM_KEY_CODE(direction, keycode)           \
  NS_ASSERTION(keycode >= NS_VK_END && keycode <= NS_VK_DOWN,    \
               "Illegal key code");                              \
  const nsStyleVisibility* vis = GetStyleVisibility();           \
  if (vis->mDirection == NS_STYLE_DIRECTION_RTL)                 \
    direction = DirectionFromKeyCode_rl_tb[keycode - NS_VK_END]; \
  else                                                           \
    direction = DirectionFromKeyCode_lr_tb[keycode - NS_VK_END];
#else
#define NS_DIRECTION_FROM_KEY_CODE(direction, keycode)           \
    direction = DirectionFromKeyCode_lr_tb[keycode - NS_VK_END];
#endif

class nsIMenuParent : public nsISupports {

public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IMENUPARENT_IID)

  virtual nsIMenuFrame *GetCurrentMenuItem() = 0;
  NS_IMETHOD SetCurrentMenuItem(nsIMenuFrame* aMenuItem) = 0;
  virtual nsIMenuFrame *GetNextMenuItem(nsIMenuFrame* aStart) = 0;
  virtual nsIMenuFrame *GetPreviousMenuItem(nsIMenuFrame* aStart) = 0;

  NS_IMETHOD SetActive(PRBool aActiveFlag) = 0;
  NS_IMETHOD GetIsActive(PRBool& isActive) = 0;
  NS_IMETHOD GetWidget(nsIWidget **aWidget) = 0;
  
  NS_IMETHOD IsMenuBar(PRBool& isMenuBar) = 0;
  NS_IMETHOD ConsumeOutsideClicks(PRBool& aConsumeOutsideClicks) = 0;
  NS_IMETHOD ClearRecentlyRolledUp() = 0;
  NS_IMETHOD RecentlyRolledUp(nsIMenuFrame *aMenuFrame, PRBool *aJustRolledUp) = 0;

  NS_IMETHOD DismissChain() = 0;
  NS_IMETHOD HideChain() = 0;
  NS_IMETHOD KillPendingTimers() = 0;
  NS_IMETHOD CancelPendingTimers() = 0;

  NS_IMETHOD AttachedDismissalListener() = 0;

  NS_IMETHOD InstallKeyboardNavigator() = 0;
  NS_IMETHOD RemoveKeyboardNavigator() = 0;

  
  NS_IMETHOD KeyboardNavigation(PRUint32 aKeyCode, PRBool& aHandledFlag) = 0;
  NS_IMETHOD ShortcutNavigation(nsIDOMKeyEvent* aKeyEvent, PRBool& aHandledFlag) = 0;
  
  NS_IMETHOD Escape(PRBool& aHandledFlag) = 0;
  
  NS_IMETHOD Enter() = 0;

  NS_IMETHOD SetIsContextMenu(PRBool aIsContextMenu) = 0;
  NS_IMETHOD GetIsContextMenu(PRBool& aIsContextMenu) = 0;

  NS_IMETHOD GetParentPopup(nsIMenuParent** aResult) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIMenuParent, NS_IMENUPARENT_IID)

#endif

