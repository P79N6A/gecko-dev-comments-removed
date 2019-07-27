




#include "2D.h"
#include <vector>

namespace mozilla {
namespace gfx {

struct FlatPathOp
{
  enum OpType {
    OP_MOVETO,
    OP_LINETO,
  };

  OpType mType;
  Point mPoint;
};

class FlattenedPath : public PathSink
{
public:
  MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(FlattenedPath)
  FlattenedPath() : mCachedLength(0)
                  , mCalculatedLength(false)
  {
  }

  virtual void MoveTo(const Point &aPoint);
  virtual void LineTo(const Point &aPoint);
  virtual void BezierTo(const Point &aCP1,
                        const Point &aCP2,
                        const Point &aCP3);
  virtual void QuadraticBezierTo(const Point &aCP1,
                                 const Point &aCP2);
  virtual void Close();
  virtual void Arc(const Point &aOrigin, float aRadius, float aStartAngle,
                   float aEndAngle, bool aAntiClockwise = false);

  virtual Point CurrentPoint() const { return mPathOps.empty() ? Point() : mPathOps[mPathOps.size() - 1].mPoint; }

  Float ComputeLength();
  Point ComputePointAtLength(Float aLength, Point *aTangent);

private:
  Float mCachedLength;
  bool mCalculatedLength;
  Point mLastMove;

  std::vector<FlatPathOp> mPathOps;
};

} 
} 
