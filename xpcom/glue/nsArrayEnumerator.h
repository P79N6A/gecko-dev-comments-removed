






































#ifndef nsArrayEnumerator_h__
#define nsArrayEnumerator_h__



#include "nscore.h"

class nsISimpleEnumerator;
class nsIArray;
class nsCOMArray_base;



NS_COM_GLUE nsresult
NS_NewArrayEnumerator(nsISimpleEnumerator* *result NS_OUTPARAM,
                      nsIArray* array);





NS_COM_GLUE nsresult
NS_NewArrayEnumerator(nsISimpleEnumerator* *aResult NS_OUTPARAM,
                      const nsCOMArray_base& aArray);

#endif
