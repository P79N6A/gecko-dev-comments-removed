





#ifndef nsNSSDialogHelper_h
#define nsNSSDialogHelper_h

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

#endif
