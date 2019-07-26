




#ifndef mozilla_dom_SVGTextContentElement_h
#define mozilla_dom_SVGTextContentElement_h

#include "mozilla/dom/SVGGraphicsElement.h"
#include "mozilla/dom/SVGAnimatedEnumeration.h"
#include "nsSVGEnum.h"
#include "nsSVGLength2.h"

static const unsigned short SVG_LENGTHADJUST_UNKNOWN          = 0;
static const unsigned short SVG_LENGTHADJUST_SPACING          = 1;
static const unsigned short SVG_LENGTHADJUST_SPACINGANDGLYPHS = 2;

class nsSVGTextContainerFrame;
class nsSVGTextFrame2;

namespace mozilla {
class nsISVGPoint;

namespace dom {

class SVGIRect;

typedef SVGGraphicsElement SVGTextContentElementBase;

class SVGTextContentElement : public SVGTextContentElementBase
{
public:
  using FragmentOrElement::TextLength;

  
  already_AddRefed<SVGAnimatedLength> TextLength();
  already_AddRefed<SVGAnimatedEnumeration> LengthAdjust();
  int32_t GetNumberOfChars();
  float GetComputedTextLength();
  void SelectSubString(uint32_t charnum, uint32_t nchars, ErrorResult& rv);
  float GetSubStringLength(uint32_t charnum, uint32_t nchars, ErrorResult& rv);
  already_AddRefed<nsISVGPoint> GetStartPositionOfChar(uint32_t charnum, ErrorResult& rv);
  already_AddRefed<nsISVGPoint> GetEndPositionOfChar(uint32_t charnum, ErrorResult& rv);
  already_AddRefed<SVGIRect> GetExtentOfChar(uint32_t charnum, ErrorResult& rv);
  float GetRotationOfChar(uint32_t charnum, ErrorResult& rv);
  int32_t GetCharNumAtPosition(nsISVGPoint& point);

protected:

  SVGTextContentElement(already_AddRefed<nsINodeInfo> aNodeInfo)
    : SVGTextContentElementBase(aNodeInfo)
  {}

  nsSVGTextContainerFrame* GetTextContainerFrame();
  nsSVGTextFrame2* GetSVGTextFrame();
  bool FrameIsSVGText();

  virtual EnumAttributesInfo GetEnumInfo() MOZ_OVERRIDE;
  virtual LengthAttributesInfo GetLengthInfo() MOZ_OVERRIDE;

  enum { LENGTHADJUST };
  nsSVGEnum mEnumAttributes[1];
  static nsSVGEnumMapping sLengthAdjustMap[];
  static EnumInfo sEnumInfo[1];

  enum { TEXTLENGTH };
  nsSVGLength2 mLengthAttributes[1];

  static LengthInfo sLengthInfo[1];
};

} 
} 

#endif 
