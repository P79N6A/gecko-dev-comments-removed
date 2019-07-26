




#ifndef mozilla_dom_SVGTextContentElement_h
#define mozilla_dom_SVGTextContentElement_h

#include "nsIDOMSVGTextContentElement.h"
#include "mozilla/dom/SVGGraphicsElement.h"

class nsSVGTextContainerFrame;

namespace mozilla {
class nsISVGPoint;

namespace dom {

typedef SVGGraphicsElement SVGTextContentElementBase;

class SVGTextContentElement : public SVGTextContentElementBase
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGTEXTCONTENTELEMENT

  using FragmentOrElement::TextLength;

  
  int32_t GetNumberOfChars();
  float GetComputedTextLength();
  float GetSubStringLength(uint32_t charnum, uint32_t nchars, ErrorResult& rv);
  already_AddRefed<nsISVGPoint> GetStartPositionOfChar(uint32_t charnum, ErrorResult& rv);
  already_AddRefed<nsISVGPoint> GetEndPositionOfChar(uint32_t charnum, ErrorResult& rv);
  already_AddRefed<nsIDOMSVGRect> GetExtentOfChar(uint32_t charnum, ErrorResult& rv);
  float GetRotationOfChar(uint32_t charnum, ErrorResult& rv);
  int32_t GetCharNumAtPosition(nsISVGPoint& point);

protected:

  SVGTextContentElement(already_AddRefed<nsINodeInfo> aNodeInfo)
    : SVGTextContentElementBase(aNodeInfo)
  {}

  nsSVGTextContainerFrame* GetTextContainerFrame();
};

} 
} 

#endif 
