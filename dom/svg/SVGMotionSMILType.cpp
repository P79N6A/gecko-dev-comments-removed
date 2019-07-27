






#include "SVGMotionSMILType.h"

#include "gfx2DGlue.h"
#include "mozilla/gfx/Point.h"
#include "nsSMILValue.h"
#include "nsDebug.h"
#include "nsMathUtils.h"
#include "nsISupportsUtils.h"
#include "nsTArray.h"
#include <math.h>

using namespace mozilla::gfx;

namespace mozilla {

 SVGMotionSMILType SVGMotionSMILType::sSingleton;



enum SegmentType {
  eSegmentType_Translation,
  eSegmentType_PathPoint
};



struct TranslationParams {  
  float mX;
  float mY;
};
struct PathPointParams {  
  
  
  Path* MOZ_OWNING_REF mPath;
  float mDistToPoint; 
                      
};
















struct MotionSegment
{
  
  
  MotionSegment()
    : mSegmentType(eSegmentType_Translation)
  { }

  
  MotionSegment(float aX, float aY, float aRotateAngle)
    : mRotateType(eRotateType_Explicit), mRotateAngle(aRotateAngle),
      mSegmentType(eSegmentType_Translation)
  {
    mU.mTranslationParams.mX = aX;
    mU.mTranslationParams.mY = aY;
  }

  
  MotionSegment(Path* aPath, float aDistToPoint,
                RotateType aRotateType, float aRotateAngle)
    : mRotateType(aRotateType), mRotateAngle(aRotateAngle),
      mSegmentType(eSegmentType_PathPoint)
  {
    mU.mPathPointParams.mPath = aPath;
    mU.mPathPointParams.mDistToPoint = aDistToPoint;

    NS_ADDREF(mU.mPathPointParams.mPath); 
  }

  
  MotionSegment(const MotionSegment& aOther)
    : mRotateType(aOther.mRotateType), mRotateAngle(aOther.mRotateAngle),
      mSegmentType(aOther.mSegmentType)
  {
    if (mSegmentType == eSegmentType_Translation) {
      mU.mTranslationParams = aOther.mU.mTranslationParams;
    } else { 
      mU.mPathPointParams = aOther.mU.mPathPointParams;
      NS_ADDREF(mU.mPathPointParams.mPath); 
    }
  }

  
  ~MotionSegment()
  {
    if (mSegmentType == eSegmentType_PathPoint) {
      NS_RELEASE(mU.mPathPointParams.mPath);
    }
  }

  
  bool operator==(const MotionSegment& aOther) const
  {
    
    if (mSegmentType != aOther.mSegmentType ||
        mRotateType  != aOther.mRotateType ||
        (mRotateType == eRotateType_Explicit &&  
         mRotateAngle != aOther.mRotateAngle)) { 
      return false;
    }

    
    if (mSegmentType == eSegmentType_Translation) {
      return mU.mTranslationParams.mX == aOther.mU.mTranslationParams.mX &&
             mU.mTranslationParams.mY == aOther.mU.mTranslationParams.mY;
    }

    
    return (mU.mPathPointParams.mPath == aOther.mU.mPathPointParams.mPath) &&
      (mU.mPathPointParams.mDistToPoint ==
       aOther.mU.mPathPointParams.mDistToPoint);
  }

  bool operator!=(const MotionSegment& aOther) const
  {
    return !(*this == aOther);
  }

  
  
  RotateType mRotateType; 
  float mRotateAngle;     
  const SegmentType mSegmentType; 
                                  

