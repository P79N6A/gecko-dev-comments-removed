





#ifndef MOZILLA_SVGATTRVALUEWRAPPER_H__
#define MOZILLA_SVGATTRVALUEWRAPPER_H__






#include "nsStringGlue.h"

class nsSVGAngle;
class nsSVGIntegerPair;
class nsSVGLength2;
class nsSVGNumberPair;
class nsSVGViewBox;

namespace mozilla {
class SVGLengthList;
class SVGNumberList;
class SVGPathData;
class SVGPointList;
class SVGAnimatedPreserveAspectRatio;
class SVGStringList;
class SVGTransformList;
}

namespace mozilla {

class SVGAttrValueWrapper
{
public:
  static void ToString(const nsSVGAngle* aAngle, nsAString& aResult);
  static void ToString(const nsSVGIntegerPair* aIntegerPair,
                       nsAString& aResult);
  static void ToString(const nsSVGLength2* aLength, nsAString& aResult);
  static void ToString(const mozilla::SVGLengthList* aLengthList,
                       nsAString& aResult);
  static void ToString(const mozilla::SVGNumberList* aNumberList,
                       nsAString& aResult);
  static void ToString(const nsSVGNumberPair* aNumberPair, nsAString& aResult);
  static void ToString(const mozilla::SVGPathData* aPathData,
                       nsAString& aResult);
  static void ToString(const mozilla::SVGPointList* aPointList,
                       nsAString& aResult);
  static void ToString(
    const mozilla::SVGAnimatedPreserveAspectRatio* aPreserveAspectRatio,
    nsAString& aResult);
  static void ToString(const mozilla::SVGStringList* aStringList,
                       nsAString& aResult);
  static void ToString(const mozilla::SVGTransformList* aTransformList,
                       nsAString& aResult);
  static void ToString(const nsSVGViewBox* aViewBox, nsAString& aResult);
};

} 

#endif 
