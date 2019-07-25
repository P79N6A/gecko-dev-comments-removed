




































#include "nsIDOMMediaError.h"
#include "nsISupports.h"

class nsMediaError : public nsIDOMMediaError
{
public:
  nsMediaError(PRUint16 aCode);

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIDOMMEDIAERROR

private:
  
  PRUint16 mCode;
};
