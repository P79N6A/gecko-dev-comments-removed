



































#ifndef MOZILLA_SVGPATHSEGUTILS_H__
#define MOZILLA_SVGPATHSEGUTILS_H__

#include "nsIDOMSVGPathSeg.h"
#include "nsIContent.h"
#include "nsAString.h"
#include "nsContentUtils.h"
#include "gfxPoint.h"

#define NS_SVG_PATH_SEG_MAX_ARGS 7

namespace mozilla {






struct SVGPathTraversalState
{
  SVGPathTraversalState()
    : start(0.0, 0.0)
    , pos(0.0, 0.0)
    , cp1(0.0, 0.0)
    , cp2(0.0, 0.0)
  {}

  gfxPoint start; 

  gfxPoint pos;   

  gfxPoint cp1;   
                  
                  

  gfxPoint cp2;   
                  
                  
};















class SVGPathSegUtils
{
private:
  SVGPathSegUtils(){} 

public:

  static void GetValueAsString(const float *aSeg, nsAString& aValue);

  








  static float EncodeType(PRUint32 aType) {
    PR_STATIC_ASSERT(sizeof(PRUint32) == sizeof(float));
    NS_ABORT_IF_FALSE(IsValidType(aType), "Seg type not recognized");
    return *(reinterpret_cast<float*>(&aType));
  }

  static PRUint32 DecodeType(float aType) {
    PR_STATIC_ASSERT(sizeof(PRUint32) == sizeof(float));
    PRUint32 type = *(reinterpret_cast<PRUint32*>(&aType));
    NS_ABORT_IF_FALSE(IsValidType(type), "Seg type not recognized");
    return type;
  }

  static PRUnichar GetPathSegTypeAsLetter(PRUint32 aType) {
    NS_ABORT_IF_FALSE(IsValidType(aType), "Seg type not recognized");

    static const PRUnichar table[] = {
      PRUnichar('x'),  
      PRUnichar('z'),  
      PRUnichar('M'),  
      PRUnichar('m'),  
      PRUnichar('L'),  
      PRUnichar('l'),  
      PRUnichar('C'),  
      PRUnichar('c'),  
      PRUnichar('Q'),  
      PRUnichar('q'),  
      PRUnichar('A'),  
      PRUnichar('a'),  
      PRUnichar('H'),  
      PRUnichar('h'),  
      PRUnichar('V'),  
      PRUnichar('v'),  
      PRUnichar('S'),  
      PRUnichar('s'),  
      PRUnichar('T'),  
      PRUnichar('t')   
    };

    return table[aType];
  }

  static PRUint32 ArgCountForType(PRUint32 aType) {
    NS_ABORT_IF_FALSE(IsValidType(aType), "Seg type not recognized");

    static const PRUint8 table[] = {
      0,  
      0,  
      2,  
      2,  
      2,  
      2,  
      6,  
      6,  
      4,  
      4,  
      7,  
      7,  
      1,  
      1,  
      1,  
      1,  
      4,  
      4,  
      2,  
      2   
    };

    return table[aType];
  }

  



  static PRUint32 ArgCountForType(float aType) {
    return ArgCountForType(DecodeType(aType));
  }

  static inline PRBool IsValidType(PRUint32 aType) {
    return aType >= nsIDOMSVGPathSeg::PATHSEG_CLOSEPATH &&
           aType <= nsIDOMSVGPathSeg::PATHSEG_CURVETO_QUADRATIC_SMOOTH_REL;
  }

  static inline PRBool IsCubicType(PRUint32 aType) {
    return aType == nsIDOMSVGPathSeg::PATHSEG_CURVETO_CUBIC_REL ||
           aType == nsIDOMSVGPathSeg::PATHSEG_CURVETO_CUBIC_ABS ||
           aType == nsIDOMSVGPathSeg::PATHSEG_CURVETO_CUBIC_SMOOTH_REL ||
           aType == nsIDOMSVGPathSeg::PATHSEG_CURVETO_CUBIC_SMOOTH_ABS;
  }

  static inline PRBool IsQuadraticType(PRUint32 aType) {
    return aType == nsIDOMSVGPathSeg::PATHSEG_CURVETO_QUADRATIC_REL ||
           aType == nsIDOMSVGPathSeg::PATHSEG_CURVETO_QUADRATIC_ABS ||
           aType == nsIDOMSVGPathSeg::PATHSEG_CURVETO_QUADRATIC_SMOOTH_REL ||
           aType == nsIDOMSVGPathSeg::PATHSEG_CURVETO_QUADRATIC_SMOOTH_ABS;
  }

  


  static float GetLength(const float *aSeg, SVGPathTraversalState &aState);

  static void ToString(const float *aSeg, nsAString& aValue);
};

} 

#endif 
