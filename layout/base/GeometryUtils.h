




#ifndef MOZILLA_GEOMETRYUTILS_H_
#define MOZILLA_GEOMETRYUTILS_H_

#include "mozilla/ErrorResult.h"
#include "nsTArray.h"
#include "nsCOMPtr.h"






class nsINode;

namespace mozilla {

namespace dom {
struct BoxQuadOptions;
struct ConvertCoordinateOptions;
class DOMQuad;
class DOMRectReadOnly;
class DOMPoint;
struct DOMPointInit;
class OwningTextOrElementOrDocument;
class TextOrElementOrDocument;
}

typedef dom::TextOrElementOrDocument GeometryNode;
typedef dom::OwningTextOrElementOrDocument OwningGeometryNode;





void GetBoxQuads(nsINode* aNode,
                 const dom::BoxQuadOptions& aOptions,
                 nsTArray<nsRefPtr<dom::DOMQuad> >& aResult,
                 ErrorResult& aRv);

already_AddRefed<dom::DOMQuad>
ConvertQuadFromNode(nsINode* aTo, dom::DOMQuad& aQuad,
                    const GeometryNode& aFrom,
                    const dom::ConvertCoordinateOptions& aOptions,
                    ErrorResult& aRv);

already_AddRefed<dom::DOMQuad>
ConvertRectFromNode(nsINode* aTo, dom::DOMRectReadOnly& aRect,
                    const GeometryNode& aFrom,
                    const dom::ConvertCoordinateOptions& aOptions,
                    ErrorResult& aRv);

already_AddRefed<dom::DOMPoint>
ConvertPointFromNode(nsINode* aTo, const dom::DOMPointInit& aPoint,
                     const GeometryNode& aFrom,
                     const dom::ConvertCoordinateOptions& aOptions,
                     ErrorResult& aRv);

}

#endif 
