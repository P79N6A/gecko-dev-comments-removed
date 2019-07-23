






































#ifndef nsClipboard_h__
#define nsClipboard_h__

#include "nsIClipboard.h"
#include "nsITransferable.h"
#include "nsIClipboardOwner.h"
#include <nsCOMPtr.h>
#include "nsWidget.h"

#include <X11/X.h>
#include <X11/Xlib.h>

class nsITransferable;
class nsIClipboardOwner;
class nsIWidget;





class nsClipboard : public nsIClipboard
{

public:
  nsClipboard();
  virtual ~nsClipboard();

  static void Shutdown();

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSICLIPBOARD

protected:
  void Init(void);

  static nsEventStatus PR_CALLBACK Callback(nsGUIEvent *event);
  PRBool  mIgnoreEmptyNotification;
  inline nsITransferable *GetTransferable(PRInt32 aWhichClipboard);
  NS_IMETHOD SetNativeClipboardData(PRInt32 aWhichClipboard);

private:
  static nsITransferable*     mTransferable; 
  nsCOMPtr<nsIClipboardOwner> mSelectionOwner;
  nsCOMPtr<nsIClipboardOwner> mGlobalOwner;
  nsCOMPtr<nsITransferable>   mSelectionTransferable;
  nsCOMPtr<nsITransferable>   mGlobalTransferable;


  
  
  PRBool mBlocking;

  static Window sWindow;
  nsWidget *sWidget;
  static Display *sDisplay;

};

#endif 
