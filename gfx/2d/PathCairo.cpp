




#include "PathCairo.h"
#include <math.h>
#include "DrawTargetCairo.h"
#include "Logging.h"
#include "PathHelpers.h"
#include "HelpersCairo.h"

namespace mozilla {
namespace gfx {

CairoPathContext::CairoPathContext(cairo_t* aCtx, DrawTargetCairo* aDrawTarget,
                                   FillRule aFillRule,
                                   const Matrix& aTransform )
 : mTransform(aTransform)
 , mContext(aCtx)
 , mDrawTarget(aDrawTarget)
 , mFillRule(aFillRule)
{
  cairo_reference(mContext);
  cairo_set_fill_rule(mContext, GfxFillRuleToCairoFillRule(mFillRule));

  
  
  
  if (mDrawTarget && !mTransform.IsIdentity()) {
    DuplicateContextAndPath(mTransform);

    ForgetDrawTarget();
  } else if (mDrawTarget) {
    mDrawTarget->SetPathObserver(this);
  }
}

CairoPathContext::~CairoPathContext()
{
  if (mDrawTarget) {
    mDrawTarget->SetPathObserver(nullptr);
  }
  cairo_destroy(mContext);
}

void
CairoPathContext::ObserveTarget(DrawTargetCairo* aDrawTarget)
{
  if (!aDrawTarget) {
    return;
  }

  if (mDrawTarget) {
    mDrawTarget->SetPathObserver(nullptr);
  }
  mDrawTarget = aDrawTarget;

  
  
  if (!mTransform.IsIdentity()) {
    ForgetDrawTarget();
    return;
  }

  mDrawTarget->SetPathObserver(this);
}

void
CairoPathContext::DuplicateContextAndPath(const Matrix& aMatrix )
{
  
  cairo_path_t* path = cairo_copy_path(mContext);
  cairo_fill_rule_t rule = cairo_get_fill_rule(mContext);

  
  cairo_surface_t* surf = cairo_get_target(mContext);
  cairo_destroy(mContext);
  mContext = cairo_create(surf);

  
  cairo_matrix_t matrix;
  GfxMatrixToCairoMatrix(aMatrix, matrix);
  cairo_transform(mContext, &matrix);

  
  cairo_append_path(mContext, path);
  cairo_set_fill_rule(mContext, rule);
  cairo_path_destroy(path);
}

void
CairoPathContext::PathWillChange()
{
  
  
  
  
  if (mDrawTarget) {
    
    
    DuplicateContextAndPath();

    ForgetDrawTarget();
  }
}

void
CairoPathContext::MatrixWillChange(const Matrix& aNewMatrix)
{
  
  
  
  
  
  
  
  
  cairo_path_t* path = cairo_copy_path(mContext);

  cairo_matrix_t origMatrix;
  cairo_get_matrix(mContext, &origMatrix);

  cairo_matrix_t newMatrix;
  GfxMatrixToCairoMatrix(aNewMatrix, newMatrix);
  cairo_set_matrix(mContext, &newMatrix);

  cairo_new_path(mContext);
  cairo_append_path(mContext, path);
  cairo_path_destroy(path);

  cairo_set_matrix(mContext, &origMatrix);
}

void
CairoPathContext::CopyPathTo(cairo_t* aToContext)
{
  if (aToContext != mContext) {
    cairo_set_fill_rule(aToContext, GfxFillRuleToCairoFillRule(mFillRule));

    cairo_matrix_t origMat;
    cairo_get_matrix(aToContext, &origMat);

    cairo_matrix_t mat;
    GfxMatrixToCairoMatrix(mTransform, mat);
    cairo_transform(aToContext, &mat);

    
    
    cairo_path_t* path = cairo_copy_path(mContext);
    cairo_new_path(aToContext);
    cairo_append_path(aToContext, path);
    cairo_path_destroy(path);

    cairo_set_matrix(aToContext, &origMat);
  }
}

void
CairoPathContext::ForgetDrawTarget()
{
  mDrawTarget = nullptr;
}

bool
CairoPathContext::ContainsPath(const Path* aPath)
{
  if (aPath->GetBackendType() != BACKEND_CAIRO) {
    return false;
  }

  const PathCairo* path = static_cast<const PathCairo*>(aPath);
  RefPtr<CairoPathContext> ctx = const_cast<PathCairo*>(path)->GetPathContext();
  return ctx == this;
}

PathBuilderCairo::PathBuilderCairo(CairoPathContext* aPathContext,
                                   const Matrix& aTransform )
 : mFillRule(aPathContext->GetFillRule())
{
  RefPtr<DrawTargetCairo> drawTarget = aPathContext->GetDrawTarget();
  mPathContext = new CairoPathContext(*aPathContext, drawTarget, mFillRule,
                                      aPathContext->GetTransform() * aTransform);

  
  
  
  
  
  
  if (!drawTarget) {
    aPathContext->DuplicateContextAndPath();
  }
}

PathBuilderCairo::PathBuilderCairo(cairo_t* aCtx, DrawTargetCairo* aDrawTarget, FillRule aFillRule)
 : mPathContext(new CairoPathContext(aCtx, aDrawTarget, aFillRule))
 , mFillRule(aFillRule)
{}

void
PathBuilderCairo::MoveTo(const Point &aPoint)
{
  cairo_move_to(*mPathContext, aPoint.x, aPoint.y);
}

void
PathBuilderCairo::LineTo(const Point &aPoint)
{
  cairo_line_to(*mPathContext, aPoint.x, aPoint.y);
}

void
PathBuilderCairo::BezierTo(const Point &aCP1,
                           const Point &aCP2,
                           const Point &aCP3)
{
  cairo_curve_to(*mPathContext, aCP1.x, aCP1.y, aCP2.x, aCP2.y, aCP3.x, aCP3.y);
}

void
PathBuilderCairo::QuadraticBezierTo(const Point &aCP1,
                                    const Point &aCP2)
{
  
  
  
  
  Point CP0 = CurrentPoint();
  Point CP1 = (CP0 + aCP1 * 2.0) / 3.0;
  Point CP2 = (aCP2 + aCP1 * 2.0) / 3.0;
  Point CP3 = aCP2;

  cairo_curve_to(*mPathContext, CP1.x, CP1.y, CP2.x, CP2.y, CP3.x, CP3.y);
}

void
PathBuilderCairo::Close()
{
  cairo_close_path(*mPathContext);
}

void
PathBuilderCairo::Arc(const Point &aOrigin, float aRadius, float aStartAngle,
                     float aEndAngle, bool aAntiClockwise)
{
  ArcToBezier(this, aOrigin, aRadius, aStartAngle, aEndAngle, aAntiClockwise);
}

Point
PathBuilderCairo::CurrentPoint() const
{
  double x, y;
  cairo_get_current_point(*mPathContext, &x, &y);
  return Point(x, y);
}

TemporaryRef<Path>
PathBuilderCairo::Finish()
{
  RefPtr<PathCairo> path = new PathCairo(*mPathContext,
                                         mPathContext->GetDrawTarget(),
                                         mFillRule,
                                         mPathContext->GetTransform());
  return path;
}

TemporaryRef<CairoPathContext>
PathBuilderCairo::GetPathContext()
{
  return mPathContext;
}

PathCairo::PathCairo(cairo_t* aCtx, DrawTargetCairo* aDrawTarget, FillRule aFillRule, const Matrix& aTransform)
 : mPathContext(new CairoPathContext(aCtx, aDrawTarget, aFillRule, aTransform))
 , mFillRule(aFillRule)
{}

TemporaryRef<PathBuilder>
PathCairo::CopyToBuilder(FillRule aFillRule) const
{
  
  
  RefPtr<PathBuilderCairo> builder = new PathBuilderCairo(mPathContext);
  return builder;
}

TemporaryRef<PathBuilder>
PathCairo::TransformedCopyToBuilder(const Matrix &aTransform, FillRule aFillRule) const
{
  
  
  RefPtr<PathBuilderCairo> builder = new PathBuilderCairo(mPathContext,
                                                          aTransform);
  return builder;
}

bool
PathCairo::ContainsPoint(const Point &aPoint, const Matrix &aTransform) const
{
  Matrix inverse = aTransform;
  inverse.Invert();
  Point transformed = inverse * aPoint;

  return cairo_in_fill(*mPathContext, transformed.x, transformed.y);
}

Rect
PathCairo::GetBounds(const Matrix &aTransform) const
{
  double x1, y1, x2, y2;

  cairo_path_extents(*mPathContext, &x1, &y1, &x2, &y2);
  Rect bounds(x1, y1, x2 - x1, y2 - y1);
  return aTransform.TransformBounds(bounds);
}

Rect
PathCairo::GetStrokedBounds(const StrokeOptions &aStrokeOptions,
                            const Matrix &aTransform) const
{
  double x1, y1, x2, y2;

  SetCairoStrokeOptions(*mPathContext, aStrokeOptions);

  cairo_stroke_extents(*mPathContext, &x1, &y1, &x2, &y2);
  Rect bounds(x1, y1, x2 - x1, y2 - y1);
  return aTransform.TransformBounds(bounds);
}

TemporaryRef<CairoPathContext>
PathCairo::GetPathContext()
{
  return mPathContext;
}

void
PathCairo::CopyPathTo(cairo_t* aContext, DrawTargetCairo* aDrawTarget)
{
  if (mPathContext->GetContext() != aContext) {
    mPathContext->CopyPathTo(aContext);

    
    
    mPathContext->ObserveTarget(aDrawTarget);
  }
}

}
}
