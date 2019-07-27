




#include "2D.h"
#include "PathAnalysis.h"
#include "PathHelpers.h"

namespace mozilla {
namespace gfx {

static float CubicRoot(float aValue) {
  if (aValue < 0.0) {
    return -CubicRoot(-aValue);
  }
  else {
    return powf(aValue, 1.0f / 3.0f);
  }
}

struct BezierControlPoints
{
  BezierControlPoints() {}
  BezierControlPoints(const Point &aCP1, const Point &aCP2,
                      const Point &aCP3, const Point &aCP4)
    : mCP1(aCP1), mCP2(aCP2), mCP3(aCP3), mCP4(aCP4)
  {
  }

  Point mCP1, mCP2, mCP3, mCP4;
};

void
FlattenBezier(const BezierControlPoints &aPoints,
              PathSink *aSink, Float aTolerance);


Path::Path()
{
}

Path::~Path()
{
}

Float
Path::ComputeLength()
{
  EnsureFlattenedPath();
  return mFlattenedPath->ComputeLength();
}

Point
Path::ComputePointAtLength(Float aLength, Point* aTangent)
{
  EnsureFlattenedPath();
  return mFlattenedPath->ComputePointAtLength(aLength, aTangent);
}

void
Path::EnsureFlattenedPath()
{
  if (!mFlattenedPath) {
    mFlattenedPath = new FlattenedPath();
    StreamToSink(mFlattenedPath);
  }
}



const Float kFlatteningTolerance = 0.0001f;

void
FlattenedPath::MoveTo(const Point &aPoint)
{
  MOZ_ASSERT(!mCalculatedLength);
  FlatPathOp op;
  op.mType = FlatPathOp::OP_MOVETO;
  op.mPoint = aPoint;
  mPathOps.push_back(op);

  mLastMove = aPoint;
}

void
FlattenedPath::LineTo(const Point &aPoint)
{
  MOZ_ASSERT(!mCalculatedLength);
  FlatPathOp op;
  op.mType = FlatPathOp::OP_LINETO;
  op.mPoint = aPoint;
  mPathOps.push_back(op);
}

void
FlattenedPath::BezierTo(const Point &aCP1,
                        const Point &aCP2,
                        const Point &aCP3)
{
  MOZ_ASSERT(!mCalculatedLength);
  FlattenBezier(BezierControlPoints(CurrentPoint(), aCP1, aCP2, aCP3), this, kFlatteningTolerance);
}

void
FlattenedPath::QuadraticBezierTo(const Point &aCP1,
                                 const Point &aCP2)
{
  MOZ_ASSERT(!mCalculatedLength);
  
  
  
  
  Point CP0 = CurrentPoint();
  Point CP1 = (CP0 + aCP1 * 2.0) / 3.0;
  Point CP2 = (aCP2 + aCP1 * 2.0) / 3.0;
  Point CP3 = aCP2;

  BezierTo(CP1, CP2, CP3);
}

void
FlattenedPath::Close()
{
  MOZ_ASSERT(!mCalculatedLength);
  LineTo(mLastMove);
}

void
FlattenedPath::Arc(const Point &aOrigin, float aRadius, float aStartAngle,
                   float aEndAngle, bool aAntiClockwise)
{
  ArcToBezier(this, aOrigin, Size(aRadius, aRadius), aStartAngle, aEndAngle, aAntiClockwise);
}

Float
FlattenedPath::ComputeLength()
{
  if (!mCalculatedLength) {
    Point currentPoint;

    for (uint32_t i = 0; i < mPathOps.size(); i++) {
      if (mPathOps[i].mType == FlatPathOp::OP_MOVETO) {
        currentPoint = mPathOps[i].mPoint;
      } else {
        mCachedLength += Distance(currentPoint, mPathOps[i].mPoint);
        currentPoint = mPathOps[i].mPoint;
      }
    }

    mCalculatedLength =  true;
  }

  return mCachedLength;
}

Point
FlattenedPath::ComputePointAtLength(Float aLength, Point *aTangent)
{
  
  
  
  Point lastPointSinceMove;
  Point currentPoint;
  for (uint32_t i = 0; i < mPathOps.size(); i++) {
    if (mPathOps[i].mType == FlatPathOp::OP_MOVETO) {
      if (Distance(currentPoint, mPathOps[i].mPoint)) {
        lastPointSinceMove = currentPoint;
      }
      currentPoint = mPathOps[i].mPoint;
    } else {
      Float segmentLength = Distance(currentPoint, mPathOps[i].mPoint);

      if (segmentLength) {
        lastPointSinceMove = currentPoint;
        if (segmentLength > aLength) {
          Point currentVector = mPathOps[i].mPoint - currentPoint;
          Point tangent = currentVector / segmentLength;
          if (aTangent) {
            *aTangent = tangent;
          }
          return currentPoint + tangent * aLength;
        }
      }

      aLength -= segmentLength;
      currentPoint = mPathOps[i].mPoint;
    }
  }

  Point currentVector = currentPoint - lastPointSinceMove;
  if (aTangent) {
    if (hypotf(currentVector.x, currentVector.y)) {
      *aTangent = currentVector / hypotf(currentVector.x, currentVector.y);
    } else {
      *aTangent = Point();
    }
  }
  return currentPoint;
}



static void 
SplitBezier(const BezierControlPoints &aControlPoints,
            BezierControlPoints *aFirstSegmentControlPoints,
            BezierControlPoints *aSecondSegmentControlPoints,
            Float t)
{
  MOZ_ASSERT(aSecondSegmentControlPoints);
  
  *aSecondSegmentControlPoints = aControlPoints;

  Point cp1a = aControlPoints.mCP1 + (aControlPoints.mCP2 - aControlPoints.mCP1) * t;
  Point cp2a = aControlPoints.mCP2 + (aControlPoints.mCP3 - aControlPoints.mCP2) * t;
  Point cp1aa = cp1a + (cp2a - cp1a) * t;
  Point cp3a = aControlPoints.mCP3 + (aControlPoints.mCP4 - aControlPoints.mCP3) * t;
  Point cp2aa = cp2a + (cp3a - cp2a) * t;
  Point cp1aaa = cp1aa + (cp2aa - cp1aa) * t;
  aSecondSegmentControlPoints->mCP4 = aControlPoints.mCP4;

  if(aFirstSegmentControlPoints) {
    aFirstSegmentControlPoints->mCP1 = aControlPoints.mCP1;
    aFirstSegmentControlPoints->mCP2 = cp1a;
    aFirstSegmentControlPoints->mCP3 = cp1aa;
    aFirstSegmentControlPoints->mCP4 = cp1aaa;
  }
  aSecondSegmentControlPoints->mCP1 = cp1aaa;
  aSecondSegmentControlPoints->mCP2 = cp2aa;
  aSecondSegmentControlPoints->mCP3 = cp3a;
}

static void
FlattenBezierCurveSegment(const BezierControlPoints &aControlPoints,
                          PathSink *aSink,
                          Float aTolerance)
{
  







  BezierControlPoints currentCP = aControlPoints;

  Float t = 0;
  while (t < 1.0f) {
    Point cp21 = currentCP.mCP2 - currentCP.mCP3;
    Point cp31 = currentCP.mCP3 - currentCP.mCP1;

    Float s3 = (cp31.x * cp21.y - cp31.y * cp21.x) / hypotf(cp21.x, cp21.y);

    t = 2 * Float(sqrt(aTolerance / (3. * abs(s3))));

    if (t >= 1.0f) {
      aSink->LineTo(aControlPoints.mCP4);
      break;
    }

    Point prevCP2, prevCP3, nextCP1, nextCP2, nextCP3;
    SplitBezier(currentCP, nullptr, &currentCP, t);

    aSink->LineTo(currentCP.mCP1);
  }
}

static inline void
FindInflectionApproximationRange(BezierControlPoints aControlPoints,
                                 Float *aMin, Float *aMax, Float aT,
                                 Float aTolerance)
{
    SplitBezier(aControlPoints, nullptr, &aControlPoints, aT);

    Point cp21 = aControlPoints.mCP2 - aControlPoints.mCP1;
    Point cp41 = aControlPoints.mCP4 - aControlPoints.mCP1;

    if (cp21.x == 0.f && cp21.y == 0.f) {
      

      
      
      *aMin = aT - CubicRoot(abs(aTolerance / (cp41.x - cp41.y)));
      *aMax = aT + CubicRoot(abs(aTolerance / (cp41.x - cp41.y)));
      return;
    }

    Float s3 = (cp41.x * cp21.y - cp41.y * cp21.x) / hypotf(cp21.x, cp21.y);

    if (s3 == 0) {
      
      
      
      *aMin = -1.0f;
      *aMax = 2.0f;
      return;
    }

    Float tf = CubicRoot(abs(aTolerance / s3));

    *aMin = aT - tf * (1 - aT);
    *aMax = aT + tf * (1 - aT);
}




















































static inline void
FindInflectionPoints(const BezierControlPoints &aControlPoints,
                     Float *aT1, Float *aT2, uint32_t *aCount)
{
  
  
  
  Point A = aControlPoints.mCP2 - aControlPoints.mCP1;
  Point B = aControlPoints.mCP3 - (aControlPoints.mCP2 * 2) + aControlPoints.mCP1;
  Point C = aControlPoints.mCP4 - (aControlPoints.mCP3 * 3) + (aControlPoints.mCP2 * 3) - aControlPoints.mCP1;

  Float a = Float(B.x) * C.y - Float(B.y) * C.x;
  Float b = Float(A.x) * C.y - Float(A.y) * C.x;
  Float c = Float(A.x) * B.y - Float(A.y) * B.x;

  if (a == 0) {
    
    if (b == 0) {
      
      
      
      
      
      
      
      if (c == 0) {
        *aCount = 1;
        *aT1 = 0;
        return;
      }
      *aCount = 0;
      return;
    }
    *aT1 = -c / b;
    *aCount = 1;
    return;
  } else {
    Float discriminant = b * b - 4 * a * c;

    if (discriminant < 0) {
      
      *aCount = 0;
    } else if (discriminant == 0) {
      *aCount = 1;
      *aT1 = -b / (2 * a);
    } else {
      





      Float q = sqrtf(discriminant);
      if (b < 0) {
        q = b - q;
      } else {
        q = b + q;
      }
      q *= Float(-1./2);

      *aT1 = q / a;
      *aT2 = c / q;
      if (*aT1 > *aT2) {
        std::swap(*aT1, *aT2);
      }
      *aCount = 2;
    }
  }

  return;
}

void
FlattenBezier(const BezierControlPoints &aControlPoints,
              PathSink *aSink, Float aTolerance)
{
  Float t1;
  Float t2;
  uint32_t count;

  FindInflectionPoints(aControlPoints, &t1, &t2, &count);

  
  if (count == 0 || ((t1 < 0 || t1 > 1.0) && ((t2 < 0 || t2 > 1.0) || count == 1)) ) {
    FlattenBezierCurveSegment(aControlPoints, aSink, aTolerance);
    return;
  }

  Float t1min = t1, t1max = t1, t2min = t2, t2max = t2;

  BezierControlPoints remainingCP = aControlPoints;

  
  
  if (count > 0 && t1 >= 0 && t1 < 1.0) {
    FindInflectionApproximationRange(aControlPoints, &t1min, &t1max, t1, aTolerance);
  }
  if (count > 1 && t2 >= 0 && t2 < 1.0) {
    FindInflectionApproximationRange(aControlPoints, &t2min, &t2max, t2, aTolerance);
  }
  BezierControlPoints nextCPs = aControlPoints;
  BezierControlPoints prevCPs;

  
  
  if (t1min > 0) {
    
    
    SplitBezier(aControlPoints, &prevCPs,
                &remainingCP, t1min);
    FlattenBezierCurveSegment(prevCPs, aSink, aTolerance);
  }
  if (t1max >= 0 && t1max < 1.0 && (count == 1 || t2min > t1max)) {
    
    
    
    SplitBezier(aControlPoints, nullptr, &nextCPs, t1max);

    aSink->LineTo(nextCPs.mCP1);

    if (count == 1 || (count > 1 && t2min >= 1.0)) {
      
      FlattenBezierCurveSegment(nextCPs, aSink, aTolerance);
    }
  } else if (count > 1 && t2min > 1.0) {
    
    
    
    aSink->LineTo(aControlPoints.mCP4);
    return;
  }

  if (count > 1 && t2min < 1.0 && t2max > 0) {
    if (t2min > 0 && t2min < t1max) {
      
      
      SplitBezier(aControlPoints, nullptr, &nextCPs, t1max);
      aSink->LineTo(nextCPs.mCP1);
    } else if (t2min > 0 && t1max > 0) {
      SplitBezier(aControlPoints, nullptr, &nextCPs, t1max);

      
      Float t2mina = (t2min - t1max) / (1 - t1max);
      SplitBezier(nextCPs, &prevCPs, &nextCPs, t2mina);
      FlattenBezierCurveSegment(prevCPs, aSink, aTolerance);
    } else if (t2min > 0) {
      
      SplitBezier(aControlPoints, &prevCPs, &nextCPs, t2min);
      FlattenBezierCurveSegment(prevCPs, aSink, aTolerance);
    }
    if (t2max < 1.0) {
      
      SplitBezier(aControlPoints, nullptr, &nextCPs, t2max);

      
      
      aSink->LineTo(nextCPs.mCP1);
      FlattenBezierCurveSegment(nextCPs, aSink, aTolerance);
    } else {
      
      aSink->LineTo(aControlPoints.mCP4);
      return;
    }
  }
}

}
}
