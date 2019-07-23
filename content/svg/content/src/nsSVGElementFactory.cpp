






































#include "nsCOMPtr.h"
#include "nsContentCreatorFunctions.h"
#include "nsIAtom.h"
#include "nsINodeInfo.h"
#include "nsGkAtoms.h"
#include "nsContentDLF.h"
#include "nsContentUtils.h"
#include "nsSVGUtils.h"
#include "nsDebug.h"

nsresult
NS_NewSVGAElement(nsIContent **aResult, nsINodeInfo *aNodeInfo);
nsresult
NS_NewSVGAltGlyphElement(nsIContent **aResult, nsINodeInfo *aNodeInfo);
nsresult
NS_NewSVGPolylineElement(nsIContent **aResult, nsINodeInfo *aNodeInfo);
nsresult
NS_NewSVGPolygonElement(nsIContent **aResult, nsINodeInfo *aNodeInfo);
nsresult
NS_NewSVGCircleElement(nsIContent **aResult, nsINodeInfo *aNodeInfo);
nsresult
NS_NewSVGEllipseElement(nsIContent **aResult, nsINodeInfo *aNodeInfo);
nsresult
NS_NewSVGLineElement(nsIContent **aResult, nsINodeInfo *aNodeInfo);
nsresult
NS_NewSVGRectElement(nsIContent **aResult, nsINodeInfo *aNodeInfo);
nsresult
NS_NewSVGGElement(nsIContent **aResult, nsINodeInfo *aNodeInfo);
nsresult
NS_NewSVGSVGElement(nsIContent **aResult, nsINodeInfo *aNodeInfo,
                    PRBool aFromParser);
nsresult
NS_NewSVGForeignObjectElement(nsIContent **aResult, nsINodeInfo *aNodeInfo);
nsresult
NS_NewSVGPathElement(nsIContent **aResult, nsINodeInfo *aNodeInfo);
nsresult
NS_NewSVGTextElement(nsIContent **aResult, nsINodeInfo *aNodeInfo);
nsresult
NS_NewSVGTSpanElement(nsIContent **aResult, nsINodeInfo *aNodeInfo);
nsresult
NS_NewSVGImageElement(nsIContent **aResult, nsINodeInfo *aNodeInfo);
nsresult
NS_NewSVGStyleElement(nsIContent **aResult, nsINodeInfo *aNodeInfo);
nsresult
NS_NewSVGLinearGradientElement(nsIContent **aResult, nsINodeInfo *aNodeInfo);
nsresult
NS_NewSVGMetadataElement(nsIContent **aResult, nsINodeInfo *aNodeInfo);
nsresult
NS_NewSVGRadialGradientElement(nsIContent **aResult, nsINodeInfo *aNodeInfo);
nsresult
NS_NewSVGStopElement(nsIContent **aResult, nsINodeInfo *aNodeInfo);
nsresult
NS_NewSVGDefsElement(nsIContent **aResult, nsINodeInfo *aNodeInfo);
nsresult
NS_NewSVGDescElement(nsIContent **aResult, nsINodeInfo *aNodeInfo);
nsresult
NS_NewSVGScriptElement(nsIContent **aResult, nsINodeInfo *aNodeInfo,
                       PRBool aFromParser);
