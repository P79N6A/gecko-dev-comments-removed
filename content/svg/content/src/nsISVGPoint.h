




#pragma once

#include "nsAutoPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsWrapperCache.h"
#include "mozilla/dom/SVGPointBinding.h"

class nsSVGElement;


#define MOZILLA_NSISVGPOINT_IID \
  { 0xd6b6c440, 0xaf8d, 0x40ee, \
    { 0x85, 0x6b, 0x02, 0xa3, 0x17, 0xca, 0xb2, 0x75 } }

namespace mozilla {

namespace dom {
class SVGMatrix;
}







class nsISVGPoint : public nsISupports,
                    public nsWrapperCache
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(MOZILLA_NSISVGPOINT_IID)

  


  explicit nsISVGPoint()
  {
    SetIsDOMBinding();
  }

  
  virtual float X() = 0;
  virtual void SetX(float aX, ErrorResult& rv) = 0;
  virtual float Y() = 0;
  virtual void SetY(float aY, ErrorResult& rv) = 0;
  virtual already_AddRefed<nsISVGPoint> MatrixTransform(dom::SVGMatrix& matrix) = 0;
  virtual JSObject* WrapObject(JSContext *cx, JSObject *scope,
                               bool *triedToWrap)
    { return dom::SVGPointBinding::Wrap(cx, scope, this, triedToWrap); }

  virtual nsISupports* GetParentObject() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsISVGPoint, MOZILLA_NSISVGPOINT_IID)

} 

