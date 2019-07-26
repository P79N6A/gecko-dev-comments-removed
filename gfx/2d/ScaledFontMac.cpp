




#include "ScaledFontMac.h"
#ifdef USE_SKIA
#include "PathSkia.h"
#include "skia/SkPaint.h"
#include "skia/SkPath.h"
#include "skia/SkTypeface_mac.h"
#endif
#include "DrawTargetCG.h"
#include <vector>
#include <dlfcn.h>


extern "C" {
CGPathRef CGFontGetGlyphPath(CGFontRef fontRef, CGAffineTransform *textTransform, int unknown, CGGlyph glyph);
};


namespace mozilla {
namespace gfx {

ScaledFontMac::CTFontDrawGlyphsFuncT* ScaledFontMac::CTFontDrawGlyphsPtr = nullptr;
bool ScaledFontMac::sSymbolLookupDone = false;

ScaledFontMac::ScaledFontMac(CGFontRef aFont, Float aSize)
  : ScaledFontBase(aSize)
{
  if (!sSymbolLookupDone) {
    CTFontDrawGlyphsPtr =
      (CTFontDrawGlyphsFuncT*)dlsym(RTLD_DEFAULT, "CTFontDrawGlyphs");
    sSymbolLookupDone = true;
  }

  
  mFont = CGFontRetain(aFont);
  if (CTFontDrawGlyphsPtr != nullptr) {
    
    mCTFont = CTFontCreateWithGraphicsFont(aFont, aSize, nullptr, nullptr);
  } else {
    mCTFont = nullptr;
  }
}

ScaledFontMac::~ScaledFontMac()
{
  if (mCTFont) {
    CFRelease(mCTFont);
  }
  CGFontRelease(mFont);
}

#ifdef USE_SKIA
SkTypeface* ScaledFontMac::GetSkTypeface()
{
  if (!mTypeface) {
    if (mCTFont) {
      mTypeface = SkCreateTypefaceFromCTFont(mCTFont);
    } else {
      CTFontRef fontFace = CTFontCreateWithGraphicsFont(mFont, mSize, nullptr, nullptr);
      mTypeface = SkCreateTypefaceFromCTFont(fontFace);
      CFRelease(fontFace);
    }
  }
  return mTypeface;
}
#endif








TemporaryRef<Path>
ScaledFontMac::GetPathForGlyphs(const GlyphBuffer &aBuffer, const DrawTarget *aTarget)
{
  if (aTarget->GetType() == BACKEND_COREGRAPHICS || aTarget->GetType() == BACKEND_COREGRAPHICS_ACCELERATED) {
      CGMutablePathRef path = CGPathCreateMutable();

      for (unsigned int i = 0; i < aBuffer.mNumGlyphs; i++) {
          
          CGAffineTransform flip = CGAffineTransformMakeScale(1, -1);
          CGPathRef glyphPath = ::CGFontGetGlyphPath(mFont, &flip, 0, aBuffer.mGlyphs[i].mIndex);

          CGAffineTransform matrix = CGAffineTransformMake(mSize, 0, 0, mSize,
                                                           aBuffer.mGlyphs[i].mPosition.x,
                                                           aBuffer.mGlyphs[i].mPosition.y);
          CGPathAddPath(path, &matrix, glyphPath);
          CGPathRelease(glyphPath);
      }
      TemporaryRef<Path> ret = new PathCG(path, FILL_WINDING);
      CGPathRelease(path);
      return ret;
  } else {
      return ScaledFontBase::GetPathForGlyphs(aBuffer, aTarget);
  }
}

void
ScaledFontMac::CopyGlyphsToBuilder(const GlyphBuffer &aBuffer, PathBuilder *aBuilder)
{
  PathBuilderCG *pathBuilderCG =
    static_cast<PathBuilderCG*>(aBuilder);
  
  for (unsigned int i = 0; i < aBuffer.mNumGlyphs; i++) {
    
    CGAffineTransform flip = CGAffineTransformMakeScale(1, -1);
    CGPathRef glyphPath = ::CGFontGetGlyphPath(mFont, &flip, 0, aBuffer.mGlyphs[i].mIndex);

    CGAffineTransform matrix = CGAffineTransformMake(mSize, 0, 0, mSize,
                                                     aBuffer.mGlyphs[i].mPosition.x,
                                                     aBuffer.mGlyphs[i].mPosition.y);
    CGPathAddPath(pathBuilderCG->mCGPath, &matrix, glyphPath);
    CGPathRelease(glyphPath);
  }
}


}
}
