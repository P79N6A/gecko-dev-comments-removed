




#include "BorrowedContext.h"
#include "DataSurfaceHelpers.h"
#include "DrawTargetCG.h"
#include "Logging.h"
#include "SourceSurfaceCG.h"
#include "Rect.h"
#include "ScaledFontMac.h"
#include "Tools.h"
#include "PathHelpers.h"
#include <vector>
#include <algorithm>
#include "MacIOSurface.h"
#include "FilterNodeSoftware.h"
#include "mozilla/Assertions.h"
#include "mozilla/FloatingPoint.h"
#include "mozilla/Types.h" 
#include "mozilla/Vector.h"

using namespace std;




CG_EXTERN void CGContextSetCTM(CGContextRef, CGAffineTransform);

namespace mozilla {
namespace gfx {

template <typename T>
static CGRect RectToCGRect(const T& r)
{
  return CGRectMake(r.x, r.y, r.width, r.height);
}

CGBlendMode ToBlendMode(CompositionOp op)
{
  CGBlendMode mode;
  switch (op) {
    case CompositionOp::OP_OVER:
      mode = kCGBlendModeNormal;
      break;
    case CompositionOp::OP_ADD:
      mode = kCGBlendModePlusLighter;
      break;
    case CompositionOp::OP_ATOP:
      mode = kCGBlendModeSourceAtop;
      break;
    case CompositionOp::OP_OUT:
      mode = kCGBlendModeSourceOut;
      break;
    case CompositionOp::OP_IN:
      mode = kCGBlendModeSourceIn;
      break;
    case CompositionOp::OP_SOURCE:
      mode = kCGBlendModeCopy;
      break;
    case CompositionOp::OP_DEST_IN:
      mode = kCGBlendModeDestinationIn;
      break;
    case CompositionOp::OP_DEST_OUT:
      mode = kCGBlendModeDestinationOut;
      break;
    case CompositionOp::OP_DEST_OVER:
      mode = kCGBlendModeDestinationOver;
      break;
    case CompositionOp::OP_DEST_ATOP:
      mode = kCGBlendModeDestinationAtop;
      break;
    case CompositionOp::OP_XOR:
      mode = kCGBlendModeXOR;
      break;
    case CompositionOp::OP_MULTIPLY:
      mode = kCGBlendModeMultiply;
      break;
    case CompositionOp::OP_SCREEN:
      mode = kCGBlendModeScreen;
      break;
    case CompositionOp::OP_OVERLAY:
      mode = kCGBlendModeOverlay;
      break;
    case CompositionOp::OP_DARKEN:
      mode = kCGBlendModeDarken;
      break;
    case CompositionOp::OP_LIGHTEN:
      mode = kCGBlendModeLighten;
      break;
    case CompositionOp::OP_COLOR_DODGE:
      mode = kCGBlendModeColorDodge;
      break;
    case CompositionOp::OP_COLOR_BURN:
      mode = kCGBlendModeColorBurn;
      break;
    case CompositionOp::OP_HARD_LIGHT:
      mode = kCGBlendModeHardLight;
      break;
    case CompositionOp::OP_SOFT_LIGHT:
      mode = kCGBlendModeSoftLight;
      break;
    case CompositionOp::OP_DIFFERENCE:
      mode = kCGBlendModeDifference;
      break;
    case CompositionOp::OP_EXCLUSION:
      mode = kCGBlendModeExclusion;
      break;
    case CompositionOp::OP_HUE:
      mode = kCGBlendModeHue;
      break;
    case CompositionOp::OP_SATURATION:
      mode = kCGBlendModeSaturation;
      break;
    case CompositionOp::OP_COLOR:
      mode = kCGBlendModeColor;
      break;
    case CompositionOp::OP_LUMINOSITY:
      mode = kCGBlendModeLuminosity;
      break;
      



    default:
      mode = kCGBlendModeNormal;
  }
  return mode;
}

static CGInterpolationQuality
InterpolationQualityFromFilter(Filter aFilter)
{
  switch (aFilter) {
    default:
    case Filter::LINEAR:
      return kCGInterpolationLow;
    case Filter::POINT:
      return kCGInterpolationNone;
    case Filter::GOOD:
      return kCGInterpolationLow;
  }
}


DrawTargetCG::DrawTargetCG()
  : mColorSpace(nullptr)
  , mCg(nullptr)
  , mMayContainInvalidPremultipliedData(false)
{
}

DrawTargetCG::~DrawTargetCG()
{
  if (mSnapshot) {
    if (mSnapshot->refCount() > 1) {
      
      mSnapshot->DrawTargetWillGoAway();
    }
    mSnapshot = nullptr;
  }

  
  
  CGColorSpaceRelease(mColorSpace);
  CGContextRelease(mCg);
}

DrawTargetType
DrawTargetCG::GetType() const
{
  return GetBackendType() == BackendType::COREGRAPHICS_ACCELERATED ?
           DrawTargetType::HARDWARE_RASTER : DrawTargetType::SOFTWARE_RASTER;
}

BackendType
DrawTargetCG::GetBackendType() const
{
  
  
  if (GetContextType(mCg) == CG_CONTEXT_TYPE_IOSURFACE) {
    return BackendType::COREGRAPHICS_ACCELERATED;
  } else {
    return BackendType::COREGRAPHICS;
  }
}

TemporaryRef<SourceSurface>
DrawTargetCG::Snapshot()
{
  if (!mSnapshot) {
    if (GetContextType(mCg) == CG_CONTEXT_TYPE_IOSURFACE) {
      return new SourceSurfaceCGIOSurfaceContext(this);
    }
    Flush();
    mSnapshot = new SourceSurfaceCGBitmapContext(this);
  }

  return mSnapshot;
}

TemporaryRef<DrawTarget>
DrawTargetCG::CreateSimilarDrawTarget(const IntSize &aSize, SurfaceFormat aFormat) const
{
  
  
  RefPtr<DrawTargetCG> newTarget = new DrawTargetCG();
  if (newTarget->Init(GetBackendType(), aSize, aFormat)) {
    return newTarget.forget();
  }
  return nullptr;
}

TemporaryRef<SourceSurface>
DrawTargetCG::CreateSourceSurfaceFromData(unsigned char *aData,
                                           const IntSize &aSize,
                                           int32_t aStride,
                                           SurfaceFormat aFormat) const
{
  RefPtr<SourceSurfaceCG> newSurf = new SourceSurfaceCG();

  if (!newSurf->InitFromData(aData, aSize, aStride, aFormat)) {
    return nullptr;
  }

  return newSurf.forget();
}

static void releaseDataSurface(void* info, const void *data, size_t size)
{
  static_cast<DataSourceSurface*>(info)->Release();
}




static CGImageRef
GetRetainedImageFromSourceSurface(SourceSurface *aSurface)
{
  switch(aSurface->GetType()) {
    case SurfaceType::COREGRAPHICS_IMAGE:
      return CGImageRetain(static_cast<SourceSurfaceCG*>(aSurface)->GetImage());

    case SurfaceType::COREGRAPHICS_CGCONTEXT:
      return CGImageRetain(static_cast<SourceSurfaceCGContext*>(aSurface)->GetImage());

    default:
    {
      RefPtr<DataSourceSurface> data = aSurface->GetDataSurface();
      if (!data) {
        MOZ_CRASH("unsupported source surface");
      }
      data.get()->AddRef();
      return CreateCGImage(releaseDataSurface, data.get(),
                           data->GetData(), data->GetSize(),
                           data->Stride(), data->GetFormat());
    }
  }
}

TemporaryRef<SourceSurface>
DrawTargetCG::OptimizeSourceSurface(SourceSurface *aSurface) const
{
  return aSurface;
}

class UnboundnessFixer
{
    CGRect mClipBounds;
    CGLayerRef mLayer;
    CGContextRef mLayerCg;
  public:
    UnboundnessFixer() : mLayerCg(nullptr) {}

    CGContextRef Check(CGContextRef baseCg, CompositionOp blend, const Rect* maskBounds = nullptr)
    {
      MOZ_ASSERT(baseCg);
      if (!IsOperatorBoundByMask(blend)) {
        mClipBounds = CGContextGetClipBoundingBox(baseCg);
        
        
        if (CGRectIsEmpty(mClipBounds) ||
            (maskBounds && maskBounds->Contains(CGRectToRect(mClipBounds)))) {
          return baseCg;
        }

        
        

        
        
        mLayer = CGLayerCreateWithContext(baseCg, mClipBounds.size, nullptr);
        mLayerCg = CGLayerGetContext(mLayer);
        
        
        
        if (MOZ2D_ERROR_IF(!mLayerCg)) {
          return nullptr;
        }
        CGContextTranslateCTM(mLayerCg, -mClipBounds.origin.x, mClipBounds.origin.y + mClipBounds.size.height);
        CGContextScaleCTM(mLayerCg, 1, -1);

        return mLayerCg;
      } else {
        return baseCg;
      }
    }

