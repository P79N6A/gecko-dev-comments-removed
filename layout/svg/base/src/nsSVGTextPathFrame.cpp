



































#include "nsSVGTextPathFrame.h"
#include "nsSVGTextFrame.h"
#include "nsIDOMSVGTextPathElement.h"
#include "nsIDOMSVGAnimatedLength.h"
#include "nsSVGLength.h"
#include "nsIDOMSVGURIReference.h"
#include "nsSVGUtils.h"
#include "nsContentUtils.h"
#include "nsIDOMSVGAnimatedPathData.h"
#include "nsSVGPathElement.h"
#include "nsSVGTextPathElement.h"


NS_IMPL_ISUPPORTS1(nsSVGPathListener, nsIMutationObserver)

nsSVGPathListener::nsSVGPathListener(nsIContent *aPathElement,
                                     nsSVGTextPathFrame *aTextPathFrame) :
  mTextPathFrame(aTextPathFrame)
{
  mObservedPath = do_GetWeakReference(aPathElement);
  aPathElement->AddMutationObserver(this);
}

nsSVGPathListener::~nsSVGPathListener()
{
  nsCOMPtr<nsIContent> path = do_QueryReferent(mObservedPath);
  if (path)
    path->RemoveMutationObserver(this);
}

void
nsSVGPathListener::AttributeChanged(nsIDocument *aDocument,
                                    nsIContent *aContent,
                                    PRInt32 aNameSpaceID,
                                    nsIAtom *aAttribute,
                                    PRInt32 aModType,
                                    PRUint32 aStateMask)
{
  mTextPathFrame->NotifyGlyphMetricsChange();
}




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

NS_IMETHODIMP
nsSVGTextPathFrame::Init(nsIContent*      aContent,
                         nsIFrame*        aParent,
                         nsIFrame*        aPrevInFlow)
{
  nsSVGTextPathFrameBase::Init(aContent, aParent, aPrevInFlow);

  {
    nsCOMPtr<nsIDOMSVGURIReference> aRef = do_QueryInterface(mContent);
    if (aRef)
      aRef->GetHref(getter_AddRefs(mHref));
    if (!mHref)
      return NS_ERROR_FAILURE;
  }

  return NS_OK;
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
  nsIFrame *path = nsnull;

  nsAutoString str;
  mHref->GetAnimVal(str);

  nsCOMPtr<nsIURI> targetURI;
  nsCOMPtr<nsIURI> base = mContent->GetBaseURI();
  nsContentUtils::NewURIWithDocumentCharset(getter_AddRefs(targetURI), str,
                                            mContent->GetCurrentDoc(), base);

  nsSVGUtils::GetReferencedFrame(&path, targetURI, mContent,
                                 PresContext()->PresShell());
  if (!path || (path->GetType() != nsGkAtoms::svgPathGeometryFrame))
    return nsnull;
  return path;
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

  if (!mPathListener) {
    mPathListener = new nsSVGPathListener(path->GetContent(), this);
  }

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
    mPathListener = nsnull;
    NotifyGlyphMetricsChange();
  }

  return NS_OK;
}
