




#ifdef _MSC_VER
#define _USE_MATH_DEFINES
#endif
#include <math.h>

#include "mozilla/Alignment.h"

#include "cairo.h"

#include "gfxContext.h"

#include "gfxColor.h"
#include "gfxMatrix.h"
#include "gfxUtils.h"
#include "gfxASurface.h"
#include "gfxPattern.h"
#include "gfxPlatform.h"
#include "gfxTeeSurface.h"
#include "GeckoProfiler.h"
#include "gfx2DGlue.h"
#include "mozilla/gfx/PathHelpers.h"
#include "mozilla/gfx/DrawTargetTiled.h"
#include <algorithm>

#if CAIRO_HAS_DWRITE_FONT
#include "gfxWindowsPlatform.h"
#endif

using namespace mozilla;
using namespace mozilla::gfx;

UserDataKey gfxContext::sDontUseAsSourceKey;


PatternFromState::operator mozilla::gfx::Pattern&()
{
  gfxContext::AzureState &state = mContext->CurrentState();

  if (state.pattern) {
    return *state.pattern->GetPattern(mContext->mDT, state.patternTransformChanged ? &state.patternTransform : nullptr);
  }

  if (state.sourceSurface) {
    Matrix transform = state.surfTransform;

    if (state.patternTransformChanged) {
      Matrix mat = mContext->GetDTTransform();
      if (!mat.Invert()) {
        mPattern = new (mColorPattern.addr())
        ColorPattern(Color()); 
        return *mPattern;
      }
      transform = transform * state.patternTransform * mat;
    }

    mPattern = new (mSurfacePattern.addr())
    SurfacePattern(state.sourceSurface, ExtendMode::CLAMP, transform);
    return *mPattern;
  }

  mPattern = new (mColorPattern.addr())
  ColorPattern(state.color);
  return *mPattern;
}


gfxContext::gfxContext(DrawTarget *aTarget, const Point& aDeviceOffset)
  : mPathIsRect(false)
  , mTransformChanged(false)
  , mRefCairo(nullptr)
  , mDT(aTarget)
  , mOriginalDT(aTarget)
{
  MOZ_ASSERT(aTarget, "Don't create a gfxContext without a DrawTarget");

  MOZ_COUNT_CTOR(gfxContext);

  mStateStack.SetLength(1);
  CurrentState().drawTarget = mDT;
  CurrentState().deviceOffset = aDeviceOffset;
  mDT->SetTransform(Matrix());
}

 already_AddRefed<gfxContext>
gfxContext::ContextForDrawTarget(DrawTarget* aTarget)
{
  Matrix transform = aTarget->GetTransform();
  nsRefPtr<gfxContext> result = new gfxContext(aTarget);
  result->SetMatrix(ThebesMatrix(transform));
  return result.forget();
}

gfxContext::~gfxContext()
{
  if (mRefCairo) {
    cairo_destroy(mRefCairo);
  }
  for (int i = mStateStack.Length() - 1; i >= 0; i--) {
    for (unsigned int c = 0; c < mStateStack[i].pushedClips.Length(); c++) {
      mDT->PopClip();
    }

    if (mStateStack[i].clipWasReset) {
      break;
    }
  }
  mDT->Flush();
  MOZ_COUNT_DTOR(gfxContext);
}

already_AddRefed<gfxASurface>
gfxContext::CurrentSurface(gfxFloat *dx, gfxFloat *dy)
{
  if (mDT->GetBackendType() == BackendType::CAIRO) {
    cairo_surface_t *s =
    (cairo_surface_t*)mDT->GetNativeSurface(NativeSurfaceType::CAIRO_SURFACE);
    if (s) {
      if (dx && dy) {
        *dx = -CurrentState().deviceOffset.x;
        *dy = -CurrentState().deviceOffset.y;
      }
      return gfxASurface::Wrap(s);
    }
  }

  if (dx && dy) {
    *dx = *dy = 0;
  }
  
  return nullptr;
}

cairo_t *
gfxContext::GetCairo()
{
  if (mDT->GetBackendType() == BackendType::CAIRO) {
    cairo_t *ctx =
      (cairo_t*)mDT->GetNativeSurface(NativeSurfaceType::CAIRO_CONTEXT);
    if (ctx) {
      return ctx;
    }
  }

  if (mRefCairo) {
    
    return mRefCairo;
  }

  mRefCairo = cairo_create(gfxPlatform::GetPlatform()->ScreenReferenceSurface()->CairoSurface()); 

  return mRefCairo;
}

void
gfxContext::Save()
{
  CurrentState().transform = mTransform;
  mStateStack.AppendElement(AzureState(CurrentState()));
  CurrentState().clipWasReset = false;
  CurrentState().pushedClips.Clear();
}

void
gfxContext::Restore()
{
  for (unsigned int c = 0; c < CurrentState().pushedClips.Length(); c++) {
    mDT->PopClip();
  }

  if (CurrentState().clipWasReset &&
      CurrentState().drawTarget == mStateStack[mStateStack.Length() - 2].drawTarget) {
    PushClipsToDT(mDT);
  }

  mStateStack.RemoveElementAt(mStateStack.Length() - 1);

  mDT = CurrentState().drawTarget;

  ChangeTransform(CurrentState().transform, false);
}


