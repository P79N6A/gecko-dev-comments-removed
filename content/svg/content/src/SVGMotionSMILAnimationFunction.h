




































#ifndef MOZILLA_SVGMOTIONSMILANIMATIONFUNCTION_H_
#define MOZILLA_SVGMOTIONSMILANIMATIONFUNCTION_H_

#include "nsSMILAnimationFunction.h"
#include "SVGMotionSMILType.h" 
#include "gfxPath.h"  

class nsSVGMpathElement;

namespace mozilla {







class SVGMotionSMILAnimationFunction : public nsSMILAnimationFunction
{
public:
  SVGMotionSMILAnimationFunction();
  NS_OVERRIDE virtual PRBool SetAttr(nsIAtom* aAttribute,
                                     const nsAString& aValue,
                                     nsAttrValue& aResult,
                                     nsresult* aParseResult = nsnull);
  NS_OVERRIDE virtual PRBool UnsetAttr(nsIAtom* aAttribute);

  
  
  
  
  
  void MpathChanged() { mIsPathStale = mHasChanged = PR_TRUE; }

protected:
  enum PathSourceType {
    
    
    ePathSourceType_None,      
    ePathSourceType_ByAttr,    
    ePathSourceType_ToAttr,    
    ePathSourceType_ValuesAttr,
    ePathSourceType_PathAttr,
    ePathSourceType_Mpath
  };

  NS_OVERRIDE virtual nsSMILCalcMode GetCalcMode() const;
  NS_OVERRIDE virtual nsresult GetValues(const nsISMILAttr& aSMILAttr,
                                         nsSMILValueArray& aResult);
  NS_OVERRIDE virtual void CheckValueListDependentAttrs(PRUint32 aNumValues);
  NS_OVERRIDE virtual PRBool TreatSingleValueAsStatic() const;

  void     CheckKeyPoints();
  nsresult SetKeyPoints(const nsAString& aKeyPoints, nsAttrValue& aResult);
  void     UnsetKeyPoints();
  nsresult SetRotate(const nsAString& aRotate, nsAttrValue& aResult);
  void     UnsetRotate();

  
  void     MarkStaleIfAttributeAffectsPath(nsIAtom* aAttribute);
  void     RebuildPathAndVertices(const nsIContent* aContextElem);
  void     RebuildPathAndVerticesFromMpathElem(nsSVGMpathElement* aMpathElem);
  void     RebuildPathAndVerticesFromPathAttr();
  void     RebuildPathAndVerticesFromBasicAttrs(const nsIContent* aContextElem);
  PRBool   GenerateValuesForPathAndPoints(gfxFlattenedPath* aPath,
                                          PRBool aIsKeyPoints,
                                          nsTArray<double>& aPointDistances,
                                          nsTArray<nsSMILValue>& aResult);

  
  
  nsTArray<double>           mKeyPoints; 

  RotateType                 mRotateType;  
  float                      mRotateAngle; 

  PathSourceType             mPathSourceType; 
  nsRefPtr<gfxFlattenedPath> mPath;           
  nsTArray<double>           mPathVertices; 

  PRPackedBool               mIsPathStale;
};

} 

#endif 
