



































#include "DrawTargetCG.h"
#include "SourceSurfaceCG.h"
#include "Rect.h"



namespace mozilla {
namespace gfx {

static CGRect RectToCGRect(Rect r)
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
    case OP_SOURCE:
      mode = kCGBlendModeCopy;
      break;
    case OP_CLEAR:
      mode = kCGBlendModeClear;
      break;
    case OP_ADD:
      mode = kCGBlendModePlusLighter;
      break;
    case OP_ATOP:
      mode = kCGBlendModeSourceAtop;
      break;
    default:
      mode = kCGBlendModeNormal;
  }
  return mode;
}



DrawTargetCG::DrawTargetCG()
{
}

DrawTargetCG::~DrawTargetCG()
{
}

TemporaryRef<SourceSurface>
DrawTargetCG::Snapshot()
{
  return NULL;
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

TemporaryRef<SourceSurface>
DrawTargetCG::OptimizeSourceSurface(SourceSurface *aSurface) const
{
  return NULL;
}

void
DrawTargetCG::DrawSurface(SourceSurface *aSurface,
                           const Rect &aDest,
                           const Rect &aSource,
                           const DrawOptions &aOptions,
                           const DrawSurfaceOptions &aSurfOptions)
{
  CGImageRef image;
  CGImageRef subimage = NULL;
  if (aSurface->GetType() == COREGRAPHICS_IMAGE) {
    image = static_cast<SourceSurfaceCG*>(aSurface)->GetImage();
    


    {
      subimage = CGImageCreateWithImageInRect(image, RectToCGRect(aSource));
      image = subimage;
    }

    CGContextDrawImage(mCg, RectToCGRect(aDest), image);

    CGImageRelease(subimage);
  }
}

void
DrawTargetCG::FillRect(const Rect &aRect,
                        const Pattern &aPattern,
                        const DrawOptions &aOptions)
{
  
  if (aPattern.GetType() == COLOR) {
    Color color = static_cast<const ColorPattern*>(&aPattern)->mColor;
    
    CGContextSetRGBFillColor(mCg, color.mR, color.mG, color.mB, color.mA);
  }

  CGContextSetBlendMode(mCg, ToBlendMode(aOptions.mCompositionOp));
  CGContextFillRect(mCg, RectToCGRect(aRect));
}


bool
DrawTargetCG::Init(const IntSize &aSize)
{
  CGColorSpaceRef cgColorspace;
  cgColorspace = CGColorSpaceCreateDeviceRGB();

  mSize = aSize;

  int bitsPerComponent = 8;
  int stride = mSize.width;

  CGBitmapInfo bitinfo;

  bitinfo = kCGBitmapByteOrder32Host | kCGImageAlphaPremultipliedFirst;

  
  mCg = CGBitmapContextCreate (NULL,
                         mSize.width,
			 mSize.height,
			 bitsPerComponent,
			 stride,
			 cgColorspace,
			 bitinfo);

  CGColorSpaceRelease (cgColorspace);

  return true;
}
}
}
