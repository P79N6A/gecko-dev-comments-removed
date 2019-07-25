





































#ifndef nsMenuItemX_h_
#define nsMenuItemX_h_

#include "nsMenuBaseX.h"
#include "nsMenuGroupOwnerX.h"
#include "nsChangeObserver.h"
#include "nsAutoPtr.h"

#import <Cocoa/Cocoa.h>

class nsString;
class nsMenuItemIconX;
class nsMenuX;

enum {
  knsMenuItemNoModifier      = 0,
  knsMenuItemShiftModifier   = (1 << 0),
  knsMenuItemAltModifier     = (1 << 1),
  knsMenuItemControlModifier = (1 << 2),
  knsMenuItemCommandModifier = (1 << 3)
};

enum EMenuItemType {
  eRegularMenuItemType = 0,
  eCheckboxMenuItemType,
  eRadioMenuItemType,
  eSeparatorMenuItemType
};




class nsMenuItemX : public nsMenuObjectX,
                    public nsChangeObserver
{
public:
  nsMenuItemX();
  virtual ~nsMenuItemX();

  NS_DECL_CHANGEOBSERVER

  
  void*             NativeData()     {return (void*)mNativeMenuItem;}
  nsMenuObjectTypeX MenuObjectType() {return eMenuItemObjectType;}

  
  nsresult      Create(nsMenuX* aParent, const nsString& aLabel, EMenuItemType aItemType,
                       nsMenuGroupOwnerX* aMenuGroupOwner, nsIContent* aNode);
  nsresult      SetChecked(bool aIsChecked);
  EMenuItemType GetMenuItemType();
  void          DoCommand();
  nsresult      DispatchDOMEvent(const nsString &eventName, bool* preventDefaultCalled);
  void          SetupIcon();

protected:
  void UncheckRadioSiblings(nsIContent* inCheckedElement);
  void SetKeyEquiv();

  EMenuItemType             mType;
  
  NSMenuItem*               mNativeMenuItem;      
  nsMenuX*                  mMenuParent;          
  nsMenuGroupOwnerX*        mMenuGroupOwner;      
  nsCOMPtr<nsIContent>      mCommandContent;
  
  nsRefPtr<nsMenuItemIconX> mIcon;
  bool                      mIsChecked;
};

#endif 
