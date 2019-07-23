




































#ifndef nsIMenuFrame_h___
#define nsIMenuFrame_h___


#define NS_IMENUFRAME_IID \
{ 0x2281efc8, 0xa8ba, 0x4a73, { 0x8c, 0xf7, 0xdb, 0x4e, 0xec, 0xa5, 0xea, 0xec } }

class nsIMenuParent;
class nsIDOMElement;
class nsIDOMKeyEvent;

enum nsMenuType {
  eMenuType_Normal = 0,
  eMenuType_Checkbox = 1,
  eMenuType_Radio = 2
};

class nsIMenuFrame : public nsISupports {

public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IMENUFRAME_IID)

  NS_IMETHOD ActivateMenu(PRBool aActivate) = 0;
  NS_IMETHOD SelectMenu(PRBool aFlag) = 0;
  NS_IMETHOD OpenMenu(PRBool aFlag) = 0;

  NS_IMETHOD MenuIsOpen(PRBool& aResult) = 0;
  NS_IMETHOD MenuIsContainer(PRBool& aResult) = 0;
  NS_IMETHOD MenuIsChecked(PRBool& aResult) = 0;
  NS_IMETHOD MenuIsDisabled(PRBool& aResult) = 0;

  NS_IMETHOD SelectFirstItem() = 0;

  NS_IMETHOD Escape(PRBool& aHandledFlag) = 0;
  NS_IMETHOD Enter() = 0;
  NS_IMETHOD ShortcutNavigation(nsIDOMKeyEvent* aKeyEvent, PRBool& aHandledFlag) = 0;
  NS_IMETHOD KeyboardNavigation(PRUint32 aKeyCode, PRBool& aHandledFlag) = 0;

  virtual nsIMenuParent *GetMenuParent() = 0;
  virtual nsIFrame *GetMenuChild() = 0;
 
  NS_IMETHOD GetRadioGroupName(nsString &aName) = 0;
  NS_IMETHOD GetMenuType(nsMenuType &aType) = 0;

  NS_IMETHOD MarkAsGenerated() = 0;

  NS_IMETHOD UngenerateMenu() = 0;

  NS_IMETHOD GetActiveChild(nsIDOMElement** aResult)=0;
  NS_IMETHOD SetActiveChild(nsIDOMElement* aChild)=0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIMenuFrame, NS_IMENUFRAME_IID)

#endif

