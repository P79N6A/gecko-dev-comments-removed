




#include "gfxTypes.h"
#include "gfxPattern.h"
#include "gfxASurface.h"
#include "gfxPlatform.h"
#include "gfx2DGlue.h"
#include "gfxGradientCache.h"
#include "mozilla/gfx/2D.h"

#include "cairo.h"

#include <vector>

using namespace mozilla::gfx;

gfxPattern::gfxPattern(const gfxRGBA& aColor)
  : mExtend(EXTEND_NONE)
{
  mGfxPattern.InitColorPattern(Color(aColor.r, aColor.g, aColor.b, aColor.a));
}


gfxPattern::gfxPattern(gfxFloat x0, gfxFloat y0, gfxFloat x1, gfxFloat y1)
  : mExtend(EXTEND_NONE)
{
  mGfxPattern.InitLinearGradientPattern(Point(x0, y0), Point(x1, y1), nullptr);
}


gfxPattern::gfxPattern(gfxFloat cx0, gfxFloat cy0, gfxFloat radius0,
                       gfxFloat cx1, gfxFloat cy1, gfxFloat radius1)
  : mExtend(EXTEND_NONE)
{
  mGfxPattern.InitRadialGradientPattern(Point(cx0, cy0), Point(cx1, cy1),
                                        radius0, radius1, nullptr);
}


gfxPattern::gfxPattern(SourceSurface *aSurface, const Matrix &aPatternToUserSpace)
  : mPatternToUserSpace(aPatternToUserSpace)
  , mExtend(EXTEND_NONE)
{
  mGfxPattern.InitSurfacePattern(aSurface, ToExtendMode(mExtend), Matrix(), 
                                 mozilla::gfx::Filter::GOOD);
}

void
gfxPattern::AddColorStop(gfxFloat offset, const gfxRGBA& c)
{
  if (mGfxPattern.GetPattern()->GetType() != PatternType::LINEAR_GRADIENT &&
      mGfxPattern.GetPattern()->GetType() != PatternType::RADIAL_GRADIENT) {
    return;
  }

  mStops = nullptr;
  gfxRGBA color = c;
  if (gfxPlatform::GetCMSMode() == eCMSMode_All) {
    qcms_transform *transform = gfxPlatform::GetCMSRGBTransform();
    if (transform) {
      gfxPlatform::TransformPixel(color, color, transform);
    }
  }

  GradientStop stop;
  stop.offset = offset;
  stop.color = ToColor(color);
  mStopsList.AppendElement(stop);
}

void
gfxPattern::SetColorStops(GradientStops* aStops)
{
  mStops = aStops;
}

void
gfxPattern::CacheColorStops(DrawTarget *aDT)
{
  mStops = gfxGradientCache::GetOrCreateGradientStops(aDT, mStopsList,
                                                      ToExtendMode(mExtend));
}

void
gfxPattern::SetMatrix(const gfxMatrix& aPatternToUserSpace)
{
  mPatternToUserSpace = ToMatrix(aPatternToUserSpace);
  
  
  
  mPatternToUserSpace.Invert();
}

gfxMatrix
gfxPattern::GetMatrix() const
{
  
  
  gfxMatrix mat = ThebesMatrix(mPatternToUserSpace);
  mat.Invert();
  return mat;
}

gfxMatrix
gfxPattern::GetInverseMatrix() const
{
  return ThebesMatrix(mPatternToUserSpace);
}

Pattern*
gfxPattern::GetPattern(DrawTarget *aTarget,
                       Matrix *aOriginalUserToDevice)
{
  Matrix patternToUser = mPatternToUserSpace;

  if (aOriginalUserToDevice &&
      *aOriginalUserToDevice != aTarget->GetTransform()) {
    
    
    
    
    
    
    
    

    Matrix deviceToCurrentUser = aTarget->GetTransform();
    deviceToCurrentUser.Invert();

    patternToUser = patternToUser * *aOriginalUserToDevice * deviceToCurrentUser;
  }
  patternToUser.NudgeToIntegers();

  if (!mStops &&
      !mStopsList.IsEmpty()) {
    mStops = aTarget->CreateGradientStops(mStopsList.Elements(),
                                          mStopsList.Length(),
                                          ToExtendMode(mExtend));
  }

  switch (mGfxPattern.GetPattern()->GetType()) {
  case PatternType::SURFACE: {
    SurfacePattern* surfacePattern = static_cast<SurfacePattern*>(mGfxPattern.GetPattern());
    surfacePattern->mMatrix = patternToUser;
    surfacePattern->mExtendMode = ToExtendMode(mExtend);
    break;
  }
  case PatternType::LINEAR_GRADIENT: {
    LinearGradientPattern* linearGradientPattern = static_cast<LinearGradientPattern*>(mGfxPattern.GetPattern());
    linearGradientPattern->mMatrix = patternToUser;
    linearGradientPattern->mStops = mStops;
    break;
  }
  case PatternType::RADIAL_GRADIENT: {
    RadialGradientPattern* radialGradientPattern = static_cast<RadialGradientPattern*>(mGfxPattern.GetPattern());
    radialGradientPattern->mMatrix = patternToUser;
    radialGradientPattern->mStops = mStops;
    break;
  }
  default:
    
    break;
  }

  return mGfxPattern.GetPattern();
}

void
gfxPattern::SetExtend(GraphicsExtend extend)
{
  mExtend = extend;
  mStops = nullptr;
}

bool
gfxPattern::IsOpaque()
{
  if (mGfxPattern.GetPattern()->GetType() != PatternType::SURFACE) {
    return false;
  }

  if (static_cast<SurfacePattern*>(mGfxPattern.GetPattern())->mSurface->GetFormat() == SurfaceFormat::B8G8R8X8) {
    return true;
  }
  return false;
}

gfxPattern::GraphicsExtend
gfxPattern::Extend() const
{
  return mExtend;
}

void
gfxPattern::SetFilter(GraphicsFilter filter)
{
  if (mGfxPattern.GetPattern()->GetType() != PatternType::SURFACE) {
    return;
  }

  static_cast<SurfacePattern*>(mGfxPattern.GetPattern())->mFilter = ToFilter(filter);
}

GraphicsFilter
gfxPattern::Filter() const
{
  if (mGfxPattern.GetPattern()->GetType() != PatternType::SURFACE) {
    return GraphicsFilter::FILTER_GOOD;
  }
  return ThebesFilter(static_cast<const SurfacePattern*>(mGfxPattern.GetPattern())->mFilter);
}

bool
gfxPattern::GetSolidColor(gfxRGBA& aColor)
{
  if (mGfxPattern.GetPattern()->GetType() == PatternType::COLOR) {
    aColor = ThebesColor(static_cast<ColorPattern*>(mGfxPattern.GetPattern())->mColor);
    return true;
  }

 return false;
}

gfxPattern::GraphicsPatternType
gfxPattern::GetType() const
{
  return ThebesPatternType(mGfxPattern.GetPattern()->GetType());
}

int
gfxPattern::CairoStatus()
{
  return CAIRO_STATUS_SUCCESS;
}
