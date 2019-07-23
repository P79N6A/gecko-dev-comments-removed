






































#include "nsTransitionManager.h"
#include "nsIContent.h"
#include "nsStyleContext.h"
#include "nsCSSProps.h"
#include "mozilla/TimeStamp.h"
#include "nsRefreshDriver.h"
#include "nsRuleProcessorData.h"
#include "nsIStyleRule.h"
#include "nsRuleWalker.h"
#include "nsRuleData.h"
#include "nsSMILKeySpline.h"
#include "gfxColor.h"
#include "nsCSSPropertySet.h"
#include "nsStyleAnimation.h"
#include "nsCSSDataBlock.h"

using mozilla::TimeStamp;
using mozilla::TimeDuration;





struct ElementPropertyTransition
{
  nsCSSProperty mProperty;
  
  
  
  
  nsStyleAnimation::Value mStartValue, mEndValue, mCurrentValue;
  TimeStamp mStartTime; 

  
  TimeDuration mDuration;
  nsSMILKeySpline mTimingFunction;
};









class ElementTransitionsStyleRule : public nsIStyleRule
{
public:
  
  NS_DECL_ISUPPORTS

  
  NS_IMETHOD MapRuleInfoInto(nsRuleData* aRuleData);
#ifdef DEBUG
  NS_IMETHOD List(FILE* out = stdout, PRInt32 aIndent = 0) const;
#endif

  ElementTransitionsStyleRule(ElementTransitions *aOwner,
                           TimeStamp aRefreshTime)
    : mElementTransitions(aOwner)
    , mRefreshTime(aRefreshTime)
  {}

  void Disconnect() { mElementTransitions = nsnull; }

  ElementTransitions *ElementData() { return mElementTransitions; }
  TimeStamp RefreshTime() { return mRefreshTime; }

private:
  ElementTransitions *mElementTransitions;
  
  TimeStamp mRefreshTime;
};








class CoverTransitionStartStyleRule : public nsIStyleRule
{
public:
  
  NS_DECL_ISUPPORTS

  
  NS_IMETHOD MapRuleInfoInto(nsRuleData* aRuleData);
#ifdef DEBUG
  NS_IMETHOD List(FILE* out = stdout, PRInt32 aIndent = 0) const;
#endif

  NS_HIDDEN_(void) CoverValue(nsCSSProperty aProperty,
                              nsStyleAnimation::Value &aStartValue)
  {
    CoveredValue v = { aProperty, aStartValue };
    mCoveredValues.AppendElement(v);
  }

  struct CoveredValue {
    nsCSSProperty mProperty;
    nsStyleAnimation::Value mCoveredValue;
  };

private:
  nsTArray<CoveredValue> mCoveredValues;
};

struct ElementTransitions : public PRCList
{
  ElementTransitions(nsIContent *aElement, nsIAtom *aElementProperty,
                     nsTransitionManager *aTransitionManager)
    : mElement(aElement)
    , mElementProperty(aElementProperty)
    , mTransitionManager(aTransitionManager)
  {
    PR_INIT_CLIST(this);
  }
  ~ElementTransitions()
  {
    DropStyleRule();
    PR_REMOVE_LINK(this);
    mTransitionManager->TransitionsRemoved();
  }

  void Destroy()
  {
    
    mElement->DeleteProperty(mElementProperty);
  }

  void DropStyleRule();
  PRBool EnsureStyleRuleFor(TimeStamp aRefreshTime);


  
  nsTArray<ElementPropertyTransition> mPropertyTransitions;

  
  
  nsRefPtr<ElementTransitionsStyleRule> mStyleRule;

  nsIContent *mElement;

  
  
  nsIAtom *mElementProperty;

  nsTransitionManager *mTransitionManager;
};

static void
ElementTransitionsPropertyDtor(void           *aObject,
                               nsIAtom        *aPropertyName,
                               void           *aPropertyValue,
                               void           *aData)
{
  ElementTransitions *et = static_cast<ElementTransitions*>(aPropertyValue);
  delete et;
}