    void Fix(CGContextRef baseCg)
    {
        if (mLayerCg) {
            
            MOZ_ASSERT(baseCg);
            CGContextTranslateCTM(baseCg, 0, mClipBounds.size.height);
            CGContextScaleCTM(baseCg, 1, -1);
            mClipBounds.origin.y *= -1;
            CGContextDrawLayerAtPoint(baseCg, mClipBounds.origin, mLayer);
            CGContextRelease(mLayerCg);
        }
    }
};

void
DrawTargetCG::DrawSurface(SourceSurface *aSurface,
                           const Rect &aDest,
                           const Rect &aSource,
                           const DrawSurfaceOptions &aSurfOptions,
                           const DrawOptions &aDrawOptions)
{
  if (MOZ2D_ERROR_IF(!mCg)) {
    return;
  }
  MarkChanged();

  CGContextSaveGState(mCg);

  CGContextSetBlendMode(mCg, ToBlendMode(aDrawOptions.mCompositionOp));
  UnboundnessFixer fixer;
  CGContextRef cg = fixer.Check(mCg, aDrawOptions.mCompositionOp, &aDest);
  if (MOZ2D_ERROR_IF(!cg)) {
    return;
  }
  CGContextSetAlpha(cg, aDrawOptions.mAlpha);
  CGContextSetShouldAntialias(cg, aDrawOptions.mAntialiasMode != AntialiasMode::NONE);

  CGContextConcatCTM(cg, GfxMatrixToCGAffineTransform(mTransform));

  CGContextSetInterpolationQuality(cg, InterpolationQualityFromFilter(aSurfOptions.mFilter));

  CGImageRef image = GetRetainedImageFromSourceSurface(aSurface);

  if (aSurfOptions.mFilter == Filter::POINT) {
    CGImageRef subimage = CGImageCreateWithImageInRect(image, RectToCGRect(aSource));
    CGImageRelease(image);

    CGContextScaleCTM(cg, 1, -1);

    CGRect flippedRect = CGRectMake(aDest.x, -(aDest.y + aDest.height),
                                    aDest.width, aDest.height);

    CGContextDrawImage(cg, flippedRect, subimage);
    CGImageRelease(subimage);
  } else {
    CGRect destRect = CGRectMake(aDest.x, aDest.y, aDest.width, aDest.height);
    CGContextClipToRect(cg, destRect);

    float xScale = aSource.width / aDest.width;
    float yScale = aSource.height / aDest.height;
    CGContextTranslateCTM(cg, aDest.x - aSource.x / xScale, aDest.y - aSource.y / yScale);

    CGRect adjustedDestRect = CGRectMake(0, 0, CGImageGetWidth(image) / xScale,
                                         CGImageGetHeight(image) / yScale);

    CGContextTranslateCTM(cg, 0, CGRectGetHeight(adjustedDestRect));
    CGContextScaleCTM(cg, 1, -1);

    CGContextDrawImage(cg, adjustedDestRect, image);
    CGImageRelease(image);
  }

  fixer.Fix(mCg);

  CGContextRestoreGState(mCg);
}

TemporaryRef<FilterNode>
DrawTargetCG::CreateFilter(FilterType aType)
{
  return FilterNodeSoftware::Create(aType);
}

void
DrawTargetCG::DrawFilter(FilterNode *aNode,
                         const Rect &aSourceRect,
                         const Point &aDestPoint,
                         const DrawOptions &aOptions)
{
  FilterNodeSoftware* filter = static_cast<FilterNodeSoftware*>(aNode);
  filter->Draw(this, aSourceRect, aDestPoint, aOptions);
}

static CGColorRef ColorToCGColor(CGColorSpaceRef aColorSpace, const Color& aColor)
{
  CGFloat components[4] = {aColor.r, aColor.g, aColor.b, aColor.a};
  return CGColorCreate(aColorSpace, components);
}

class GradientStopsCG : public GradientStops
{
  public:
  MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(GradientStopsCG)

  GradientStopsCG(CGColorSpaceRef aColorSpace,
                  const std::vector<GradientStop>& aStops,
                  ExtendMode aExtendMode)
    : mGradient(nullptr)
  {
    

    mExtend = aExtendMode;
    if (aExtendMode == ExtendMode::CLAMP) {
      size_t numStops = aStops.size();

      std::vector<CGFloat> colors;
      std::vector<CGFloat> offsets;
      colors.reserve(numStops*4);
      offsets.reserve(numStops);

      for (size_t i = 0; i < numStops; i++) {
        colors.push_back(aStops[i].color.r);
        colors.push_back(aStops[i].color.g);
        colors.push_back(aStops[i].color.b);
        colors.push_back(aStops[i].color.a);

        offsets.push_back(aStops[i].offset);
      }

      mGradient = CGGradientCreateWithColorComponents(aColorSpace,
                                                      &colors.front(),
                                                      &offsets.front(),
                                                      offsets.size());
    } else {
      mStops = aStops;
    }

  }

  virtual ~GradientStopsCG() {
    
    CGGradientRelease(mGradient);
  }

  
  
  BackendType GetBackendType() const { return BackendType::COREGRAPHICS; }
  
