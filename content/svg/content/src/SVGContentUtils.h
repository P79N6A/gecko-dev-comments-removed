




#ifndef MOZILLA_SVGCONTENTUTILS_H
#define MOZILLA_SVGCONTENTUTILS_H


#define _USE_MATH_DEFINES
#include <math.h>

#include "gfxMatrix.h"
#include "mozilla/RangedPtr.h"

class nsIContent;
class nsIDocument;
class nsIFrame;
class nsPresContext;
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
} 

inline bool
IsSVGWhitespace(char aChar)
{
  return aChar == '\x20' || aChar == '\x9' ||
         aChar == '\xD'  || aChar == '\xA';
}

inline bool
IsSVGWhitespace(PRUnichar aChar)
{
  return aChar == PRUnichar('\x20') || aChar == PRUnichar('\x9') ||
         aChar == PRUnichar('\xD')  || aChar == PRUnichar('\xA');
}





class SVGContentUtils
{
public:
  typedef mozilla::SVGAnimatedPreserveAspectRatio SVGAnimatedPreserveAspectRatio;
  typedef mozilla::SVGPreserveAspectRatio SVGPreserveAspectRatio;

  


  static mozilla::dom::SVGSVGElement *GetOuterSVGElement(nsSVGElement *aSVGElement);

  







  static void ActivateByHyperlink(nsIContent *aContent);

  






  static float GetFontSize(mozilla::dom::Element *aElement);
  static float GetFontSize(nsIFrame *aFrame);
  static float GetFontSize(nsStyleContext *aStyleContext);
  






  static float GetFontXHeight(mozilla::dom::Element *aElement);
  static float GetFontXHeight(nsIFrame *aFrame);
  static float GetFontXHeight(nsStyleContext *aStyleContext);

  


  static nsresult ReportToConsole(nsIDocument* doc,
                                  const char* aWarning,
                                  const PRUnichar **aParams,
                                  uint32_t aParamsLength);

  static gfxMatrix GetCTM(nsSVGElement *aElement, bool aScreenCTM);

  



  static bool EstablishesViewport(nsIContent *aContent);

  static nsSVGElement*
  GetNearestViewportElement(nsIContent *aContent);

  
  enum ctxDirection { X, Y, XY };

  


  static double ComputeNormalizedHypotenuse(double aWidth, double aHeight);

  
  static float
  AngleBisect(float a1, float a2);

  

  static gfxMatrix
  GetViewBoxTransform(float aViewportWidth, float aViewportHeight,
                      float aViewboxX, float aViewboxY,
                      float aViewboxWidth, float aViewboxHeight,
                      const SVGAnimatedPreserveAspectRatio &aPreserveAspectRatio);

  static gfxMatrix
  GetViewBoxTransform(float aViewportWidth, float aViewportHeight,
                      float aViewboxX, float aViewboxY,
                      float aViewboxWidth, float aViewboxHeight,
                      const SVGPreserveAspectRatio &aPreserveAspectRatio);

  static mozilla::RangedPtr<const PRUnichar>
  GetStartRangedPtr(const nsAString& aString);

  static mozilla::RangedPtr<const PRUnichar>
  GetEndRangedPtr(const nsAString& aString);

  


  static inline bool IsDigit(PRUnichar aCh)
  {
    return aCh >= '0' && aCh <= '9';
  }

 


  static inline uint32_t DecimalDigitValue(PRUnichar aCh)
  {
    MOZ_ASSERT(IsDigit(aCh), "Digit expected");
    return aCh - '0';
  }

  






  static inline bool
  ParseOptionalSign(mozilla::RangedPtr<const PRUnichar>& aIter,
                    const mozilla::RangedPtr<const PRUnichar>& aEnd,
                    int32_t& aSignMultiplier)
  {
    if (aIter == aEnd) {
      return false;
    }
    aSignMultiplier = *aIter == '-' ? -1 : 1;

    mozilla::RangedPtr<const PRUnichar> iter(aIter);

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
  ParseNumber(mozilla::RangedPtr<const PRUnichar>& aIter,
              const mozilla::RangedPtr<const PRUnichar>& aEnd,
              floatType& aValue);

  





  template<class floatType>
  static bool
  ParseNumber(const nsAString& aString, floatType& aValue);

  






  static bool ParseInteger(mozilla::RangedPtr<const PRUnichar>& aIter,
                           const mozilla::RangedPtr<const PRUnichar>& aEnd,
                           int32_t& aValue);

  





  static bool ParseInteger(const nsAString& aString, int32_t& aValue);

  




  static float CoordToFloat(nsPresContext *aPresContext,
                            nsSVGElement *aContent,
                            const nsStyleCoord &aCoord);
};

#endif
