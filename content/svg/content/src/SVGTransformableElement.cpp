




#include "SVGTransformableElement.h"
#include "DOMSVGAnimatedTransformList.h"

namespace mozilla {
namespace dom {




NS_IMPL_ADDREF_INHERITED(SVGTransformableElement, SVGLocatableElement)
NS_IMPL_RELEASE_INHERITED(SVGTransformableElement, SVGLocatableElement)

NS_INTERFACE_MAP_BEGIN(SVGTransformableElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGTransformable)
NS_INTERFACE_MAP_END_INHERITING(SVGLocatableElement)






NS_IMETHODIMP
SVGTransformableElement::GetTransform(nsISupports **aTransform)
{
  *aTransform = Transform().get();
  return NS_OK;
}

already_AddRefed<DOMSVGAnimatedTransformList>
SVGTransformableElement::Transform()
{
  
  
  return DOMSVGAnimatedTransformList::GetDOMWrapper(
           GetAnimatedTransformList(DO_ALLOCATE), this).get();

}

} 
} 

