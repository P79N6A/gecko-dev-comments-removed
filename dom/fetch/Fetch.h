




#ifndef mozilla_dom_Fetch_h
#define mozilla_dom_Fetch_h

#include "nsError.h"

class nsCString;
class nsIInputStream;

namespace mozilla {
namespace dom {

class OwningArrayBufferOrArrayBufferViewOrScalarValueStringOrURLSearchParams;






nsresult
ExtractByteStreamFromBody(const OwningArrayBufferOrArrayBufferViewOrScalarValueStringOrURLSearchParams& aBodyInit,
                          nsIInputStream** aStream,
                          nsCString& aContentType);

} 
} 

#endif 
