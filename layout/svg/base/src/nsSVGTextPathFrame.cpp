



































#include "nsSVGTextPathFrame.h"
#include "nsIDOMSVGTextPathElement.h"
#include "nsSVGLength2.h"
#include "nsIDOMSVGURIReference.h"
#include "nsSVGEffects.h"
#include "nsContentUtils.h"
#include "nsSVGPathElement.h"
#include "nsSVGTextPathElement.h"




nsIFrame*
NS_NewSVGTextPathFrame(nsIPresShell* aPresShell, nsIContent* aContent,
                       nsIFrame* parentFrame, nsStyleContext* aContext)
{
  NS_ASSERTION(parentFrame, "null parent");
  if (parentFrame->GetType() != nsGkAtoms::svgTextFrame) {
    NS_ERROR("trying to construct an SVGTextPathFrame for an invalid container");
    return nsnull;
  }
  
  nsCOMPtr<nsIDOMSVGTextPathElement> textPath = do_QueryInterface(aContent);
  if (!textPath) {
    NS_ERROR("Can't create frame! Content is not an SVG textPath");
    return nsnull;
  }

  return new (aPresShell) nsSVGTextPathFrame(aContext);
}

nsIAtom *
nsSVGTextPathFrame::GetType() const
{
  return nsGkAtoms::svgTextPathFrame;
}


NS_IMETHODIMP_(already_AddRefed<nsIDOMSVGLengthList>)
nsSVGTextPathFrame::GetX()
{
  return nsnull;
}

NS_IMETHODIMP_(already_AddRefed<nsIDOMSVGLengthList>)
nsSVGTextPathFrame::GetY()
{
  return nsnull;
}

NS_IMETHODIMP_(already_AddRefed<nsIDOMSVGLengthList>)
nsSVGTextPathFrame::GetDx()
{
  return nsnull;
}

NS_IMETHODIMP_(already_AddRefed<nsIDOMSVGLengthList>)
nsSVGTextPathFrame::GetDy()
{
  return nsnull;
}




nsIFrame *
nsSVGTextPathFrame::GetPathFrame()
{
  nsSVGTextPathProperty *property =
    static_cast<nsSVGTextPathProperty*>(GetProperty(nsGkAtoms::href));

  if (!property) {
    nsSVGTextPathElement *tp = static_cast<nsSVGTextPathElement*>(mContent);
    nsAutoString href;
    tp->mStringAttributes[nsSVGTextPathElement::HREF].GetAnimValue(href, tp);
    if (href.IsEmpty()) {
      return nsnull; 
    }

    nsCOMPtr<nsIURI> targetURI;
    nsCOMPtr<nsIURI> base = mContent->GetBaseURI();
    nsContentUtils::NewURIWithDocumentCharset(getter_AddRefs(targetURI), href,
                                              mContent->GetCurrentDoc(), base);

    property = nsSVGEffects::GetTextPathProperty(
                               targetURI, this, nsGkAtoms::href);
    if (!property)
      return nsnull;
  }

  nsIFrame *result = property->GetReferencedFrame();
  if (!result || result->GetType() != nsGkAtoms::svgPathGeometryFrame)
    return nsnull;

  return result;
}

already_AddRefed<gfxFlattenedPath>
nsSVGTextPathFrame::GetFlattenedPath()
{
  nsIFrame *path = GetPathFrame();
  return path ? GetFlattenedPath(path) : nsnull;
}
 
already_AddRefed<gfxFlattenedPath>
nsSVGTextPathFrame::GetFlattenedPath(nsIFrame *path)
{
  NS_PRECONDITION(path, "Unexpected null path");
  nsSVGPathGeometryElement *element = static_cast<nsSVGPathGeometryElement*>
                                                 (path->GetContent());

  nsCOMPtr<nsIDOMSVGMatrix> localTM = element->GetLocalTransformMatrix();

  return element->GetFlattenedPath(localTM);
}

gfxFloat
nsSVGTextPathFrame::GetStartOffset()
{
  nsSVGTextPathElement *tp = static_cast<nsSVGTextPathElement*>(mContent);
  nsSVGLength2 *length = &tp->mLengthAttributes[nsSVGTextPathElement::STARTOFFSET];
  float val = length->GetAnimValInSpecifiedUnits();

  if (val == 0.0f)
    return 0.0;

  if (length->IsPercentage()) {
    nsRefPtr<gfxFlattenedPath> data = GetFlattenedPath();
    return data ? (val * data->GetLength() / 100.0) : 0.0;
  } else {
    return val * GetPathScale();
  }
}

gfxFloat
nsSVGTextPathFrame::GetPathScale() 
{
  nsIFrame *pathFrame = GetPathFrame();
  if (!pathFrame)
    return 1.0;

  nsSVGPathElement *path = static_cast<nsSVGPathElement*>(pathFrame->GetContent());
  float pl = path->mPathLength.GetAnimValue();

  if (pl == 0.0f)
    return 1.0;

  nsRefPtr<gfxFlattenedPath> data = GetFlattenedPath(pathFrame);
  return data ? data->GetLength() / pl : 1.0; 
}




NS_IMETHODIMP
nsSVGTextPathFrame::AttributeChanged(PRInt32         aNameSpaceID,
                                     nsIAtom*        aAttribute,
                                     PRInt32         aModType)
{
  if (aNameSpaceID == kNameSpaceID_None &&
      aAttribute == nsGkAtoms::startOffset) {
    NotifyGlyphMetricsChange();
  } else if (aNameSpaceID == kNameSpaceID_XLink &&
             aAttribute == nsGkAtoms::href) {
    
    DeleteProperty(nsGkAtoms::href);
    NotifyGlyphMetricsChange();
  }

  return NS_OK;
}
