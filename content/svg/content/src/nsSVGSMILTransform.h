




































#ifndef NS_SVGSMILTRANSFORM_H_
#define NS_SVGSMILTRANSFORM_H_

























class nsSVGSMILTransform
{
public:
  enum TransformType
  {
    TRANSFORM_TRANSLATE,
    TRANSFORM_SCALE,
    TRANSFORM_ROTATE,
    TRANSFORM_SKEWX,
    TRANSFORM_SKEWY,
    TRANSFORM_MATRIX
  };

  nsSVGSMILTransform(TransformType aType)
  : mTransformType(aType)
  {
    for (int i = 0; i < 6; ++i) {
      mParams[i] = 0;
    }
  }

  nsSVGSMILTransform(TransformType aType, float (&aParams)[3])
  : mTransformType(aType)
  {
    for (int i = 0; i < 3; ++i) {
      mParams[i] = aParams[i];
    }
    for (int i = 3; i < 6; ++i) {
      mParams[i] = 0;
    }
  }

  nsSVGSMILTransform(float (&aParams)[6])
  : mTransformType(TRANSFORM_MATRIX)
  {
    for (int i = 0; i < 6; ++i) {
      mParams[i] = aParams[i];
    }
  }

  TransformType mTransformType;

  float mParams[6];
};

#endif
