





































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
  NS_IMETHOD HasDataMatchingFlavors(const char** aFlavorList, PRUint32 aLength,
                                    PRInt32 aWhichClipboard, PRBool *_retval);

protected:
  NS_IMETHOD SetNativeClipboardData (PRInt32 aWhichClipboard);
  NS_IMETHOD GetNativeClipboardData (nsITransferable * aTransferable, PRInt32 aWhichClipboard);

private:
  static UINT     GetFormat(const char* aMimeStr);
};

#endif
