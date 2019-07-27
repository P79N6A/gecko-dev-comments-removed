




#ifndef mozilla_dom_Fetch_h
#define mozilla_dom_Fetch_h

#include "mozilla/dom/UnionTypes.h"

class nsIInputStream;

namespace mozilla {
namespace dom {






nsresult
ExtractByteStreamFromBody(const OwningArrayBufferOrArrayBufferViewOrScalarValueStringOrURLSearchParams& aBodyInit,
                          nsIInputStream** aStream,
                          nsCString& aContentType);

} 
} 

#endif 