void
gfxContext::NewPath()
{
  mPath = nullptr;
  mPathBuilder = nullptr;
  mPathIsRect = false;
  mTransformChanged = false;
}

void
gfxContext::ClosePath()
{
  EnsurePathBuilder();
  mPathBuilder->Close();
}

TemporaryRef<Path> gfxContext::GetPath()
{
  EnsurePath();
  return mPath;
}

void gfxContext::SetPath(Path* path)
{
  MOZ_ASSERT(path->GetBackendType() == mDT->GetBackendType() ||
             (mDT->GetBackendType() == BackendType::DIRECT2D1_1 && path->GetBackendType() == BackendType::DIRECT2D));
  mPath = path;
  mPathBuilder = nullptr;
  mPathIsRect = false;
  mTransformChanged = false;
}

gfxPoint
gfxContext::CurrentPoint()
{
  EnsurePathBuilder();
  return ThebesPoint(mPathBuilder->CurrentPoint());
}

void
gfxContext::Stroke()
{
  Stroke(PatternFromState(this));
}

void
gfxContext::Stroke(const Pattern& aPattern)
{
  AzureState &state = CurrentState();
  if (mPathIsRect) {
    MOZ_ASSERT(!mTransformChanged);

    mDT->StrokeRect(mRect, aPattern,
                    state.strokeOptions,
                    DrawOptions(1.0f, GetOp(), state.aaMode));
  } else {
    EnsurePath();

    mDT->Stroke(mPath, aPattern, state.strokeOptions,
                DrawOptions(1.0f, GetOp(), state.aaMode));
  }
}

void
gfxContext::Fill()
{
  Fill(PatternFromState(this));
}

void
gfxContext::Fill(const Pattern& aPattern)
{
  PROFILER_LABEL("gfxContext", "Fill",
    js::ProfileEntry::Category::GRAPHICS);
  FillAzure(aPattern, 1.0f);
}

void
gfxContext::FillWithOpacity(gfxFloat aOpacity)
{
  FillWithOpacity(PatternFromState(this), aOpacity);
}

void
gfxContext::FillWithOpacity(const Pattern& aPattern, gfxFloat aOpacity)
{
  FillAzure(aPattern, Float(aOpacity));
}

void
gfxContext::MoveTo(const gfxPoint& pt)
{
  EnsurePathBuilder();
  mPathBuilder->MoveTo(ToPoint(pt));
}

void
gfxContext::LineTo(const gfxPoint& pt)
{
  EnsurePathBuilder();
  mPathBuilder->LineTo(ToPoint(pt));
}

void
gfxContext::CurveTo(const gfxPoint& pt1, const gfxPoint& pt2, const gfxPoint& pt3)
{
  EnsurePathBuilder();
  mPathBuilder->BezierTo(ToPoint(pt1), ToPoint(pt2), ToPoint(pt3));
}

void
gfxContext::QuadraticCurveTo(const gfxPoint& pt1, const gfxPoint& pt2)
{
  EnsurePathBuilder();
  mPathBuilder->QuadraticBezierTo(ToPoint(pt1), ToPoint(pt2));
}

void
gfxContext::Line(const gfxPoint& start, const gfxPoint& end)
{
  EnsurePathBuilder();
  mPathBuilder->MoveTo(ToPoint(start));
  mPathBuilder->LineTo(ToPoint(end));
}





void
gfxContext::Rectangle(const gfxRect& rect, bool snapToPixels)
{
  Rect rec = ToRect(rect);

  if (snapToPixels) {
    gfxRect newRect(rect);
    if (UserToDevicePixelSnapped(newRect, true)) {
      gfxMatrix mat = ThebesMatrix(mTransform);
      if (mat.Invert()) {
        
        rec = ToRect(mat.TransformBounds(newRect));
      } else {
        rec = Rect();
      }
    }
  }

  if (!mPathBuilder && !mPathIsRect) {
    mPathIsRect = true;
    mRect = rec;
    return;
  }

  EnsurePathBuilder();

  mPathBuilder->MoveTo(rec.TopLeft());
  mPathBuilder->LineTo(rec.TopRight());
  mPathBuilder->LineTo(rec.BottomRight());
  mPathBuilder->LineTo(rec.BottomLeft());
  mPathBuilder->Close();
}

void
gfxContext::Polygon(const gfxPoint *points, uint32_t numPoints)
{
  if (numPoints == 0) {
    return;
  }

  EnsurePathBuilder();

  mPathBuilder->MoveTo(ToPoint(points[0]));
  for (uint32_t i = 1; i < numPoints; i++) {
    mPathBuilder->LineTo(ToPoint(points[i]));
  }
}

void
gfxContext::DrawSurface(gfxASurface *surface, const gfxSize& size)
{
  
  RefPtr<SourceSurface> surf =
    gfxPlatform::GetPlatform()->GetSourceSurfaceForSurface(mDT, surface);

  if (!surf) {
    return;
  }

  Rect rect(0, 0, Float(size.width), Float(size.height));
  rect.Intersect(Rect(0, 0, Float(surf->GetSize().width), Float(surf->GetSize().height)));

  
  mDT->DrawSurface(surf, rect, rect);
}


void
gfxContext::Multiply(const gfxMatrix& matrix)
{
  ChangeTransform(ToMatrix(matrix) * mTransform);
}