NS_IMPL_ISUPPORTS1(ElementTransitionsStyleRule, nsIStyleRule)

NS_IMETHODIMP
ElementTransitionsStyleRule::MapRuleInfoInto(nsRuleData* aRuleData)
{
  nsStyleContext *contextParent = aRuleData->mStyleContext->GetParent();
  if (contextParent && contextParent->HasPseudoElementData()) {
    
    
    return NS_OK;
  }

  ElementTransitions *et = ElementData();
  NS_ENSURE_TRUE(et, NS_OK); 
  for (PRUint32 i = 0, i_end = et->mPropertyTransitions.Length();
       i < i_end; ++i)
  {
    ElementPropertyTransition &pt = et->mPropertyTransitions[i];
    if (aRuleData->mSIDs & nsCachedStyleData::GetBitForSID(
                             nsCSSProps::kSIDTable[pt.mProperty]))
    {
      double timePortion =
        (RefreshTime() - pt.mStartTime).ToSeconds() / pt.mDuration.ToSeconds();
      if (timePortion < 0.0)
        timePortion = 0.0; 
      if (timePortion > 1.0)
        timePortion = 1.0; 

      double valuePortion =
        pt.mTimingFunction.GetSplineValue(timePortion);
#ifdef DEBUG
      PRBool ok =
#endif
        nsStyleAnimation::Interpolate(pt.mProperty,
                                      pt.mStartValue, pt.mEndValue,
                                      valuePortion, pt.mCurrentValue);
      NS_ABORT_IF_FALSE(ok, "could not interpolate values");

      void *prop =
        nsCSSExpandedDataBlock::RuleDataPropertyAt(aRuleData, pt.mProperty);
#ifdef DEBUG
      ok =
#endif
        nsStyleAnimation::UncomputeValue(pt.mProperty, aRuleData->mPresContext,
                                         pt.mCurrentValue, prop);
      NS_ABORT_IF_FALSE(ok, "could not store computed value");
    }
  }

  return NS_OK;
}

#ifdef DEBUG
NS_IMETHODIMP
ElementTransitionsStyleRule::List(FILE* out, PRInt32 aIndent) const
{
  
  return NS_OK;
}
#endif

void
ElementTransitions::DropStyleRule()
{
  if (mStyleRule) {
    mStyleRule->Disconnect();
    mStyleRule = nsnull;
  }
}

PRBool
ElementTransitions::EnsureStyleRuleFor(TimeStamp aRefreshTime)
{
  if (!mStyleRule || mStyleRule->RefreshTime() != aRefreshTime) {
    DropStyleRule();

    ElementTransitionsStyleRule *newRule =
      new ElementTransitionsStyleRule(this, aRefreshTime);
    if (!newRule) {
      NS_WARNING("out of memory");
      return PR_FALSE;
    }

    mStyleRule = newRule;
  }

  return PR_TRUE;
}

NS_IMPL_ISUPPORTS1(CoverTransitionStartStyleRule, nsIStyleRule)

NS_IMETHODIMP
CoverTransitionStartStyleRule::MapRuleInfoInto(nsRuleData* aRuleData)
{
  for (PRUint32 i = 0, i_end = mCoveredValues.Length(); i < i_end; ++i) {
    CoveredValue &cv = mCoveredValues[i];
    if (aRuleData->mSIDs & nsCachedStyleData::GetBitForSID(
                             nsCSSProps::kSIDTable[cv.mProperty]))
    {
      void *prop =
        nsCSSExpandedDataBlock::RuleDataPropertyAt(aRuleData, cv.mProperty);
#ifdef DEBUG
      PRBool ok =
#endif
        nsStyleAnimation::UncomputeValue(cv.mProperty, aRuleData->mPresContext,
                                         cv.mCoveredValue, prop);
      NS_ABORT_IF_FALSE(ok, "could not store computed value");
    }
  }

  return NS_OK;
}

#ifdef DEBUG
NS_IMETHODIMP
CoverTransitionStartStyleRule::List(FILE* out, PRInt32 aIndent) const
{
  
  return NS_OK;
}
#endif





