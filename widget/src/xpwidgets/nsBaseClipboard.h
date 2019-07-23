




































#ifndef nsBaseClipboard_h__
#define nsBaseClipboard_h__

#include "nsIClipboard.h"
#include "nsITransferable.h"

class nsITransferable;
class nsDataObj;
class nsIClipboardOwner;
class nsIWidget;





class nsBaseClipboard : public nsIClipboard
{

public:
  nsBaseClipboard();
  virtual ~nsBaseClipboard();

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSICLIPBOARD
  
protected:

  NS_IMETHOD SetNativeClipboardData ( PRInt32 aWhichClipboard ) = 0;
  NS_IMETHOD GetNativeClipboardData ( nsITransferable * aTransferable, PRInt32 aWhichClipboard ) = 0;

  PRBool              mIgnoreEmptyNotification;
  nsIClipboardOwner * mClipboardOwner;
  nsITransferable   * mTransferable;

};

#endif 