void
gfxContext::SetMatrix(const gfxMatrix& matrix)
{
  ChangeTransform(ToMatrix(matrix));
}

gfxMatrix
gfxContext::CurrentMatrix() const
{
  return ThebesMatrix(mTransform);
}

gfxPoint
gfxContext::DeviceToUser(const gfxPoint& point) const
{
  Matrix matrix = mTransform;
  matrix.Invert();
  return ThebesPoint(matrix * ToPoint(point));
}

gfxSize
gfxContext::DeviceToUser(const gfxSize& size) const
{
  Matrix matrix = mTransform;
  matrix.Invert();
  return ThebesSize(matrix * ToSize(size));
}

gfxRect
gfxContext::DeviceToUser(const gfxRect& rect) const
{
  Matrix matrix = mTransform;
  matrix.Invert();
  return ThebesRect(matrix.TransformBounds(ToRect(rect)));
}

gfxPoint
gfxContext::UserToDevice(const gfxPoint& point) const
{
  return ThebesPoint(mTransform * ToPoint(point));
}

gfxSize
gfxContext::UserToDevice(const gfxSize& size) const
{
  const Matrix &matrix = mTransform;

  gfxSize newSize;
  newSize.width = size.width * matrix._11 + size.height * matrix._12;
  newSize.height = size.width * matrix._21 + size.height * matrix._22;
  return newSize;
}

gfxRect
gfxContext::UserToDevice(const gfxRect& rect) const
{
  const Matrix &matrix = mTransform;
  return ThebesRect(matrix.TransformBounds(ToRect(rect)));
}

bool
gfxContext::UserToDevicePixelSnapped(gfxRect& rect, bool ignoreScale) const
{
  if (mDT->GetUserData(&sDisablePixelSnapping))
      return false;

  
  
  
  const gfxFloat epsilon = 0.0000001;
#define WITHIN_E(a,b) (fabs((a)-(b)) < epsilon)
  Matrix mat = mTransform;
  if (!ignoreScale &&
      (!WITHIN_E(mat._11,1.0) || !WITHIN_E(mat._22,1.0) ||
        !WITHIN_E(mat._12,0.0) || !WITHIN_E(mat._21,0.0)))
      return false;
#undef WITHIN_E

  gfxPoint p1 = UserToDevice(rect.TopLeft());
  gfxPoint p2 = UserToDevice(rect.TopRight());
  gfxPoint p3 = UserToDevice(rect.BottomRight());

  
  
  
  
  
  
  if (p2 == gfxPoint(p1.x, p3.y) || p2 == gfxPoint(p3.x, p1.y)) {
      p1.Round();
      p3.Round();

      rect.MoveTo(gfxPoint(std::min(p1.x, p3.x), std::min(p1.y, p3.y)));
      rect.SizeTo(gfxSize(std::max(p1.x, p3.x) - rect.X(),
                          std::max(p1.y, p3.y) - rect.Y()));
      return true;
  }

  return false;
}

bool
gfxContext::UserToDevicePixelSnapped(gfxPoint& pt, bool ignoreScale) const
{
  if (mDT->GetUserData(&sDisablePixelSnapping))
      return false;

  
  
  
  const gfxFloat epsilon = 0.0000001;
#define WITHIN_E(a,b) (fabs((a)-(b)) < epsilon)
  Matrix mat = mTransform;
  if (!ignoreScale &&
      (!WITHIN_E(mat._11,1.0) || !WITHIN_E(mat._22,1.0) ||
        !WITHIN_E(mat._12,0.0) || !WITHIN_E(mat._21,0.0)))
      return false;
#undef WITHIN_E

  pt = UserToDevice(pt);
  pt.Round();
  return true;
}

void
gfxContext::SetAntialiasMode(AntialiasMode mode)
{
  CurrentState().aaMode = mode;
}

AntialiasMode
gfxContext::CurrentAntialiasMode() const
{
  return CurrentState().aaMode;
}

void
gfxContext::SetDash(gfxLineType ltype)
{
  static double dash[] = {5.0, 5.0};
  static double dot[] = {1.0, 1.0};

  switch (ltype) {
      case gfxLineDashed:
          SetDash(dash, 2, 0.0);
          break;
      case gfxLineDotted:
          SetDash(dot, 2, 0.0);
          break;
      case gfxLineSolid:
      default:
          SetDash(nullptr, 0, 0.0);
          break;
  }
}

void
gfxContext::SetDash(gfxFloat *dashes, int ndash, gfxFloat offset)
{
  AzureState &state = CurrentState();

  state.dashPattern.SetLength(ndash);
  for (int i = 0; i < ndash; i++) {
    state.dashPattern[i] = Float(dashes[i]);
  }
  state.strokeOptions.mDashLength = ndash;
  state.strokeOptions.mDashOffset = Float(offset);
  state.strokeOptions.mDashPattern = ndash ? state.dashPattern.Elements()
                                           : nullptr;
}

bool
gfxContext::CurrentDash(FallibleTArray<gfxFloat>& dashes, gfxFloat* offset) const
{
  const AzureState &state = CurrentState();
  int count = state.strokeOptions.mDashLength;

  if (count <= 0 || !dashes.SetLength(count)) {
    return false;
  }

  for (int i = 0; i < count; i++) {
    dashes[i] = state.dashPattern[i];
  }

  *offset = state.strokeOptions.mDashOffset;

  return true;
}