nsTransitionManager::nsTransitionManager(nsPresContext *aPresContext)
  : mPresContext(aPresContext)
{
  PR_INIT_CLIST(&mElementTransitions);
}

nsTransitionManager::~nsTransitionManager()
{
  
  while (!PR_CLIST_IS_EMPTY(&mElementTransitions)) {
    ElementTransitions *head = static_cast<ElementTransitions*>(
                                 PR_LIST_HEAD(&mElementTransitions));
    head->Destroy();
  }
}

already_AddRefed<nsIStyleRule>
nsTransitionManager::StyleContextChanged(nsIContent *aElement,
                                         nsStyleContext *aOldStyleContext,
                                         nsStyleContext *aNewStyleContext)
{
  NS_PRECONDITION(aOldStyleContext->GetPseudo() ==
                      aNewStyleContext->GetPseudo(),
                  "pseudo type mismatch");
  
  
  
  
  NS_PRECONDITION(aOldStyleContext->HasPseudoElementData() ==
                      aNewStyleContext->HasPseudoElementData(),
                  "pseudo type mismatch");

  
  
  const nsStyleDisplay *disp = aNewStyleContext->GetStyleDisplay();
  if (disp->mTransitionPropertyCount == 1 &&
      disp->mTransitions[0].GetDelay() == 0.0f &&
      disp->mTransitions[0].GetDuration() == 0.0f) {
    return nsnull;
  }      


  if (aNewStyleContext->PresContext()->IsProcessingAnimationStyleChange()) {
    return nsnull;
  }
  
  nsCSSPseudoElements::Type pseudoType = aNewStyleContext->GetPseudoType();
  if (pseudoType != nsCSSPseudoElements::ePseudo_NotPseudoElement &&
      pseudoType != nsCSSPseudoElements::ePseudo_before &&
      pseudoType != nsCSSPseudoElements::ePseudo_after) {
    return nsnull;
  }
  if (aNewStyleContext->GetParent() &&
      aNewStyleContext->GetParent()->HasPseudoElementData()) {
    
    
    
    return nsnull;
  }

  
  
  
  
  PRBool startedAny = PR_FALSE;
  nsCSSPropertySet whichStarted;
  ElementTransitions *et = nsnull;
  for (PRUint32 i = disp->mTransitionPropertyCount; i-- != 0; ) {
    const nsTransition& t = disp->mTransitions[i];
    
    
    if (t.GetDelay() != 0.0f || t.GetDuration() != 0.0f) {
      et = GetElementTransitions(aElement, pseudoType, PR_FALSE);

      
      
      nsCSSProperty property = t.GetProperty();
      if (property == eCSSPropertyExtra_no_properties ||
          property == eCSSProperty_UNKNOWN) {
        
      } else if (property == eCSSPropertyExtra_all_properties) {
        for (nsCSSProperty p = nsCSSProperty(0); 
             p < eCSSProperty_COUNT_no_shorthands;
             p = nsCSSProperty(p + 1)) {
          ConsiderStartingTransition(p, t, aElement, et,
                                     aOldStyleContext, aNewStyleContext,
                                     &startedAny, &whichStarted);
        }
      } else if (nsCSSProps::IsShorthand(property)) {
        CSSPROPS_FOR_SHORTHAND_SUBPROPERTIES(subprop, property) {
          ConsiderStartingTransition(*subprop, t, aElement, et,
                                     aOldStyleContext, aNewStyleContext,
                                     &startedAny, &whichStarted);
        }
      } else {
        ConsiderStartingTransition(property, t, aElement, et,
                                   aOldStyleContext, aNewStyleContext,
                                   &startedAny, &whichStarted);
      }
    }
  }

  if (!startedAny) {
    return nsnull;
  }

  NS_ABORT_IF_FALSE(et, "must have element transitions if we started "
                        "any transitions");

  
  
  
  
  
  
  
  
  
  
  
  
  
  

  nsRefPtr<CoverTransitionStartStyleRule> coverRule =
    new CoverTransitionStartStyleRule;
  if (!coverRule) {
    NS_WARNING("out of memory");
    return nsnull;
  }
  
  nsTArray<ElementPropertyTransition> &pts = et->mPropertyTransitions;
  for (PRUint32 i = 0, i_end = pts.Length(); i < i_end; ++i) {
    ElementPropertyTransition &pt = pts[i];
    if (whichStarted.HasProperty(pt.mProperty)) {
      coverRule->CoverValue(pt.mProperty, pt.mStartValue);
    }
  }

  return already_AddRefed<nsIStyleRule>(
           static_cast<nsIStyleRule*>(coverRule.forget().get()));
}

