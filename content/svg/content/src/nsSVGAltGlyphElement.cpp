

































#include "nsGkAtoms.h"
#include "nsIDOMSVGAltGlyphElement.h"
#include "nsIDOMSVGURIReference.h"
#include "nsSVGString.h"
#include "nsSVGTextPositioningElement.h"
#include "nsContentUtils.h"

typedef nsSVGTextPositioningElement nsSVGAltGlyphElementBase;

class nsSVGAltGlyphElement : public nsSVGAltGlyphElementBase, 
                             public nsIDOMSVGAltGlyphElement,
                             public nsIDOMSVGURIReference
{
protected:
  friend nsresult NS_NewSVGAltGlyphElement(nsIContent **aResult,
                                           already_AddRefed<nsINodeInfo> aNodeInfo);
  nsSVGAltGlyphElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  
public:
  
  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGALTGLYPHELEMENT
  NS_DECL_NSIDOMSVGURIREFERENCE

  
  
  NS_FORWARD_NSIDOMNODE(nsSVGAltGlyphElementBase::)
  NS_FORWARD_NSIDOMELEMENT(nsSVGAltGlyphElementBase::)
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGAltGlyphElementBase::)
  NS_FORWARD_NSIDOMSVGTEXTCONTENTELEMENT(nsSVGAltGlyphElementBase::)
  NS_FORWARD_NSIDOMSVGTEXTPOSITIONINGELEMENT(nsSVGAltGlyphElementBase::)

  
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsXPCClassInfo* GetClassInfo();
protected:

  
  virtual StringAttributesInfo GetStringInfo();

  virtual bool IsEventName(nsIAtom* aName);

  enum { HREF };
  nsSVGString mStringAttributes[1];
  static StringInfo sStringInfo[1];

};

nsSVGElement::StringInfo nsSVGAltGlyphElement::sStringInfo[1] =
{
  { &nsGkAtoms::href, kNameSpaceID_XLink, PR_FALSE }
};

NS_IMPL_NS_NEW_SVG_ELEMENT(AltGlyph)




NS_IMPL_ADDREF_INHERITED(nsSVGAltGlyphElement,nsSVGAltGlyphElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGAltGlyphElement,nsSVGAltGlyphElementBase)

DOMCI_NODE_DATA(SVGAltGlyphElement, nsSVGAltGlyphElement)

NS_INTERFACE_TABLE_HEAD(nsSVGAltGlyphElement)
  NS_NODE_INTERFACE_TABLE7(nsSVGAltGlyphElement, nsIDOMNode, nsIDOMElement,
                           nsIDOMSVGElement, nsIDOMSVGAltGlyphElement,
                           nsIDOMSVGTextPositioningElement, nsIDOMSVGTextContentElement,
                           nsIDOMSVGURIReference)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGAltGlyphElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGAltGlyphElementBase)




nsSVGAltGlyphElement::nsSVGAltGlyphElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsSVGAltGlyphElementBase(aNodeInfo)
{
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




NS_IMETHODIMP_(bool)
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




bool
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