gfxFloat
gfxContext::CurrentDashOffset() const
{
  return CurrentState().strokeOptions.mDashOffset;
}

void
gfxContext::SetLineWidth(gfxFloat width)
{
  CurrentState().strokeOptions.mLineWidth = Float(width);
}

gfxFloat
gfxContext::CurrentLineWidth() const
{
  return CurrentState().strokeOptions.mLineWidth;
}

void
gfxContext::SetOperator(GraphicsOperator op)
{
  if (op == OPERATOR_CLEAR) {
    CurrentState().opIsClear = true;
    return;
  }
  CurrentState().opIsClear = false;
  CurrentState().op = CompositionOpForOp(op);
}

gfxContext::GraphicsOperator
gfxContext::CurrentOperator() const
{
  return ThebesOp(CurrentState().op);
}

void
gfxContext::SetLineCap(GraphicsLineCap cap)
{
  CurrentState().strokeOptions.mLineCap = ToCapStyle(cap);
}

gfxContext::GraphicsLineCap
gfxContext::CurrentLineCap() const
{
  return ThebesLineCap(CurrentState().strokeOptions.mLineCap);
}

void
gfxContext::SetLineJoin(GraphicsLineJoin join)
{
  CurrentState().strokeOptions.mLineJoin = ToJoinStyle(join);
}

gfxContext::GraphicsLineJoin
gfxContext::CurrentLineJoin() const
{
  return ThebesLineJoin(CurrentState().strokeOptions.mLineJoin);
}

void
gfxContext::SetMiterLimit(gfxFloat limit)
{
  CurrentState().strokeOptions.mMiterLimit = Float(limit);
}

gfxFloat
gfxContext::CurrentMiterLimit() const
{
  return CurrentState().strokeOptions.mMiterLimit;
}

void
gfxContext::SetFillRule(FillRule rule)
{
  CurrentState().fillRule = rule;
}

FillRule
gfxContext::CurrentFillRule() const
{
  return CurrentState().fillRule;
}


void
gfxContext::Clip(const Rect& rect)
{
  AzureState::PushedClip clip = { nullptr, rect, mTransform };
  CurrentState().pushedClips.AppendElement(clip);
  mDT->PushClipRect(rect);
  NewPath();
}

void
gfxContext::Clip(const gfxRect& rect)
{
  Clip(ToRect(rect));
}

void
gfxContext::Clip()
{
  if (mPathIsRect) {
    MOZ_ASSERT(!mTransformChanged);

    AzureState::PushedClip clip = { nullptr, mRect, mTransform };
    CurrentState().pushedClips.AppendElement(clip);
    mDT->PushClipRect(mRect);
  } else {
    EnsurePath();
    mDT->PushClip(mPath);
    AzureState::PushedClip clip = { mPath, Rect(), mTransform };
    CurrentState().pushedClips.AppendElement(clip);
  }
}

void
gfxContext::ResetClip()
{
  for (int i = mStateStack.Length() - 1; i >= 0; i--) {
    for (unsigned int c = 0; c < mStateStack[i].pushedClips.Length(); c++) {
      mDT->PopClip();
    }

    if (mStateStack[i].clipWasReset) {
      break;
    }
  }
  CurrentState().pushedClips.Clear();
  CurrentState().clipWasReset = true;
}

void
gfxContext::UpdateSurfaceClip()
{
}

gfxRect
gfxContext::GetClipExtents()
{
  Rect rect = GetAzureDeviceSpaceClipBounds();

  if (rect.width == 0 || rect.height == 0) {
    return gfxRect(0, 0, 0, 0);
  }

  Matrix mat = mTransform;
  mat.Invert();
  rect = mat.TransformBounds(rect);

  return ThebesRect(rect);
}

bool
gfxContext::ClipContainsRect(const gfxRect& aRect)
{
  unsigned int lastReset = 0;
  for (int i = mStateStack.Length() - 2; i > 0; i--) {
    if (mStateStack[i].clipWasReset) {
      lastReset = i;
      break;
    }
  }

  
  
  
  Rect clipBounds(0, 0, Float(mDT->GetSize().width), Float(mDT->GetSize().height));

  for (unsigned int i = lastReset; i < mStateStack.Length(); i++) {
    for (unsigned int c = 0; c < mStateStack[i].pushedClips.Length(); c++) {
      AzureState::PushedClip &clip = mStateStack[i].pushedClips[c];
      if (clip.path || !clip.transform.IsRectilinear()) {
        
        
        return false;
      } else {
        Rect clipRect = mTransform.TransformBounds(clip.rect);

        clipBounds.IntersectRect(clipBounds, clipRect);
      }
    }
  }

  return clipBounds.Contains(ToRect(aRect));
}



void
gfxContext::SetColor(const gfxRGBA& c)
{
  CurrentState().pattern = nullptr;
  CurrentState().sourceSurfCairo = nullptr;
  CurrentState().sourceSurface = nullptr;
  CurrentState().color = ToDeviceColor(c);
}

void
gfxContext::SetDeviceColor(const gfxRGBA& c)
{
  CurrentState().pattern = nullptr;
  CurrentState().sourceSurfCairo = nullptr;
  CurrentState().sourceSurface = nullptr;
  CurrentState().color = ToColor(c);
}

