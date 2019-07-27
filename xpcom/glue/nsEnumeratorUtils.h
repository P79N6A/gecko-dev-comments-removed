




#ifndef nsEnumeratorUtils_h__
#define nsEnumeratorUtils_h__

#include "nscore.h"

class nsISupports;
class nsISimpleEnumerator;

NS_COM_GLUE nsresult NS_NewEmptyEnumerator(nsISimpleEnumerator** aResult);

NS_COM_GLUE nsresult NS_NewSingletonEnumerator(nsISimpleEnumerator** aResult,
                                               nsISupports* aSingleton);

NS_COM_GLUE nsresult NS_NewUnionEnumerator(nsISimpleEnumerator** aResult,
                                           nsISimpleEnumerator* aFirstEnumerator,
                                           nsISimpleEnumerator* aSecondEnumerator);

#endif 
