





#include "SVGAttrValueWrapper.h"
#include "nsSVGAngle.h"
#include "nsSVGIntegerPair.h"
#include "nsSVGLength2.h"
#include "nsSVGNumberPair.h"
#include "nsSVGViewBox.h"
#include "SVGAnimatedPreserveAspectRatio.h"
#include "SVGLengthList.h"
#include "SVGNumberList.h"
#include "SVGPathData.h"
#include "SVGPointList.h"
#include "SVGStringList.h"
#include "SVGTransformList.h"

using namespace mozilla;

 void
SVGAttrValueWrapper::ToString(const nsSVGAngle* aAngle, nsAString& aResult)
{
  aAngle->GetBaseValueString(aResult);
}

 void
SVGAttrValueWrapper::ToString(const nsSVGIntegerPair* aIntegerPair,
                              nsAString& aResult)
{
  aIntegerPair->GetBaseValueString(aResult);
}

 void
SVGAttrValueWrapper::ToString(const nsSVGLength2* aLength, nsAString& aResult)
{
  aLength->GetBaseValueString(aResult);
}

 void
SVGAttrValueWrapper::ToString(const SVGLengthList* aLengthList,
                              nsAString& aResult)
{
  aLengthList->GetValueAsString(aResult);
}

 void
SVGAttrValueWrapper::ToString(const SVGNumberList* aNumberList,
                              nsAString& aResult)
{
  aNumberList->GetValueAsString(aResult);
}

 void
SVGAttrValueWrapper::ToString(const nsSVGNumberPair* aNumberPair,
                              nsAString& aResult)
{
  aNumberPair->GetBaseValueString(aResult);
}

 void
SVGAttrValueWrapper::ToString(const SVGPathData* aPathData, nsAString& aResult)
{
  aPathData->GetValueAsString(aResult);
}

 void
SVGAttrValueWrapper::ToString(const SVGPointList* aPointList,
                              nsAString& aResult)
{
  aPointList->GetValueAsString(aResult);
}

 void
SVGAttrValueWrapper::ToString(
  const SVGAnimatedPreserveAspectRatio* aPreserveAspectRatio,
  nsAString& aResult)
{
  aPreserveAspectRatio->GetBaseValueString(aResult);
}

 void
SVGAttrValueWrapper::ToString(const SVGStringList* aStringList,
                              nsAString& aResult)
{
  aStringList->GetValue(aResult);
}

 void
SVGAttrValueWrapper::ToString(const SVGTransformList* aTransformList,
                              nsAString& aResult)
{
  aTransformList->GetValueAsString(aResult);
}

 void
SVGAttrValueWrapper::ToString(const nsSVGViewBox* aViewBox, nsAString& aResult)
{
  aViewBox->GetBaseValueString(aResult);
}
