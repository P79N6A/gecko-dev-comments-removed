




#ifndef MOZILLA_GFX_PATHRECORDING_H_
#define MOZILLA_GFX_PATHRECORDING_H_

#include "2D.h"
#include <vector>
#include <ostream>

namespace mozilla {
namespace gfx {

struct PathOp
{
  enum OpType {
    OP_MOVETO = 0,
    OP_LINETO,
    OP_BEZIERTO,
    OP_QUADRATICBEZIERTO,
    OP_CLOSE
  };

  OpType mType;
  Point mP1;
  Point mP2;
  Point mP3;
};

const int32_t sPointCount[] = { 1, 1, 3, 2, 0, 0 };

class PathRecording;
class DrawEventRecorderPrivate;

class PathBuilderRecording : public PathBuilder
{
public:
  PathBuilderRecording(PathBuilder *aBuilder, FillRule aFillRule)
    : mPathBuilder(aBuilder), mFillRule(aFillRule)
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

  
  virtual void Arc(const Point &, float, float, float, bool) { }

  


  virtual Point CurrentPoint() const;

  virtual TemporaryRef<Path> Finish();

private:
  friend class PathRecording;

  RefPtr<PathBuilder> mPathBuilder;
  FillRule mFillRule;
  std::vector<PathOp> mPathOps;
};

class PathRecording : public Path
{
public:
  PathRecording(Path *aPath, const std::vector<PathOp> aOps, FillRule aFillRule)
    : mPath(aPath), mPathOps(aOps), mFillRule(aFillRule)
  {
  }

  ~PathRecording();

  virtual BackendType GetBackendType() const { return BACKEND_RECORDING; }
  virtual TemporaryRef<PathBuilder> CopyToBuilder(FillRule aFillRule = FILL_WINDING) const;
  virtual TemporaryRef<PathBuilder> TransformedCopyToBuilder(const Matrix &aTransform,
                                                             FillRule aFillRule = FILL_WINDING) const;
  virtual bool ContainsPoint(const Point &aPoint, const Matrix &aTransform) const
  { return mPath->ContainsPoint(aPoint, aTransform); }
  virtual bool StrokeContainsPoint(const StrokeOptions &aStrokeOptions,
                                   const Point &aPoint,
                                   const Matrix &aTransform) const
  { return mPath->StrokeContainsPoint(aStrokeOptions, aPoint, aTransform); }
  
  virtual Rect GetBounds(const Matrix &aTransform = Matrix()) const
  { return mPath->GetBounds(aTransform); }
  
  virtual Rect GetStrokedBounds(const StrokeOptions &aStrokeOptions,
                                const Matrix &aTransform = Matrix()) const
  { return mPath->GetStrokedBounds(aStrokeOptions, aTransform); }
  
  virtual FillRule GetFillRule() const { return mFillRule; }

  void StorePath(std::ostream &aStream) const;
  static void ReadPathToBuilder(std::istream &aStream, PathBuilder *aBuilder);

private:
  friend class DrawTargetRecording;
  friend class RecordedPathCreation;

  RefPtr<Path> mPath;
  std::vector<PathOp> mPathOps;
  FillRule mFillRule;

  
  std::vector<DrawEventRecorderPrivate*> mStoredRecorders;
};

}
}

#endif 
