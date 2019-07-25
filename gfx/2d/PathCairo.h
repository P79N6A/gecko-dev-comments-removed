




#ifndef MOZILLA_GFX_PATH_CAIRO_H_
#define MOZILLA_GFX_PATH_CAIRO_H_

#include "2D.h"
#include "cairo.h"

namespace mozilla {
namespace gfx {

class DrawTargetCairo;





















class CairoPathContext : public RefCounted<CairoPathContext>
{
public:
  
  
  
  CairoPathContext(cairo_t* aCtx, DrawTargetCairo* aDrawTarget);

  
  CairoPathContext(CairoPathContext& aPathContext);

  ~CairoPathContext();

  
  
  
  void CopyPathTo(cairo_t* aToContext, Matrix& aTransform);

  
  
  void PathWillChange();

  
  
  
  void ForgetDrawTarget();

  
  void DuplicateContextAndPath();

  
  bool ContainsPath(const Path* path);

  cairo_t* GetContext() const { return mContext; }
  DrawTargetCairo* GetDrawTarget() const { return mDrawTarget; }
  operator cairo_t* () const { return mContext; }

private: 
  cairo_t* mContext;
  
  DrawTargetCairo* mDrawTarget;
};

class PathBuilderCairo : public PathBuilder
{
public:
  
  
  
  PathBuilderCairo(cairo_t* aCtx, DrawTargetCairo* aDrawTarget, FillRule aFillRule);

  
  
  PathBuilderCairo(CairoPathContext* aContext, FillRule aFillRule, const Matrix& aTransform = Matrix());

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
  virtual Point CurrentPoint() const;
  virtual TemporaryRef<Path> Finish();

  TemporaryRef<CairoPathContext> GetPathContext();

private: 
  void PrepareForWrite();

  RefPtr<CairoPathContext> mPathContext;
  Matrix mTransform;
  FillRule mFillRule;
};

class PathCairo : public Path
{
public:
  PathCairo(CairoPathContext* aPathContex, Matrix& aTransform, FillRule aFillRule);

  virtual BackendType GetBackendType() const { return BACKEND_CAIRO; }

  virtual TemporaryRef<PathBuilder> CopyToBuilder(FillRule aFillRule = FILL_WINDING) const;
  virtual TemporaryRef<PathBuilder> TransformedCopyToBuilder(const Matrix &aTransform,
                                                             FillRule aFillRule = FILL_WINDING) const;

  virtual bool ContainsPoint(const Point &aPoint, const Matrix &aTransform) const;

  virtual Rect GetBounds(const Matrix &aTransform = Matrix()) const;

  virtual Rect GetStrokedBounds(const StrokeOptions &aStrokeOptions,
                                const Matrix &aTransform = Matrix()) const;

  virtual FillRule GetFillRule() const { return mFillRule; }

  TemporaryRef<CairoPathContext> GetPathContext();

  
  
  
  void CopyPathTo(cairo_t* aContext, DrawTargetCairo* aDrawTarget);

private:
  RefPtr<CairoPathContext> mPathContext;
  Matrix mTransform;
  FillRule mFillRule;
};

}
}

#endif 
