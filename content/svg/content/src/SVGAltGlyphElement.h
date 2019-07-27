




#ifndef mozilla_dom_SVGAltGlyphElement_h
#define mozilla_dom_SVGAltGlyphElement_h

#include "mozilla/dom/SVGTextPositioningElement.h"
#include "nsSVGString.h"

nsresult NS_NewSVGAltGlyphElement(nsIContent **aResult,
                                  already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo);

namespace mozilla {
namespace dom {

typedef SVGTextPositioningElement SVGAltGlyphElementBase;

class SVGAltGlyphElement MOZ_FINAL : public SVGAltGlyphElementBase
{
protected:
  friend nsresult (::NS_NewSVGAltGlyphElement(nsIContent **aResult,
                                              already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo));
  explicit SVGAltGlyphElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo);
  virtual JSObject* WrapNode(JSContext *cx) MOZ_OVERRIDE;

public:
  
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const MOZ_OVERRIDE;

  virtual nsresult Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const MOZ_OVERRIDE;

  
  already_AddRefed<SVGAnimatedString> Href();
  void GetGlyphRef(nsAString & aGlyphRef);
  void SetGlyphRef(const nsAString & aGlyphRef, ErrorResult& rv);
  void GetFormat(nsAString & aFormat);
  void SetFormat(const nsAString & aFormat, ErrorResult& rv);

protected:

  
  virtual EnumAttributesInfo GetEnumInfo() MOZ_OVERRIDE;
  virtual LengthAttributesInfo GetLengthInfo() MOZ_OVERRIDE;
  virtual StringAttributesInfo GetStringInfo() MOZ_OVERRIDE;

  enum { HREF };
  nsSVGString mStringAttributes[1];
  static StringInfo sStringInfo[1];

  nsSVGEnum mEnumAttributes[1];
  virtual nsSVGEnum* EnumAttributes() MOZ_OVERRIDE
    { return mEnumAttributes; }

  nsSVGLength2 mLengthAttributes[1];
  virtual nsSVGLength2* LengthAttributes() MOZ_OVERRIDE
    { return mLengthAttributes; }
};

} 
} 

#endif 