  CGGradientRef mGradient;
  std::vector<GradientStop> mStops;
  ExtendMode mExtend;
};

TemporaryRef<GradientStops>
DrawTargetCG::CreateGradientStops(GradientStop *aStops, uint32_t aNumStops,
                                  ExtendMode aExtendMode) const
{
  std::vector<GradientStop> stops(aStops, aStops+aNumStops);
  return new GradientStopsCG(mColorSpace, stops, aExtendMode);
}

static void
UpdateLinearParametersToIncludePoint(double *min_t, double *max_t,
                                     CGPoint *start,
                                     double dx, double dy,
                                     double x, double y)
{
  MOZ_ASSERT(IsFinite(x) && IsFinite(y));

  
















  double px = x - start->x;
  double py = y - start->y;
  double numerator = dx * px + dy * py;
  double denominator = dx * dx + dy * dy;
  double t = numerator / denominator;

  if (*min_t > t) {
    *min_t = t;
  }
  if (*max_t < t) {
    *max_t = t;
  }
}






static void
CalculateRepeatingGradientParams(CGPoint *aStart, CGPoint *aEnd,
                                 CGRect aExtents, int *aRepeatStartFactor,
                                 int *aRepeatEndFactor)
{
  double t_min = INFINITY;
  double t_max = -INFINITY;
  double dx = aEnd->x - aStart->x;
  double dy = aEnd->y - aStart->y;

  double bounds_x1 = aExtents.origin.x;
  double bounds_y1 = aExtents.origin.y;
  double bounds_x2 = aExtents.origin.x + aExtents.size.width;
  double bounds_y2 = aExtents.origin.y + aExtents.size.height;

  UpdateLinearParametersToIncludePoint(&t_min, &t_max, aStart, dx, dy,
                                       bounds_x1, bounds_y1);
  UpdateLinearParametersToIncludePoint(&t_min, &t_max, aStart, dx, dy,
                                       bounds_x2, bounds_y1);
  UpdateLinearParametersToIncludePoint(&t_min, &t_max, aStart, dx, dy,
                                       bounds_x2, bounds_y2);
  UpdateLinearParametersToIncludePoint(&t_min, &t_max, aStart, dx, dy,
                                       bounds_x1, bounds_y2);

  MOZ_ASSERT(!isinf(t_min) && !isinf(t_max),
             "The first call to UpdateLinearParametersToIncludePoint should have made t_min and t_max non-infinite.");

  
  
  
  t_min = floor (t_min);
  t_max = ceil (t_max);
  aEnd->x = aStart->x + dx * t_max;
  aEnd->y = aStart->y + dy * t_max;
  aStart->x = aStart->x + dx * t_min;
  aStart->y = aStart->y + dy * t_min;

  *aRepeatStartFactor = t_min;
  *aRepeatEndFactor = t_max;
}

static CGGradientRef
CreateRepeatingGradient(CGColorSpaceRef aColorSpace,
                        CGContextRef cg, GradientStopsCG* aStops,
                        int aRepeatStartFactor, int aRepeatEndFactor,
                        bool aReflect)
{
  int repeatCount = aRepeatEndFactor - aRepeatStartFactor;
  uint32_t stopCount = aStops->mStops.size();
  double scale = 1./repeatCount;

  std::vector<CGFloat> colors;
  std::vector<CGFloat> offsets;
  colors.reserve(stopCount*repeatCount*4);
  offsets.reserve(stopCount*repeatCount);

  for (int j = aRepeatStartFactor; j < aRepeatEndFactor; j++) {
    bool isReflected = aReflect && (j % 2) != 0;
    for (uint32_t i = 0; i < stopCount; i++) {
      uint32_t stopIndex = isReflected ? stopCount - i - 1 : i;
      colors.push_back(aStops->mStops[stopIndex].color.r);
      colors.push_back(aStops->mStops[stopIndex].color.g);
      colors.push_back(aStops->mStops[stopIndex].color.b);
      colors.push_back(aStops->mStops[stopIndex].color.a);

      CGFloat offset = aStops->mStops[stopIndex].offset;
      if (isReflected) {
        offset = 1 - offset;
      }
      offsets.push_back((offset + (j - aRepeatStartFactor)) * scale);
    }
  }

  CGGradientRef gradient = CGGradientCreateWithColorComponents(aColorSpace,
                                                               &colors.front(),
                                                               &offsets.front(),
                                                               repeatCount*stopCount);
  return gradient;
}

static void
DrawLinearRepeatingGradient(CGColorSpaceRef aColorSpace, CGContextRef cg,
                            const LinearGradientPattern &aPattern,
                            const CGRect &aExtents, bool aReflect)
{
  GradientStopsCG *stops = static_cast<GradientStopsCG*>(aPattern.mStops.get());
  CGPoint startPoint = { aPattern.mBegin.x, aPattern.mBegin.y };
  CGPoint endPoint = { aPattern.mEnd.x, aPattern.mEnd.y };

  int repeatStartFactor = 0, repeatEndFactor = 1;
  
  if (aPattern.mEnd.x != aPattern.mBegin.x ||
      aPattern.mEnd.y != aPattern.mBegin.y) {
    CalculateRepeatingGradientParams(&startPoint, &endPoint, aExtents,
                                     &repeatStartFactor, &repeatEndFactor);
  }

  CGGradientRef gradient = CreateRepeatingGradient(aColorSpace, cg, stops, repeatStartFactor, repeatEndFactor, aReflect);

  CGContextDrawLinearGradient(cg, gradient, startPoint, endPoint,
                              kCGGradientDrawsBeforeStartLocation | kCGGradientDrawsAfterEndLocation);
  CGGradientRelease(gradient);
}

static CGPoint CGRectTopLeft(CGRect a)
{ return a.origin; }
static CGPoint CGRectBottomLeft(CGRect a)
{ return CGPointMake(a.origin.x, a.origin.y + a.size.height); }
static CGPoint CGRectTopRight(CGRect a)
{ return CGPointMake(a.origin.x + a.size.width, a.origin.y); }
static CGPoint CGRectBottomRight(CGRect a)
{ return CGPointMake(a.origin.x + a.size.width, a.origin.y + a.size.height); }

static CGFloat
CGPointDistance(CGPoint a, CGPoint b)
{
  return hypot(a.x-b.x, a.y-b.y);
}

static void
DrawRadialRepeatingGradient(CGColorSpaceRef aColorSpace, CGContextRef cg,
                            const RadialGradientPattern &aPattern,
                            const CGRect &aExtents, bool aReflect)
{
  GradientStopsCG *stops = static_cast<GradientStopsCG*>(aPattern.mStops.get());
  CGPoint startCenter = { aPattern.mCenter1.x, aPattern.mCenter1.y };
  CGFloat startRadius = aPattern.mRadius1;
  CGPoint endCenter   = { aPattern.mCenter2.x, aPattern.mCenter2.y };
  CGFloat endRadius   = aPattern.mRadius2;

  
  CGFloat minimumEndRadius = endRadius;
  minimumEndRadius = max(minimumEndRadius, CGPointDistance(endCenter, CGRectTopLeft(aExtents)));
  minimumEndRadius = max(minimumEndRadius, CGPointDistance(endCenter, CGRectBottomLeft(aExtents)));
  minimumEndRadius = max(minimumEndRadius, CGPointDistance(endCenter, CGRectTopRight(aExtents)));
  minimumEndRadius = max(minimumEndRadius, CGPointDistance(endCenter, CGRectBottomRight(aExtents)));

  CGFloat length = endRadius - startRadius;
  int repeatStartFactor = 0, repeatEndFactor = 1;
  while (endRadius < minimumEndRadius) {
    endRadius += length;
    repeatEndFactor++;
  }

  while (startRadius-length >= 0) {
    startRadius -= length;
    repeatStartFactor--;
  }

  CGGradientRef gradient = CreateRepeatingGradient(aColorSpace, cg, stops, repeatStartFactor, repeatEndFactor, aReflect);

  
  CGContextDrawRadialGradient(cg, gradient, startCenter, startRadius, endCenter, endRadius,
                              kCGGradientDrawsBeforeStartLocation | kCGGradientDrawsAfterEndLocation);
  CGGradientRelease(gradient);
}

static void
DrawGradient(CGColorSpaceRef aColorSpace,
             CGContextRef cg, const Pattern &aPattern, const CGRect &aExtents)
{
  if (MOZ2D_ERROR_IF(!cg)) {
    return;
  }

  if (CGRectIsEmpty(aExtents)) {
    return;
  }

  if (aPattern.GetType() == PatternType::LINEAR_GRADIENT) {
    const LinearGradientPattern& pat = static_cast<const LinearGradientPattern&>(aPattern);
    GradientStopsCG *stops = static_cast<GradientStopsCG*>(pat.mStops.get());
    CGAffineTransform patternMatrix = GfxMatrixToCGAffineTransform(pat.mMatrix);
    CGContextConcatCTM(cg, patternMatrix);
    CGRect extents = CGRectApplyAffineTransform(aExtents, CGAffineTransformInvert(patternMatrix));
    if (stops->mExtend == ExtendMode::CLAMP) {

      
      CGPoint startPoint = { pat.mBegin.x, pat.mBegin.y };
      CGPoint endPoint   = { pat.mEnd.x,   pat.mEnd.y };

      
      
      

      CGContextDrawLinearGradient(cg, stops->mGradient, startPoint, endPoint,
                                  kCGGradientDrawsBeforeStartLocation | kCGGradientDrawsAfterEndLocation);
    } else if (stops->mExtend == ExtendMode::REPEAT || stops->mExtend == ExtendMode::REFLECT) {
      DrawLinearRepeatingGradient(aColorSpace, cg, pat, extents, stops->mExtend == ExtendMode::REFLECT);
    }
  } else if (aPattern.GetType() == PatternType::RADIAL_GRADIENT) {
    const RadialGradientPattern& pat = static_cast<const RadialGradientPattern&>(aPattern);
    CGAffineTransform patternMatrix = GfxMatrixToCGAffineTransform(pat.mMatrix);
    CGContextConcatCTM(cg, patternMatrix);
    CGRect extents = CGRectApplyAffineTransform(aExtents, CGAffineTransformInvert(patternMatrix));
    GradientStopsCG *stops = static_cast<GradientStopsCG*>(pat.mStops.get());
    if (stops->mExtend == ExtendMode::CLAMP) {

      
      CGPoint startCenter = { pat.mCenter1.x, pat.mCenter1.y };
      CGFloat startRadius = pat.mRadius1;
      CGPoint endCenter   = { pat.mCenter2.x, pat.mCenter2.y };
      CGFloat endRadius   = pat.mRadius2;

      
      CGContextDrawRadialGradient(cg, stops->mGradient, startCenter, startRadius, endCenter, endRadius,
                                  kCGGradientDrawsBeforeStartLocation | kCGGradientDrawsAfterEndLocation);
    } else if (stops->mExtend == ExtendMode::REPEAT || stops->mExtend == ExtendMode::REFLECT) {
      DrawRadialRepeatingGradient(aColorSpace, cg, pat, extents, stops->mExtend == ExtendMode::REFLECT);
    }
  } else {
    assert(0);
  }

}

static void
drawPattern(void *info, CGContextRef context)
{
  CGImageRef image = static_cast<CGImageRef>(info);
  CGRect rect = {{0, 0},
    {static_cast<CGFloat>(CGImageGetWidth(image)),
     static_cast<CGFloat>(CGImageGetHeight(image))}};
  CGContextDrawImage(context, rect, image);
}

static void
releaseInfo(void *info)
{
  CGImageRef image = static_cast<CGImageRef>(info);
  CGImageRelease(image);
}

CGPatternCallbacks patternCallbacks = {
  0,
  drawPattern,
  releaseInfo
};

static bool
isGradient(const Pattern &aPattern)
{
  return aPattern.GetType() == PatternType::LINEAR_GRADIENT || aPattern.GetType() == PatternType::RADIAL_GRADIENT;
}

static bool
isNonRepeatingSurface(const Pattern& aPattern)
{
  return aPattern.GetType() == PatternType::SURFACE &&
    static_cast<const SurfacePattern&>(aPattern).mExtendMode != ExtendMode::REPEAT;
}



static CGPatternRef
CreateCGPattern(const Pattern &aPattern, CGAffineTransform aUserSpace)
{
  const SurfacePattern& pat = static_cast<const SurfacePattern&>(aPattern);
  
  CGImageRef image = GetRetainedImageFromSourceSurface(pat.mSurface.get());
  Matrix patTransform = pat.mMatrix;
  if (!pat.mSamplingRect.IsEmpty()) {
    CGImageRef temp = CGImageCreateWithImageInRect(image, RectToCGRect(pat.mSamplingRect));
    CGImageRelease(image);
    image = temp;
    patTransform.PreTranslate(pat.mSamplingRect.x, pat.mSamplingRect.y);
  }
  CGFloat xStep, yStep;
  switch (pat.mExtendMode) {
    case ExtendMode::CLAMP:
      
      xStep = static_cast<CGFloat>(1 << 22);
      yStep = static_cast<CGFloat>(1 << 22);
      break;
    case ExtendMode::REFLECT:
      assert(0);
    case ExtendMode::REPEAT:
      xStep = static_cast<CGFloat>(CGImageGetWidth(image));
      yStep = static_cast<CGFloat>(CGImageGetHeight(image));
      
      
      
      
      
      
      
      
      
  }

  
  
  
  CGRect bounds = {
    {0, 0,},
    {static_cast<CGFloat>(CGImageGetWidth(image)), static_cast<CGFloat>(CGImageGetHeight(image))}
  };
  CGAffineTransform transform =
      CGAffineTransformConcat(CGAffineTransformConcat(CGAffineTransformMakeScale(1,
                                                                                 -1),
                                                      GfxMatrixToCGAffineTransform(patTransform)),
                              aUserSpace);
  transform = CGAffineTransformTranslate(transform, 0, -static_cast<float>(CGImageGetHeight(image)));
  return CGPatternCreate(image, bounds, transform, xStep, yStep, kCGPatternTilingConstantSpacing,
                         true, &patternCallbacks);
}

static void
SetFillFromPattern(CGContextRef cg, CGColorSpaceRef aColorSpace, const Pattern &aPattern)
{
  if (MOZ2D_ERROR_IF(!cg)) {
    return;
  }

  assert(!isGradient(aPattern));
  if (aPattern.GetType() == PatternType::COLOR) {

    const Color& color = static_cast<const ColorPattern&>(aPattern).mColor;
    
    CGColorRef cgcolor = ColorToCGColor(aColorSpace, color);
    CGContextSetFillColorWithColor(cg, cgcolor);
    CGColorRelease(cgcolor);
  } else if (aPattern.GetType() == PatternType::SURFACE) {

    CGColorSpaceRef patternSpace;
    patternSpace = CGColorSpaceCreatePattern (nullptr);
    CGContextSetFillColorSpace(cg, patternSpace);
    CGColorSpaceRelease(patternSpace);

    CGPatternRef pattern = CreateCGPattern(aPattern, CGContextGetCTM(cg));
    const SurfacePattern& pat = static_cast<const SurfacePattern&>(aPattern);
    CGContextSetInterpolationQuality(cg, InterpolationQualityFromFilter(pat.mFilter));
    CGFloat alpha = 1.;
    CGContextSetFillPattern(cg, pattern, &alpha);
    CGPatternRelease(pattern);
  }
}

static void
SetStrokeFromPattern(CGContextRef cg, CGColorSpaceRef aColorSpace, const Pattern &aPattern)
{
  assert(!isGradient(aPattern));
  if (aPattern.GetType() == PatternType::COLOR) {
    const Color& color = static_cast<const ColorPattern&>(aPattern).mColor;
    
    CGColorRef cgcolor = ColorToCGColor(aColorSpace, color);
    CGContextSetStrokeColorWithColor(cg, cgcolor);
    CGColorRelease(cgcolor);
  } else if (aPattern.GetType() == PatternType::SURFACE) {
    CGColorSpaceRef patternSpace;
    patternSpace = CGColorSpaceCreatePattern (nullptr);
    CGContextSetStrokeColorSpace(cg, patternSpace);
    CGColorSpaceRelease(patternSpace);

    CGPatternRef pattern = CreateCGPattern(aPattern, CGContextGetCTM(cg));
    const SurfacePattern& pat = static_cast<const SurfacePattern&>(aPattern);
    CGContextSetInterpolationQuality(cg, InterpolationQualityFromFilter(pat.mFilter));
    CGFloat alpha = 1.;
    CGContextSetStrokePattern(cg, pattern, &alpha);
    CGPatternRelease(pattern);
  }

}

void
DrawTargetCG::MaskSurface(const Pattern &aSource,
                          SourceSurface *aMask,
                          Point aOffset,
                          const DrawOptions &aDrawOptions)
{
  if (MOZ2D_ERROR_IF(!mCg)) {
    return;
  }

  MarkChanged();

  CGContextSaveGState(mCg);

  CGContextSetBlendMode(mCg, ToBlendMode(aDrawOptions.mCompositionOp));
  UnboundnessFixer fixer;
  CGContextRef cg = fixer.Check(mCg, aDrawOptions.mCompositionOp);
  if (MOZ2D_ERROR_IF(!cg)) {
    return;
  }

  CGContextSetAlpha(cg, aDrawOptions.mAlpha);
  CGContextSetShouldAntialias(cg, aDrawOptions.mAntialiasMode != AntialiasMode::NONE);

  CGContextConcatCTM(cg, GfxMatrixToCGAffineTransform(mTransform));
  CGImageRef image = GetRetainedImageFromSourceSurface(aMask);

  
  CGContextScaleCTM(cg, 1, -1);

  IntSize size = aMask->GetSize();

  CGContextClipToMask(cg, CGRectMake(aOffset.x, -(aOffset.y + size.height), size.width, size.height), image);

  CGContextScaleCTM(cg, 1, -1);
  if (isGradient(aSource)) {
    
    
    DrawGradient(mColorSpace, cg, aSource, CGRectMake(aOffset.x, aOffset.y, size.width, size.height));
  } else {
    SetFillFromPattern(cg, mColorSpace, aSource);
    CGContextFillRect(cg, CGRectMake(aOffset.x, aOffset.y, size.width, size.height));
  }

  CGImageRelease(image);

  fixer.Fix(mCg);

  CGContextRestoreGState(mCg);
}



void
DrawTargetCG::FillRect(const Rect &aRect,
                       const Pattern &aPattern,
                       const DrawOptions &aDrawOptions)
{
  if (MOZ2D_ERROR_IF(!mCg)) {
    return;
  }

  MarkChanged();

  CGContextSaveGState(mCg);

  UnboundnessFixer fixer;
  CGContextRef cg = fixer.Check(mCg, aDrawOptions.mCompositionOp, &aRect);
  if (MOZ2D_ERROR_IF(!cg)) {
    return;
  }

  CGContextSetAlpha(mCg, aDrawOptions.mAlpha);
  CGContextSetShouldAntialias(cg, aDrawOptions.mAntialiasMode != AntialiasMode::NONE);
  CGContextSetBlendMode(mCg, ToBlendMode(aDrawOptions.mCompositionOp));

  CGContextConcatCTM(cg, GfxMatrixToCGAffineTransform(mTransform));

  if (isGradient(aPattern)) {
    CGContextClipToRect(cg, RectToCGRect(aRect));
    CGRect clipBounds = CGContextGetClipBoundingBox(cg);
    DrawGradient(mColorSpace, cg, aPattern, clipBounds);
  } else if (isNonRepeatingSurface(aPattern)) {
    
    
    
    const SurfacePattern& pat = static_cast<const SurfacePattern&>(aPattern);
    CGImageRef image = GetRetainedImageFromSourceSurface(pat.mSurface.get());
    Matrix transform = pat.mMatrix;
    if (!pat.mSamplingRect.IsEmpty()) {
      CGImageRef temp = CGImageCreateWithImageInRect(image, RectToCGRect(pat.mSamplingRect));
      CGImageRelease(image);
      image = temp;
      transform.PreTranslate(pat.mSamplingRect.x, pat.mSamplingRect.y);
    }
    CGContextClipToRect(cg, RectToCGRect(aRect));
    CGContextConcatCTM(cg, GfxMatrixToCGAffineTransform(transform));
    CGContextTranslateCTM(cg, 0, CGImageGetHeight(image));
    CGContextScaleCTM(cg, 1, -1);

    CGRect imageRect = CGRectMake(0, 0, CGImageGetWidth(image), CGImageGetHeight(image));

    CGContextSetInterpolationQuality(cg, InterpolationQualityFromFilter(pat.mFilter));

    CGContextDrawImage(cg, imageRect, image);
    CGImageRelease(image);
  } else {
    SetFillFromPattern(cg, mColorSpace, aPattern);
    CGContextFillRect(cg, RectToCGRect(aRect));
  }

  fixer.Fix(mCg);
  CGContextRestoreGState(mCg);
}

static Float
DashPeriodLength(const StrokeOptions& aStrokeOptions)
{
  Float length = 0;
  for (size_t i = 0; i < aStrokeOptions.mDashLength; i++) {
    length += aStrokeOptions.mDashPattern[i];
  }
  if (aStrokeOptions.mDashLength & 1) {
    
    
    
    length += length;
  }
  return length;
}

inline Float
RoundDownToMultiple(Float aValue, Float aFactor)
{
  return floorf(aValue / aFactor) * aFactor;
}

static Rect
UserSpaceStrokeClip(const Rect &aDeviceClip,
                   const Matrix &aTransform,
                   const StrokeOptions &aStrokeOptions)
{
  Matrix inverse = aTransform;
  if (!inverse.Invert()) {
    return Rect();
  }
  Rect deviceClip = aDeviceClip;
  deviceClip.Inflate(MaxStrokeExtents(aStrokeOptions, aTransform));
  return inverse.TransformBounds(deviceClip);
}

static Rect
ShrinkClippedStrokedRect(const Rect &aStrokedRect, const Rect &aDeviceClip,
                         const Matrix &aTransform,
                         const StrokeOptions &aStrokeOptions)
{
  Rect userSpaceStrokeClip =
    UserSpaceStrokeClip(aDeviceClip, aTransform, aStrokeOptions);

  Rect intersection = aStrokedRect.Intersect(userSpaceStrokeClip);
  Float dashPeriodLength = DashPeriodLength(aStrokeOptions);
  if (intersection.IsEmpty() || dashPeriodLength == 0.0f) {
    return intersection;
  }

  
  
  Margin insetBy = aStrokedRect - intersection;
  insetBy.top = RoundDownToMultiple(insetBy.top, dashPeriodLength);
  insetBy.right = RoundDownToMultiple(insetBy.right, dashPeriodLength);
  insetBy.bottom = RoundDownToMultiple(insetBy.bottom, dashPeriodLength);
  insetBy.left = RoundDownToMultiple(insetBy.left, dashPeriodLength);

  Rect shrunkRect = aStrokedRect;
  shrunkRect.Deflate(insetBy);
  return shrunkRect;
}






static bool
IntersectLineWithRect(const Point& aP1, const Point& aP2, const Rect& aClip,
                      Float* aStart, Float* aEnd)
{
  Float t0 = 0.0f;
  Float t1 = 1.0f;
  Point vector = aP2 - aP1;
  for (uint32_t edge = 0; edge < 4; edge++) {
    Float p, q;
    switch (edge) {
      case 0: p = -vector.x; q = aP1.x - aClip.x; break;
      case 1: p =  vector.x; q = aClip.XMost() - aP1.x; break;
      case 2: p = -vector.y; q = aP1.y - aClip.y; break;
      case 3: p =  vector.y; q = aClip.YMost() - aP1.y; break;
    }

    if (p == 0.0f) {
      
      if (q < 0.0f) {
        return false;
      }
      continue;
    }

    Float r = q / p;
    if (p < 0) {
      t0 = std::max(t0, r);
    } else {
      t1 = std::min(t1, r);
    }

    if (t0 > t1) {
      return false;
    }
  }

  Float length = vector.Length();
  *aStart = t0 * length;
  *aEnd = t1 * length;
  return true;
}



static bool
ShrinkClippedStrokedLine(Point &aP1, Point& aP2, const Rect &aDeviceClip,
                         const Matrix &aTransform,
                         const StrokeOptions &aStrokeOptions)
{
  Rect userSpaceStrokeClip =
    UserSpaceStrokeClip(aDeviceClip, aTransform, aStrokeOptions);

  Point vector = aP2 - aP1;
  Float length = vector.Length();

  if (length == 0.0f) {
    return true;
  }

  Float start = 0;
  Float end = length;
  if (!IntersectLineWithRect(aP1, aP2, userSpaceStrokeClip, &start, &end)) {
    return false;
  }

  Float dashPeriodLength = DashPeriodLength(aStrokeOptions);
  if (dashPeriodLength > 0.0f) {
    
    
    start = RoundDownToMultiple(start, dashPeriodLength);
    end = length - RoundDownToMultiple(length - end, dashPeriodLength);
  }

  Point startPoint = aP1;
  aP1 = Point(startPoint.x + start * vector.x / length,
              startPoint.y + start * vector.y / length);
  aP2 = Point(startPoint.x + end * vector.x / length,
              startPoint.y + end * vector.y / length);
  return true;
}

void
DrawTargetCG::StrokeLine(const Point &aP1, const Point &aP2, const Pattern &aPattern, const StrokeOptions &aStrokeOptions, const DrawOptions &aDrawOptions)
{
  if (!std::isfinite(aP1.x) ||
      !std::isfinite(aP1.y) ||
      !std::isfinite(aP2.x) ||
      !std::isfinite(aP2.y)) {
    return;
  }

  if (MOZ2D_ERROR_IF(!mCg)) {
    return;
  }

  Point p1 = aP1;
  Point p2 = aP2;

  Rect deviceClip(0, 0, mSize.width, mSize.height);
  if (!ShrinkClippedStrokedLine(p1, p2, deviceClip, mTransform, aStrokeOptions)) {
    return;
  }

  MarkChanged();

  CGContextSaveGState(mCg);

  UnboundnessFixer fixer;
  CGContextRef cg = fixer.Check(mCg, aDrawOptions.mCompositionOp);
  if (MOZ2D_ERROR_IF(!cg)) {
    return;
  }
  CGContextSetAlpha(mCg, aDrawOptions.mAlpha);
  CGContextSetShouldAntialias(cg, aDrawOptions.mAntialiasMode != AntialiasMode::NONE);
  CGContextSetBlendMode(mCg, ToBlendMode(aDrawOptions.mCompositionOp));

  CGContextConcatCTM(cg, GfxMatrixToCGAffineTransform(mTransform));

  CGContextBeginPath(cg);
  CGContextMoveToPoint(cg, p1.x, p1.y);
  CGContextAddLineToPoint(cg, p2.x, p2.y);

  SetStrokeOptions(cg, aStrokeOptions);

  if (isGradient(aPattern)) {
    CGContextReplacePathWithStrokedPath(cg);
    CGRect extents = CGContextGetPathBoundingBox(cg);
    
    CGContextClip(cg);
    DrawGradient(mColorSpace, cg, aPattern, extents);
  } else {
    SetStrokeFromPattern(cg, mColorSpace, aPattern);
    CGContextStrokePath(cg);
  }

  fixer.Fix(mCg);
  CGContextRestoreGState(mCg);
}

static bool
IsInteger(Float aValue)
{
  return floorf(aValue) == aValue;
}

static bool
IsPixelAlignedStroke(const Rect& aRect, Float aLineWidth)
{
  Float halfWidth = aLineWidth/2;
  return IsInteger(aLineWidth) &&
         IsInteger(aRect.x - halfWidth) && IsInteger(aRect.y - halfWidth) &&
         IsInteger(aRect.XMost() - halfWidth) && IsInteger(aRect.YMost() - halfWidth);
}

void
DrawTargetCG::StrokeRect(const Rect &aRect,
                         const Pattern &aPattern,
                         const StrokeOptions &aStrokeOptions,
                         const DrawOptions &aDrawOptions)
{
  if (MOZ2D_ERROR_IF(!mCg)) {
    return;
  }

  if (!aRect.IsFinite()) {
    return;
  }

  
  
  
  
  Rect rect = aRect;
  if (!rect.IsEmpty()) {
    Rect deviceClip(0, 0, mSize.width, mSize.height);
    rect = ShrinkClippedStrokedRect(rect, deviceClip, mTransform, aStrokeOptions);
    if (rect.IsEmpty()) {
      return;
    }
  }

  MarkChanged();

  CGContextSaveGState(mCg);

  UnboundnessFixer fixer;
  CGContextRef cg = fixer.Check(mCg, aDrawOptions.mCompositionOp);
  if (MOZ2D_ERROR_IF(!cg)) {
    return;
  }

  CGContextSetAlpha(mCg, aDrawOptions.mAlpha);
  CGContextSetBlendMode(mCg, ToBlendMode(aDrawOptions.mCompositionOp));

  
  
  
  
  
  bool pixelAlignedStroke = mTransform.IsAllIntegers() &&
    mTransform.PreservesAxisAlignedRectangles() &&
    aPattern.GetType() == PatternType::COLOR &&
    IsPixelAlignedStroke(rect, aStrokeOptions.mLineWidth);
  CGContextSetShouldAntialias(cg,
    aDrawOptions.mAntialiasMode != AntialiasMode::NONE && !pixelAlignedStroke);

  CGContextConcatCTM(cg, GfxMatrixToCGAffineTransform(mTransform));

  SetStrokeOptions(cg, aStrokeOptions);

  if (isGradient(aPattern)) {
    
    CGContextBeginPath(cg);
    CGContextAddRect(cg, RectToCGRect(rect));
    CGContextReplacePathWithStrokedPath(cg);
    CGRect extents = CGContextGetPathBoundingBox(cg);
    
    CGContextClip(cg);
    DrawGradient(mColorSpace, cg, aPattern, extents);
  } else {
    SetStrokeFromPattern(cg, mColorSpace, aPattern);
    
    
    
    
    
    CGContextBeginPath(cg);
    CGContextMoveToPoint(cg, rect.x, rect.y);
    CGContextAddLineToPoint(cg, rect.XMost(), rect.y);
    CGContextAddLineToPoint(cg, rect.XMost(), rect.YMost());
    CGContextAddLineToPoint(cg, rect.x, rect.YMost());
    CGContextClosePath(cg);
    CGContextStrokePath(cg);
  }

  fixer.Fix(mCg);
  CGContextRestoreGState(mCg);
}


void
DrawTargetCG::ClearRect(const Rect &aRect)
{
  if (MOZ2D_ERROR_IF(!mCg)) {
    return;
  }

  MarkChanged();

  CGContextSaveGState(mCg);
  CGContextConcatCTM(mCg, GfxMatrixToCGAffineTransform(mTransform));

  CGContextClearRect(mCg, RectToCGRect(aRect));

  CGContextRestoreGState(mCg);
}

void
DrawTargetCG::Stroke(const Path *aPath, const Pattern &aPattern, const StrokeOptions &aStrokeOptions, const DrawOptions &aDrawOptions)
{
  if (MOZ2D_ERROR_IF(!mCg)) {
    return;
  }

  if (!aPath->GetBounds().IsFinite()) {
    return;
  }

  MarkChanged();

  CGContextSaveGState(mCg);

  UnboundnessFixer fixer;
  CGContextRef cg = fixer.Check(mCg, aDrawOptions.mCompositionOp);
  if (MOZ2D_ERROR_IF(!cg)) {
    return;
  }

  CGContextSetAlpha(mCg, aDrawOptions.mAlpha);
  CGContextSetShouldAntialias(cg, aDrawOptions.mAntialiasMode != AntialiasMode::NONE);
  CGContextSetBlendMode(mCg, ToBlendMode(aDrawOptions.mCompositionOp));

  CGContextConcatCTM(cg, GfxMatrixToCGAffineTransform(mTransform));


  CGContextBeginPath(cg);

  assert(aPath->GetBackendType() == BackendType::COREGRAPHICS);
  const PathCG *cgPath = static_cast<const PathCG*>(aPath);
  CGContextAddPath(cg, cgPath->GetPath());

  SetStrokeOptions(cg, aStrokeOptions);

  if (isGradient(aPattern)) {
    CGContextReplacePathWithStrokedPath(cg);
    CGRect extents = CGContextGetPathBoundingBox(cg);
    
    CGContextClip(cg);
    DrawGradient(mColorSpace, cg, aPattern, extents);
  } else {
    

    SetStrokeFromPattern(cg, mColorSpace, aPattern);
    CGContextStrokePath(cg);
  }

  fixer.Fix(mCg);
  CGContextRestoreGState(mCg);
}

void
DrawTargetCG::Fill(const Path *aPath, const Pattern &aPattern, const DrawOptions &aDrawOptions)
{
  if (MOZ2D_ERROR_IF(!mCg)) {
    return;
  }

  MarkChanged();

  assert(aPath->GetBackendType() == BackendType::COREGRAPHICS);

  CGContextSaveGState(mCg);

  CGContextSetBlendMode(mCg, ToBlendMode(aDrawOptions.mCompositionOp));
  UnboundnessFixer fixer;
  CGContextRef cg = fixer.Check(mCg, aDrawOptions.mCompositionOp);
  if (MOZ2D_ERROR_IF(!cg)) {
    return;
  }

  CGContextSetAlpha(cg, aDrawOptions.mAlpha);
  CGContextSetShouldAntialias(cg, aDrawOptions.mAntialiasMode != AntialiasMode::NONE);

  CGContextConcatCTM(cg, GfxMatrixToCGAffineTransform(mTransform));

  CGContextBeginPath(cg);
  
  const PathCG *cgPath = static_cast<const PathCG*>(aPath);

  if (isGradient(aPattern)) {
    
    CGRect extents;
    if (CGPathIsEmpty(cgPath->GetPath())) {
      
      
      CGContextClipToRect(mCg, CGRectZero);
      extents = CGRectZero;
    } else {
      CGContextAddPath(cg, cgPath->GetPath());
      extents = CGContextGetPathBoundingBox(cg);
      if (cgPath->GetFillRule() == FillRule::FILL_EVEN_ODD)
        CGContextEOClip(mCg);
      else
        CGContextClip(mCg);
    }

    DrawGradient(mColorSpace, cg, aPattern, extents);
  } else {
    CGContextAddPath(cg, cgPath->GetPath());

    SetFillFromPattern(cg, mColorSpace, aPattern);

    if (cgPath->GetFillRule() == FillRule::FILL_EVEN_ODD)
      CGContextEOFillPath(cg);
    else
      CGContextFillPath(cg);
  }

  fixer.Fix(mCg);
  CGContextRestoreGState(mCg);
}

CGRect ComputeGlyphsExtents(CGRect *bboxes, CGPoint *positions, CFIndex count, float scale)
{
  CGFloat x1, x2, y1, y2;
  if (count < 1)
    return CGRectZero;

  x1 = bboxes[0].origin.x + positions[0].x;
  x2 = bboxes[0].origin.x + positions[0].x + scale*bboxes[0].size.width;
  y1 = bboxes[0].origin.y + positions[0].y;
  y2 = bboxes[0].origin.y + positions[0].y + scale*bboxes[0].size.height;

  
  for (int i = 1; i < count; i++) {
    x1 = min(x1, bboxes[i].origin.x + positions[i].x);
    y1 = min(y1, bboxes[i].origin.y + positions[i].y);
    x2 = max(x2, bboxes[i].origin.x + positions[i].x + scale*bboxes[i].size.width);
    y2 = max(y2, bboxes[i].origin.y + positions[i].y + scale*bboxes[i].size.height);
  }

  CGRect extents = {{x1, y1}, {x2-x1, y2-y1}};
  return extents;
}

typedef void (*CGContextSetFontSmoothingBackgroundColorFunc) (CGContextRef cgContext, CGColorRef color);

static CGContextSetFontSmoothingBackgroundColorFunc
GetCGContextSetFontSmoothingBackgroundColorFunc()
{
  static CGContextSetFontSmoothingBackgroundColorFunc func = nullptr;
  static bool lookedUpFunc = false;
  if (!lookedUpFunc) {
    func = (CGContextSetFontSmoothingBackgroundColorFunc)dlsym(
      RTLD_DEFAULT, "CGContextSetFontSmoothingBackgroundColor");
    lookedUpFunc = true;
  }
  return func;
}

void
DrawTargetCG::FillGlyphs(ScaledFont *aFont, const GlyphBuffer &aBuffer, const Pattern &aPattern, const DrawOptions &aDrawOptions,
                         const GlyphRenderingOptions *aGlyphRenderingOptions)
{
  if (MOZ2D_ERROR_IF(!mCg)) {
    return;
  }

  MarkChanged();

  assert(aBuffer.mNumGlyphs);
  CGContextSaveGState(mCg);

  if (aGlyphRenderingOptions && aGlyphRenderingOptions->GetType() == FontType::MAC) {
    Color fontSmoothingBackgroundColor =
      static_cast<const GlyphRenderingOptionsCG*>(aGlyphRenderingOptions)->FontSmoothingBackgroundColor();
    if (fontSmoothingBackgroundColor.a > 0) {
      CGContextSetFontSmoothingBackgroundColorFunc setFontSmoothingBGColorFunc =
        GetCGContextSetFontSmoothingBackgroundColorFunc();
      if (setFontSmoothingBGColorFunc) {
        CGColorRef color = ColorToCGColor(mColorSpace, fontSmoothingBackgroundColor);
        setFontSmoothingBGColorFunc(mCg, color);
        CGColorRelease(color);

        
        
        
        
        
        
        
        
        
        
        
        
        
        mMayContainInvalidPremultipliedData = true;
      }
    }
  }

  CGContextSetBlendMode(mCg, ToBlendMode(aDrawOptions.mCompositionOp));
  UnboundnessFixer fixer;
  CGContextRef cg = fixer.Check(mCg, aDrawOptions.mCompositionOp);
  if (MOZ2D_ERROR_IF(!cg)) {
    return;
  }

  CGContextSetAlpha(cg, aDrawOptions.mAlpha);
  CGContextSetShouldAntialias(cg, aDrawOptions.mAntialiasMode != AntialiasMode::NONE);
  if (aDrawOptions.mAntialiasMode != AntialiasMode::DEFAULT) {
    CGContextSetShouldSmoothFonts(cg, aDrawOptions.mAntialiasMode == AntialiasMode::SUBPIXEL);
  }

  CGContextConcatCTM(cg, GfxMatrixToCGAffineTransform(mTransform));

  ScaledFontMac* macFont = static_cast<ScaledFontMac*>(aFont);

  
  
  
  
  Vector<CGGlyph, 64> glyphs;
  Vector<CGPoint, 64> positions;
  if (!glyphs.resizeUninitialized(aBuffer.mNumGlyphs) ||
      !positions.resizeUninitialized(aBuffer.mNumGlyphs)) {
    MOZ_CRASH("glyphs/positions allocation failed");
  }

  
  CGContextScaleCTM(cg, 1, -1);
  
  
  
  

  for (unsigned int i = 0; i < aBuffer.mNumGlyphs; i++) {
    glyphs[i] = aBuffer.mGlyphs[i].mIndex;
    
    positions[i] = CGPointMake(aBuffer.mGlyphs[i].mPosition.x,
                              -aBuffer.mGlyphs[i].mPosition.y);
  }

  
  if (isGradient(aPattern)) {
    CGContextSetTextDrawingMode(cg, kCGTextClip);
    CGRect extents;
    if (ScaledFontMac::CTFontDrawGlyphsPtr != nullptr) {
      CGRect *bboxes = new CGRect[aBuffer.mNumGlyphs];
      CTFontGetBoundingRectsForGlyphs(macFont->mCTFont, kCTFontDefaultOrientation,
                                      glyphs.begin(), bboxes, aBuffer.mNumGlyphs);
      extents = ComputeGlyphsExtents(bboxes, positions.begin(), aBuffer.mNumGlyphs, 1.0f);
      ScaledFontMac::CTFontDrawGlyphsPtr(macFont->mCTFont, glyphs.begin(),
                                         positions.begin(), aBuffer.mNumGlyphs, cg);
      delete[] bboxes;
    } else {
      CGRect *bboxes = new CGRect[aBuffer.mNumGlyphs];
      CGFontGetGlyphBBoxes(macFont->mFont, glyphs.begin(), aBuffer.mNumGlyphs, bboxes);
      extents = ComputeGlyphsExtents(bboxes, positions.begin(), aBuffer.mNumGlyphs, macFont->mSize);

      CGContextSetFont(cg, macFont->mFont);
      CGContextSetFontSize(cg, macFont->mSize);
      CGContextShowGlyphsAtPositions(cg, glyphs.begin(), positions.begin(),
                                     aBuffer.mNumGlyphs);
      delete[] bboxes;
    }
    CGContextScaleCTM(cg, 1, -1);
    DrawGradient(mColorSpace, cg, aPattern, extents);
  } else {
    
    
    CGContextSetTextDrawingMode(cg, kCGTextFill);
    SetFillFromPattern(cg, mColorSpace, aPattern);
    if (ScaledFontMac::CTFontDrawGlyphsPtr != nullptr) {
      ScaledFontMac::CTFontDrawGlyphsPtr(macFont->mCTFont, glyphs.begin(),
                                         positions.begin(),
                                         aBuffer.mNumGlyphs, cg);
    } else {
      CGContextSetFont(cg, macFont->mFont);
      CGContextSetFontSize(cg, macFont->mSize);
      CGContextShowGlyphsAtPositions(cg, glyphs.begin(), positions.begin(),
                                     aBuffer.mNumGlyphs);
    }
  }

  fixer.Fix(mCg);
  CGContextRestoreGState(cg);
}

extern "C" {
void
CGContextResetClip(CGContextRef);
};

void
DrawTargetCG::CopySurface(SourceSurface *aSurface,
                          const IntRect& aSourceRect,
                          const IntPoint &aDestination)
{
  if (MOZ2D_ERROR_IF(!mCg)) {
    return;
  }

  MarkChanged();

  if (aSurface->GetType() == SurfaceType::COREGRAPHICS_IMAGE ||
      aSurface->GetType() == SurfaceType::COREGRAPHICS_CGCONTEXT ||
      aSurface->GetType() == SurfaceType::DATA) {
    CGImageRef image = GetRetainedImageFromSourceSurface(aSurface);

    

    CGContextSaveGState(mCg);

    
    CGContextResetClip(mCg);
    CGRect destRect = CGRectMake(aDestination.x, aDestination.y,
                                 aSourceRect.width, aSourceRect.height);
    CGContextClipToRect(mCg, destRect);

    CGContextSetBlendMode(mCg, kCGBlendModeCopy);

    CGContextScaleCTM(mCg, 1, -1);

    CGRect flippedRect = CGRectMake(aDestination.x - aSourceRect.x, -(aDestination.y - aSourceRect.y + double(CGImageGetHeight(image))),
                                    CGImageGetWidth(image), CGImageGetHeight(image));

    
    
    if (mFormat == SurfaceFormat::A8) {
      CGContextClearRect(mCg, flippedRect);
    }
    CGContextDrawImage(mCg, flippedRect, image);

    CGContextRestoreGState(mCg);
    CGImageRelease(image);
  }
}

void
DrawTargetCG::DrawSurfaceWithShadow(SourceSurface *aSurface, const Point &aDest, const Color &aColor, const Point &aOffset, Float aSigma, CompositionOp aOperator)
{
  if (MOZ2D_ERROR_IF(!mCg)) {
    return;
  }

  MarkChanged();

  CGImageRef image = GetRetainedImageFromSourceSurface(aSurface);

  IntSize size = aSurface->GetSize();
  CGContextSaveGState(mCg);
  
  CGContextSetBlendMode(mCg, ToBlendMode(aOperator));

  CGContextScaleCTM(mCg, 1, -1);

  CGRect flippedRect = CGRectMake(aDest.x, -(aDest.y + size.height),
                                  size.width, size.height);

  CGColorRef color = ColorToCGColor(mColorSpace, aColor);
  CGSize offset = {aOffset.x, -aOffset.y};
  
  CGContextSetShadowWithColor(mCg, offset, 2*aSigma, color);
  CGColorRelease(color);

  CGContextDrawImage(mCg, flippedRect, image);

  CGImageRelease(image);
  CGContextRestoreGState(mCg);

}

bool
DrawTargetCG::Init(BackendType aType,
                   unsigned char* aData,
                   const IntSize &aSize,
                   int32_t aStride,
                   SurfaceFormat aFormat)
{
  
  
  if (aSize.width <= 0 ||
      aSize.height <= 0 ||
      size_t(aSize.width) > GetMaxSurfaceSize() ||
      size_t(aSize.height) > GetMaxSurfaceSize())
  {
    gfxWarning() << "Failed to Init() DrawTargetCG because of bad size.";
    mColorSpace = nullptr;
    mCg = nullptr;
    return false;
  }

  

  
  mColorSpace = CGColorSpaceCreateDeviceRGB();

  if (aData == nullptr && aType != BackendType::COREGRAPHICS_ACCELERATED) {
    
    size_t bufLen = BufferSizeFromStrideAndHeight(aStride, aSize.height);
    if (bufLen == 0) {
      mColorSpace = nullptr;
      mCg = nullptr;
      return false;
    }
    static_assert(sizeof(decltype(mData[0])) == 1,
                  "mData.Realloc() takes an object count, so its objects must be 1-byte sized if we use bufLen");
    mData.Realloc( bufLen, true);
    aData = static_cast<unsigned char*>(mData);
  }

  mSize = aSize;

  if (aType == BackendType::COREGRAPHICS_ACCELERATED) {
    RefPtr<MacIOSurface> ioSurface = MacIOSurface::CreateIOSurface(aSize.width, aSize.height);
    mCg = ioSurface->CreateIOSurfaceContext();
    
    
  }

  mFormat = SurfaceFormat::B8G8R8A8;

  if (!mCg || aType == BackendType::COREGRAPHICS) {
    int bitsPerComponent = 8;

    CGBitmapInfo bitinfo;
    if (aFormat == SurfaceFormat::A8) {
      if (mColorSpace)
        CGColorSpaceRelease(mColorSpace);
      mColorSpace = nullptr;
      bitinfo = kCGImageAlphaOnly;
      mFormat = SurfaceFormat::A8;
    } else {
      bitinfo = kCGBitmapByteOrder32Host;
      if (aFormat == SurfaceFormat::B8G8R8X8) {
        bitinfo |= kCGImageAlphaNoneSkipFirst;
        mFormat = aFormat;
      } else {
        bitinfo |= kCGImageAlphaPremultipliedFirst;
      }
    }
    
    mCg = CGBitmapContextCreate (aData,
                                 mSize.width,
                                 mSize.height,
                                 bitsPerComponent,
                                 aStride,
                                 mColorSpace,
                                 bitinfo);
  }

  assert(mCg);
  if (!mCg) {
    gfxCriticalError() << "Failed to create CG context";
    return false;
  }

  
  
  CGContextTranslateCTM(mCg, 0, mSize.height);
  CGContextScaleCTM(mCg, 1, -1);
  
  
  
  
  
  
  
  CGContextSetInterpolationQuality(mCg, kCGInterpolationLow);


  if (aType == BackendType::COREGRAPHICS_ACCELERATED) {
    
    
    
    ClearRect(Rect(0, 0, mSize.width, mSize.height));
  }

  return true;
}

static void
EnsureValidPremultipliedData(CGContextRef aContext)
{
  if (CGBitmapContextGetBitsPerPixel(aContext) != 32 ||
      CGBitmapContextGetAlphaInfo(aContext) != kCGImageAlphaPremultipliedFirst) {
    return;
  }

  uint8_t* bitmapData = (uint8_t*)CGBitmapContextGetData(aContext);
  int w = CGBitmapContextGetWidth(aContext);
  int h = CGBitmapContextGetHeight(aContext);
  int stride = CGBitmapContextGetBytesPerRow(aContext);
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      int i = y * stride + x * 4;
      uint8_t a = bitmapData[i + 3];

      
      if (bitmapData[i + 0] > a) {
        bitmapData[i + 0] = a;
      }
      if (bitmapData[i + 1] > a) {
        bitmapData[i + 1] = a;
      }
      if (bitmapData[i + 2] > a) {
        bitmapData[i + 2] = a;
      }
    }
  }
}

