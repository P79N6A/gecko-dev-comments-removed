







































#include "nsIDOMWindowInternal.h"







class nsNSSDialogHelper
{
public:
  const static char *kDefaultOpenWindowParam;
  
  
  static nsresult openDialog(
                  nsIDOMWindowInternal *window,
                  const char *url,
                  nsISupports *params);
};

