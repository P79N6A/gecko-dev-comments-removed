





#ifndef MOZILLA_SVGTRANSFORM_H__
#define MOZILLA_SVGTRANSFORM_H__

#include "gfxMatrix.h"
#include "nsDebug.h"

namespace mozilla {


static const unsigned short SVG_TRANSFORM_UNKNOWN = 0;
static const unsigned short SVG_TRANSFORM_MATRIX = 1;
static const unsigned short SVG_TRANSFORM_TRANSLATE = 2;
static const unsigned short SVG_TRANSFORM_SCALE = 3;
static const unsigned short SVG_TRANSFORM_ROTATE = 4;
static const unsigned short SVG_TRANSFORM_SKEWX = 5;
static const unsigned short SVG_TRANSFORM_SKEWY = 6;




class nsSVGTransform
{
public:
  
  nsSVGTransform()
    : mMatrix() 
    , mAngle(0.f)
    , mOriginX(0.f)
    , mOriginY(0.f)
    , mType(SVG_TRANSFORM_MATRIX)
  { }

  explicit nsSVGTransform(const gfxMatrix& aMatrix)
    : mMatrix(aMatrix)
    , mAngle(0.f)
    , mOriginX(0.f)
    , mOriginY(0.f)
    , mType(SVG_TRANSFORM_MATRIX)
  { }

  bool operator==(const nsSVGTransform& rhs) const {
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
  uint16_t Type() const {
    return mType;
  }

  const gfxMatrix& GetMatrix() const { return mMatrix; }
  void SetMatrix(const gfxMatrix& aMatrix);
  void SetTranslate(float aTx, float aTy);
  void SetScale(float aSx, float aSy);
  void SetRotate(float aAngle, float aCx, float aCy);
  nsresult SetSkewX(float aAngle);
  nsresult SetSkewY(float aAngle);

  static bool MatricesEqual(const gfxMatrix& a, const gfxMatrix& b)
  {
    return a._11 == b._11 &&
           a._12 == b._12 &&
           a._21 == b._21 &&
           a._22 == b._22 &&
           a._31 == b._31 &&
           a._32 == b._32;
  }

protected:
  gfxMatrix mMatrix;
  float mAngle, mOriginX, mOriginY;
  uint16_t mType;
};
























class SVGTransformSMILData
{
public:
  
  
  static const uint32_t NUM_SIMPLE_PARAMS = 3;

  
  
  static const uint32_t NUM_STORED_PARAMS = 6;

  explicit SVGTransformSMILData(uint16_t aType)
  : mTransformType(aType)
  {
    NS_ABORT_IF_FALSE(aType >= SVG_TRANSFORM_MATRIX &&
                      aType <= SVG_TRANSFORM_SKEWY,
                      "Unexpected transform type");
    for (uint32_t i = 0; i < NUM_STORED_PARAMS; ++i) {
      mParams[i] = 0.f;
    }
  }

  SVGTransformSMILData(uint16_t aType, float (&aParams)[NUM_SIMPLE_PARAMS])
  : mTransformType(aType)
  {
    NS_ABORT_IF_FALSE(aType >= SVG_TRANSFORM_TRANSLATE &&
                      aType <= SVG_TRANSFORM_SKEWY,
                      "Expected 'simple' transform type");
    for (uint32_t i = 0; i < NUM_SIMPLE_PARAMS; ++i) {
      mParams[i] = aParams[i];
    }
    for (uint32_t i = NUM_SIMPLE_PARAMS; i < NUM_STORED_PARAMS; ++i) {
      mParams[i] = 0.f;
    }
  }

  
  explicit SVGTransformSMILData(const nsSVGTransform& aTransform);
  nsSVGTransform ToSVGTransform() const;

  bool operator==(const SVGTransformSMILData& aOther) const
  {
    if (mTransformType != aOther.mTransformType)
      return false;

    for (uint32_t i = 0; i < NUM_STORED_PARAMS; ++i) {
      if (mParams[i] != aOther.mParams[i]) {
        return false;
      }
    }

    return true;
  }

  bool operator!=(const SVGTransformSMILData& aOther) const
  {
    return !(*this == aOther);
  }

  uint16_t mTransformType;
  float    mParams[NUM_STORED_PARAMS];
};

} 

#endif 