bool
gfxContext::GetDeviceColor(gfxRGBA& c)
{
  if (CurrentState().sourceSurface) {
    return false;
  }
  if (CurrentState().pattern) {
    gfxRGBA color;
    return CurrentState().pattern->GetSolidColor(c);
  }

  c = ThebesRGBA(CurrentState().color);
  return true;
}

void
gfxContext::SetSource(gfxASurface *surface, const gfxPoint& offset)
{
  CurrentState().surfTransform = Matrix(1.0f, 0, 0, 1.0f, Float(offset.x), Float(offset.y));
  CurrentState().pattern = nullptr;
  CurrentState().patternTransformChanged = false;
  
  
  CurrentState().sourceSurfCairo = surface;
  CurrentState().sourceSurface =
  gfxPlatform::GetPlatform()->GetSourceSurfaceForSurface(mDT, surface);
  CurrentState().color = Color(0, 0, 0, 0);
}

void
gfxContext::SetPattern(gfxPattern *pattern)
{
  CurrentState().sourceSurfCairo = nullptr;
  CurrentState().sourceSurface = nullptr;
  CurrentState().patternTransformChanged = false;
  CurrentState().pattern = pattern;
}

already_AddRefed<gfxPattern>
gfxContext::GetPattern()
{
  nsRefPtr<gfxPattern> pat;

  AzureState &state = CurrentState();
  if (state.pattern) {
    pat = state.pattern;
  } else if (state.sourceSurface) {
    NS_ASSERTION(false, "Ugh, this isn't good.");
  } else {
    pat = new gfxPattern(ThebesRGBA(state.color));
  }
  return pat.forget();
}

void
gfxContext::SetFontSmoothingBackgroundColor(const Color& aColor)
{
  CurrentState().fontSmoothingBackgroundColor = aColor;
}

Color
gfxContext::GetFontSmoothingBackgroundColor()
{
  return CurrentState().fontSmoothingBackgroundColor;
}


void
gfxContext::Mask(SourceSurface* aSurface, const Matrix& aTransform)
{
  Matrix old = mTransform;
  Matrix mat = aTransform * mTransform;

  ChangeTransform(mat);
  mDT->MaskSurface(PatternFromState(this), aSurface, Point(),
                   DrawOptions(1.0f, CurrentState().op, CurrentState().aaMode));
  ChangeTransform(old);
}

void
gfxContext::Mask(gfxASurface *surface, const gfxPoint& offset)
{
  PROFILER_LABEL("gfxContext", "Mask",
    js::ProfileEntry::Category::GRAPHICS);

  
  RefPtr<SourceSurface> sourceSurf =
  gfxPlatform::GetPlatform()->GetSourceSurfaceForSurface(mDT, surface);

  if (!sourceSurf) {
    return;
  }

  gfxPoint pt = surface->GetDeviceOffset();

  Mask(sourceSurf, Point(offset.x - pt.x, offset.y - pt.y));
}

void
gfxContext::Mask(SourceSurface *surface, const Point& offset)
{
  
  mDT->MaskSurface(PatternFromState(this),
            surface,
            offset,
            DrawOptions(1.0f, CurrentState().op, CurrentState().aaMode));
}

void
gfxContext::Paint(gfxFloat alpha)
{
  PROFILER_LABEL("gfxContext", "Paint",
    js::ProfileEntry::Category::GRAPHICS);

  AzureState &state = CurrentState();

  if (state.sourceSurface && !state.sourceSurfCairo &&
      !state.patternTransformChanged && !state.opIsClear)
  {
    
    
    Matrix oldMat = mDT->GetTransform();

    IntSize surfSize = state.sourceSurface->GetSize();

    mDT->SetTransform(Matrix::Translation(-state.deviceOffset.x,
                                          -state.deviceOffset.y));

    mDT->DrawSurface(state.sourceSurface,
                     Rect(state.sourceSurfaceDeviceOffset, Size(surfSize.width, surfSize.height)),
                     Rect(Point(), Size(surfSize.width, surfSize.height)),
                     DrawSurfaceOptions(), DrawOptions(alpha, GetOp()));
    mDT->SetTransform(oldMat);
    return;
  }

  Matrix mat = mDT->GetTransform();
  mat.Invert();
  Rect paintRect = mat.TransformBounds(Rect(Point(0, 0), Size(mDT->GetSize())));

  if (state.opIsClear) {
    mDT->ClearRect(paintRect);
  } else {
    mDT->FillRect(paintRect, PatternFromState(this),
                  DrawOptions(Float(alpha), GetOp()));
  }
}



void
gfxContext::PushGroup(gfxContentType content)
{
  DrawTarget* oldDT = mDT;

  PushNewDT(content);

  if (oldDT != mDT) {
    PushClipsToDT(mDT);
  }
  mDT->SetTransform(GetDTTransform());
}

static gfxRect
GetRoundOutDeviceClipExtents(gfxContext* aCtx)
{
  gfxContextMatrixAutoSaveRestore save(aCtx);
  aCtx->SetMatrix(gfxMatrix());
  gfxRect r = aCtx->GetClipExtents();
  r.RoundOut();
  return r;
}

