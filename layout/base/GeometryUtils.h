




#ifndef MOZILLA_GEOMETRYUTILS_H_
#define MOZILLA_GEOMETRYUTILS_H_

#include "mozilla/ErrorResult.h"
#include "nsTArray.h"
#include "nsCOMPtr.h"






class nsINode;
class nsIDocument;

namespace mozilla {

namespace dom {
struct BoxQuadOptions;
class DOMQuad;
class Element;
class Text;
}





void GetBoxQuads(nsINode* aNode,
                 const dom::BoxQuadOptions& aOptions,
                 nsTArray<nsRefPtr<dom::DOMQuad> >& aResult,
                 ErrorResult& aRv);

}

#endif 