void
DrawTargetCG::Flush()
{
  if (GetContextType(mCg) == CG_CONTEXT_TYPE_IOSURFACE) {
    CGContextFlush(mCg);
  } else if (GetContextType(mCg) == CG_CONTEXT_TYPE_BITMAP &&
             mMayContainInvalidPremultipliedData) {
    
    
    
    
    
    
    
    
    EnsureValidPremultipliedData(mCg);
    mMayContainInvalidPremultipliedData = false;
  }
}

bool
DrawTargetCG::Init(CGContextRef cgContext, const IntSize &aSize)
{
  
  
  if (aSize.width == 0 || aSize.height == 0) {
    mColorSpace = nullptr;
    mCg = nullptr;
    return false;
  }

  

  
  mColorSpace = CGColorSpaceCreateDeviceRGB();

  mSize = aSize;

  mCg = cgContext;
  CGContextRetain(mCg);

  assert(mCg);
  if (!mCg) {
    gfxCriticalError() << "Invalid CG context at Init";
    return false;
  }

  
  
  
  
  
  
  

  mFormat = SurfaceFormat::B8G8R8A8;
  if (GetContextType(mCg) == CG_CONTEXT_TYPE_BITMAP) {
    CGColorSpaceRef colorspace;
    CGBitmapInfo bitinfo = CGBitmapContextGetBitmapInfo(mCg);
    colorspace = CGBitmapContextGetColorSpace (mCg);
    if (CGColorSpaceGetNumberOfComponents(colorspace) == 1) {
      mFormat = SurfaceFormat::A8;
    } else if ((bitinfo & kCGBitmapAlphaInfoMask) == kCGImageAlphaNoneSkipFirst) {
      mFormat = SurfaceFormat::B8G8R8X8;
    }
  }

  return true;
}

