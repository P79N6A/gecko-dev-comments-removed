




#include "gfxTypes.h"
#include "gfxPattern.h"
#include "gfxASurface.h"
#include "gfxPlatform.h"
#include "gfx2DGlue.h"
#include "gfxGradientCache.h"

#include "cairo.h"

#include <vector>

using namespace mozilla::gfx;

gfxPattern::gfxPattern(const gfxRGBA& aColor)
  : mGfxPattern(nullptr)
{
    mPattern = cairo_pattern_create_rgba(aColor.r, aColor.g, aColor.b, aColor.a);
}


gfxPattern::gfxPattern(gfxFloat x0, gfxFloat y0, gfxFloat x1, gfxFloat y1)
  : mGfxPattern(nullptr)
{
    mPattern = cairo_pattern_create_linear(x0, y0, x1, y1);
}


gfxPattern::gfxPattern(gfxFloat cx0, gfxFloat cy0, gfxFloat radius0,
                       gfxFloat cx1, gfxFloat cy1, gfxFloat radius1)
  : mGfxPattern(nullptr)
{
    mPattern = cairo_pattern_create_radial(cx0, cy0, radius0,
                                           cx1, cy1, radius1);
}


gfxPattern::gfxPattern(SourceSurface *aSurface, const Matrix &aTransform)
  : mPattern(nullptr)
  , mGfxPattern(nullptr)
  , mSourceSurface(aSurface)
  , mTransform(aTransform)
  , mExtend(EXTEND_NONE)
  , mFilter(mozilla::gfx::Filter::GOOD)
{
}

gfxPattern::~gfxPattern()
{
    cairo_pattern_destroy(mPattern);

    if (mGfxPattern) {
      mGfxPattern->~Pattern();
    }
}

void
gfxPattern::AddColorStop(gfxFloat offset, const gfxRGBA& c)
{
  if (mPattern) {
    mStops = nullptr;
    if (gfxPlatform::GetCMSMode() == eCMSMode_All) {
        gfxRGBA cms;
        qcms_transform *transform = gfxPlatform::GetCMSRGBTransform();
        if (transform)
          gfxPlatform::TransformPixel(c, cms, transform);

        
        
        cairo_pattern_add_color_stop_rgba(mPattern, offset,
                                          cms.r, cms.g, cms.b, c.a);
    }
    else
        cairo_pattern_add_color_stop_rgba(mPattern, offset, c.r, c.g, c.b, c.a);
  }
}

void
gfxPattern::SetColorStops(GradientStops* aStops)
{
  mStops = aStops;
}

void
gfxPattern::CacheColorStops(DrawTarget *aDT)
{
  if (mPattern) {
    mStops = nullptr;
    nsTArray<GradientStop> stops;
    int count = 0;
    cairo_pattern_get_color_stop_count(mPattern, &count);
    stops.SetLength(count);
    for (int n = 0; n < count; ++n) {
      double offset, r, g, b, a;
      cairo_pattern_get_color_stop_rgba(mPattern, n, &offset, &r, &g, &b, &a);
      stops[n].color = Color(r, g, b, a);
      stops[n].offset = offset;
    }

    GraphicsExtend extend = GraphicsExtend(cairo_pattern_get_extend(mPattern));

    mStops = gfxGradientCache::GetOrCreateGradientStops(aDT, stops,
                                                        ToExtendMode(extend));
  }
}

void
gfxPattern::SetMatrix(const gfxMatrix& matrix)
{
  if (mPattern) {
    cairo_matrix_t mat = *reinterpret_cast<const cairo_matrix_t*>(&matrix);
    cairo_pattern_set_matrix(mPattern, &mat);
  } else {
    mTransform = ToMatrix(matrix);
    
    
    
    mTransform.Invert();
  }
}

gfxMatrix
gfxPattern::GetMatrix() const
{
  if (mPattern) {
    cairo_matrix_t mat;
    cairo_pattern_get_matrix(mPattern, &mat);
    return gfxMatrix(*reinterpret_cast<gfxMatrix*>(&mat));
  } else {
    
    
    gfxMatrix mat = ThebesMatrix(mTransform);
    mat.Invert();
    return mat;
  }
}

gfxMatrix
gfxPattern::GetInverseMatrix() const
{
  if (mPattern) {
    cairo_matrix_t mat;
    cairo_pattern_get_matrix(mPattern, &mat);
    cairo_matrix_invert(&mat);
    return gfxMatrix(*reinterpret_cast<gfxMatrix*>(&mat));
  } else {
    return ThebesMatrix(mTransform);
  }
}