void
gfxContext::PushGroupAndCopyBackground(gfxContentType content)
{
  IntRect clipExtents;
  if (mDT->GetFormat() != SurfaceFormat::B8G8R8X8) {
    gfxRect clipRect = GetRoundOutDeviceClipExtents(this);
    clipExtents = IntRect(clipRect.x, clipRect.y, clipRect.width, clipRect.height);
  }
  if ((mDT->GetFormat() == SurfaceFormat::B8G8R8X8 ||
       mDT->GetOpaqueRect().Contains(clipExtents)) &&
      !mDT->GetUserData(&sDontUseAsSourceKey)) {
    DrawTarget *oldDT = mDT;
    RefPtr<SourceSurface> source = mDT->Snapshot();
    Point oldDeviceOffset = CurrentState().deviceOffset;

    PushNewDT(gfxContentType::COLOR);

    if (oldDT == mDT) {
      
      return;
    }

    Point offset = CurrentState().deviceOffset - oldDeviceOffset;
    Rect surfRect(0, 0, Float(mDT->GetSize().width), Float(mDT->GetSize().height));
    Rect sourceRect = surfRect + offset;

    mDT->SetTransform(Matrix());

    
    
    
    if (source->GetType() == SurfaceType::TILED) {
      SnapshotTiled *sourceTiled = static_cast<SnapshotTiled*>(source.get());
      for (uint32_t i = 0; i < sourceTiled->mSnapshots.size(); i++) {
        Rect tileSourceRect = sourceRect.Intersect(Rect(sourceTiled->mOrigins[i].x,
                                                        sourceTiled->mOrigins[i].y,
                                                        sourceTiled->mSnapshots[i]->GetSize().width,
                                                        sourceTiled->mSnapshots[i]->GetSize().height));

        if (tileSourceRect.IsEmpty()) {
          continue;
        }
        Rect tileDestRect = tileSourceRect - offset;
        tileSourceRect -= sourceTiled->mOrigins[i];

        mDT->DrawSurface(sourceTiled->mSnapshots[i], tileDestRect, tileSourceRect);
      }
    } else {
      mDT->DrawSurface(source, surfRect, sourceRect);
    }
    mDT->SetOpaqueRect(oldDT->GetOpaqueRect());

    PushClipsToDT(mDT);
    mDT->SetTransform(GetDTTransform());
    return;
  }
  PushGroup(content);
}

already_AddRefed<gfxPattern>
gfxContext::PopGroup()
{
  RefPtr<SourceSurface> src = mDT->Snapshot();
  Point deviceOffset = CurrentState().deviceOffset;

  Restore();

  Matrix mat = mTransform;
  mat.Invert();
  mat.PreTranslate(deviceOffset.x, deviceOffset.y); 

  nsRefPtr<gfxPattern> pat = new gfxPattern(src, mat);

  return pat.forget();
}

TemporaryRef<SourceSurface>
gfxContext::PopGroupToSurface(Matrix* aTransform)
{
  RefPtr<SourceSurface> src = mDT->Snapshot();
  Point deviceOffset = CurrentState().deviceOffset;

  Restore();

  Matrix mat = mTransform;
  mat.Invert();

  Matrix deviceOffsetTranslation;
  deviceOffsetTranslation.PreTranslate(deviceOffset.x, deviceOffset.y);

  *aTransform = deviceOffsetTranslation * mat;
  return src;
}

void
gfxContext::PopGroupToSource()
{
  RefPtr<SourceSurface> src = mDT->Snapshot();
  Point deviceOffset = CurrentState().deviceOffset;
  Restore();
  CurrentState().sourceSurfCairo = nullptr;
  CurrentState().sourceSurface = src;
  CurrentState().sourceSurfaceDeviceOffset = deviceOffset;
  CurrentState().pattern = nullptr;
  CurrentState().patternTransformChanged = false;

  Matrix mat = mTransform;
  mat.Invert();
  mat.PreTranslate(deviceOffset.x, deviceOffset.y); 

  CurrentState().surfTransform = mat;
}

void
gfxContext::RoundedRectangle(const gfxRect& rect,
                             const gfxCornerSizes& corners,
                             bool draw_clockwise)
{
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

  EnsurePathBuilder();
  RectCornerRadii radii(ToSize(corners[NS_CORNER_TOP_LEFT]),
                        ToSize(corners[NS_CORNER_TOP_RIGHT]),
                        ToSize(corners[NS_CORNER_BOTTOM_RIGHT]),
                        ToSize(corners[NS_CORNER_BOTTOM_LEFT]));
  AppendRoundedRectToPath(mPathBuilder, ToRect(rect), radii, draw_clockwise);
}

#ifdef MOZ_DUMP_PAINTING
void
gfxContext::WriteAsPNG(const char* aFile)
{
  gfxUtils::WriteAsPNG(mDT, aFile);
}

void 
gfxContext::DumpAsDataURI()
{
  gfxUtils::DumpAsDataURI(mDT);
}

void 
gfxContext::CopyAsDataURI()
{
  gfxUtils::CopyAsDataURI(mDT);
}
#endif