nsresult
NS_NewSVGUseElement(nsIContent **aResult, nsINodeInfo *aNodeInfo);
nsresult
NS_NewSVGSymbolElement(nsIContent **aResult, nsINodeInfo *aNodeInfo);
nsresult
NS_NewSVGMarkerElement(nsIContent **aResult, nsINodeInfo *aNodeInfo);
nsresult
NS_NewSVGTitleElement(nsIContent **aResult, nsINodeInfo *aNodeInfo);
nsresult
NS_NewSVGClipPathElement(nsIContent **aResult, nsINodeInfo *aNodeInfo);
nsresult
NS_NewSVGTextPathElement(nsIContent **aResult, nsINodeInfo *aNodeInfo);
nsresult
NS_NewSVGFilterElement(nsIContent **aResult, nsINodeInfo *aNodeInfo);
nsresult
NS_NewSVGFEBlendElement(nsIContent **aResult, nsINodeInfo *aNodeInfo);
nsresult
NS_NewSVGFEColorMatrixElement(nsIContent **aResult, nsINodeInfo *aNodeInfo);
nsresult
NS_NewSVGFEComponentTransferElement(nsIContent **aResult, nsINodeInfo *aNodeInfo);
nsresult
NS_NewSVGFECompositeElement(nsIContent **aResult, nsINodeInfo *aNodeInfo);
nsresult
NS_NewSVGFEFuncRElement(nsIContent **aResult, nsINodeInfo *aNodeInfo);
nsresult
NS_NewSVGFEFuncGElement(nsIContent **aResult, nsINodeInfo *aNodeInfo);
nsresult
NS_NewSVGFEFuncBElement(nsIContent **aResult, nsINodeInfo *aNodeInfo);
nsresult
NS_NewSVGFEFuncAElement(nsIContent **aResult, nsINodeInfo *aNodeInfo);
nsresult
NS_NewSVGFEGaussianBlurElement(nsIContent **aResult, nsINodeInfo *aNodeInfo);
nsresult
NS_NewSVGFEMergeElement(nsIContent **aResult, nsINodeInfo *aNodeInfo);
nsresult
NS_NewSVGFEMergeNodeElement(nsIContent **aResult, nsINodeInfo *aNodeInfo);
nsresult
NS_NewSVGFEMorphologyElement(nsIContent **aResult, nsINodeInfo *aNodeInfo);
nsresult
NS_NewSVGFEOffsetElement(nsIContent **aResult, nsINodeInfo *aNodeInfo);
nsresult
NS_NewSVGPatternElement(nsIContent **aResult, nsINodeInfo *aNodeInfo);
nsresult
NS_NewSVGMaskElement(nsIContent **aResult, nsINodeInfo *aNodeInfo);
nsresult
NS_NewSVGFEFloodElement(nsIContent **aResult, nsINodeInfo *aNodeInfo);
nsresult
NS_NewSVGFETileElement(nsIContent **aResult, nsINodeInfo *aNodeInfo);
nsresult
NS_NewSVGFETurbulenceElement(nsIContent **aResult, nsINodeInfo *aNodeInfo);
nsresult
NS_NewSVGSwitchElement(nsIContent **aResult, nsINodeInfo *aNodeInfo);
nsresult
NS_NewSVGFEConvolveMatrixElement(nsIContent **aResult, nsINodeInfo *aNodeInfo);
nsresult
NS_NewSVGFEDistantLightElement(nsIContent **aResult, nsINodeInfo *aNodeInfo);
nsresult
NS_NewSVGFEPointLightElement(nsIContent **aResult, nsINodeInfo *aNodeInfo);
nsresult
NS_NewSVGFESpotLightElement(nsIContent **aResult, nsINodeInfo *aNodeInfo);
nsresult
NS_NewSVGFEDiffuseLightingElement(nsIContent **aResult, nsINodeInfo *aNodeInfo);
nsresult
NS_NewSVGFESpecularLightingElement(nsIContent **aResult, nsINodeInfo *aNodeInfo);
nsresult
NS_NewSVGFEImageElement(nsIContent **aResult, nsINodeInfo *aNodeInfo);
nsresult
NS_NewSVGFEDisplacementMapElement(nsIContent **aResult, nsINodeInfo *aNodeInfo);

#ifdef MOZ_SMIL
nsresult
NS_NewSVGAnimateElement(nsIContent **aResult, nsINodeInfo *aNodeInfo);
nsresult
NS_NewSVGAnimateTransformElement(nsIContent **aResult, nsINodeInfo *aNodeInfo);
nsresult
NS_NewSVGSetElement(nsIContent **aResult, nsINodeInfo *aNodeInfo);
#endif 

