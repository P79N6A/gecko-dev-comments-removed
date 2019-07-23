





































#ifndef nsClipboard_h_
#define nsClipboard_h_

#include "nsBaseClipboard.h"

#import <Cocoa/Cocoa.h>

class nsITransferable;


class nsClipboard : public nsBaseClipboard
{

public:
  nsClipboard();
  virtual ~nsClipboard();

  
  NS_IMETHOD  HasDataMatchingFlavors(nsISupportsArray *aFlavorList, PRInt32 aWhichClipboard, PRBool *_retval);

  
  static NSDictionary* PasteboardDictFromTransferable(nsITransferable *aTransferable);
  static nsresult CopyPasteboardDataToTransferable(NSPasteboard* aPasteboard, nsITransferable* aTransferable, PRUint32 aItemIndex);

protected:

  
  NS_IMETHOD SetNativeClipboardData(PRInt32 aWhichClipboard);
  NS_IMETHOD GetNativeClipboardData(nsITransferable * aTransferable, PRInt32 aWhichClipboard);

};

#endif 
