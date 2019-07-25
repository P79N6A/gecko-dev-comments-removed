



#include "DrawTargetCG.h"
#include "SourceSurfaceCG.h"
#include "Rect.h"
#include "ScaledFontMac.h"
#include "Tools.h"
#include <vector>




CG_EXTERN void CGContextSetCTM(CGContextRef, CGAffineTransform);

namespace mozilla {
namespace gfx {

static CGRect RectToCGRect(Rect r)
{
  return CGRectMake(r.x, r.y, r.width, r.height);
}

static CGRect IntRectToCGRect(IntRect r)
{
  return CGRectMake(r.x, r.y, r.width, r.height);
}

CGBlendMode ToBlendMode(CompositionOp op)
{
  CGBlendMode mode;
  switch (op) {
    case OP_OVER:
      mode = kCGBlendModeNormal;
      break;
    case OP_ADD:
      mode = kCGBlendModePlusLighter;
      break;
    case OP_ATOP:
      mode = kCGBlendModeSourceAtop;
      break;
    case OP_OUT:
      mode = kCGBlendModeSourceOut;
      break;
    case OP_IN:
      mode = kCGBlendModeSourceIn;
      break;
    case OP_SOURCE:
      mode = kCGBlendModeCopy;
      break;
    case OP_DEST_IN:
      mode = kCGBlendModeDestinationIn;
      break;
    case OP_DEST_OUT:
      mode = kCGBlendModeDestinationOut;
      break;
    case OP_DEST_OVER:
      mode = kCGBlendModeDestinationOver;
      break;
    case OP_DEST_ATOP:
      mode = kCGBlendModeDestinationAtop;
      break;
    case OP_XOR:
      mode = kCGBlendModeXOR;
      break;
      



    default:
      mode = kCGBlendModeNormal;
  }
  return mode;
}



DrawTargetCG::DrawTargetCG() : mSnapshot(NULL)
{
}

DrawTargetCG::~DrawTargetCG()
{
  MarkChanged();

  
  if (mColorSpace)
    CGColorSpaceRelease(mColorSpace);
  if (mCg)
    CGContextRelease(mCg);
  free(mData);
}

TemporaryRef<SourceSurface>
DrawTargetCG::Snapshot()
{
  if (!mSnapshot) {
    mSnapshot = new SourceSurfaceCGBitmapContext(this);
  }

  return mSnapshot;
}

TemporaryRef<DrawTarget>
DrawTargetCG::CreateSimilarDrawTarget(const IntSize &aSize, SurfaceFormat aFormat) const
{
  
  
  RefPtr<DrawTargetCG> newTarget = new DrawTargetCG();
  if (newTarget->Init(aSize, aFormat)) {
    return newTarget;
  } else {
    return NULL;
  }
}

TemporaryRef<SourceSurface>
DrawTargetCG::CreateSourceSurfaceFromData(unsigned char *aData,
                                           const IntSize &aSize,
                                           int32_t aStride,
                                           SurfaceFormat aFormat) const
{
  RefPtr<SourceSurfaceCG> newSurf = new SourceSurfaceCG();

 if (!newSurf->InitFromData(aData, aSize, aStride, aFormat)) {
    return NULL;
  }

  return newSurf;
}

static CGImageRef
GetImageFromSourceSurface(SourceSurface *aSurface)
{
  if (aSurface->GetType() == SURFACE_COREGRAPHICS_IMAGE)
    return static_cast<SourceSurfaceCG*>(aSurface)->GetImage();
  else if (aSurface->GetType() == SURFACE_COREGRAPHICS_CGCONTEXT)
    return static_cast<SourceSurfaceCGBitmapContext*>(aSurface)->GetImage();
  else if (aSurface->GetType() == SURFACE_DATA)
    return static_cast<DataSourceSurfaceCG*>(aSurface)->GetImage();
  abort();
}

TemporaryRef<SourceSurface>
DrawTargetCG::OptimizeSourceSurface(SourceSurface *aSurface) const
{
  return NULL;
}

class UnboundnessFixer
{
    CGRect mClipBounds;
    CGLayerRef mLayer;
    CGContextRef mCg;
  public:
    UnboundnessFixer() : mCg(NULL) {}

