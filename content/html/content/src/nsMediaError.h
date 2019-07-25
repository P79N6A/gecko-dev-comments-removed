




#include "nsIDOMMediaError.h"
#include "nsISupports.h"
#include "mozilla/Attributes.h"

class nsMediaError MOZ_FINAL : public nsIDOMMediaError
{
public:
  nsMediaError(uint16_t aCode);

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIDOMMEDIAERROR

private:
  
  uint16_t mCode;
};
