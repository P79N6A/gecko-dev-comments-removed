





































#ifndef nsMenuBaseX_h_
#define nsMenuBaseX_h_

#import <Foundation/Foundation.h>

#include "nsCOMPtr.h"
#include "nsIContent.h"

enum nsMenuObjectTypeX {
  eMenuBarObjectType,
  eSubmenuObjectType,
  eMenuItemObjectType,
  eStandaloneNativeMenuObjectType,
};






class nsMenuObjectX
{
public:
  virtual ~nsMenuObjectX() { }
  virtual nsMenuObjectTypeX MenuObjectType()=0;
  virtual void*             NativeData()=0;
  nsIContent*               Content() { return mContent; }

protected:
  nsCOMPtr<nsIContent> mContent;
};






class nsMenuGroupOwnerX;

@interface MenuItemInfo : NSObject
{
  nsMenuGroupOwnerX * mMenuGroupOwner;
}

- (id) initWithMenuGroupOwner:(nsMenuGroupOwnerX *)aMenuGroupOwner;
- (nsMenuGroupOwnerX *) menuGroupOwner;
- (void) setMenuGroupOwner:(nsMenuGroupOwnerX *)aMenuGroupOwner;

@end






enum {
  eCommand_ID_About = 1,
  eCommand_ID_Prefs = 2,
  eCommand_ID_Quit  = 3,
  eCommand_ID_Last  = 4
};

#endif
