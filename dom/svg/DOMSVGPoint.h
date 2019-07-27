




#ifndef MOZILLA_DOMSVGPOINT_H__
#define MOZILLA_DOMSVGPOINT_H__

#include "DOMSVGPointList.h"
#include "mozilla/gfx/2D.h"
#include "nsAutoPtr.h"
#include "nsDebug.h"
#include "nsISVGPoint.h"
#include "SVGPoint.h"
#include "mozilla/Attributes.h"
#include "mozilla/FloatingPoint.h"

class nsSVGElement;

namespace mozilla {

namespace dom {
class SVGMatrix;
}















class DOMSVGPoint final : public nsISVGPoint
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

    
    MOZ_ASSERT(aList && aListIndex <= MaxListIndex(), "bad arg");

    MOZ_ASSERT(IndexIsValid(), "Bad index for DOMSVGPoint!");
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
    NS_ASSERTION(IsFinite(mPt.mX) && IsFinite(mPt.mX),
                 "DOMSVGPoint coords are not finite");
  }


  
  virtual float X() override;
  virtual void SetX(float aX, ErrorResult& rv) override;
  virtual float Y() override;
  virtual void SetY(float aY, ErrorResult& rv) override;
  virtual already_AddRefed<nsISVGPoint> MatrixTransform(dom::SVGMatrix& matrix) override;
  nsISupports* GetParentObject() override {
    return mList;
  }

  virtual DOMSVGPoint* Copy() override {
    return new DOMSVGPoint(this);
  }

protected:

  nsSVGElement* Element() {
    return mList->Element();
  }
};

} 

#endif 