nsresult
NS_NewSVGElement(nsIContent** aResult, nsINodeInfo *aNodeInfo,
                 PRBool aFromParser)
{
  NS_PRECONDITION(NS_SVGEnabled(),
                  "creating an SVG element while SVG disabled");

  static const char kSVGStyleSheetURI[] = "resource://gre/res/svg.css";

  
  nsIDocument *doc = aNodeInfo->GetDocument();
  if (doc)
    doc->EnsureCatalogStyleSheet(kSVGStyleSheetURI);

  nsIAtom *name = aNodeInfo->NameAtom();
  
  if (name == nsGkAtoms::a)
    return NS_NewSVGAElement(aResult, aNodeInfo);
  if (name == nsGkAtoms::altGlyph)
    return NS_NewSVGAltGlyphElement(aResult, aNodeInfo);
  if (name == nsGkAtoms::polyline)
    return NS_NewSVGPolylineElement(aResult, aNodeInfo);
  if (name == nsGkAtoms::polygon)
    return NS_NewSVGPolygonElement(aResult, aNodeInfo);
  if (name == nsGkAtoms::circle)
    return NS_NewSVGCircleElement(aResult, aNodeInfo);
  if (name == nsGkAtoms::ellipse)
    return NS_NewSVGEllipseElement(aResult, aNodeInfo);
  if (name == nsGkAtoms::line)
    return NS_NewSVGLineElement(aResult, aNodeInfo);
  if (name == nsGkAtoms::rect)
    return NS_NewSVGRectElement(aResult, aNodeInfo);
  if (name == nsGkAtoms::svg)
    return NS_NewSVGSVGElement(aResult, aNodeInfo, aFromParser);
  if (name == nsGkAtoms::g)
    return NS_NewSVGGElement(aResult, aNodeInfo);
  if (name == nsGkAtoms::foreignObject)
    return NS_NewSVGForeignObjectElement(aResult, aNodeInfo);
  if (name == nsGkAtoms::path)
    return NS_NewSVGPathElement(aResult, aNodeInfo);
  if (name == nsGkAtoms::text)
    return NS_NewSVGTextElement(aResult, aNodeInfo);
  if (name == nsGkAtoms::tspan)
    return NS_NewSVGTSpanElement(aResult, aNodeInfo);
  if (name == nsGkAtoms::image)
    return NS_NewSVGImageElement(aResult, aNodeInfo);
  if (name == nsGkAtoms::style)
    return NS_NewSVGStyleElement(aResult, aNodeInfo);
  if (name == nsGkAtoms::linearGradient)
    return NS_NewSVGLinearGradientElement(aResult, aNodeInfo);
  if (name == nsGkAtoms::metadata)
    return NS_NewSVGMetadataElement(aResult, aNodeInfo);
  if (name == nsGkAtoms::radialGradient)
    return NS_NewSVGRadialGradientElement(aResult, aNodeInfo);
  if (name == nsGkAtoms::stop)
    return NS_NewSVGStopElement(aResult, aNodeInfo);
  if (name == nsGkAtoms::defs)
    return NS_NewSVGDefsElement(aResult, aNodeInfo);
  if (name == nsGkAtoms::desc)
    return NS_NewSVGDescElement(aResult, aNodeInfo);
  if (name == nsGkAtoms::script)
    return NS_NewSVGScriptElement(aResult, aNodeInfo, aFromParser);
  if (name == nsGkAtoms::use)
    return NS_NewSVGUseElement(aResult, aNodeInfo);
  if (name == nsGkAtoms::symbol)
    return NS_NewSVGSymbolElement(aResult, aNodeInfo);
  if (name == nsGkAtoms::marker)
    return NS_NewSVGMarkerElement(aResult, aNodeInfo);
  if (name == nsGkAtoms::title)
    return NS_NewSVGTitleElement(aResult, aNodeInfo);
  if (name == nsGkAtoms::clipPath)
    return NS_NewSVGClipPathElement(aResult, aNodeInfo);
  if (name == nsGkAtoms::textPath)
    return NS_NewSVGTextPathElement(aResult, aNodeInfo);
  if (name == nsGkAtoms::filter)
    return NS_NewSVGFilterElement(aResult, aNodeInfo);
  if (name == nsGkAtoms::feBlend)
    return NS_NewSVGFEBlendElement(aResult, aNodeInfo);
  if (name == nsGkAtoms::feColorMatrix)
    return NS_NewSVGFEColorMatrixElement(aResult, aNodeInfo);
  if (name == nsGkAtoms::feComponentTransfer)
    return NS_NewSVGFEComponentTransferElement(aResult, aNodeInfo);
  if (name == nsGkAtoms::feComposite)
    return NS_NewSVGFECompositeElement(aResult, aNodeInfo);
  if (name == nsGkAtoms::feFuncR)
    return NS_NewSVGFEFuncRElement(aResult, aNodeInfo);
  if (name == nsGkAtoms::feFuncG)
    return NS_NewSVGFEFuncGElement(aResult, aNodeInfo);
  if (name == nsGkAtoms::feFuncB)
    return NS_NewSVGFEFuncBElement(aResult, aNodeInfo);
  if (name == nsGkAtoms::feFuncA)
    return NS_NewSVGFEFuncAElement(aResult, aNodeInfo);
  if (name == nsGkAtoms::feGaussianBlur)
    return NS_NewSVGFEGaussianBlurElement(aResult, aNodeInfo);
  if (name == nsGkAtoms::feMerge)
    return NS_NewSVGFEMergeElement(aResult, aNodeInfo);
  if (name == nsGkAtoms::feMergeNode)
    return NS_NewSVGFEMergeNodeElement(aResult, aNodeInfo);
  if (name == nsGkAtoms::feMorphology)
    return NS_NewSVGFEMorphologyElement(aResult, aNodeInfo);
  if (name == nsGkAtoms::feOffset)
    return NS_NewSVGFEOffsetElement(aResult, aNodeInfo);
  if (name == nsGkAtoms::feFlood)
    return NS_NewSVGFEFloodElement(aResult, aNodeInfo);
  if (name == nsGkAtoms::feTile)
    return NS_NewSVGFETileElement(aResult, aNodeInfo);
  if (name == nsGkAtoms::feTurbulence)
    return NS_NewSVGFETurbulenceElement(aResult, aNodeInfo);
  if (name == nsGkAtoms::feConvolveMatrix)
    return NS_NewSVGFEConvolveMatrixElement(aResult, aNodeInfo);
  if (name == nsGkAtoms::feDistantLight)
    return NS_NewSVGFEDistantLightElement(aResult, aNodeInfo);
  if (name == nsGkAtoms::fePointLight)
    return NS_NewSVGFEPointLightElement(aResult, aNodeInfo);
  if (name == nsGkAtoms::feSpotLight)
    return NS_NewSVGFESpotLightElement(aResult, aNodeInfo);
  if (name == nsGkAtoms::feDiffuseLighting)
    return NS_NewSVGFEDiffuseLightingElement(aResult, aNodeInfo);
  if (name == nsGkAtoms::feSpecularLighting)
    return NS_NewSVGFESpecularLightingElement(aResult, aNodeInfo);
  if (name == nsGkAtoms::feImage)
    return NS_NewSVGFEImageElement(aResult, aNodeInfo);
  if (name == nsGkAtoms::feDisplacementMap)
    return NS_NewSVGFEDisplacementMapElement(aResult, aNodeInfo);
  if (name == nsGkAtoms::pattern)
    return NS_NewSVGPatternElement(aResult, aNodeInfo);
  if (name == nsGkAtoms::mask)
    return NS_NewSVGMaskElement(aResult, aNodeInfo);
  if (name == nsGkAtoms::svgSwitch)
    return NS_NewSVGSwitchElement(aResult, aNodeInfo);
#ifdef MOZ_SMIL
  if (NS_SMILEnabled()) {
    if (name == nsGkAtoms::animate)
      return NS_NewSVGAnimateElement(aResult, aNodeInfo);
    if (name == nsGkAtoms::animateTransform)
      return NS_NewSVGAnimateTransformElement(aResult, aNodeInfo);
    if (name == nsGkAtoms::set)
      return NS_NewSVGSetElement(aResult, aNodeInfo);
  }
#endif 

  
  return NS_NewXMLElement(aResult, aNodeInfo);
}

