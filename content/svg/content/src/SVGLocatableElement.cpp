



#include "mozilla/dom/SVGLocatableElement.h"
#include "nsIFrame.h"
#include "nsISVGChildFrame.h"
#include "nsSVGRect.h"
#include "nsSVGUtils.h"
#include "SVGContentUtils.h"
#include "mozilla/dom/SVGMatrix.h"
#include "mozilla/dom/SVGSVGElement.h"

namespace mozilla {
namespace dom {




NS_IMPL_ADDREF_INHERITED(SVGLocatableElement, nsSVGElement)
NS_IMPL_RELEASE_INHERITED(SVGLocatableElement, nsSVGElement)

NS_INTERFACE_MAP_BEGIN(SVGLocatableElement)
  NS_INTERFACE_MAP_ENTRY(mozilla::dom::SVGLocatableElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGElement)




nsSVGElement*
SVGLocatableElement::GetNearestViewportElement()
{
  return SVGContentUtils::GetNearestViewportElement(this);
}

nsSVGElement*
SVGLocatableElement::GetFarthestViewportElement()
{
  return SVGContentUtils::GetOuterSVGElement(this);
}

already_AddRefed<nsIDOMSVGRect>
SVGLocatableElement::GetBBox(ErrorResult& rv)
{
  nsIFrame* frame = GetPrimaryFrame(Flush_Layout);

  if (!frame || (frame->GetStateBits() & NS_STATE_SVG_NONDISPLAY_CHILD)) {
    rv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  nsISVGChildFrame* svgframe = do_QueryFrame(frame);
  if (!svgframe) {
    rv.Throw(NS_ERROR_NOT_IMPLEMENTED); 
    return nullptr;
  }

  nsCOMPtr<nsIDOMSVGRect> rect;
  rv = NS_NewSVGRect(getter_AddRefs(rect), nsSVGUtils::GetBBox(frame));
  return rect.forget();
}

already_AddRefed<SVGMatrix>
SVGLocatableElement::GetCTM()
{
  nsIDocument* currentDoc = GetCurrentDoc();
  if (currentDoc) {
    
    currentDoc->FlushPendingNotifications(Flush_Layout);
  }
  gfxMatrix m = SVGContentUtils::GetCTM(this, false);
  nsCOMPtr<SVGMatrix> mat = m.IsSingular() ? nullptr : new SVGMatrix(m);
  return mat.forget();
}

already_AddRefed<SVGMatrix>
SVGLocatableElement::GetScreenCTM()
{
  nsIDocument* currentDoc = GetCurrentDoc();
  if (currentDoc) {
    
    currentDoc->FlushPendingNotifications(Flush_Layout);
  }
  gfxMatrix m = SVGContentUtils::GetCTM(this, true);
  nsCOMPtr<SVGMatrix> mat = m.IsSingular() ? nullptr : new SVGMatrix(m);
  return mat.forget();
}

already_AddRefed<SVGMatrix>
SVGLocatableElement::GetTransformToElement(SVGLocatableElement& aElement,
                                           ErrorResult& rv)
{
  
  nsCOMPtr<SVGMatrix> ourScreenCTM = GetScreenCTM();
  nsCOMPtr<SVGMatrix> targetScreenCTM = aElement.GetScreenCTM();
  if (!ourScreenCTM || !targetScreenCTM) {
    rv.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
    return nullptr;
  }
  nsCOMPtr<SVGMatrix> tmp = targetScreenCTM->Inverse(rv);
  if (rv.Failed()) return nullptr;

  nsCOMPtr<SVGMatrix> mat = tmp->Multiply(*ourScreenCTM).get();
  return mat.forget();
}

} 
} 

