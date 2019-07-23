




































#ifndef nsSupportsArrayEnumerator_h___
#define nsSupportsArrayEnumerator_h___

#include "nsIEnumerator.h"

class nsISupportsArray;

class nsSupportsArrayEnumerator : public nsIBidirectionalEnumerator {
public:
  NS_DECL_ISUPPORTS

  nsSupportsArrayEnumerator(nsISupportsArray* array);

  
  NS_DECL_NSIENUMERATOR

  
  NS_DECL_NSIBIDIRECTIONALENUMERATOR

private:
  ~nsSupportsArrayEnumerator();

protected:
  nsISupportsArray*     mArray;
  PRInt32               mCursor;

};

#endif 

