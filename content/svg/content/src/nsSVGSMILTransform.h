




































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

  
  
  static const PRUint32 NUM_SIMPLE_PARAMS = 3;

  
  
  static const PRUint32 NUM_STORED_PARAMS = 6;

  explicit nsSVGSMILTransform(TransformType aType)
  : mTransformType(aType)
  {
    for (PRUint32 i = 0; i < NUM_STORED_PARAMS; ++i) {
      mParams[i] = 0;
    }
  }

  nsSVGSMILTransform(TransformType aType, float (&aParams)[NUM_SIMPLE_PARAMS])
  : mTransformType(aType)
  {
    for (PRUint32 i = 0; i < NUM_SIMPLE_PARAMS; ++i) {
      mParams[i] = aParams[i];
    }
    for (PRUint32 i = NUM_SIMPLE_PARAMS; i < NUM_STORED_PARAMS; ++i) {
      mParams[i] = 0;
    }
  }

  explicit nsSVGSMILTransform(float (&aParams)[NUM_STORED_PARAMS])
  : mTransformType(TRANSFORM_MATRIX)
  {
    for (PRUint32 i = 0; i < NUM_STORED_PARAMS; ++i) {
      mParams[i] = aParams[i];
    }
  }

  PRBool operator==(const nsSVGSMILTransform& aOther) const
  {
    if (mTransformType != aOther.mTransformType)
      return PR_FALSE;

    for (PRUint32 i = 0; i < NUM_STORED_PARAMS; ++i) {
      if (mParams[i] != aOther.mParams[i]) {
        return PR_FALSE;
      }
    }

    
    return PR_TRUE;
  }

  PRBool operator!=(const nsSVGSMILTransform& aOther) const
  {
    return !(*this == aOther);
  }

  TransformType mTransformType;

  float mParams[NUM_STORED_PARAMS];
};

#endif