bool
DrawTargetCG::Init(BackendType aType, const IntSize &aSize, SurfaceFormat &aFormat)
{
  int32_t stride = GetAlignedStride<16>(aSize.width * BytesPerPixel(aFormat));
  
  
  return Init(aType, nullptr, aSize, stride, aFormat);
}

TemporaryRef<PathBuilder>
DrawTargetCG::CreatePathBuilder(FillRule aFillRule) const
{
  return new PathBuilderCG(aFillRule);
}

void*
DrawTargetCG::GetNativeSurface(NativeSurfaceType aType)
{
  if ((aType == NativeSurfaceType::CGCONTEXT && GetContextType(mCg) == CG_CONTEXT_TYPE_BITMAP) ||
      (aType == NativeSurfaceType::CGCONTEXT_ACCELERATED && GetContextType(mCg) == CG_CONTEXT_TYPE_IOSURFACE)) {
    return mCg;
  } else {
    return nullptr;
  }
}

void
DrawTargetCG::Mask(const Pattern &aSource,
                   const Pattern &aMask,
                   const DrawOptions &aDrawOptions)
{
  MarkChanged();

  CGContextSaveGState(mCg);

  if (isGradient(aMask)) {
    assert(0);
  } else {
    if (aMask.GetType() == PatternType::COLOR) {
      DrawOptions drawOptions(aDrawOptions);
      const Color& color = static_cast<const ColorPattern&>(aMask).mColor;
      drawOptions.mAlpha *= color.a;
      assert(0);
      
      
      
    } else if (aMask.GetType() == PatternType::SURFACE) {
      const SurfacePattern& pat = static_cast<const SurfacePattern&>(aMask);
      CGImageRef mask = GetRetainedImageFromSourceSurface(pat.mSurface.get());
      MOZ_ASSERT(pat.mSamplingRect.IsEmpty(), "Sampling rect not supported with masks!");
      Rect rect(0,0, CGImageGetWidth(mask), CGImageGetHeight(mask));
      
      CGContextClipToMask(mCg, RectToCGRect(rect), mask);
      FillRect(rect, aSource, aDrawOptions);
      CGImageRelease(mask);
    }
  }

  CGContextRestoreGState(mCg);
}

