




#ifndef MOZILLA_DOMSVGPOINT_H__
#define MOZILLA_DOMSVGPOINT_H__

#include "DOMSVGPointList.h"
#include "mozilla/gfx/2D.h"
#include "nsAutoPtr.h"
#include "nsDebug.h"
#include "nsISVGPoint.h"
#include "SVGPoint.h"
#include "mozilla/Attributes.h"

class nsSVGElement;

namespace mozilla {

namespace dom {
class SVGMatrix;
}















class DOMSVGPoint MOZ_FINAL : public nsISVGPoint
{
  friend class AutoChangePointNotifier;

  typedef mozilla::gfx::Point Point;

public:
  


  DOMSVGPoint(DOMSVGPointList *aList,
              uint32_t aListIndex,
              bool aIsAnimValItem)
    : nsISVGPoint()
  {
    mList = aList;
    mListIndex = aListIndex;
    mIsAnimValItem = aIsAnimValItem;

    
    NS_ABORT_IF_FALSE(aList &&
                      aListIndex <= MaxListIndex(), "bad arg");

    NS_ABORT_IF_FALSE(IndexIsValid(), "Bad index for DOMSVGPoint!");
  }

  explicit DOMSVGPoint(const DOMSVGPoint *aPt = nullptr)
    : nsISVGPoint()
  {
    if (aPt) {
      mPt = aPt->ToSVGPoint();
    }
  }

  DOMSVGPoint(float aX, float aY)
    : nsISVGPoint()
  {
    mPt.mX = aX;
    mPt.mY = aY;
  }

  explicit DOMSVGPoint(const Point& aPt)
    : nsISVGPoint()
  {
    mPt.mX = aPt.x;
    mPt.mY = aPt.y;
    NS_ASSERTION(NS_finite(mPt.mX) && NS_finite(mPt.mX),
                 "DOMSVGPoint coords are not finite");
  }


  
  virtual float X();
  virtual void SetX(float aX, ErrorResult& rv);
  virtual float Y();
  virtual void SetY(float aY, ErrorResult& rv);
  virtual already_AddRefed<nsISVGPoint> MatrixTransform(dom::SVGMatrix& matrix);
  nsISupports* GetParentObject() MOZ_OVERRIDE {
    return mList;
  }

  virtual DOMSVGPoint* Copy() MOZ_OVERRIDE {
    return new DOMSVGPoint(this);
  }

protected:

  nsSVGElement* Element() {
    return mList->Element();
  }
};

} 

#endif 
