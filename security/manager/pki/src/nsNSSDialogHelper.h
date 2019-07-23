







































#include "nsIDOMWindowInternal.h"







class nsNSSDialogHelper
{
public:
  
  
  static nsresult openDialog(
                  nsIDOMWindowInternal *window,
                  const char *url,
                  nsISupports *params,
                  PRBool modal = PR_TRUE);
};

