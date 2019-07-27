




#ifndef MOZILLA_SVGCONTENTUTILS_H
#define MOZILLA_SVGCONTENTUTILS_H


#define _USE_MATH_DEFINES
#include <math.h>

#include "mozilla/gfx/2D.h" 
#include "mozilla/gfx/Matrix.h"
#include "mozilla/RangedPtr.h"
#include "nsError.h"
#include "nsStringFwd.h"
#include "gfx2DGlue.h"

class gfxTextContextPaint;
class nsIContent;
class nsIDocument;
class nsIFrame;
class nsStyleContext;
class nsStyleCoord;
class nsSVGElement;

namespace mozilla {
class SVGAnimatedPreserveAspectRatio;
class SVGPreserveAspectRatio;
namespace dom {
class Element;
class SVGSVGElement;
} 

namespace gfx {
class Matrix;
} 
} 

inline bool
IsSVGWhitespace(char aChar)
{
  return aChar == '\x20' || aChar == '\x9' ||
         aChar == '\xD'  || aChar == '\xA';
}

inline bool
IsSVGWhitespace(char16_t aChar)
{
  return aChar == char16_t('\x20') || aChar == char16_t('\x9') ||
         aChar == char16_t('\xD')  || aChar == char16_t('\xA');
}





class SVGContentUtils
{
public:
  typedef mozilla::gfx::Float Float;
  typedef mozilla::gfx::StrokeOptions StrokeOptions;
  typedef mozilla::SVGAnimatedPreserveAspectRatio SVGAnimatedPreserveAspectRatio;
  typedef mozilla::SVGPreserveAspectRatio SVGPreserveAspectRatio;

  


  static mozilla::dom::SVGSVGElement *GetOuterSVGElement(nsSVGElement *aSVGElement);

  







  static void ActivateByHyperlink(nsIContent *aContent);

  








  struct AutoStrokeOptions : public StrokeOptions {
    AutoStrokeOptions()
    {
      MOZ_ASSERT(mDashLength == 0, "InitDashPattern() depends on this");
    }
    ~AutoStrokeOptions() {
      if (mDashPattern && mDashPattern != mSmallArray) {
        delete [] mDashPattern;
      }
    }
    





    Float* InitDashPattern(size_t aDashCount) {
      if (aDashCount <= MOZ_ARRAY_LENGTH(mSmallArray)) {
        mDashPattern = mSmallArray;
        return mSmallArray;
      }
      Float* nonConstArray = new (mozilla::fallible) Float[aDashCount];
      mDashPattern = nonConstArray;
      return nonConstArray;
    }
    void DiscardDashPattern() {
      if (mDashPattern && mDashPattern != mSmallArray) {
        delete [] mDashPattern;
      }
      mDashLength = 0;
      mDashPattern = nullptr;
    }
  private:
    
    Float mSmallArray[16];
  };

  enum StrokeOptionFlags {
    eAllStrokeOptions,
    eIgnoreStrokeDashing
  };
  static void GetStrokeOptions(AutoStrokeOptions* aStrokeOptions,
                               nsSVGElement* aElement,
                               nsStyleContext* aStyleContext,
                               gfxTextContextPaint *aContextPaint,
                               StrokeOptionFlags aFlags = eAllStrokeOptions);

  








  static Float GetStrokeWidth(nsSVGElement* aElement,
                              nsStyleContext* aStyleContext,
                              gfxTextContextPaint *aContextPaint);

  






  static float GetFontSize(mozilla::dom::Element *aElement);
  static float GetFontSize(nsIFrame *aFrame);
  static float GetFontSize(nsStyleContext *aStyleContext);
  






  static float GetFontXHeight(mozilla::dom::Element *aElement);
  static float GetFontXHeight(nsIFrame *aFrame);
  static float GetFontXHeight(nsStyleContext *aStyleContext);

  


  static nsresult ReportToConsole(nsIDocument* doc,
                                  const char* aWarning,
                                  const char16_t **aParams,
                                  uint32_t aParamsLength);

  static mozilla::gfx::Matrix GetCTM(nsSVGElement *aElement, bool aScreenCTM);

  



  static bool EstablishesViewport(nsIContent *aContent);

  static nsSVGElement*
  GetNearestViewportElement(nsIContent *aContent);

  
  enum ctxDirection { X, Y, XY };

  


  static double ComputeNormalizedHypotenuse(double aWidth, double aHeight);

  
  static float
  AngleBisect(float a1, float a2);

  

  static mozilla::gfx::Matrix
  GetViewBoxTransform(float aViewportWidth, float aViewportHeight,
                      float aViewboxX, float aViewboxY,
                      float aViewboxWidth, float aViewboxHeight,
                      const SVGAnimatedPreserveAspectRatio &aPreserveAspectRatio);

  static mozilla::gfx::Matrix
  GetViewBoxTransform(float aViewportWidth, float aViewportHeight,
                      float aViewboxX, float aViewboxY,
                      float aViewboxWidth, float aViewboxHeight,
                      const SVGPreserveAspectRatio &aPreserveAspectRatio);

  static mozilla::RangedPtr<const char16_t>
  GetStartRangedPtr(const nsAString& aString);

  static mozilla::RangedPtr<const char16_t>
  GetEndRangedPtr(const nsAString& aString);

  


  static inline bool IsDigit(char16_t aCh)
  {
    return aCh >= '0' && aCh <= '9';
  }

 


  static inline uint32_t DecimalDigitValue(char16_t aCh)
  {
    MOZ_ASSERT(IsDigit(aCh), "Digit expected");
    return aCh - '0';
  }

  






  static inline bool
  ParseOptionalSign(mozilla::RangedPtr<const char16_t>& aIter,
                    const mozilla::RangedPtr<const char16_t>& aEnd,
                    int32_t& aSignMultiplier)
  {
    if (aIter == aEnd) {
      return false;
    }
    aSignMultiplier = *aIter == '-' ? -1 : 1;

    mozilla::RangedPtr<const char16_t> iter(aIter);

    if (*iter == '-' || *iter == '+') {
      ++iter;
      if (iter == aEnd) {
        return false;
      }
    }
    aIter = iter;
    return true;
  }

  






  template<class floatType>
  static bool
  ParseNumber(mozilla::RangedPtr<const char16_t>& aIter,
              const mozilla::RangedPtr<const char16_t>& aEnd,
              floatType& aValue);

  





  template<class floatType>
  static bool
  ParseNumber(const nsAString& aString, floatType& aValue);

  






  static bool ParseInteger(mozilla::RangedPtr<const char16_t>& aIter,
                           const mozilla::RangedPtr<const char16_t>& aEnd,
                           int32_t& aValue);

  





  static bool ParseInteger(const nsAString& aString, int32_t& aValue);

  




  static float CoordToFloat(nsSVGElement *aContent,
                            const nsStyleCoord &aCoord);
  




  static mozilla::TemporaryRef<mozilla::gfx::Path>
  GetPath(const nsAString& aPathString);
};

#endif
