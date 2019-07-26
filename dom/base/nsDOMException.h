




#include "mozilla/NullPtr.h"
#include "nsError.h"
class nsIException;

nsresult
NS_GetNameAndMessageForDOMNSResult(nsresult aNSResult, const char** aName,
                                   const char** aMessage,
                                   uint16_t* aCode = nullptr);

nsresult
NS_NewDOMException(nsresult aNSResult, nsIException* aDefaultException,
                   nsIException** aException);
