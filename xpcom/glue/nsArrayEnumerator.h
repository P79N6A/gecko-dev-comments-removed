





#ifndef nsArrayEnumerator_h__
#define nsArrayEnumerator_h__



#include "nscore.h"

class nsISimpleEnumerator;
class nsIArray;
class nsCOMArray_base;



nsresult
NS_NewArrayEnumerator(nsISimpleEnumerator** aResult,
                      nsIArray* aArray);





nsresult
NS_NewArrayEnumerator(nsISimpleEnumerator** aResult,
                      const nsCOMArray_base& aArray);

#endif