void
nsTransitionManager::ConsiderStartingTransition(nsCSSProperty aProperty,
                       const nsTransition& aTransition,
                       nsIContent *aElement,
                       ElementTransitions *&aElementTransitions,
                       nsStyleContext *aOldStyleContext,
                       nsStyleContext *aNewStyleContext,
                       PRBool *aStartedAny,
                       nsCSSPropertySet *aWhichStarted)
{
  
  NS_ABORT_IF_FALSE(!nsCSSProps::IsShorthand(aProperty),
                    "property out of range");

  if (aWhichStarted->HasProperty(aProperty)) {
    
    
    
    
    return;
  }

  if (nsCSSProps::kAnimTypeTable[aProperty] == eStyleAnimType_None) {
    return;
  }

  ElementPropertyTransition pt;
  nsStyleAnimation::Value dummyValue;
  
  
  
  
  
  PRBool shouldAnimate =
    nsStyleAnimation::ExtractComputedValue(aProperty, aOldStyleContext,
                                           pt.mStartValue) &&
    nsStyleAnimation::ExtractComputedValue(aProperty, aNewStyleContext,
                                           pt.mEndValue) &&
    pt.mStartValue != pt.mEndValue &&
    
    
    
    nsStyleAnimation::Interpolate(aProperty, pt.mStartValue, pt.mEndValue,
                                  0.5, dummyValue);

  PRUint32 currentIndex = nsTArray<ElementPropertyTransition>::NoIndex;
  if (aElementTransitions) {
    nsTArray<ElementPropertyTransition> &pts =
      aElementTransitions->mPropertyTransitions;
    for (PRUint32 i = 0, i_end = pts.Length(); i < i_end; ++i) {
      if (pts[i].mProperty == aProperty) {
        currentIndex = i;
        break;
      }
    }
  }

  nsPresContext *presContext = aNewStyleContext->PresContext();

  if (!shouldAnimate) {
    if (currentIndex != nsTArray<ElementPropertyTransition>::NoIndex) {
      
      
      
      
      nsTArray<ElementPropertyTransition> &pts =
        aElementTransitions->mPropertyTransitions;
      pts.RemoveElementAt(currentIndex);
      if (pts.IsEmpty()) {
        aElementTransitions->Destroy();
        
        aElementTransitions = nsnull;
      }
      presContext->PresShell()->RestyleForAnimation(aElement);
    }
    return;
  }

  
  
  
  
  double durationFraction = 1.0;

  
  
  
  if (currentIndex != nsTArray<ElementPropertyTransition>::NoIndex) {
    const nsStyleAnimation::Value &endVal =
      aElementTransitions->mPropertyTransitions[currentIndex].mEndValue;

    if (endVal == pt.mEndValue) {
      
      
      
      
      
      presContext->PresShell()->RestyleForAnimation(aElement);
      return;
    }

    double fullDistance, remainingDistance;
#ifdef DEBUG
    PRBool ok =
#endif
      nsStyleAnimation::ComputeDistance(aProperty, pt.mStartValue,
                                        pt.mEndValue, fullDistance);
    NS_ABORT_IF_FALSE(ok, "could not compute distance");
    NS_ABORT_IF_FALSE(fullDistance >= 0.0, "distance must be positive");

    if (nsStyleAnimation::ComputeDistance(aProperty, endVal, pt.mEndValue,
                                          remainingDistance)) {
      NS_ABORT_IF_FALSE(remainingDistance >= 0.0, "distance must be positive");
      durationFraction = fullDistance / remainingDistance;
      if (durationFraction > 1.0) {
        durationFraction = 1.0;
      }
    }
  }


  nsRefreshDriver *rd = presContext->RefreshDriver();

  pt.mProperty = aProperty;
  float delay = aTransition.GetDelay();
  float duration = aTransition.GetDuration();
  if (durationFraction != 1.0) {
    
    
    
    
    if (delay < 0.0f)
        delay *= durationFraction;
    duration *= durationFraction;
  }
  pt.mStartTime = rd->MostRecentRefresh() +
                  TimeDuration::FromMilliseconds(delay);
  pt.mDuration = TimeDuration::FromMilliseconds(duration);
  const nsTimingFunction &tf = aTransition.GetTimingFunction();
  pt.mTimingFunction.Init(tf.mX1, tf.mY1, tf.mX2, tf.mY2);

  if (!aElementTransitions) {
    aElementTransitions =
      GetElementTransitions(aElement, aNewStyleContext->GetPseudoType(),
                            PR_TRUE);
    if (!aElementTransitions) {
      NS_WARNING("allocating ElementTransitions failed");
      return;
    }
  }
  
  nsTArray<ElementPropertyTransition> &pts =
    aElementTransitions->mPropertyTransitions;
#ifdef DEBUG
  for (PRUint32 i = 0, i_end = pts.Length(); i < i_end; ++i) {
    NS_ABORT_IF_FALSE(i == currentIndex ||
                      pts[i].mProperty != aProperty,
                      "duplicate transitions for property");
  }
#endif
  if (currentIndex != nsTArray<ElementPropertyTransition>::NoIndex) {
    pts[currentIndex] = pt;
  } else {
    if (!pts.AppendElement(pt)) {
      NS_WARNING("out of memory");
      return;
    }
  }

  presContext->PresShell()->RestyleForAnimation(aElement);

  *aStartedAny = PR_TRUE;
  aWhichStarted->AddProperty(aProperty);
}

