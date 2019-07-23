

































#include "nsSVGStylableElement.h"
#include "nsGkAtoms.h"
#include "nsIDOMSVGAltGlyphElement.h"
#include "nsIDOMSVGURIReference.h"
#include "nsSVGString.h"
#include "nsSVGTextPositioningElement.h"

typedef nsSVGStylableElement nsSVGAltGlyphElementBase;

class nsSVGAltGlyphElement : public nsSVGAltGlyphElementBase,
                             public nsIDOMSVGAltGlyphElement,
                             public nsIDOMSVGURIReference, 
                             public nsSVGTextPositioningElement 
{
protected:
  friend nsresult NS_NewSVGAltGlyphElement(nsIContent **aResult,
                                           nsINodeInfo *aNodeInfo);
  nsSVGAltGlyphElement(nsINodeInfo* aNodeInfo);
  nsresult Init();
  
public:
  
  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGALTGLYPHELEMENT
  NS_DECL_NSIDOMSVGURIREFERENCE

  
  
  NS_FORWARD_NSIDOMNODE(nsSVGAltGlyphElementBase::)
  NS_FORWARD_NSIDOMELEMENT(nsSVGAltGlyphElementBase::)
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGAltGlyphElementBase::)
  NS_FORWARD_NSIDOMSVGTEXTCONTENTELEMENT(nsSVGTextContentElement::)
  NS_FORWARD_NSIDOMSVGTEXTPOSITIONINGELEMENT(nsSVGTextPositioningElement::)

  
  NS_IMETHOD_(PRBool) IsAttributeMapped(const nsIAtom* aAttribute) const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

protected:
  virtual nsSVGTextContainerFrame* GetTextContainerFrame() {
    return do_QueryFrame(GetPrimaryFrame(Flush_Layout));
  }

  
  virtual StringAttributesInfo GetStringInfo();

  virtual PRBool IsEventName(nsIAtom* aName);

  enum { HREF };
  nsSVGString mStringAttributes[1];
  static StringInfo sStringInfo[1];

};

nsSVGElement::StringInfo nsSVGAltGlyphElement::sStringInfo[1] =
{
  { &nsGkAtoms::href, kNameSpaceID_XLink }
};

NS_IMPL_NS_NEW_SVG_ELEMENT(AltGlyph)




NS_IMPL_ADDREF_INHERITED(nsSVGAltGlyphElement,nsSVGAltGlyphElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGAltGlyphElement,nsSVGAltGlyphElementBase)

DOMCI_DATA(SVGAltGlyphElement, nsSVGAltGlyphElement)

NS_INTERFACE_TABLE_HEAD(nsSVGAltGlyphElement)
  NS_NODE_INTERFACE_TABLE7(nsSVGAltGlyphElement, nsIDOMNode, nsIDOMElement,
                           nsIDOMSVGElement, nsIDOMSVGAltGlyphElement,
                           nsIDOMSVGTextPositioningElement, nsIDOMSVGTextContentElement,
                           nsIDOMSVGURIReference)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGAltGlyphElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGAltGlyphElementBase)




nsSVGAltGlyphElement::nsSVGAltGlyphElement(nsINodeInfo *aNodeInfo)
  : nsSVGAltGlyphElementBase(aNodeInfo)
{
}

nsresult
nsSVGAltGlyphElement::Init()
{
  nsresult rv = nsSVGAltGlyphElementBase::Init();
  NS_ENSURE_SUCCESS(rv,rv);

  rv = Initialise(this);
  NS_ENSURE_SUCCESS(rv,rv);

  return rv;
}




NS_IMPL_ELEMENT_CLONE_WITH_INIT(nsSVGAltGlyphElement)





NS_IMETHODIMP nsSVGAltGlyphElement::GetHref(nsIDOMSVGAnimatedString * *aHref)
{
  return mStringAttributes[HREF].ToDOMAnimatedString(aHref, this);
}





NS_IMETHODIMP nsSVGAltGlyphElement::GetGlyphRef(nsAString & aGlyphRef)
{
  GetAttr(kNameSpaceID_None, nsGkAtoms::glyphRef, aGlyphRef);

  return NS_OK;
}

NS_IMETHODIMP nsSVGAltGlyphElement::SetGlyphRef(const nsAString & aGlyphRef)
{
  return SetAttr(kNameSpaceID_None, nsGkAtoms::glyphRef, aGlyphRef, PR_TRUE);
}


NS_IMETHODIMP nsSVGAltGlyphElement::GetFormat(nsAString & aFormat)
{
  GetAttr(kNameSpaceID_None, nsGkAtoms::format, aFormat);

  return NS_OK;
}

NS_IMETHODIMP nsSVGAltGlyphElement::SetFormat(const nsAString & aFormat)
{
  return SetAttr(kNameSpaceID_None, nsGkAtoms::format, aFormat, PR_TRUE);
}




NS_IMETHODIMP_(PRBool)
nsSVGAltGlyphElement::IsAttributeMapped(const nsIAtom* name) const
{
  static const MappedAttributeEntry* const map[] = {
    sColorMap,
    sFillStrokeMap,
    sFontSpecificationMap,
    sGraphicsMap,
    sTextContentElementsMap
  };
  
  return FindAttributeDependence(name, map, NS_ARRAY_LENGTH(map)) ||
    nsSVGAltGlyphElementBase::IsAttributeMapped(name);
}




PRBool
nsSVGAltGlyphElement::IsEventName(nsIAtom* aName)
{
  return nsContentUtils::IsEventAttributeName(aName, EventNameType_SVGGraphic);
}

nsSVGElement::StringAttributesInfo
nsSVGAltGlyphElement::GetStringInfo()
{
  return StringAttributesInfo(mStringAttributes, sStringInfo,
                              NS_ARRAY_LENGTH(sStringInfo));
}
