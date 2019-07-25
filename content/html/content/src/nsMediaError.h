




































#include "nsIDOMMediaError.h"
#include "nsISupports.h"
#include "mozilla/Attributes.h"

class nsMediaError MOZ_FINAL : public nsIDOMMediaError
{
public:
  nsMediaError(PRUint16 aCode);

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIDOMMEDIAERROR

private:
  
  PRUint16 mCode;
};
