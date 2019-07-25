




#include "PathCairo.h"
#include <math.h>
#include "DrawTargetCairo.h"
#include "Logging.h"
#include "PathHelpers.h"
#include "HelpersCairo.h"

namespace mozilla {
namespace gfx {

CairoPathContext::CairoPathContext(cairo_t* aCtx, DrawTargetCairo* aDrawTarget)
 : mContext(aCtx)
 , mDrawTarget(aDrawTarget)
{
  cairo_reference(mContext);

  
  aDrawTarget->SetPathObserver(this);
  cairo_new_path(mContext);
}

CairoPathContext::CairoPathContext(CairoPathContext& aPathContext)
 : mContext(aPathContext.mContext)
 , mDrawTarget(nullptr)
{
  cairo_reference(mContext);
  DuplicateContextAndPath();
}

CairoPathContext::~CairoPathContext()
{
  if (mDrawTarget) {
    DrawTargetCairo* drawTarget = mDrawTarget;
    ForgetDrawTarget();

    
    
    drawTarget->SetPathObserver(nullptr);
  }
  cairo_destroy(mContext);
}

void
CairoPathContext::DuplicateContextAndPath()
{
  
  cairo_path_t* path = cairo_copy_path(mContext);
  cairo_fill_rule_t rule = cairo_get_fill_rule(mContext);

  
  cairo_surface_t* surf = cairo_get_target(mContext);
  cairo_matrix_t matrix;
  cairo_get_matrix(mContext, &matrix);
  cairo_destroy(mContext);

  mContext = cairo_create(surf);

  
  
  
  cairo_set_matrix(mContext, &matrix);

  
  cairo_append_path(mContext, path);
  cairo_path_destroy(path);
}

void
CairoPathContext::ForgetDrawTarget()
{
  
  
  
  mDrawTarget = nullptr;
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
CairoPathContext::CopyPathTo(cairo_t* aToContext, Matrix& aTransform)
{
  if (aToContext != mContext) {
    CairoTempMatrix tempMatrix(mContext, aTransform);
    cairo_path_t* path = cairo_copy_path(mContext);
    cairo_new_path(aToContext);
    cairo_append_path(aToContext, path);
    cairo_path_destroy(path);
  }
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
                                   FillRule aFillRule,
                                   const Matrix& aTransform )
 : mPathContext(aPathContext)
 , mTransform(aTransform)
 , mFillRule(aFillRule)
{}

PathBuilderCairo::PathBuilderCairo(cairo_t* aCtx, DrawTargetCairo* aDrawTarget, FillRule aFillRule)
 : mPathContext(new CairoPathContext(aCtx, aDrawTarget))
 , mTransform(aDrawTarget->GetTransform())
 , mFillRule(aFillRule)
{}

void
PathBuilderCairo::MoveTo(const Point &aPoint)
{
  PrepareForWrite();
  CairoTempMatrix tempMatrix(*mPathContext, mTransform);
  cairo_move_to(*mPathContext, aPoint.x, aPoint.y);
}

void
PathBuilderCairo::LineTo(const Point &aPoint)
{
  PrepareForWrite();
  CairoTempMatrix tempMatrix(*mPathContext, mTransform);
  cairo_line_to(*mPathContext, aPoint.x, aPoint.y);
}

void
PathBuilderCairo::BezierTo(const Point &aCP1,
                           const Point &aCP2,
                           const Point &aCP3)
{
  PrepareForWrite();
  CairoTempMatrix tempMatrix(*mPathContext, mTransform);
  cairo_curve_to(*mPathContext, aCP1.x, aCP1.y, aCP2.x, aCP2.y, aCP3.x, aCP3.y);
}

void
PathBuilderCairo::QuadraticBezierTo(const Point &aCP1,
                                    const Point &aCP2)
{
  PrepareForWrite();
  CairoTempMatrix tempMatrix(*mPathContext, mTransform);

  
  
  
  
  Point CP0 = CurrentPoint();
  Point CP1 = (CP0 + aCP1 * 2.0) / 3.0;
  Point CP2 = (aCP2 + aCP1 * 2.0) / 3.0;
  Point CP3 = aCP2;

  cairo_curve_to(*mPathContext, CP1.x, CP1.y, CP2.x, CP2.y, CP3.x, CP3.y);
}

void
PathBuilderCairo::Close()
{
  PrepareForWrite();
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
  CairoTempMatrix tempMatrix(*mPathContext, mTransform);
  double x, y;
  cairo_get_current_point(*mPathContext, &x, &y);
  return Point(x, y);
}

TemporaryRef<Path>
PathBuilderCairo::Finish()
{
  return new PathCairo(mPathContext, mTransform, mFillRule);
}

TemporaryRef<CairoPathContext>
PathBuilderCairo::GetPathContext()
{
  return mPathContext;
}

void
PathBuilderCairo::PrepareForWrite()
{
  
  
  
  
  if (mPathContext->refCount() != 1) {
    mPathContext = new CairoPathContext(*mPathContext);
  }
}

PathCairo::PathCairo(CairoPathContext* aPathContext, Matrix& aTransform,
                     FillRule aFillRule)
 : mPathContext(aPathContext)
 , mTransform(aTransform)
 , mFillRule(aFillRule)
{}

TemporaryRef<PathBuilder>
PathCairo::CopyToBuilder(FillRule aFillRule) const
{
  return new PathBuilderCairo(mPathContext, aFillRule, mTransform);
}

TemporaryRef<PathBuilder>
PathCairo::TransformedCopyToBuilder(const Matrix &aTransform, FillRule aFillRule) const
{
  
  
  
  
  Matrix inverse = aTransform;
  inverse.Invert();

  return new PathBuilderCairo(mPathContext, aFillRule, mTransform * inverse);
}

bool
PathCairo::ContainsPoint(const Point &aPoint, const Matrix &aTransform) const
{
  CairoTempMatrix(*mPathContext, mTransform);

  Matrix inverse = aTransform;
  inverse.Invert();
  Point transformed = inverse * aPoint;

  
  cairo_set_fill_rule(*mPathContext, GfxFillRuleToCairoFillRule(mFillRule));
  return cairo_in_fill(*mPathContext, transformed.x, transformed.y);
}

Rect
PathCairo::GetBounds(const Matrix &aTransform) const
{
  CairoTempMatrix(*mPathContext, mTransform);

  double x1, y1, x2, y2;

  cairo_path_extents(*mPathContext, &x1, &y1, &x2, &y2);
  Rect bounds(x1, y1, x2 - x1, y2 - y1);
  return aTransform.TransformBounds(bounds);
}

Rect
PathCairo::GetStrokedBounds(const StrokeOptions &aStrokeOptions,
                            const Matrix &aTransform) const
{
  CairoTempMatrix(*mPathContext, mTransform);

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
  mPathContext->CopyPathTo(aContext, mTransform);
  cairo_set_fill_rule(aContext, GfxFillRuleToCairoFillRule(mFillRule));
}

}
}
