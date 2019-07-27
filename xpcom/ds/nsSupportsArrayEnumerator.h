





#ifndef nsSupportsArrayEnumerator_h___
#define nsSupportsArrayEnumerator_h___

#include "nsCOMPtr.h"
#include "nsIEnumerator.h"
#include "mozilla/Attributes.h"

class nsISupportsArray;

class nsSupportsArrayEnumerator final : public nsIBidirectionalEnumerator
{
public:
  NS_DECL_ISUPPORTS

  explicit nsSupportsArrayEnumerator(nsISupportsArray* aArray);

  
  NS_DECL_NSIENUMERATOR

  
  NS_DECL_NSIBIDIRECTIONALENUMERATOR

private:
  ~nsSupportsArrayEnumerator();

protected:
  nsCOMPtr<nsISupportsArray> mArray;
  int32_t               mCursor;

};

#endif 

