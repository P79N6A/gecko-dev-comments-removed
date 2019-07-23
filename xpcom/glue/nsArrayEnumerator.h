






































#ifndef nsArrayEnumerator_h__
#define nsArrayEnumerator_h__



#include "nscore.h"

class nsISimpleEnumerator;
class nsIArray;
class nsCOMArray_base;



NS_COM_GLUE nsresult
NS_NewArrayEnumerator(nsISimpleEnumerator* *result,
                      nsIArray* array);





NS_COM_GLUE nsresult
NS_NewArrayEnumerator(nsISimpleEnumerator* *aResult,
                      const nsCOMArray_base& aArray);

#endif
