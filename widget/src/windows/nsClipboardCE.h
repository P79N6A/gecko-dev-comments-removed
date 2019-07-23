





































#ifndef nsClipboardCE_h__
#define nsClipboardCE_h__

#include "nsBaseClipboard.h"

#include <windows.h>





class nsClipboard :
  public nsBaseClipboard
{
public:
  nsClipboard();
  virtual ~nsClipboard();

protected:
  NS_IMETHOD SetNativeClipboardData (PRInt32 aWhichClipboard);
  NS_IMETHOD GetNativeClipboardData (nsITransferable * aTransferable, PRInt32 aWhichClipboard);
};

#endif
