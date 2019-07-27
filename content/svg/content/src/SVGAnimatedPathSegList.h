




#ifndef MOZILLA_SVGANIMATEDPATHSEGLIST_H__
#define MOZILLA_SVGANIMATEDPATHSEGLIST_H__

#include "mozilla/Attributes.h"
#include "mozilla/MemoryReporting.h"
#include "nsAutoPtr.h"
#include "nsISMILAttr.h"
#include "SVGPathData.h"

class nsSMILValue;
class nsSVGElement;

namespace mozilla {

namespace dom {
class SVGAnimationElement;
}
















class SVGAnimatedPathSegList MOZ_FINAL
{
  
  friend class DOMSVGPathSeg;
  friend class DOMSVGPathSegList;

public:
  SVGAnimatedPathSegList() {}

  





  const SVGPathData& GetBaseValue() const {
    return mBaseVal;
  }

  nsresult SetBaseValueString(const nsAString& aValue);

  void ClearBaseValue();

  


  const SVGPathData& GetAnimValue() const {
    return mAnimVal ? *mAnimVal : mBaseVal;
  }

  nsresult SetAnimValue(const SVGPathData& aValue,
                        nsSVGElement *aElement);

  void ClearAnimValue(nsSVGElement *aElement);

  



  void *GetBaseValKey() const {
    return (void*)&mBaseVal;
  }
  void *GetAnimValKey() const {
    return (void*)&mAnimVal;
  }
  
  bool IsAnimating() const {
    return !!mAnimVal;
  }

  
  nsISMILAttr* ToSMILAttr(nsSVGElement* aElement);

  size_t SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const;

private:

  
  
  
  

  SVGPathData mBaseVal;
  nsAutoPtr<SVGPathData> mAnimVal;

  struct SMILAnimatedPathSegList : public nsISMILAttr
  {
  public:
    SMILAnimatedPathSegList(SVGAnimatedPathSegList* aVal,
                            nsSVGElement* aElement)
      : mVal(aVal)
      , mElement(aElement)
    {}

    
    
    
    SVGAnimatedPathSegList *mVal;
    nsSVGElement *mElement;

    
    virtual nsresult ValueFromString(const nsAString& aStr,
                                     const dom::SVGAnimationElement* aSrcElement,
                                     nsSMILValue& aValue,
                                     bool& aPreventCachingOfSandwich) const MOZ_OVERRIDE;
    virtual nsSMILValue GetBaseValue() const MOZ_OVERRIDE;
    virtual void ClearAnimValue() MOZ_OVERRIDE;
    virtual nsresult SetAnimValue(const nsSMILValue& aValue) MOZ_OVERRIDE;
  };
};

} 

#endif 