ElementTransitions*
nsTransitionManager::GetElementTransitions(nsIContent *aElement,
                                           nsCSSPseudoElements::Type aPseudoType,
                                           PRBool aCreateIfNeeded)
{
  nsIAtom *propName;
  if (aPseudoType == nsCSSPseudoElements::ePseudo_before) {
    propName = nsGkAtoms::transitionsOfBeforeProperty;
  } else if (aPseudoType == nsCSSPseudoElements::ePseudo_after) {
    propName = nsGkAtoms::transitionsOfAfterProperty;
  } else {
    NS_ASSERTION(aPseudoType == nsCSSPseudoElements::ePseudo_NotPseudoElement ||
		 !aCreateIfNeeded,
                 "should never try to create transitions for pseudo "
                 "other than :before or :after");
    propName = nsGkAtoms::transitionsProperty;
  }
  ElementTransitions *et = static_cast<ElementTransitions*>(
                             aElement->GetProperty(propName));
  if (!et && aCreateIfNeeded) {
    
    et = new ElementTransitions(aElement, propName, this);
    if (!et) {
      NS_WARNING("out of memory");
      return nsnull;
    }
    nsresult rv = aElement->SetProperty(propName, et,
                                        ElementTransitionsPropertyDtor, nsnull);
    if (NS_FAILED(rv)) {
      NS_WARNING("SetProperty failed");
      delete et;
      return nsnull;
    }

    AddElementTransitions(et);
  }

  return et;
}

void
nsTransitionManager::AddElementTransitions(ElementTransitions* aElementTransitions)
{
  if (PR_CLIST_IS_EMPTY(&mElementTransitions)) {
    
    nsRefreshDriver *rd = mPresContext->RefreshDriver();
    rd->AddRefreshObserver(this, Flush_Style);
  }

  PR_INSERT_BEFORE(aElementTransitions, &mElementTransitions);
}





