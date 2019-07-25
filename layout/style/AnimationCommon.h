





































#ifndef mozilla_css_AnimationCommon_h
#define mozilla_css_AnimationCommon_h

#include "nsIStyleRuleProcessor.h"
#include "nsIStyleRule.h"
#include "nsRefreshDriver.h"
#include "prclist.h"
#include "nsStyleAnimation.h"
#include "nsCSSProperty.h"
#include "mozilla/dom/Element.h"
#include "nsSMILKeySpline.h"
#include "nsStyleStruct.h"

class nsPresContext;

namespace mozilla {
namespace css {

struct CommonElementAnimationData;

class CommonAnimationManager : public nsIStyleRuleProcessor,
                               public nsARefreshObserver {
public:
  CommonAnimationManager(nsPresContext *aPresContext);
  virtual ~CommonAnimationManager();

  
  NS_DECL_ISUPPORTS

  
  virtual nsRestyleHint HasStateDependentStyle(StateRuleProcessorData* aData);
  virtual PRBool HasDocumentStateDependentStyle(StateRuleProcessorData* aData);
  virtual nsRestyleHint
    HasAttributeDependentStyle(AttributeRuleProcessorData* aData);
  virtual PRBool MediumFeaturesChanged(nsPresContext* aPresContext);

  


  void Disconnect();

  static PRBool ExtractComputedValueForTransition(
                  nsCSSProperty aProperty,
                  nsStyleContext* aStyleContext,
                  nsStyleAnimation::Value& aComputedValue);
protected:
  friend struct CommonElementAnimationData; 

  void AddElementData(CommonElementAnimationData* aData);
  void ElementDataRemoved();
  void RemoveAllElementData();

  PRCList mElementData;
  nsPresContext *mPresContext; 
};




class AnimValuesStyleRule : public nsIStyleRule
{
public:
  
  NS_DECL_ISUPPORTS

  
  virtual void MapRuleInfoInto(nsRuleData* aRuleData);
#ifdef DEBUG
  virtual void List(FILE* out = stdout, PRInt32 aIndent = 0) const;
#endif

  void AddValue(nsCSSProperty aProperty, nsStyleAnimation::Value &aStartValue)
  {
    PropertyValuePair v = { aProperty, aStartValue };
    mPropertyValuePairs.AppendElement(v);
  }

  
  nsStyleAnimation::Value* AddEmptyValue(nsCSSProperty aProperty)
  {
    PropertyValuePair *p = mPropertyValuePairs.AppendElement();
    p->mProperty = aProperty;
    return &p->mValue;
  }

  struct PropertyValuePair {
    nsCSSProperty mProperty;
    nsStyleAnimation::Value mValue;
  };

private:
  InfallibleTArray<PropertyValuePair> mPropertyValuePairs;
};

class ComputedTimingFunction {
public:
  typedef nsTimingFunction::Type Type;
  void Init(const nsTimingFunction &aFunction);
  double GetValue(double aPortion) const;
private:
  Type mType;
  nsSMILKeySpline mTimingFunction;
  PRUint32 mSteps;
};

struct CommonElementAnimationData : public PRCList
{
  CommonElementAnimationData(dom::Element *aElement, nsIAtom *aElementProperty,
                             CommonAnimationManager *aManager)
    : mElement(aElement)
    , mElementProperty(aElementProperty)
    , mManager(aManager)
#ifdef DEBUG
    , mCalledPropertyDtor(false)
#endif
  {
    MOZ_COUNT_CTOR(CommonElementAnimationData);
    PR_INIT_CLIST(this);
  }
  ~CommonElementAnimationData()
  {
    NS_ABORT_IF_FALSE(mCalledPropertyDtor,
                      "must call destructor through element property dtor");
    MOZ_COUNT_DTOR(CommonElementAnimationData);
    PR_REMOVE_LINK(this);
    mManager->ElementDataRemoved();
  }

  void Destroy()
  {
    
    mElement->DeleteProperty(mElementProperty);
  }

  dom::Element *mElement;

  
  
  nsIAtom *mElementProperty;

  CommonAnimationManager *mManager;

#ifdef DEBUG
  bool mCalledPropertyDtor;
#endif
};

}
}

#endif 
