





































#ifndef MOZILLA_SVGTRANSFORM_H__
#define MOZILLA_SVGTRANSFORM_H__

#include "gfxMatrix.h"
#include "nsIDOMSVGTransform.h"

namespace mozilla {




class SVGTransform
{
public:
  
  SVGTransform()
    : mMatrix() 
    , mAngle(0.f)
    , mOriginX(0.f)
    , mOriginY(0.f)
    , mType(nsIDOMSVGTransform::SVG_TRANSFORM_MATRIX)
  { }

  SVGTransform(const gfxMatrix& aMatrix)
    : mMatrix(aMatrix)
    , mAngle(0.f)
    , mOriginX(0.f)
    , mOriginY(0.f)
    , mType(nsIDOMSVGTransform::SVG_TRANSFORM_MATRIX)
  { }

  PRBool operator==(const SVGTransform& rhs) const {
    return mType == rhs.mType &&
      MatricesEqual(mMatrix, rhs.mMatrix) &&
      mAngle == rhs.mAngle &&
      mOriginX == rhs.mOriginX &&
      mOriginY == rhs.mOriginY;
  }

  void GetValueAsString(nsAString& aValue) const;

  float Angle() const {
    return mAngle;
  }
  void GetRotationOrigin(float& aOriginX, float& aOriginY) const {
    aOriginX = mOriginX;
    aOriginY = mOriginY;
  }
  PRUint16 Type() const {
    return mType;
  }

  const gfxMatrix& Matrix() const { return mMatrix; }
  void SetMatrix(const gfxMatrix& aMatrix);
  void SetTranslate(float aTx, float aTy);
  void SetScale(float aSx, float aSy);
  void SetRotate(float aAngle, float aCx, float aCy);
  nsresult SetSkewX(float aAngle);
  nsresult SetSkewY(float aAngle);

protected:
  static PRBool MatricesEqual(const gfxMatrix& a, const gfxMatrix& b)
  {
    return a.xx == b.xx &&
           a.yx == b.yx &&
           a.xy == b.xy &&
           a.yy == b.yy &&
           a.x0 == b.x0 &&
           a.y0 == b.y0;
  }

  gfxMatrix mMatrix;
  float mAngle, mOriginX, mOriginY;
  PRUint16 mType;
};

} 

#endif 
