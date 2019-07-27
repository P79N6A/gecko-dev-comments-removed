




#ifndef MOZILLA_SVGMOTIONSMILANIMATIONFUNCTION_H_
#define MOZILLA_SVGMOTIONSMILANIMATIONFUNCTION_H_

#include "mozilla/gfx/2D.h"
#include "mozilla/RefPtr.h"
#include "nsAutoPtr.h"
#include "nsSMILAnimationFunction.h"
#include "nsTArray.h"
#include "SVGMotionSMILType.h"  

class nsAttrValue;
class nsIAtom;
class nsIContent;
class nsISMILAttr;
class nsSMILValue;

namespace mozilla {

namespace dom {
class SVGMPathElement;
}







class SVGMotionSMILAnimationFunction MOZ_FINAL : public nsSMILAnimationFunction
{
  typedef mozilla::gfx::Path Path;

public:
  SVGMotionSMILAnimationFunction();
  virtual bool SetAttr(nsIAtom* aAttribute,
                       const nsAString& aValue,
                       nsAttrValue& aResult,
                       nsresult* aParseResult = nullptr) MOZ_OVERRIDE;
  virtual bool UnsetAttr(nsIAtom* aAttribute) MOZ_OVERRIDE;

  
  
  
  
  
  void MpathChanged() { mIsPathStale = mHasChanged = true; }

protected:
  enum PathSourceType {
    
    
    ePathSourceType_None,      
    ePathSourceType_ByAttr,    
    ePathSourceType_ToAttr,    
    ePathSourceType_ValuesAttr,
    ePathSourceType_PathAttr,
    ePathSourceType_Mpath
  };

  virtual nsSMILCalcMode GetCalcMode() const MOZ_OVERRIDE;
  virtual nsresult GetValues(const nsISMILAttr& aSMILAttr,
                             nsSMILValueArray& aResult) MOZ_OVERRIDE;
  virtual void CheckValueListDependentAttrs(uint32_t aNumValues) MOZ_OVERRIDE;

  virtual bool IsToAnimation() const MOZ_OVERRIDE;

  void     CheckKeyPoints();
  nsresult SetKeyPoints(const nsAString& aKeyPoints, nsAttrValue& aResult);
  void     UnsetKeyPoints();
  nsresult SetRotate(const nsAString& aRotate, nsAttrValue& aResult);
  void     UnsetRotate();

  
  void     MarkStaleIfAttributeAffectsPath(nsIAtom* aAttribute);
  void     RebuildPathAndVertices(const nsIContent* aContextElem);
  void     RebuildPathAndVerticesFromMpathElem(dom::SVGMPathElement* aMpathElem);
  void     RebuildPathAndVerticesFromPathAttr();
  void     RebuildPathAndVerticesFromBasicAttrs(const nsIContent* aContextElem);
  bool     GenerateValuesForPathAndPoints(Path* aPath,
                                          bool aIsKeyPoints,
                                          FallibleTArray<double>& aPointDistances,
                                          nsSMILValueArray& aResult);

  
  
  FallibleTArray<double>     mKeyPoints; 

  RotateType                 mRotateType;  
  float                      mRotateAngle; 

  PathSourceType             mPathSourceType; 
  RefPtr<Path>               mPath;           
  FallibleTArray<double>     mPathVertices; 

  bool                       mIsPathStale;
};

} 

#endif 