  union { 
    TranslationParams mTranslationParams;
    PathPointParams mPathPointParams;
  } mU;
};

typedef FallibleTArray<MotionSegment> MotionSegmentArray;


static MotionSegmentArray&
ExtractMotionSegmentArray(nsSMILValue& aValue)
{
  return *static_cast<MotionSegmentArray*>(aValue.mU.mPtr);
}

static const MotionSegmentArray&
ExtractMotionSegmentArray(const nsSMILValue& aValue)
{
  return *static_cast<const MotionSegmentArray*>(aValue.mU.mPtr);
}




void
SVGMotionSMILType::Init(nsSMILValue& aValue) const
{
  MOZ_ASSERT(aValue.IsNull(), "Unexpected SMIL type");

  aValue.mType = this;
  aValue.mU.mPtr = new MotionSegmentArray(1);
}

void
SVGMotionSMILType::Destroy(nsSMILValue& aValue) const
{
  MOZ_ASSERT(aValue.mType == this, "Unexpected SMIL type");

  MotionSegmentArray* arr = static_cast<MotionSegmentArray*>(aValue.mU.mPtr);
  delete arr;

  aValue.mU.mPtr = nullptr;
  aValue.mType = nsSMILNullType::Singleton();
}

nsresult
SVGMotionSMILType::Assign(nsSMILValue& aDest, const nsSMILValue& aSrc) const
{
  MOZ_ASSERT(aDest.mType == aSrc.mType, "Incompatible SMIL types");
  MOZ_ASSERT(aDest.mType == this, "Unexpected SMIL type");

  const MotionSegmentArray& srcArr = ExtractMotionSegmentArray(aSrc);
  MotionSegmentArray& dstArr = ExtractMotionSegmentArray(aDest);

  
  if (!dstArr.SetCapacity(srcArr.Length())) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  dstArr = srcArr; 
  return NS_OK;
}

bool
SVGMotionSMILType::IsEqual(const nsSMILValue& aLeft,
                           const nsSMILValue& aRight) const
{
  MOZ_ASSERT(aLeft.mType == aRight.mType, "Incompatible SMIL types");
  MOZ_ASSERT(aLeft.mType == this, "Unexpected SMIL type");

  const MotionSegmentArray& leftArr = ExtractMotionSegmentArray(aLeft);
  const MotionSegmentArray& rightArr = ExtractMotionSegmentArray(aRight);

  
  if (leftArr.Length() != rightArr.Length()) {
    return false;
  }

  
  uint32_t length = leftArr.Length(); 
  for (uint32_t i = 0; i < length; ++i) {
    if (leftArr[i] != rightArr[i]) {
      return false;
    }
  }

  return true; 
}


inline static void
GetAngleAndPointAtDistance(Path* aPath, float aDistance,
                           RotateType aRotateType,
                           float& aRotateAngle, 
                           Point& aPoint)       
{
  if (aRotateType == eRotateType_Explicit) {
    
    aPoint = aPath->ComputePointAtLength(aDistance);
  } else {
    Point tangent; 
    aPoint = aPath->ComputePointAtLength(aDistance, &tangent);
    float tangentAngle = atan2(tangent.y, tangent.x);
    if (aRotateType == eRotateType_Auto) {
      aRotateAngle = tangentAngle;
    } else {
      MOZ_ASSERT(aRotateType == eRotateType_AutoReverse);
      aRotateAngle = M_PI + tangentAngle;
    }
  }
}

nsresult
SVGMotionSMILType::Add(nsSMILValue& aDest, const nsSMILValue& aValueToAdd,
                       uint32_t aCount) const
{
  MOZ_ASSERT(aDest.mType == aValueToAdd.mType,
             "Incompatible SMIL types");
  MOZ_ASSERT(aDest.mType == this, "Unexpected SMIL type");

  MotionSegmentArray& dstArr = ExtractMotionSegmentArray(aDest);
  const MotionSegmentArray& srcArr = ExtractMotionSegmentArray(aValueToAdd);

  
  
  
  
  
  
  
  
  MOZ_ASSERT(srcArr.Length() == 1, "Invalid source segment arr to add");
  MOZ_ASSERT(dstArr.Length() == 1, "Invalid dest segment arr to add to");
  const MotionSegment& srcSeg = srcArr[0];
  const MotionSegment& dstSeg = dstArr[0];
  MOZ_ASSERT(srcSeg.mSegmentType == eSegmentType_PathPoint,
             "expecting to be adding points from a motion path");
  MOZ_ASSERT(dstSeg.mSegmentType == eSegmentType_PathPoint,
             "expecting to be adding points from a motion path");

  const PathPointParams& srcParams = srcSeg.mU.mPathPointParams;
  const PathPointParams& dstParams = dstSeg.mU.mPathPointParams;

  MOZ_ASSERT(srcSeg.mRotateType  == dstSeg.mRotateType &&
             srcSeg.mRotateAngle == dstSeg.mRotateAngle,
             "unexpected angle mismatch");
  MOZ_ASSERT(srcParams.mPath == dstParams.mPath,
             "unexpected path mismatch");
  Path* path = srcParams.mPath;

  
  float rotateAngle = dstSeg.mRotateAngle;
  Point dstPt;
  GetAngleAndPointAtDistance(path, dstParams.mDistToPoint, dstSeg.mRotateType,
                             rotateAngle, dstPt);

  Point srcPt = path->ComputePointAtLength(srcParams.mDistToPoint);

  float newX = dstPt.x + srcPt.x * aCount;
  float newY = dstPt.y + srcPt.y * aCount;

  
  
  dstArr.Clear();
  dstArr.AppendElement(MotionSegment(newX, newY, rotateAngle));
  return NS_OK;
}

nsresult
SVGMotionSMILType::SandwichAdd(nsSMILValue& aDest,
                               const nsSMILValue& aValueToAdd) const
{
  MOZ_ASSERT(aDest.mType == aValueToAdd.mType,
             "Incompatible SMIL types");
  MOZ_ASSERT(aDest.mType == this, "Unexpected SMIL type");
  MotionSegmentArray& dstArr = ExtractMotionSegmentArray(aDest);
  const MotionSegmentArray& srcArr = ExtractMotionSegmentArray(aValueToAdd);

  
  MOZ_ASSERT(srcArr.Length() == 1,
             "Trying to do sandwich add of more than one value");

  if (!dstArr.AppendElement(srcArr[0])) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  
  return NS_OK;
}

nsresult
SVGMotionSMILType::ComputeDistance(const nsSMILValue& aFrom,
                                   const nsSMILValue& aTo,
                                   double& aDistance) const
{
  MOZ_ASSERT(aFrom.mType == aTo.mType, "Incompatible SMIL types");
  MOZ_ASSERT(aFrom.mType == this, "Unexpected SMIL type");
  const MotionSegmentArray& fromArr = ExtractMotionSegmentArray(aFrom);
  const MotionSegmentArray& toArr = ExtractMotionSegmentArray(aTo);

  
  
  MOZ_ASSERT(fromArr.Length() == 1,
             "Wrong number of elements in from value");
  MOZ_ASSERT(toArr.Length() == 1,
             "Wrong number of elements in to value");

  const MotionSegment& from = fromArr[0];
  const MotionSegment& to = toArr[0];

  MOZ_ASSERT(from.mSegmentType == to.mSegmentType,
             "Mismatched MotionSegment types");
  if (from.mSegmentType == eSegmentType_PathPoint) {
    const PathPointParams& fromParams = from.mU.mPathPointParams;
    const PathPointParams& toParams   = to.mU.mPathPointParams;
    MOZ_ASSERT(fromParams.mPath == toParams.mPath,
               "Interpolation endpoints should be from same path");
    MOZ_ASSERT(fromParams.mDistToPoint <= toParams.mDistToPoint,
               "To value shouldn't be before from value on path");
    aDistance = fabs(toParams.mDistToPoint - fromParams.mDistToPoint);
  } else {
    const TranslationParams& fromParams = from.mU.mTranslationParams;
    const TranslationParams& toParams   = to.mU.mTranslationParams;
    float dX = toParams.mX - fromParams.mX;
    float dY = toParams.mY - fromParams.mY;
    aDistance = NS_hypot(dX, dY);
  }

  return NS_OK;
}


static inline float
InterpolateFloat(const float& aStartFlt, const float& aEndFlt,
                 const double& aUnitDistance)
{
  return aStartFlt + aUnitDistance * (aEndFlt - aStartFlt);
}

nsresult
SVGMotionSMILType::Interpolate(const nsSMILValue& aStartVal,
                               const nsSMILValue& aEndVal,
                               double aUnitDistance,
                               nsSMILValue& aResult) const
{
  MOZ_ASSERT(aStartVal.mType == aEndVal.mType,
             "Trying to interpolate different types");
  MOZ_ASSERT(aStartVal.mType == this,
             "Unexpected types for interpolation");
  MOZ_ASSERT(aResult.mType == this, "Unexpected result type");
  MOZ_ASSERT(aUnitDistance >= 0.0 && aUnitDistance <= 1.0,
             "unit distance value out of bounds");

  const MotionSegmentArray& startArr = ExtractMotionSegmentArray(aStartVal);
  const MotionSegmentArray& endArr = ExtractMotionSegmentArray(aEndVal);
  MotionSegmentArray& resultArr = ExtractMotionSegmentArray(aResult);

  MOZ_ASSERT(startArr.Length() <= 1,
             "Invalid start-point for animateMotion interpolation");
  MOZ_ASSERT(endArr.Length() == 1,
             "Invalid end-point for animateMotion interpolation");
  MOZ_ASSERT(resultArr.IsEmpty(),
             "Expecting result to be just-initialized w/ empty array");

  const MotionSegment& endSeg = endArr[0];
  MOZ_ASSERT(endSeg.mSegmentType == eSegmentType_PathPoint,
             "Expecting to be interpolating along a path");

  const PathPointParams& endParams = endSeg.mU.mPathPointParams;
  
  
  
  Path* path = endParams.mPath;
  RotateType rotateType  = endSeg.mRotateType;
  float rotateAngle      = endSeg.mRotateAngle;

  float startDist;
  if (startArr.IsEmpty()) {
    startDist = 0.0f;
  } else {
    const MotionSegment& startSeg = startArr[0];
    MOZ_ASSERT(startSeg.mSegmentType == eSegmentType_PathPoint,
               "Expecting to be interpolating along a path");
    const PathPointParams& startParams = startSeg.mU.mPathPointParams;
    MOZ_ASSERT(startSeg.mRotateType  == endSeg.mRotateType &&
               startSeg.mRotateAngle == endSeg.mRotateAngle,
               "unexpected angle mismatch");
    MOZ_ASSERT(startParams.mPath == endParams.mPath,
               "unexpected path mismatch");
    startDist = startParams.mDistToPoint;
  }

  
  float resultDist = InterpolateFloat(startDist, endParams.mDistToPoint,
                                      aUnitDistance);

  
  
  resultArr.AppendElement(MotionSegment(path, resultDist,
                                        rotateType, rotateAngle));
  return NS_OK;
}

