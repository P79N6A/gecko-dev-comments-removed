




#ifndef mozilla_dom_SVGAnimatedRect_h
#define mozilla_dom_SVGAnimatedRect_h

#include "nsCycleCollectionParticipant.h"
#include "mozilla/dom/SVGRectBinding.h"
#include "mozilla/Attributes.h"
#include "mozilla/ErrorResult.h"
#include "nsWrapperCache.h"
#include "nsSVGElement.h"

class nsSVGViewBox;

namespace mozilla {
namespace dom {

class SVGAnimatedRect MOZ_FINAL : public nsWrapperCache
{
public:
  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(SVGAnimatedRect)
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(SVGAnimatedRect)

  SVGAnimatedRect(nsSVGViewBox* aVal, nsSVGElement* aSVGElement);

  virtual ~SVGAnimatedRect();

  nsSVGElement* GetParentObject() const
  {
    return mSVGElement;
  }

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  already_AddRefed<SVGIRect> GetBaseVal();

  already_AddRefed<SVGIRect> GetAnimVal();

private:
  nsSVGViewBox* mVal; 
  nsRefPtr<nsSVGElement> mSVGElement;
};

} 
} 

#endif 