Pattern*
gfxPattern::GetPattern(DrawTarget *aTarget, Matrix *aPatternTransform)
{
  if (mGfxPattern) {
    mGfxPattern->~Pattern();
    mGfxPattern = nullptr;
  }

  if (!mPattern) {
    Matrix adjustedMatrix = mTransform;
    if (aPatternTransform)
      AdjustTransformForPattern(adjustedMatrix, aTarget->GetTransform(), aPatternTransform);
    mGfxPattern = new (mSurfacePattern.addr())
      SurfacePattern(mSourceSurface, ToExtendMode(mExtend), adjustedMatrix, mFilter);
    return mGfxPattern;
  }

  GraphicsExtend extend = GraphicsExtend(cairo_pattern_get_extend(mPattern));

  switch (cairo_pattern_get_type(mPattern)) {
  case CAIRO_PATTERN_TYPE_SOLID:
    {
      double r, g, b, a;
      cairo_pattern_get_rgba(mPattern, &r, &g, &b, &a);

      new (mColorPattern.addr()) ColorPattern(Color(r, g, b, a));
      return mColorPattern.addr();
    }
  case CAIRO_PATTERN_TYPE_LINEAR:
    {
      double x1, y1, x2, y2;
      cairo_pattern_get_linear_points(mPattern, &x1, &y1, &x2, &y2);
      if (!mStops) {
        int count = 0;
        cairo_pattern_get_color_stop_count(mPattern, &count);

        std::vector<GradientStop> stops;

        for (int i = 0; i < count; i++) {
          GradientStop stop;
          double r, g, b, a, offset;
          cairo_pattern_get_color_stop_rgba(mPattern, i, &offset, &r, &g, &b, &a);

          stop.offset = offset;
          stop.color = Color(Float(r), Float(g), Float(b), Float(a));
          stops.push_back(stop);
        }

        mStops = aTarget->CreateGradientStops(&stops.front(), count, ToExtendMode(extend));
      }

      if (mStops) {
        cairo_matrix_t mat;
        cairo_pattern_get_matrix(mPattern, &mat);
        gfxMatrix matrix(*reinterpret_cast<gfxMatrix*>(&mat));

        Matrix newMat = ToMatrix(matrix);

        AdjustTransformForPattern(newMat, aTarget->GetTransform(), aPatternTransform);

        mGfxPattern = new (mLinearGradientPattern.addr())
          LinearGradientPattern(Point(x1, y1), Point(x2, y2), mStops, newMat);

        return mGfxPattern;
      }
      break;
    }
  case CAIRO_PATTERN_TYPE_RADIAL:
    {
      if (!mStops) {
        int count = 0;
        cairo_pattern_get_color_stop_count(mPattern, &count);

        std::vector<GradientStop> stops;

        for (int i = 0; i < count; i++) {
          GradientStop stop;
          double r, g, b, a, offset;
          cairo_pattern_get_color_stop_rgba(mPattern, i, &offset, &r, &g, &b, &a);

          stop.offset = offset;
          stop.color = Color(Float(r), Float(g), Float(b), Float(a));
          stops.push_back(stop);
        }

        mStops = aTarget->CreateGradientStops(&stops.front(), count, ToExtendMode(extend));
      }

      if (mStops) {
        cairo_matrix_t mat;
        cairo_pattern_get_matrix(mPattern, &mat);
        gfxMatrix matrix(*reinterpret_cast<gfxMatrix*>(&mat));

        Matrix newMat = ToMatrix(matrix);

        AdjustTransformForPattern(newMat, aTarget->GetTransform(), aPatternTransform);

        double x1, y1, x2, y2, r1, r2;
        cairo_pattern_get_radial_circles(mPattern, &x1, &y1, &r1, &x2, &y2, &r2);
        mGfxPattern = new (mRadialGradientPattern.addr())
          RadialGradientPattern(Point(x1, y1), Point(x2, y2), r1, r2, mStops, newMat);

        return mGfxPattern;
      }
      break;
    }
  default:
    
    break;
  }

  new (mColorPattern.addr()) ColorPattern(Color(0, 0, 0, 0));
  return mColorPattern.addr();
}

void
gfxPattern::SetExtend(GraphicsExtend extend)
{
  if (mPattern) {
    mStops = nullptr;
    if (extend == EXTEND_PAD_EDGE) {
      extend = EXTEND_PAD;
    }

    cairo_pattern_set_extend(mPattern, (cairo_extend_t)extend);
  } else {
    
    
    mExtend = extend;
  }
}

bool
gfxPattern::IsOpaque()
{
  if (mPattern) {
    return false;
  }

  if (mSourceSurface->GetFormat() == SurfaceFormat::B8G8R8X8) {
    return true;
  }
  return false;
}

gfxPattern::GraphicsExtend
gfxPattern::Extend() const
{
  if (mPattern) {
    return GraphicsExtend(cairo_pattern_get_extend(mPattern));
  } else {
    return mExtend;
  }
}

void
gfxPattern::SetFilter(GraphicsFilter filter)
{
  if (mPattern) {
    cairo_pattern_set_filter(mPattern, (cairo_filter_t)(int)filter);
  } else {
    mFilter = ToFilter(filter);
  }
}

GraphicsFilter
gfxPattern::Filter() const
{
  if (mPattern) {
    return (GraphicsFilter)cairo_pattern_get_filter(mPattern);
  } else {
    return ThebesFilter(mFilter);
  }
}

bool
gfxPattern::GetSolidColor(gfxRGBA& aColor)
{
    return cairo_pattern_get_rgba(mPattern,
                                  &aColor.r,
                                  &aColor.g,
                                  &aColor.b,
                                  &aColor.a) == CAIRO_STATUS_SUCCESS;
}

gfxPattern::GraphicsPatternType
gfxPattern::GetType() const
{
  if (mPattern) {
    return (GraphicsPatternType) cairo_pattern_get_type(mPattern);
  } else {
    
    MOZ_ASSERT(0);
    return PATTERN_SURFACE;
  }
}

int
gfxPattern::CairoStatus()
{
  if (mPattern) {
    return cairo_pattern_status(mPattern);
  } else {
    
    return CAIRO_STATUS_SUCCESS;
  }
}

void
gfxPattern::AdjustTransformForPattern(Matrix &aPatternTransform,
                                      const Matrix &aCurrentTransform,
                                      const Matrix *aOriginalTransform)
{
  aPatternTransform.Invert();
  if (!aOriginalTransform) {
    
    
    aPatternTransform.NudgeToIntegers();
    return;
  }
  
  

  Matrix mat = aCurrentTransform;
  mat.Invert();
  

  
  
  
  aPatternTransform = aPatternTransform * *aOriginalTransform * mat;
  aPatternTransform.NudgeToIntegers();
}
