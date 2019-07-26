




#ifndef mozilla_dom_SVGAltGlyphElement_h
#define mozilla_dom_SVGAltGlyphElement_h

#include "mozilla/dom/SVGTextPositioningElement.h"
#include "nsSVGString.h"

nsresult NS_NewSVGAltGlyphElement(nsIContent **aResult,
                                  already_AddRefed<nsINodeInfo> aNodeInfo);

namespace mozilla {
namespace dom {

typedef SVGTextPositioningElement SVGAltGlyphElementBase;

class SVGAltGlyphElement MOZ_FINAL : public SVGAltGlyphElementBase
{
protected:
  friend nsresult (::NS_NewSVGAltGlyphElement(nsIContent **aResult,
                                              already_AddRefed<nsINodeInfo> aNodeInfo));
  SVGAltGlyphElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual JSObject* WrapNode(JSContext *cx,
                             JS::Handle<JSObject*> scope) MOZ_OVERRIDE;

public:
  
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const MOZ_OVERRIDE;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const MOZ_OVERRIDE;

  
  already_AddRefed<nsIDOMSVGAnimatedString> Href();
  void GetGlyphRef(nsAString & aGlyphRef);
  void SetGlyphRef(const nsAString & aGlyphRef, ErrorResult& rv);
  void GetFormat(nsAString & aFormat);
  void SetFormat(const nsAString & aFormat, ErrorResult& rv);

protected:

  
  virtual StringAttributesInfo GetStringInfo() MOZ_OVERRIDE;

  virtual bool IsEventName(nsIAtom* aName);

  enum { HREF };
  nsSVGString mStringAttributes[1];
  static StringInfo sStringInfo[1];

};

} 
} 

#endif 
