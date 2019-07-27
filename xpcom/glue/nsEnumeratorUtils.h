





#ifndef nsEnumeratorUtils_h__
#define nsEnumeratorUtils_h__

#include "nscore.h"

class nsISupports;
class nsISimpleEnumerator;

nsresult NS_NewEmptyEnumerator(nsISimpleEnumerator** aResult);

nsresult NS_NewSingletonEnumerator(nsISimpleEnumerator** aResult,
                                   nsISupports* aSingleton);

nsresult NS_NewUnionEnumerator(nsISimpleEnumerator** aResult,
                               nsISimpleEnumerator* aFirstEnumerator,
                               nsISimpleEnumerator* aSecondEnumerator);

#endif 
