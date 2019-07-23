





































#ifndef nsMenuBaseX_h_
#define nsMenuBaseX_h_

#include "nsCOMPtr.h"
#include "nsIContent.h"

enum nsMenuObjectTypeX {
  eMenuBarObjectType,
  eSubmenuObjectType,
  eMenuItemObjectType
};






class nsMenuObjectX
{
public:
  virtual nsMenuObjectTypeX MenuObjectType()=0;
  virtual void*             NativeData()=0;
  nsIContent*               Content() { return mContent; }

protected:
  nsCOMPtr<nsIContent> mContent;
};

#endif 