void
DrawTargetCG::PushClipRect(const Rect &aRect)
{
  if (MOZ2D_ERROR_IF(!mCg)) {
    return;
  }

#ifdef DEBUG
  mSavedClipBounds.push_back(CGContextGetClipBoundingBox(mCg));
#endif

  CGContextSaveGState(mCg);

  

  CGAffineTransform previousTransform = CGContextGetCTM(mCg);
  CGContextConcatCTM(mCg, GfxMatrixToCGAffineTransform(mTransform));
  CGContextClipToRect(mCg, RectToCGRect(aRect));
  CGContextSetCTM(mCg, previousTransform);
}


void
DrawTargetCG::PushClip(const Path *aPath)
{
  if (MOZ2D_ERROR_IF(!mCg)) {
    return;
  }

#ifdef DEBUG
  mSavedClipBounds.push_back(CGContextGetClipBoundingBox(mCg));
#endif

  CGContextSaveGState(mCg);

  CGContextBeginPath(mCg);
  assert(aPath->GetBackendType() == BackendType::COREGRAPHICS);

  const PathCG *cgPath = static_cast<const PathCG*>(aPath);

  
  
  
  if (CGPathIsEmpty(cgPath->GetPath())) {
    
    CGContextClipToRect(mCg, CGRectZero);
  }


  


  CGContextSaveGState(mCg);
  CGContextConcatCTM(mCg, GfxMatrixToCGAffineTransform(mTransform));
  CGContextAddPath(mCg, cgPath->GetPath());
  CGContextRestoreGState(mCg);

  if (cgPath->GetFillRule() == FillRule::FILL_EVEN_ODD)
    CGContextEOClip(mCg);
  else
    CGContextClip(mCg);
}

