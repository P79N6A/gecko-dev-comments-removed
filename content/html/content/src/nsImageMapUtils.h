









































#ifndef nsImageMapUtils_h___
#define nsImageMapUtils_h___

class nsIDocument;
class nsIDOMHTMLMapElement;
#include "nsAString.h"

class nsImageMapUtils {
  
  nsImageMapUtils();
  ~nsImageMapUtils();

public:

  






  static already_AddRefed<nsIDOMHTMLMapElement>
         FindImageMap(nsIDocument *aDocument, const nsAString &aUsemap);
};

#endif
