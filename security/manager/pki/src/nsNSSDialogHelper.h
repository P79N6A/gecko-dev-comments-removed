







































#include "nsIDOMWindow.h"







class nsNSSDialogHelper
{
public:
  
  
  static nsresult openDialog(
                  nsIDOMWindow *window,
                  const char *url,
                  nsISupports *params,
                  PRBool modal = PR_TRUE);
};