    CGContextRef Check(CGContextRef baseCg, CompositionOp blend)
    {
      if (!IsOperatorBoundByMask(blend)) {
        mClipBounds = CGContextGetClipBoundingBox(baseCg);
        
        

        
        
        mLayer = CGLayerCreateWithContext(baseCg, mClipBounds.size, NULL);
        
        mCg = CGLayerGetContext(mLayer);
        
        
        
        CGContextTranslateCTM(mCg, -mClipBounds.origin.x, mClipBounds.origin.y + mClipBounds.size.height);
        CGContextScaleCTM(mCg, 1, -1);

        return mCg;
      } else {
        return baseCg;
      }
    }

    void Fix(CGContextRef baseCg)
    {
        if (mCg) {
            CGContextTranslateCTM(baseCg, 0, mClipBounds.size.height);
            CGContextScaleCTM(baseCg, 1, -1);
            mClipBounds.origin.y *= -1;
            CGContextDrawLayerAtPoint(baseCg, mClipBounds.origin, mLayer);
            CGContextRelease(mCg);
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
  MarkChanged();

  CGImageRef image;
  CGImageRef subimage = NULL;
  CGContextSaveGState(mCg);

  CGContextSetBlendMode(mCg, ToBlendMode(aDrawOptions.mCompositionOp));
  UnboundnessFixer fixer;
  CGContextRef cg = fixer.Check(mCg, aDrawOptions.mCompositionOp);
  CGContextSetAlpha(cg, aDrawOptions.mAlpha);

  CGContextConcatCTM(cg, GfxMatrixToCGAffineTransform(mTransform));
  image = GetImageFromSourceSurface(aSurface);
  


  {
    subimage = CGImageCreateWithImageInRect(image, RectToCGRect(aSource));
    image = subimage;
  }

  CGContextScaleCTM(cg, 1, -1);

  CGRect flippedRect = CGRectMake(aDest.x, -(aDest.y + aDest.height),
                                  aDest.width, aDest.height);

  
  if (aSurfOptions.mFilter == FILTER_POINT)
    CGContextSetInterpolationQuality(cg, kCGInterpolationNone);

  CGContextDrawImage(cg, flippedRect, image);

  fixer.Fix(mCg);

  CGContextRestoreGState(mCg);

  CGImageRelease(subimage);
}

static CGColorRef ColorToCGColor(CGColorSpaceRef aColorSpace, const Color& aColor)
{
  CGFloat components[4] = {aColor.r, aColor.g, aColor.b, aColor.a};
  return CGColorCreate(aColorSpace, components);
}

class GradientStopsCG : public GradientStops
{
  public:
  
  GradientStopsCG(GradientStop* aStops, uint32_t aNumStops, ExtendMode aExtendMode)
  {
    
    
    
    std::vector<CGFloat> colors;
    std::vector<CGFloat> offsets;
    colors.reserve(aNumStops*4);
    offsets.reserve(aNumStops);

    for (uint32_t i = 0; i < aNumStops; i++) {
      colors.push_back(aStops[i].color.r);
      colors.push_back(aStops[i].color.g);
      colors.push_back(aStops[i].color.b);
      colors.push_back(aStops[i].color.a);

      offsets.push_back(aStops[i].offset);
    }

    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    mGradient = CGGradientCreateWithColorComponents(colorSpace,
                                                    &colors.front(),
                                                    &offsets.front(),
                                                    aNumStops);
    CGColorSpaceRelease(colorSpace);
  }
  virtual ~GradientStopsCG() {
    CGGradientRelease(mGradient);
  }
  BackendType GetBackendType() const { return BACKEND_COREGRAPHICS; }
  CGGradientRef mGradient;
};

TemporaryRef<GradientStops>
DrawTargetCG::CreateGradientStops(GradientStop *aStops, uint32_t aNumStops,
                                  ExtendMode aExtendMode) const
{
  return new GradientStopsCG(aStops, aNumStops, aExtendMode);
}

static void
DrawGradient(CGContextRef cg, const Pattern &aPattern)
{
  if (aPattern.GetType() == PATTERN_LINEAR_GRADIENT) {
    const LinearGradientPattern& pat = static_cast<const LinearGradientPattern&>(aPattern);
    GradientStopsCG *stops = static_cast<GradientStopsCG*>(pat.mStops.get());
    
    CGPoint startPoint = { pat.mBegin.x, pat.mBegin.y };
    CGPoint endPoint   = { pat.mEnd.x,   pat.mEnd.y };

    
    
    

    CGContextDrawLinearGradient(cg, stops->mGradient, startPoint, endPoint,
                                kCGGradientDrawsBeforeStartLocation | kCGGradientDrawsAfterEndLocation);
  } else if (aPattern.GetType() == PATTERN_RADIAL_GRADIENT) {
    const RadialGradientPattern& pat = static_cast<const RadialGradientPattern&>(aPattern);
    GradientStopsCG *stops = static_cast<GradientStopsCG*>(pat.mStops.get());

    
    CGPoint startCenter = { pat.mCenter1.x, pat.mCenter1.y };
    CGFloat startRadius = pat.mRadius1;
    CGPoint endCenter   = { pat.mCenter2.x, pat.mCenter2.y };
    CGFloat endRadius   = pat.mRadius2;

    
    CGContextDrawRadialGradient(cg, stops->mGradient, startCenter, startRadius, endCenter, endRadius,
                                kCGGradientDrawsBeforeStartLocation | kCGGradientDrawsAfterEndLocation);
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
  return aPattern.GetType() == PATTERN_LINEAR_GRADIENT || aPattern.GetType() == PATTERN_RADIAL_GRADIENT;
}



static CGPatternRef
CreateCGPattern(const Pattern &aPattern, CGAffineTransform aUserSpace)
{
  const SurfacePattern& pat = static_cast<const SurfacePattern&>(aPattern);
  
  CGImageRef image = GetImageFromSourceSurface(pat.mSurface.get());
  CGFloat xStep, yStep;
  switch (pat.mExtendMode) {
    case EXTEND_CLAMP:
      
      xStep = static_cast<CGFloat>(1 << 22);
      yStep = static_cast<CGFloat>(1 << 22);
      break;
    case EXTEND_REFLECT:
      assert(0);
    case EXTEND_REPEAT:
      xStep = static_cast<CGFloat>(CGImageGetWidth(image));
      yStep = static_cast<CGFloat>(CGImageGetHeight(image));
      
      
      
      
      
      
      
      
      
  }

  
  
  
  CGRect bounds = {
    {0, 0,},
    {static_cast<CGFloat>(CGImageGetWidth(image)), static_cast<CGFloat>(CGImageGetHeight(image))}
  };
  CGAffineTransform transform = CGAffineTransformConcat(CGAffineTransformMakeScale(1, -1), aUserSpace);
  transform = CGAffineTransformTranslate(transform, 0, -static_cast<float>(CGImageGetHeight(image)));
  return CGPatternCreate(CGImageRetain(image), bounds, transform, xStep, yStep, kCGPatternTilingConstantSpacing,
                         true, &patternCallbacks);
}

static void
SetFillFromPattern(CGContextRef cg, CGColorSpaceRef aColorSpace, const Pattern &aPattern)
{
  assert(!isGradient(aPattern));
  if (aPattern.GetType() == PATTERN_COLOR) {

    const Color& color = static_cast<const ColorPattern&>(aPattern).mColor;
    
    CGColorRef cgcolor = ColorToCGColor(aColorSpace, color);
    CGContextSetFillColorWithColor(cg, cgcolor);
    CGColorRelease(cgcolor);
  } else if (aPattern.GetType() == PATTERN_SURFACE) {

    CGColorSpaceRef patternSpace;
    patternSpace = CGColorSpaceCreatePattern (NULL);
    CGContextSetFillColorSpace(cg, patternSpace);
    CGColorSpaceRelease(patternSpace);

    CGPatternRef pattern = CreateCGPattern(aPattern, CGContextGetCTM(cg));
    CGFloat alpha = 1.;
    CGContextSetFillPattern(cg, pattern, &alpha);
    CGPatternRelease(pattern);
  }
}

static void
SetStrokeFromPattern(CGContextRef cg, CGColorSpaceRef aColorSpace, const Pattern &aPattern)
{
  assert(!isGradient(aPattern));
  if (aPattern.GetType() == PATTERN_COLOR) {
    const Color& color = static_cast<const ColorPattern&>(aPattern).mColor;
    
    CGColorRef cgcolor = ColorToCGColor(aColorSpace, color);
    CGContextSetStrokeColorWithColor(cg, cgcolor);
    CGColorRelease(cgcolor);
  } else if (aPattern.GetType() == PATTERN_SURFACE) {
    CGColorSpaceRef patternSpace;
    patternSpace = CGColorSpaceCreatePattern (NULL);
    CGContextSetStrokeColorSpace(cg, patternSpace);
    CGColorSpaceRelease(patternSpace);

    CGPatternRef pattern = CreateCGPattern(aPattern, CGContextGetCTM(cg));
    CGFloat alpha = 1.;
    CGContextSetStrokePattern(cg, pattern, &alpha);
    CGPatternRelease(pattern);
  }

}


void
DrawTargetCG::FillRect(const Rect &aRect,
                        const Pattern &aPattern,
                        const DrawOptions &aDrawOptions)
{
  MarkChanged();

  CGContextSaveGState(mCg);

  UnboundnessFixer fixer;
  CGContextRef cg = fixer.Check(mCg, aDrawOptions.mCompositionOp);
  CGContextSetAlpha(mCg, aDrawOptions.mAlpha);
  CGContextSetBlendMode(mCg, ToBlendMode(aDrawOptions.mCompositionOp));

  CGContextConcatCTM(cg, GfxMatrixToCGAffineTransform(mTransform));

  if (isGradient(aPattern)) {
    CGContextClipToRect(cg, RectToCGRect(aRect));
    DrawGradient(cg, aPattern);
  } else {
    SetFillFromPattern(cg, mColorSpace, aPattern);
    CGContextFillRect(cg, RectToCGRect(aRect));
  }

  fixer.Fix(mCg);
  CGContextRestoreGState(mCg);
}

void
DrawTargetCG::StrokeLine(const Point &p1, const Point &p2, const Pattern &aPattern, const StrokeOptions &aStrokeOptions, const DrawOptions &aDrawOptions)
{
  MarkChanged();

  CGContextSaveGState(mCg);

  UnboundnessFixer fixer;
  CGContextRef cg = fixer.Check(mCg, aDrawOptions.mCompositionOp);
  CGContextSetAlpha(mCg, aDrawOptions.mAlpha);
  CGContextSetBlendMode(mCg, ToBlendMode(aDrawOptions.mCompositionOp));

  CGContextConcatCTM(cg, GfxMatrixToCGAffineTransform(mTransform));

  CGContextBeginPath(cg);
  CGContextMoveToPoint(cg, p1.x, p1.y);
  CGContextAddLineToPoint(cg, p2.x, p2.y);

  SetStrokeOptions(cg, aStrokeOptions);

  if (isGradient(aPattern)) {
    CGContextReplacePathWithStrokedPath(cg);
    
    CGContextClip(cg);
    DrawGradient(cg, aPattern);
  } else {
    SetStrokeFromPattern(cg, mColorSpace, aPattern);
    CGContextStrokePath(cg);
  }

  fixer.Fix(mCg);
  CGContextRestoreGState(mCg);
}

void
DrawTargetCG::StrokeRect(const Rect &aRect,
                         const Pattern &aPattern,
                         const StrokeOptions &aStrokeOptions,
                         const DrawOptions &aDrawOptions)
{
  MarkChanged();

  CGContextSaveGState(mCg);

  UnboundnessFixer fixer;
  CGContextRef cg = fixer.Check(mCg, aDrawOptions.mCompositionOp);
  CGContextSetAlpha(mCg, aDrawOptions.mAlpha);
  CGContextSetBlendMode(mCg, ToBlendMode(aDrawOptions.mCompositionOp));

  CGContextConcatCTM(cg, GfxMatrixToCGAffineTransform(mTransform));

  
  
  switch (aStrokeOptions.mLineJoin)
  {
    case JOIN_BEVEL:
      CGContextSetLineJoin(cg, kCGLineJoinBevel);
      break;
    case JOIN_ROUND:
      CGContextSetLineJoin(cg, kCGLineJoinRound);
      break;
    case JOIN_MITER:
    case JOIN_MITER_OR_BEVEL:
      CGContextSetLineJoin(cg, kCGLineJoinMiter);
      break;
  }
  CGContextSetLineWidth(cg, aStrokeOptions.mLineWidth);

  if (isGradient(aPattern)) {
    
    CGContextBeginPath(cg);
    CGContextAddRect(cg, RectToCGRect(aRect));
    CGContextReplacePathWithStrokedPath(cg);
    
    CGContextClip(cg);
    DrawGradient(cg, aPattern);
  } else {
    SetStrokeFromPattern(cg, mColorSpace, aPattern);
    CGContextStrokeRect(cg, RectToCGRect(aRect));
  }

  fixer.Fix(mCg);
  CGContextRestoreGState(mCg);
}


void
DrawTargetCG::ClearRect(const Rect &aRect)
{
  MarkChanged();

  CGContextSaveGState(mCg);
  CGContextConcatCTM(mCg, GfxMatrixToCGAffineTransform(mTransform));

  CGContextClearRect(mCg, RectToCGRect(aRect));

  CGContextRestoreGState(mCg);
}

void
DrawTargetCG::Stroke(const Path *aPath, const Pattern &aPattern, const StrokeOptions &aStrokeOptions, const DrawOptions &aDrawOptions)
{
  MarkChanged();

  CGContextSaveGState(mCg);

  UnboundnessFixer fixer;
  CGContextRef cg = fixer.Check(mCg, aDrawOptions.mCompositionOp);
  CGContextSetAlpha(mCg, aDrawOptions.mAlpha);
  CGContextSetBlendMode(mCg, ToBlendMode(aDrawOptions.mCompositionOp));

  CGContextConcatCTM(cg, GfxMatrixToCGAffineTransform(mTransform));


  CGContextBeginPath(cg);

  assert(aPath->GetBackendType() == BACKEND_COREGRAPHICS);
  const PathCG *cgPath = static_cast<const PathCG*>(aPath);
  CGContextAddPath(cg, cgPath->GetPath());

  SetStrokeOptions(cg, aStrokeOptions);

  if (isGradient(aPattern)) {
    CGContextReplacePathWithStrokedPath(cg);
    
    CGContextClip(cg);
    DrawGradient(cg, aPattern);
  } else {
    CGContextBeginPath(cg);
    
    const PathCG *cgPath = static_cast<const PathCG*>(aPath);
    CGContextAddPath(cg, cgPath->GetPath());

    SetStrokeFromPattern(cg, mColorSpace, aPattern);
    CGContextStrokePath(cg);
  }

  fixer.Fix(mCg);
  CGContextRestoreGState(mCg);
}

void
DrawTargetCG::Fill(const Path *aPath, const Pattern &aPattern, const DrawOptions &aDrawOptions)
{
  MarkChanged();

  assert(aPath->GetBackendType() == BACKEND_COREGRAPHICS);

  CGContextSaveGState(mCg);

  CGContextSetBlendMode(mCg, ToBlendMode(aDrawOptions.mCompositionOp));
  UnboundnessFixer fixer;
  CGContextRef cg = fixer.Check(mCg, aDrawOptions.mCompositionOp);
  CGContextSetAlpha(cg, aDrawOptions.mAlpha);

  CGContextConcatCTM(cg, GfxMatrixToCGAffineTransform(mTransform));

  CGContextBeginPath(cg);
  
  const PathCG *cgPath = static_cast<const PathCG*>(aPath);

  if (isGradient(aPattern)) {
    
    if (CGPathIsEmpty(cgPath->GetPath())) {
      
      
      CGContextClipToRect(mCg, CGRectZero);
    } else {
      CGContextAddPath(cg, cgPath->GetPath());
      if (cgPath->GetFillRule() == FILL_EVEN_ODD)
        CGContextEOClip(mCg);
      else
        CGContextClip(mCg);
    }

    DrawGradient(cg, aPattern);
  } else {
    CGContextAddPath(cg, cgPath->GetPath());

    SetFillFromPattern(cg, mColorSpace, aPattern);

    if (cgPath->GetFillRule() == FILL_EVEN_ODD)
      CGContextEOFillPath(cg);
    else
      CGContextFillPath(cg);
  }

  fixer.Fix(mCg);
  CGContextRestoreGState(mCg);
}


void
DrawTargetCG::FillGlyphs(ScaledFont *aFont, const GlyphBuffer &aBuffer, const Pattern &aPattern, const DrawOptions &aDrawOptions,
                         const GlyphRenderingOptions*)
{
  MarkChanged();

  assert(aBuffer.mNumGlyphs);
  CGContextSaveGState(mCg);

  CGContextSetBlendMode(mCg, ToBlendMode(aDrawOptions.mCompositionOp));
  UnboundnessFixer fixer;
  CGContextRef cg = fixer.Check(mCg, aDrawOptions.mCompositionOp);
  CGContextSetAlpha(cg, aDrawOptions.mAlpha);

  CGContextConcatCTM(cg, GfxMatrixToCGAffineTransform(mTransform));

  ScaledFontMac* cgFont = static_cast<ScaledFontMac*>(aFont);
  CGContextSetFont(cg, cgFont->mFont);
  CGContextSetFontSize(cg, cgFont->mSize);

  
  std::vector<CGGlyph> glyphs;
  std::vector<CGPoint> positions;
  glyphs.resize(aBuffer.mNumGlyphs);
  positions.resize(aBuffer.mNumGlyphs);

  
  CGAffineTransform matrix = CGAffineTransformMakeScale(1, -1);
  CGContextConcatCTM(cg, matrix);
  
  
  
  

  for (unsigned int i = 0; i < aBuffer.mNumGlyphs; i++) {
    glyphs[i] = aBuffer.mGlyphs[i].mIndex;
    
    positions[i] = CGPointMake(aBuffer.mGlyphs[i].mPosition.x,
                              -aBuffer.mGlyphs[i].mPosition.y);
  }

  
  if (isGradient(aPattern)) {
    CGContextSetTextDrawingMode(cg, kCGTextClip);
    CGContextShowGlyphsAtPositions(cg, &glyphs.front(), &positions.front(), aBuffer.mNumGlyphs);
    DrawGradient(cg, aPattern);
  } else {
    
    
    CGContextSetTextDrawingMode(cg, kCGTextFill);
    SetFillFromPattern(cg, mColorSpace, aPattern);
    CGContextShowGlyphsAtPositions(cg, &glyphs.front(), &positions.front(), aBuffer.mNumGlyphs);
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
  MarkChanged();

  CGImageRef image;
  CGImageRef subimage = NULL;
  if (aSurface->GetType() == SURFACE_COREGRAPHICS_IMAGE) {
    image = GetImageFromSourceSurface(aSurface);
    


    {
      subimage = CGImageCreateWithImageInRect(image, IntRectToCGRect(aSourceRect));
      image = subimage;
    }
    

    CGContextSaveGState(mCg);

    
    CGContextResetClip(mCg);
    CGContextSetBlendMode(mCg, kCGBlendModeCopy);

    CGContextScaleCTM(mCg, 1, -1);

    CGRect flippedRect = CGRectMake(aDestination.x, -(aDestination.y + aSourceRect.height),
                                    aSourceRect.width, aSourceRect.height);

    CGContextDrawImage(mCg, flippedRect, image);

    CGContextRestoreGState(mCg);

    CGImageRelease(subimage);
  }
}

void
DrawTargetCG::DrawSurfaceWithShadow(SourceSurface *aSurface, const Point &aDest, const Color &aColor, const Point &aOffset, Float aSigma, CompositionOp aOperator)
{
  MarkChanged();

  CGImageRef image;
  image = GetImageFromSourceSurface(aSurface);

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

  CGContextRestoreGState(mCg);

}

bool
DrawTargetCG::Init(unsigned char* aData,
                   const IntSize &aSize,
                   int32_t aStride,
                   SurfaceFormat aFormat)
{
  
  
  if (aSize.width <= 0 || aSize.height <= 0 ||
      
      
      aSize.width > 32767 || aSize.height > 32767) {
    mColorSpace = NULL;
    mCg = NULL;
    mData = NULL;
    return false;
  }

  

  
  mColorSpace = CGColorSpaceCreateDeviceRGB();

  if (aData == NULL) {
    
    mData = calloc(aSize.height * aStride, 1);
    aData = static_cast<unsigned char*>(mData);  
  } else {
    
    
    mData = NULL;
  }

  mSize = aSize;
  
  int bitsPerComponent = 8;

  CGBitmapInfo bitinfo;

  bitinfo = kCGBitmapByteOrder32Host | kCGImageAlphaPremultipliedFirst;

  
  mCg = CGBitmapContextCreate (aData,
                               mSize.width,
                               mSize.height,
                               bitsPerComponent,
                               aStride,
                               mColorSpace,
                               bitinfo);


  assert(mCg);
  
  
  CGContextTranslateCTM(mCg, 0, mSize.height);
  CGContextScaleCTM(mCg, 1, -1);
  
  
  
  
  
  
  
  CGContextSetInterpolationQuality(mCg, kCGInterpolationLow);

  
  mFormat = FORMAT_B8G8R8A8;

  return true;
}

bool
DrawTargetCG::Init(CGContextRef cgContext, const IntSize &aSize)
{
  
  
  if (aSize.width == 0 || aSize.height == 0) {
    mColorSpace = NULL;
    mCg = NULL;
    mData = NULL;
    return false;
  }

  

  
  mColorSpace = CGColorSpaceCreateDeviceRGB();

  mSize = aSize;

  mCg = cgContext;

  mData = NULL;

  assert(mCg);
  
  
  CGContextTranslateCTM(mCg, 0, mSize.height);
  CGContextScaleCTM(mCg, 1, -1);

  
  mFormat = FORMAT_B8G8R8A8;

  return true;
}

bool
DrawTargetCG::Init(const IntSize &aSize, SurfaceFormat &aFormat)
{
  int stride = aSize.width*4;
  
  
  return Init(NULL, aSize, stride, aFormat);
}

TemporaryRef<PathBuilder>
DrawTargetCG::CreatePathBuilder(FillRule aFillRule) const
{
  RefPtr<PathBuilderCG> pb = new PathBuilderCG(aFillRule);
  return pb;
}

void*
DrawTargetCG::GetNativeSurface(NativeSurfaceType aType)
{
  if (aType == NATIVE_SURFACE_CGCONTEXT) {
    return mCg;
  } else {
    return NULL;
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
    if (aMask.GetType() == PATTERN_COLOR) {
      DrawOptions drawOptions(aDrawOptions);
      const Color& color = static_cast<const ColorPattern&>(aMask).mColor;
      drawOptions.mAlpha *= color.a;
      assert(0);
      
      
      
    } else if (aMask.GetType() == PATTERN_SURFACE) {
      const SurfacePattern& pat = static_cast<const SurfacePattern&>(aMask);
      CGImageRef mask = GetImageFromSourceSurface(pat.mSurface.get());
      Rect rect(0,0, CGImageGetWidth(mask), CGImageGetHeight(mask));
      
      CGContextClipToMask(mCg, RectToCGRect(rect), mask);
      FillRect(rect, aSource, aDrawOptions);
    }
  }

  CGContextRestoreGState(mCg);
}

void
DrawTargetCG::PushClipRect(const Rect &aRect)
{
  CGContextSaveGState(mCg);

  

  CGAffineTransform previousTransform = CGContextGetCTM(mCg);
  CGContextConcatCTM(mCg, GfxMatrixToCGAffineTransform(mTransform));
  CGContextClipToRect(mCg, RectToCGRect(aRect));
  CGContextSetCTM(mCg, previousTransform);
}


void
DrawTargetCG::PushClip(const Path *aPath)
{
  CGContextSaveGState(mCg);

  CGContextBeginPath(mCg);
  assert(aPath->GetBackendType() == BACKEND_COREGRAPHICS);

  const PathCG *cgPath = static_cast<const PathCG*>(aPath);

  
  
  
  if (CGPathIsEmpty(cgPath->GetPath())) {
    
    CGContextClipToRect(mCg, CGRectZero);
  }


  


  CGContextSaveGState(mCg);
  CGContextConcatCTM(mCg, GfxMatrixToCGAffineTransform(mTransform));
  CGContextAddPath(mCg, cgPath->GetPath());
  CGContextRestoreGState(mCg);

  if (cgPath->GetFillRule() == FILL_EVEN_ODD)
    CGContextEOClip(mCg);
  else
    CGContextClip(mCg);
}

void
DrawTargetCG::PopClip()
{
  CGContextRestoreGState(mCg);
}

void
DrawTargetCG::MarkChanged()
{
  if (mSnapshot) {
    if (mSnapshot->refCount() > 1) {
      
      mSnapshot->DrawTargetWillChange();
    }
    mSnapshot = NULL;
  }
}



}
}