void
DrawTargetCG::PopClip()
{
  CGContextRestoreGState(mCg);

#ifdef DEBUG
  MOZ_ASSERT(!mSavedClipBounds.empty(), "Unbalanced PopClip");
  MOZ_ASSERT(CGRectEqualToRect(mSavedClipBounds.back(), CGContextGetClipBoundingBox(mCg)),
             "PopClip didn't restore original clip");
  mSavedClipBounds.pop_back();
#endif
}

void
DrawTargetCG::MarkChanged()
{
  if (mSnapshot) {
    if (mSnapshot->refCount() > 1) {
      
      mSnapshot->DrawTargetWillChange();
    }
    mSnapshot = nullptr;
  }
}

CGContextRef
BorrowedCGContext::BorrowCGContextFromDrawTarget(DrawTarget *aDT)
{
  if ((aDT->GetBackendType() == BackendType::COREGRAPHICS ||
       aDT->GetBackendType() == BackendType::COREGRAPHICS_ACCELERATED) &&
      !aDT->IsTiledDrawTarget() && !aDT->IsDualDrawTarget()) {
    DrawTargetCG* cgDT = static_cast<DrawTargetCG*>(aDT);
    cgDT->Flush();
    cgDT->MarkChanged();

    
    CGContextRef cg = cgDT->mCg;
    if (MOZ2D_ERROR_IF(!cg)) {
      return nullptr;
    }
    cgDT->mCg = nullptr;

    
    CGContextSaveGState(cg);

    CGContextConcatCTM(cg, GfxMatrixToCGAffineTransform(cgDT->mTransform));

    return cg;
  }
  return nullptr;
}

void
BorrowedCGContext::ReturnCGContextToDrawTarget(DrawTarget *aDT, CGContextRef cg)
{
  DrawTargetCG* cgDT = static_cast<DrawTargetCG*>(aDT);

  CGContextRestoreGState(cg);
  cgDT->mCg = cg;
}


}
}
