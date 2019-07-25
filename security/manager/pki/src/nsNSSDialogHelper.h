







































#include "nsIDOMWindow.h"







class nsNSSDialogHelper
{
public:
  
  
  static nsresult openDialog(
                  nsIDOMWindow *window,
                  const char *url,
                  nsISupports *params,
                  bool modal = true);
};

