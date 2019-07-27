





#ifndef MOZILLA_SVGTRANSFORMLISTPARSER_H__
#define MOZILLA_SVGTRANSFORMLISTPARSER_H__

#include "mozilla/Attributes.h"
#include "nsSVGDataParser.h"
#include "nsTArray.h"







namespace mozilla {

class nsSVGTransform;

class SVGTransformListParser : public nsSVGDataParser
{
public:
  explicit SVGTransformListParser(const nsAString& aValue)
    : nsSVGDataParser(aValue) {}
  
  bool Parse();

  const nsTArray<nsSVGTransform>& GetTransformList() const {
    return mTransforms;
  }

private:
  
  bool ParseArguments(float *aResult,
                      uint32_t aMaxCount,
                      uint32_t *aParsedCount);

  bool ParseTransforms();

  bool ParseTransform();

  bool ParseTranslate();
  bool ParseScale();
  bool ParseRotate();
  bool ParseSkewX();
  bool ParseSkewY();
  bool ParseMatrix();

  FallibleTArray<nsSVGTransform> mTransforms;
};

} 

#endif 