void
gfxContext::EnsurePath()
{
  if (mPathBuilder) {
    mPath = mPathBuilder->Finish();
    mPathBuilder = nullptr;
  }

  if (mPath) {
    if (mTransformChanged) {
      Matrix mat = mTransform;
      mat.Invert();
      mat = mPathTransform * mat;
      mPathBuilder = mPath->TransformedCopyToBuilder(mat, CurrentState().fillRule);
      mPath = mPathBuilder->Finish();
      mPathBuilder = nullptr;

      mTransformChanged = false;
    }

    if (CurrentState().fillRule == mPath->GetFillRule()) {
      return;
    }

    mPathBuilder = mPath->CopyToBuilder(CurrentState().fillRule);

    mPath = mPathBuilder->Finish();
    mPathBuilder = nullptr;
    return;
  }

  EnsurePathBuilder();
  mPath = mPathBuilder->Finish();
  mPathBuilder = nullptr;
}

void
gfxContext::EnsurePathBuilder()
{
  if (mPathBuilder && !mTransformChanged) {
    return;
  }

  if (mPath) {
    if (!mTransformChanged) {
      mPathBuilder = mPath->CopyToBuilder(CurrentState().fillRule);
      mPath = nullptr;
    } else {
      Matrix invTransform = mTransform;
      invTransform.Invert();
      Matrix toNewUS = mPathTransform * invTransform;
      mPathBuilder = mPath->TransformedCopyToBuilder(toNewUS, CurrentState().fillRule);
    }
    return;
  }

  DebugOnly<PathBuilder*> oldPath = mPathBuilder.get();

  if (!mPathBuilder) {
    mPathBuilder = mDT->CreatePathBuilder(CurrentState().fillRule);

    if (mPathIsRect) {
      mPathBuilder->MoveTo(mRect.TopLeft());
      mPathBuilder->LineTo(mRect.TopRight());
      mPathBuilder->LineTo(mRect.BottomRight());
      mPathBuilder->LineTo(mRect.BottomLeft());
      mPathBuilder->Close();
    }
  }

  if (mTransformChanged) {
    
    
    
    MOZ_ASSERT(oldPath);
    MOZ_ASSERT(!mPathIsRect);

    Matrix invTransform = mTransform;
    invTransform.Invert();
    Matrix toNewUS = mPathTransform * invTransform;

    RefPtr<Path> path = mPathBuilder->Finish();
    mPathBuilder = path->TransformedCopyToBuilder(toNewUS, CurrentState().fillRule);
  }

  mPathIsRect = false;
}

void
gfxContext::FillAzure(const Pattern& aPattern, Float aOpacity)
{
  AzureState &state = CurrentState();

  CompositionOp op = GetOp();

  if (mPathIsRect) {
    MOZ_ASSERT(!mTransformChanged);

    if (state.opIsClear) {
      mDT->ClearRect(mRect);
    } else if (op == CompositionOp::OP_SOURCE) {
      
      mDT->ClearRect(mRect);
      mDT->FillRect(mRect, aPattern, DrawOptions(aOpacity));
    } else {
      mDT->FillRect(mRect, aPattern, DrawOptions(aOpacity, op, state.aaMode));
    }
  } else {
    EnsurePath();

    NS_ASSERTION(!state.opIsClear, "We shouldn't be clearing complex paths!");

    mDT->Fill(mPath, aPattern, DrawOptions(aOpacity, op, state.aaMode));
  }
}

void
gfxContext::PushClipsToDT(DrawTarget *aDT)
{
  
  
  
  unsigned int lastReset = 0;
  for (int i = mStateStack.Length() - 2; i > 0; i--) {
    if (mStateStack[i].clipWasReset) {
      lastReset = i;
      break;
    }
  }

  

  
  
  for (unsigned int i = lastReset; i < mStateStack.Length() - 1; i++) {
    for (unsigned int c = 0; c < mStateStack[i].pushedClips.Length(); c++) {
      aDT->SetTransform(mStateStack[i].pushedClips[c].transform * GetDeviceTransform());
      if (mStateStack[i].pushedClips[c].path) {
        aDT->PushClip(mStateStack[i].pushedClips[c].path);
      } else {
        aDT->PushClipRect(mStateStack[i].pushedClips[c].rect);
      }
    }
  }
}

CompositionOp
gfxContext::GetOp()
{
  if (CurrentState().op != CompositionOp::OP_SOURCE) {
    return CurrentState().op;
  }

  AzureState &state = CurrentState();
  if (state.pattern) {
    if (state.pattern->IsOpaque()) {
      return CompositionOp::OP_OVER;
    } else {
      return CompositionOp::OP_SOURCE;
    }
  } else if (state.sourceSurface) {
    if (state.sourceSurface->GetFormat() == SurfaceFormat::B8G8R8X8) {
      return CompositionOp::OP_OVER;
    } else {
      return CompositionOp::OP_SOURCE;
    }
  } else {
    if (state.color.a > 0.999) {
      return CompositionOp::OP_OVER;
    } else {
      return CompositionOp::OP_SOURCE;
    }
  }
}












