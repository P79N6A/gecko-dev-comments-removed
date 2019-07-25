





#include "nsSVGTextPathFrame.h"


#include "nsContentUtils.h"
#include "nsIDOMSVGTextPathElement.h"
#include "nsSVGEffects.h"
#include "nsSVGLength2.h"
#include "nsSVGPathElement.h"
#include "nsSVGTextPathElement.h"
#include "SVGLengthList.h"

using namespace mozilla;




nsIFrame*
NS_NewSVGTextPathFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsSVGTextPathFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsSVGTextPathFrame)

#ifdef DEBUG
NS_IMETHODIMP
nsSVGTextPathFrame::Init(nsIContent* aContent,
                         nsIFrame* aParent,
                         nsIFrame* aPrevInFlow)
{
  NS_ASSERTION(aParent, "null parent");

  nsIFrame* ancestorFrame = nsSVGUtils::GetFirstNonAAncestorFrame(aParent);
  NS_ASSERTION(ancestorFrame, "Must have ancestor");

  NS_ASSERTION(ancestorFrame->GetType() == nsGkAtoms::svgTextFrame,
               "trying to construct an SVGTextPathFrame for an invalid "
               "container");
  
  nsCOMPtr<nsIDOMSVGTextPathElement> textPath = do_QueryInterface(aContent);
  NS_ASSERTION(textPath, "Content is not an SVG textPath");

  return nsSVGTextPathFrameBase::Init(aContent, aParent, aPrevInFlow);
}
#endif 

nsIAtom *
nsSVGTextPathFrame::GetType() const
{
  return nsGkAtoms::svgTextPathFrame;
}

void
nsSVGTextPathFrame::GetXY(SVGUserUnitList *aX, SVGUserUnitList *aY)
{
  
  aX->Clear();
  aY->Clear();
}

void
nsSVGTextPathFrame::GetDxDy(SVGUserUnitList *aDx, SVGUserUnitList *aDy)
{
  
  aDx->Clear();
  aDy->Clear();
}

const SVGNumberList*
nsSVGTextPathFrame::GetRotate()
{
  return nullptr;
}




nsIFrame *
nsSVGTextPathFrame::GetPathFrame()
{
  nsSVGTextPathProperty *property = static_cast<nsSVGTextPathProperty*>
    (Properties().Get(nsSVGEffects::HrefProperty()));

  if (!property) {
    nsSVGTextPathElement *tp = static_cast<nsSVGTextPathElement*>(mContent);
    nsAutoString href;
    tp->mStringAttributes[nsSVGTextPathElement::HREF].GetAnimValue(href, tp);
    if (href.IsEmpty()) {
      return nullptr; 
    }

    nsCOMPtr<nsIURI> targetURI;
    nsCOMPtr<nsIURI> base = mContent->GetBaseURI();
    nsContentUtils::NewURIWithDocumentCharset(getter_AddRefs(targetURI), href,
                                              mContent->GetCurrentDoc(), base);

    property =
      nsSVGEffects::GetTextPathProperty(targetURI, this, nsSVGEffects::HrefProperty());
    if (!property)
      return nullptr;
  }

  return property->GetReferencedFrame(nsGkAtoms::svgPathGeometryFrame, nullptr);
}

already_AddRefed<gfxFlattenedPath>
nsSVGTextPathFrame::GetFlattenedPath()
{
  nsIFrame *path = GetPathFrame();

  if (path) {
    nsSVGPathGeometryElement *element =
      static_cast<nsSVGPathGeometryElement*>(path->GetContent());

    return element->GetFlattenedPath(element->PrependLocalTransformsTo(gfxMatrix()));
  }
  return nullptr;
}
 
gfxFloat
nsSVGTextPathFrame::GetStartOffset()
{
  nsSVGTextPathElement *tp = static_cast<nsSVGTextPathElement*>(mContent);
  nsSVGLength2 *length = &tp->mLengthAttributes[nsSVGTextPathElement::STARTOFFSET];

  if (length->IsPercentage()) {
    nsRefPtr<gfxFlattenedPath> data = GetFlattenedPath();
    return data ? (length->GetAnimValInSpecifiedUnits() * data->GetLength() / 100.0) : 0.0;
  }
  return length->GetAnimValue(tp) * GetOffsetScale();
}

gfxFloat
nsSVGTextPathFrame::GetOffsetScale()
{
  nsIFrame *pathFrame = GetPathFrame();
  if (!pathFrame)
    return 1.0;

  return static_cast<nsSVGPathElement*>(pathFrame->GetContent())->
    GetPathLengthScale(nsSVGPathElement::eForTextPath);
}




NS_IMETHODIMP
nsSVGTextPathFrame::AttributeChanged(int32_t         aNameSpaceID,
                                     nsIAtom*        aAttribute,
                                     int32_t         aModType)
{
  if (aNameSpaceID == kNameSpaceID_None &&
      aAttribute == nsGkAtoms::startOffset) {
    nsSVGUtils::InvalidateAndScheduleReflowSVG(this);
    NotifyGlyphMetricsChange();
  } else if (aNameSpaceID == kNameSpaceID_XLink &&
             aAttribute == nsGkAtoms::href) {
    nsSVGUtils::InvalidateAndScheduleReflowSVG(this);
    
    Properties().Delete(nsSVGEffects::HrefProperty());
    NotifyGlyphMetricsChange();
  }

  return NS_OK;
}