 gfx::Matrix
SVGMotionSMILType::CreateMatrix(const nsSMILValue& aSMILVal)
{
  const MotionSegmentArray& arr = ExtractMotionSegmentArray(aSMILVal);

  gfx::Matrix matrix;
  uint32_t length = arr.Length();
  for (uint32_t i = 0; i < length; i++) {
    Point point;  
    float rotateAngle = arr[i].mRotateAngle; 
    if (arr[i].mSegmentType == eSegmentType_Translation) {
      point.x = arr[i].mU.mTranslationParams.mX;
      point.y = arr[i].mU.mTranslationParams.mY;
      MOZ_ASSERT(arr[i].mRotateType == eRotateType_Explicit,
                 "'auto'/'auto-reverse' should have been converted to "
                 "explicit angles when we generated this translation");
    } else {
      GetAngleAndPointAtDistance(arr[i].mU.mPathPointParams.mPath,
                                 arr[i].mU.mPathPointParams.mDistToPoint,
                                 arr[i].mRotateType,
                                 rotateAngle, point);
    }
    matrix.PreTranslate(point.x, point.y);
    matrix.PreRotate(rotateAngle);
  }
  return matrix;
}

 nsSMILValue
SVGMotionSMILType::ConstructSMILValue(Path* aPath,
                                      float aDist,
                                      RotateType aRotateType,
                                      float aRotateAngle)
{
  nsSMILValue smilVal(&SVGMotionSMILType::sSingleton);
  MotionSegmentArray& arr = ExtractMotionSegmentArray(smilVal);

  
  arr.AppendElement(MotionSegment(aPath, aDist, aRotateType, aRotateAngle));
  return smilVal;
}

} 
