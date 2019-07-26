




#ifndef mozilla_dom_SVGAnimatedRect_h
#define mozilla_dom_SVGAnimatedRect_h

#include "nsCycleCollectionParticipant.h"
#include "mozilla/dom/SVGRectBinding.h"
#include "mozilla/Attributes.h"
#include "mozilla/ErrorResult.h"
#include "nsWrapperCache.h"

class nsSVGElement;
class nsSVGViewBox;

namespace mozilla {
namespace dom {

class SVGAnimatedRect MOZ_FINAL : public nsISupports,
                                  public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(SVGAnimatedRect)

  SVGAnimatedRect(nsSVGViewBox* aVal, nsSVGElement* aSVGElement);

  virtual ~SVGAnimatedRect();

  nsSVGElement* GetParentObject() const
  {
    return mSVGElement;
  }

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  already_AddRefed<SVGIRect> GetBaseVal(ErrorResult& aRv);

  already_AddRefed<SVGIRect> GetAnimVal(ErrorResult& aRv);

private:
  nsSVGViewBox* mVal; 
  nsRefPtr<nsSVGElement> mSVGElement;
};

} 
} 

#endif 

