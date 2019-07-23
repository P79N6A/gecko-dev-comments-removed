



































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
                                    PRInt32 aModType)
{
  mTextPathFrame->UpdateGraphic();
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
  
  nsCOMPtr<nsIDOMSVGTextPathElement> tpath_elem = do_QueryInterface(aContent);
  if (!tpath_elem) {
    NS_ERROR("Trying to construct an SVGTextPathFrame for a "
             "content element that doesn't support the right interfaces");
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

  nsCOMPtr<nsIDOMSVGTextPathElement> tpath = do_QueryInterface(mContent);

  {
    nsCOMPtr<nsIDOMSVGAnimatedLength> length;
    tpath->GetStartOffset(getter_AddRefs(length));

    
#ifdef DEBUG_tor
    fprintf(stderr,
            "### Using nsSVGTextPathFrame mStartOffset hack - fix me\n");
#endif
    nsCOMPtr<nsIDOMSVGLength> offset;
    length->GetAnimVal(getter_AddRefs(offset));
    PRUint16 type;
    float value;
    offset->GetUnitType(&type);
    offset->GetValueInSpecifiedUnits(&value);
    nsCOMPtr<nsISVGLength> l;
    NS_NewSVGLength(getter_AddRefs(l), value, type);
    mStartOffset = l;

    NS_ASSERTION(mStartOffset, "no startOffset");
    if (!mStartOffset)
      return NS_ERROR_FAILURE;

    NS_NewSVGLengthList(getter_AddRefs(mX),
                        NS_STATIC_CAST(nsSVGElement*, mContent));
    if (mX) {
      nsCOMPtr<nsIDOMSVGLength> length;
      mX->AppendItem(mStartOffset, getter_AddRefs(length));
    }
  }

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
  nsIDOMSVGLengthList *retval = mX;
  NS_IF_ADDREF(retval);
  return retval;
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
nsSVGTextPathFrame::GetPathFrame() {
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
nsSVGTextPathFrame::GetFlattenedPath() {
  nsIFrame *path = GetPathFrame();
  if (!path)
    return nsnull;

  nsSVGPathGeometryElement *element = NS_STATIC_CAST(nsSVGPathGeometryElement*,
                                                     path->GetContent());

  if (!mPathListener) {
    mPathListener = new nsSVGPathListener(path->GetContent(), this);
  }

  nsCOMPtr<nsIDOMSVGMatrix> localTM = element->GetLocalTransformMatrix();

  return element->GetFlattenedPath(localTM);
}




NS_IMETHODIMP
nsSVGTextPathFrame::AttributeChanged(PRInt32         aNameSpaceID,
                                     nsIAtom*        aAttribute,
                                     PRInt32         aModType)
{
  if (aNameSpaceID == kNameSpaceID_None &&
      aAttribute == nsGkAtoms::startOffset) {
    UpdateGraphic();
  } else if (aNameSpaceID == kNameSpaceID_XLink &&
             aAttribute == nsGkAtoms::href) {
    mPathListener = nsnull;
    UpdateGraphic();
  }

  return NS_OK;
}