NS_IMPL_ADDREF_USING_AGGREGATOR(nsTransitionManager, mPresContext)
NS_IMPL_RELEASE_USING_AGGREGATOR(nsTransitionManager, mPresContext)
NS_IMPL_QUERY_INTERFACE1(nsTransitionManager, nsIStyleRuleProcessor)





nsresult
nsTransitionManager::WalkTransitionRule(RuleProcessorData* aData,
                                        nsCSSPseudoElements::Type aPseudoType)
{
  if (!aData->mPresContext->IsProcessingAnimationStyleChange()) {
    
    
    
    

    
    
    
    return NS_OK;
  }

  ElementTransitions *et =
    GetElementTransitions(aData->mContent, aPseudoType, PR_FALSE);
  if (!et) {
    return NS_OK;
  }

  if (!et->EnsureStyleRuleFor(
        aData->mPresContext->RefreshDriver()->MostRecentRefresh())) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  aData->mRuleWalker->Forward(et->mStyleRule);

  return NS_OK;
}

NS_IMETHODIMP
nsTransitionManager::RulesMatching(ElementRuleProcessorData* aData)
{
  NS_ABORT_IF_FALSE(aData->mPresContext == mPresContext,
                    "pres context mismatch");
  return WalkTransitionRule(aData,
			    nsCSSPseudoElements::ePseudo_NotPseudoElement);
}

NS_IMETHODIMP
nsTransitionManager::RulesMatching(PseudoElementRuleProcessorData* aData)
{
  NS_ABORT_IF_FALSE(aData->mPresContext == mPresContext,
                    "pres context mismatch");

  
  
  
  return WalkTransitionRule(aData, aData->mPseudoType);
}

NS_IMETHODIMP
nsTransitionManager::RulesMatching(AnonBoxRuleProcessorData* aData)
{
  return NS_OK;
}

#ifdef MOZ_XUL
NS_IMETHODIMP
nsTransitionManager::RulesMatching(XULTreeRuleProcessorData* aData)
{
  return NS_OK;
}
#endif

NS_IMETHODIMP
nsTransitionManager::HasStateDependentStyle(StateRuleProcessorData* aData,
                                            nsReStyleHint* aResult)
{
  *aResult = nsReStyleHint(0);
  return NS_OK;
}

nsReStyleHint
nsTransitionManager::HasAttributeDependentStyle(AttributeRuleProcessorData* aData)
{
  return nsReStyleHint(0);
}

NS_IMETHODIMP
nsTransitionManager::MediumFeaturesChanged(nsPresContext* aPresContext,
                                           PRBool* aRulesChanged)
{
  *aRulesChanged = PR_FALSE;
  return NS_OK;
}

 void
nsTransitionManager::WillRefresh(mozilla::TimeStamp aTime)
{
  
  
  {
    PRCList *next = PR_LIST_HEAD(&mElementTransitions);
    while (next != &mElementTransitions) {
      ElementTransitions *et = static_cast<ElementTransitions*>(next);
      next = PR_NEXT_LINK(next);

      NS_ABORT_IF_FALSE(et->mElement->GetCurrentDoc() ==
                          mPresContext->Document(),
                        "nsGenericElement::UnbindFromTree should have "
                        "destroyed the element transitions object");

      PRUint32 i = et->mPropertyTransitions.Length();
      NS_ABORT_IF_FALSE(i != 0, "empty transitions list?");
      do {
        --i;
        ElementPropertyTransition &pt = et->mPropertyTransitions[i];
        if (pt.mStartTime + pt.mDuration <= aTime) {
          
          et->mPropertyTransitions.RemoveElementAt(i);
        }
      } while (i != 0);

      
      
      mPresContext->PresShell()->RestyleForAnimation(et->mElement);

      if (et->mPropertyTransitions.IsEmpty()) {
        et->Destroy();
        
        et = nsnull;
      }
    }
  }

  
  TransitionsRemoved();
}

void
nsTransitionManager::TransitionsRemoved()
{
  
  
  if (PR_CLIST_IS_EMPTY(&mElementTransitions)) {
    mPresContext->RefreshDriver()->RemoveRefreshObserver(this, Flush_Style);
  }
}