void
gfxContext::ChangeTransform(const Matrix &aNewMatrix, bool aUpdatePatternTransform)
{
  AzureState &state = CurrentState();

  if (aUpdatePatternTransform && (state.pattern || state.sourceSurface)
      && !state.patternTransformChanged) {
    state.patternTransform = GetDTTransform();
    state.patternTransformChanged = true;
  }

  if (mPathIsRect) {
    Matrix invMatrix = aNewMatrix;
    
    invMatrix.Invert();

    Matrix toNewUS = mTransform * invMatrix;

    if (toNewUS.IsRectilinear()) {
      mRect = toNewUS.TransformBounds(mRect);
      mRect.NudgeToIntegers();
    } else {
      mPathBuilder = mDT->CreatePathBuilder(CurrentState().fillRule);
      
      mPathBuilder->MoveTo(toNewUS * mRect.TopLeft());
      mPathBuilder->LineTo(toNewUS * mRect.TopRight());
      mPathBuilder->LineTo(toNewUS * mRect.BottomRight());
      mPathBuilder->LineTo(toNewUS * mRect.BottomLeft());
      mPathBuilder->Close();

      mPathIsRect = false;
    }

    
    mTransformChanged = false;
  } else if ((mPath || mPathBuilder) && !mTransformChanged) {
    mTransformChanged = true;
    mPathTransform = mTransform;
  }

  mTransform = aNewMatrix;

  mDT->SetTransform(GetDTTransform());
}

Rect
gfxContext::GetAzureDeviceSpaceClipBounds()
{
  unsigned int lastReset = 0;
  for (int i = mStateStack.Length() - 1; i > 0; i--) {
    if (mStateStack[i].clipWasReset) {
      lastReset = i;
      break;
    }
  }

  Rect rect(CurrentState().deviceOffset.x, CurrentState().deviceOffset.y,
            Float(mDT->GetSize().width), Float(mDT->GetSize().height));
  for (unsigned int i = lastReset; i < mStateStack.Length(); i++) {
    for (unsigned int c = 0; c < mStateStack[i].pushedClips.Length(); c++) {
      AzureState::PushedClip &clip = mStateStack[i].pushedClips[c];
      if (clip.path) {
        Rect bounds = clip.path->GetBounds(clip.transform);
        rect.IntersectRect(rect, bounds);
      } else {
        rect.IntersectRect(rect, clip.transform.TransformBounds(clip.rect));
      }
    }
  }

  return rect;
}

Point
gfxContext::GetDeviceOffset() const
{
  return CurrentState().deviceOffset;
}

Matrix
gfxContext::GetDeviceTransform() const
{
  return Matrix::Translation(-CurrentState().deviceOffset.x,
                             -CurrentState().deviceOffset.y);
}

Matrix
gfxContext::GetDTTransform() const
{
  Matrix mat = mTransform;
  mat._31 -= CurrentState().deviceOffset.x;
  mat._32 -= CurrentState().deviceOffset.y;
  return mat;
}

void
gfxContext::PushNewDT(gfxContentType content)
{
  Rect clipBounds = GetAzureDeviceSpaceClipBounds();
  clipBounds.RoundOut();

  clipBounds.width = std::max(1.0f, clipBounds.width);
  clipBounds.height = std::max(1.0f, clipBounds.height);

  SurfaceFormat format = gfxPlatform::GetPlatform()->Optimal2DFormatForContent(content);

  RefPtr<DrawTarget> newDT =
    mDT->CreateSimilarDrawTarget(IntSize(int32_t(clipBounds.width), int32_t(clipBounds.height)),
                                 format);

  if (!newDT) {
    NS_WARNING("Failed to create DrawTarget of sufficient size.");
    newDT = mDT->CreateSimilarDrawTarget(IntSize(64, 64), format);

    if (!newDT) {
      if (!gfxPlatform::GetPlatform()->DidRenderingDeviceReset()) {
        
        NS_ABORT_OOM(BytesPerPixel(format) * 64 * 64);
      }
      newDT = CurrentState().drawTarget;
    }
  }

  Save();

  CurrentState().drawTarget = newDT;
  CurrentState().deviceOffset = clipBounds.TopLeft();

  mDT = newDT;
}










void
gfxContext::GetRoundOffsetsToPixels(bool *aRoundX, bool *aRoundY)
{
    *aRoundX = false;
    
    
    
    
    if (CurrentMatrix().HasNonTranslation()) {
        *aRoundY = false;
        return;
    }

    
    
    *aRoundY = true;

    cairo_t *cr = GetCairo();
    cairo_scaled_font_t *scaled_font = cairo_get_scaled_font(cr);
    
    cairo_font_options_t *font_options = cairo_font_options_create();
    cairo_scaled_font_get_font_options(scaled_font, font_options);
    cairo_hint_metrics_t hint_metrics =
        cairo_font_options_get_hint_metrics(font_options);
    cairo_font_options_destroy(font_options);

    switch (hint_metrics) {
    case CAIRO_HINT_METRICS_OFF:
        *aRoundY = false;
        return;
    case CAIRO_HINT_METRICS_DEFAULT:
        
        
        
        
        
        switch (cairo_scaled_font_get_type(scaled_font)) {
#if CAIRO_HAS_DWRITE_FONT 
        case CAIRO_FONT_TYPE_DWRITE:
            
            
            
            if (!cairo_dwrite_scaled_font_get_force_GDI_classic(scaled_font) &&
                gfxWindowsPlatform::GetPlatform()->DWriteMeasuringMode() ==
                    DWRITE_MEASURING_MODE_NATURAL) {
                return;
            }
#endif
        case CAIRO_FONT_TYPE_QUARTZ:
            
            if (cairo_surface_get_type(cairo_get_target(cr)) ==
                CAIRO_SURFACE_TYPE_QUARTZ) {
                return;
            }
        default:
            break;
        }
        
    case CAIRO_HINT_METRICS_ON:
        break;
    }
    *aRoundX = true;
    return;
}
