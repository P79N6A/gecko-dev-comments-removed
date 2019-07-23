




































#ifndef nsEnumeratorUtils_h__
#define nsEnumeratorUtils_h__

#include "nscore.h"

class nsISupports;
class nsISimpleEnumerator;

NS_COM_GLUE nsresult
NS_NewEmptyEnumerator(nsISimpleEnumerator* *aResult);

NS_COM_GLUE nsresult
NS_NewSingletonEnumerator(nsISimpleEnumerator* *result,
                          nsISupports* singleton);

NS_COM_GLUE nsresult
NS_NewUnionEnumerator(nsISimpleEnumerator* *result,
                      nsISimpleEnumerator* firstEnumerator,
                      nsISimpleEnumerator* secondEnumerator);

#endif 
