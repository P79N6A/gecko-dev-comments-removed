




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
  
  
  CairoPathContext(cairo_t* aCtx, DrawTargetCairo* aDrawTarget,
                   FillRule aFillRule,
                   const Matrix& aMatrix = Matrix());
  ~CairoPathContext();

  
  
  void CopyPathTo(cairo_t* aToContext);

  
  
  void PathWillChange();

  
  
  void MatrixWillChange(const Matrix& aMatrix);

  
  
  
  void ForgetDrawTarget();

  
  
  void DuplicateContextAndPath(const Matrix& aMatrix = Matrix());

  
  bool ContainsPath(const Path* path);

  
  
  void ObserveTarget(DrawTargetCairo* aDrawTarget);

  cairo_t* GetContext() const { return mContext; }
  DrawTargetCairo* GetDrawTarget() const { return mDrawTarget; }
  Matrix GetTransform() const { return mTransform; }
  FillRule GetFillRule() const { return mFillRule; }

  operator cairo_t* () const { return mContext; }

private: 
  CairoPathContext(const CairoPathContext&) MOZ_DELETE;

private: 
  Matrix mTransform;
  cairo_t* mContext;
  
  DrawTargetCairo* mDrawTarget;
  FillRule mFillRule;
};

class PathBuilderCairo : public PathBuilder
{
public:
  
  
  
  
  PathBuilderCairo(cairo_t* aCtx, DrawTargetCairo* aDrawTarget, FillRule aFillRule);

  
  
  
  
  explicit PathBuilderCairo(CairoPathContext* aPathContext,
                            const Matrix& aTransform = Matrix());

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
  void SetFillRule(FillRule aFillRule);

private: 
  RefPtr<CairoPathContext> mPathContext;
  FillRule mFillRule;
};

class PathCairo : public Path
{
public:
  PathCairo(cairo_t* aCtx, DrawTargetCairo* aDrawTarget, FillRule aFillRule, const Matrix& aTransform);

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
  FillRule mFillRule;
};

}
}

#endif 
