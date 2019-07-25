





































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

  bool operator==(const SVGTransform& rhs) const {
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
  static bool MatricesEqual(const gfxMatrix& a, const gfxMatrix& b)
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
























class SVGTransformSMILData
{
public:
  
  
  static const PRUint32 NUM_SIMPLE_PARAMS = 3;

  
  
  static const PRUint32 NUM_STORED_PARAMS = 6;

  explicit SVGTransformSMILData(PRUint16 aType)
  : mTransformType(aType)
  {
    NS_ABORT_IF_FALSE(aType >= nsIDOMSVGTransform::SVG_TRANSFORM_MATRIX &&
                      aType <= nsIDOMSVGTransform::SVG_TRANSFORM_SKEWY,
                      "Unexpected transform type");
    for (PRUint32 i = 0; i < NUM_STORED_PARAMS; ++i) {
      mParams[i] = 0.f;
    }
  }

  SVGTransformSMILData(PRUint16 aType, float (&aParams)[NUM_SIMPLE_PARAMS])
  : mTransformType(aType)
  {
    NS_ABORT_IF_FALSE(aType >= nsIDOMSVGTransform::SVG_TRANSFORM_TRANSLATE &&
                      aType <= nsIDOMSVGTransform::SVG_TRANSFORM_SKEWY,
                      "Expected 'simple' transform type");
    for (PRUint32 i = 0; i < NUM_SIMPLE_PARAMS; ++i) {
      mParams[i] = aParams[i];
    }
    for (PRUint32 i = NUM_SIMPLE_PARAMS; i < NUM_STORED_PARAMS; ++i) {
      mParams[i] = 0.f;
    }
  }

  
  SVGTransformSMILData(const SVGTransform& aTransform);
  SVGTransform ToSVGTransform() const;

  bool operator==(const SVGTransformSMILData& aOther) const
  {
    if (mTransformType != aOther.mTransformType)
      return PR_FALSE;

    for (PRUint32 i = 0; i < NUM_STORED_PARAMS; ++i) {
      if (mParams[i] != aOther.mParams[i]) {
        return PR_FALSE;
      }
    }

    return PR_TRUE;
  }

  bool operator!=(const SVGTransformSMILData& aOther) const
  {
    return !(*this == aOther);
  }

  PRUint16 mTransformType;
  float    mParams[NUM_STORED_PARAMS];
};

} 

#endif 
