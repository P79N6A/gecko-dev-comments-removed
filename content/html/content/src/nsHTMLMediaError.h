




































#include "nsIDOMHTMLMediaError.h"
#include "nsISupports.h"

class nsHTMLMediaError : public nsIDOMHTMLMediaError
{
public:
  nsHTMLMediaError(PRUint16 aCode);

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIDOMHTMLMEDIAERROR

private:
  
  PRUint16 mCode;
};
