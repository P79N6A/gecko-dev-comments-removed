




#include "nsCOMPtr.h"
#include "nsIException.h"

nsresult
NS_GetNameAndMessageForDOMNSResult(nsresult aNSResult, const char** aName,
                                   const char** aMessage,
                                   PRUint16* aCode = nsnull);

nsresult
NS_NewDOMException(nsresult aNSResult, nsIException* aDefaultException,
                   nsIException** aException);
