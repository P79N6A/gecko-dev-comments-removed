




#ifndef MOZILLA_SVGPATHSEGUTILS_H__
#define MOZILLA_SVGPATHSEGUTILS_H__

#include "gfxPoint.h"
#include "nsDebug.h"
#include "nsIDOMSVGPathSeg.h"
#include "nsMemory.h"

#define NS_SVG_PATH_SEG_MAX_ARGS         7
#define NS_SVG_PATH_SEG_FIRST_VALID_TYPE nsIDOMSVGPathSeg::PATHSEG_CLOSEPATH
#define NS_SVG_PATH_SEG_LAST_VALID_TYPE  nsIDOMSVGPathSeg::PATHSEG_CURVETO_QUADRATIC_SMOOTH_REL
#define NS_SVG_PATH_SEG_TYPE_COUNT       (NS_SVG_PATH_SEG_LAST_VALID_TYPE + 1)

namespace mozilla {






struct SVGPathTraversalState
{
  enum TraversalMode {
    eUpdateAll,
    eUpdateOnlyStartAndCurrentPos
  };

  SVGPathTraversalState()
    : start(0.0, 0.0)
    , pos(0.0, 0.0)
    , cp1(0.0, 0.0)
    , cp2(0.0, 0.0)
    , length(0.0)
    , mode(eUpdateAll)
  {}

  bool ShouldUpdateLengthAndControlPoints() { return mode == eUpdateAll; }

  gfxPoint start; 

  gfxPoint pos;   

  gfxPoint cp1;   
                  
                  

  gfxPoint cp2;   
                  
                  

  float length;   

  TraversalMode mode;  
};















class SVGPathSegUtils
{
private:
  SVGPathSegUtils(){} 

public:

  static void GetValueAsString(const float *aSeg, nsAString& aValue);

  








  static float EncodeType(uint32_t aType) {
    PR_STATIC_ASSERT(sizeof(uint32_t) == sizeof(float));
    NS_ABORT_IF_FALSE(IsValidType(aType), "Seg type not recognized");
    return *(reinterpret_cast<float*>(&aType));
  }

  static uint32_t DecodeType(float aType) {
    PR_STATIC_ASSERT(sizeof(uint32_t) == sizeof(float));
    uint32_t type = *(reinterpret_cast<uint32_t*>(&aType));
    NS_ABORT_IF_FALSE(IsValidType(type), "Seg type not recognized");
    return type;
  }

  static PRUnichar GetPathSegTypeAsLetter(uint32_t aType) {
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
    PR_STATIC_ASSERT(NS_ARRAY_LENGTH(table) == NS_SVG_PATH_SEG_TYPE_COUNT);

    return table[aType];
  }

  static uint32_t ArgCountForType(uint32_t aType) {
    NS_ABORT_IF_FALSE(IsValidType(aType), "Seg type not recognized");

    static const uint8_t table[] = {
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
    PR_STATIC_ASSERT(NS_ARRAY_LENGTH(table) == NS_SVG_PATH_SEG_TYPE_COUNT);

    return table[aType];
  }

  



  static uint32_t ArgCountForType(float aType) {
    return ArgCountForType(DecodeType(aType));
  }

  static bool IsValidType(uint32_t aType) {
    return aType >= NS_SVG_PATH_SEG_FIRST_VALID_TYPE &&
           aType <= NS_SVG_PATH_SEG_LAST_VALID_TYPE;
  }

  static bool IsCubicType(uint32_t aType) {
    return aType == nsIDOMSVGPathSeg::PATHSEG_CURVETO_CUBIC_REL ||
           aType == nsIDOMSVGPathSeg::PATHSEG_CURVETO_CUBIC_ABS ||
           aType == nsIDOMSVGPathSeg::PATHSEG_CURVETO_CUBIC_SMOOTH_REL ||
           aType == nsIDOMSVGPathSeg::PATHSEG_CURVETO_CUBIC_SMOOTH_ABS;
  }

  static bool IsQuadraticType(uint32_t aType) {
    return aType == nsIDOMSVGPathSeg::PATHSEG_CURVETO_QUADRATIC_REL ||
           aType == nsIDOMSVGPathSeg::PATHSEG_CURVETO_QUADRATIC_ABS ||
           aType == nsIDOMSVGPathSeg::PATHSEG_CURVETO_QUADRATIC_SMOOTH_REL ||
           aType == nsIDOMSVGPathSeg::PATHSEG_CURVETO_QUADRATIC_SMOOTH_ABS;
  }

  static bool IsArcType(uint32_t aType) {
    return aType == nsIDOMSVGPathSeg::PATHSEG_ARC_ABS || 
           aType == nsIDOMSVGPathSeg::PATHSEG_ARC_REL;
  }

  static bool IsRelativeOrAbsoluteType(uint32_t aType) {
    NS_ABORT_IF_FALSE(IsValidType(aType), "Seg type not recognized");

    
    
    PR_STATIC_ASSERT(NS_SVG_PATH_SEG_LAST_VALID_TYPE ==
                       nsIDOMSVGPathSeg::PATHSEG_CURVETO_QUADRATIC_SMOOTH_REL);

    return aType >= nsIDOMSVGPathSeg::PATHSEG_MOVETO_ABS;
  }

  static bool IsRelativeType(uint32_t aType) {
    NS_ABORT_IF_FALSE
      (IsRelativeOrAbsoluteType(aType),
       "IsRelativeType called with segment type that does not come in relative and absolute forms");

    
    
    PR_STATIC_ASSERT(NS_SVG_PATH_SEG_LAST_VALID_TYPE ==
                       nsIDOMSVGPathSeg::PATHSEG_CURVETO_QUADRATIC_SMOOTH_REL);

    return aType & 1;
  }

  static uint32_t RelativeVersionOfType(uint32_t aType) {
    NS_ABORT_IF_FALSE
      (IsRelativeOrAbsoluteType(aType),
       "RelativeVersionOfType called with segment type that does not come in relative and absolute forms");

    
    
    PR_STATIC_ASSERT(NS_SVG_PATH_SEG_LAST_VALID_TYPE ==
                       nsIDOMSVGPathSeg::PATHSEG_CURVETO_QUADRATIC_SMOOTH_REL);

    return aType | 1;
  }

  static uint32_t SameTypeModuloRelativeness(uint32_t aType1, uint32_t aType2) {
    if (!IsRelativeOrAbsoluteType(aType1)) {
      return aType1 == aType2;
    }

    return RelativeVersionOfType(aType1) == RelativeVersionOfType(aType2);
  }

  



  static void TraversePathSegment(const float* aData,
                                  SVGPathTraversalState& aState);
};

} 

#endif 
