




#ifndef nsSupportsArrayEnumerator_h___
#define nsSupportsArrayEnumerator_h___

#include "nsIEnumerator.h"
#include "mozilla/Attributes.h"

class nsISupportsArray;

class nsSupportsArrayEnumerator MOZ_FINAL : public nsIBidirectionalEnumerator {
public:
  NS_DECL_ISUPPORTS

  nsSupportsArrayEnumerator(nsISupportsArray* array);

  
  NS_DECL_NSIENUMERATOR

  
  NS_DECL_NSIBIDIRECTIONALENUMERATOR

private:
  ~nsSupportsArrayEnumerator();

protected:
  nsISupportsArray*     mArray;
  int32_t               mCursor;

};

#endif 

