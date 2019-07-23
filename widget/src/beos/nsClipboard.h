





































#ifndef nsClipboard_h__
#define nsClipboard_h__

#include "nsBaseClipboard.h"
#include <Clipboard.h>

class BView;
class nsITransferable;





class nsClipboard : public nsBaseClipboard
{

public:
  nsClipboard();
  virtual ~nsClipboard();

  static void SetTopLevelView(BView *v);


protected:
  NS_IMETHOD SetNativeClipboardData( PRInt32 aWhichClipboard );
  NS_IMETHOD GetNativeClipboardData(nsITransferable * aTransferable, PRInt32 aWhichClipboard);

  PRBool            mIgnoreEmptyNotification;

  static BView  *sView;
};

#endif 
