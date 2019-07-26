




#ifndef MOZILLA_DOMSVGPOINT_H__
#define MOZILLA_DOMSVGPOINT_H__

#include "DOMSVGPointList.h"
#include "gfxPoint.h"
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

  explicit DOMSVGPoint(const gfxPoint &aPt)
    : nsISVGPoint()
  {
    mPt.mX = float(aPt.x);
    mPt.mY = float(aPt.y);
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

  nsISVGPoint* Clone() {
    return new DOMSVGPoint(this);
  }

protected:

  nsSVGElement* Element() {
    return mList->Element();
  }
};

} 

#endif 
