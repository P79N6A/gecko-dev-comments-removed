







































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
#include "nsEventDispatcher.h"
#include "nsGUIEvent.h"
#include "mozilla/dom/Element.h"

using mozilla::TimeStamp;
using mozilla::TimeDuration;

namespace dom = mozilla::dom;





struct ElementPropertyTransition
{
  nsCSSProperty mProperty;
  
  
  
  
  nsStyleAnimation::Value mStartValue, mEndValue, mCurrentValue;
  TimeStamp mStartTime; 

  
  TimeDuration mDuration;
  nsSMILKeySpline mTimingFunction;

  PRBool IsRemovedSentinel() const
  {
    return mStartTime.IsNull();
  }

  void SetRemovedSentinel()
  {
    
    mStartTime = TimeStamp();
  }
};









class ElementTransitionsStyleRule : public nsIStyleRule
{
public:
  
  NS_DECL_ISUPPORTS

  
  virtual void MapRuleInfoInto(nsRuleData* aRuleData);
#ifdef DEBUG
  virtual void List(FILE* out = stdout, PRInt32 aIndent = 0) const;
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

  
  virtual void MapRuleInfoInto(nsRuleData* aRuleData);
#ifdef DEBUG
  virtual void List(FILE* out = stdout, PRInt32 aIndent = 0) const;
#endif

  void CoverValue(nsCSSProperty aProperty, nsStyleAnimation::Value &aStartValue)
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
  ElementTransitions(dom::Element *aElement, nsIAtom *aElementProperty,
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

  dom::Element *mElement;

  
  
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

 void
ElementTransitionsStyleRule::MapRuleInfoInto(nsRuleData* aRuleData)
{
  nsStyleContext *contextParent = aRuleData->mStyleContext->GetParent();
  if (contextParent && contextParent->HasPseudoElementData()) {
    
    
    return;
  }

  ElementTransitions *et = ElementData();
  if (NS_UNLIKELY(!et)) { 
     NS_WARNING("ElementData returned null");
     return;
  }

  for (PRUint32 i = 0, i_end = et->mPropertyTransitions.Length();
       i < i_end; ++i)
  {
    ElementPropertyTransition &pt = et->mPropertyTransitions[i];
    if (pt.IsRemovedSentinel()) {
      continue;
    }

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
}

#ifdef DEBUG
 void
ElementTransitionsStyleRule::List(FILE* out, PRInt32 aIndent) const
{
  
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

 void
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
}

#ifdef DEBUG
 void
CoverTransitionStartStyleRule::List(FILE* out, PRInt32 aIndent) const
{
  
}
#endif





nsTransitionManager::nsTransitionManager(nsPresContext *aPresContext)
  : mPresContext(aPresContext)
{
  PR_INIT_CLIST(&mElementTransitions);
}

nsTransitionManager::~nsTransitionManager()
{
  NS_ABORT_IF_FALSE(!mPresContext, "Disconnect should have been called");
}

void
nsTransitionManager::Disconnect()
{
  
  while (!PR_CLIST_IS_EMPTY(&mElementTransitions)) {
    ElementTransitions *head = static_cast<ElementTransitions*>(
                                 PR_LIST_HEAD(&mElementTransitions));
    head->Destroy();
  }

  mPresContext = nsnull;
}

static PRBool
TransExtractComputedValue(nsCSSProperty aProperty,
                          nsStyleContext* aStyleContext,
                          nsStyleAnimation::Value& aComputedValue)
{
  PRBool result =
    nsStyleAnimation::ExtractComputedValue(aProperty, aStyleContext,
                                           aComputedValue);
  if (aProperty == eCSSProperty_visibility) {
    NS_ABORT_IF_FALSE(aComputedValue.GetUnit() ==
                        nsStyleAnimation::eUnit_Enumerated,
                      "unexpected unit");
    aComputedValue.SetIntValue(aComputedValue.GetIntValue(),
                               nsStyleAnimation::eUnit_Visibility);
  }
  return result;
}

already_AddRefed<nsIStyleRule>
nsTransitionManager::StyleContextChanged(dom::Element *aElement,
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
  nsCSSPseudoElements::Type pseudoType = aNewStyleContext->GetPseudoType();
  if (pseudoType != nsCSSPseudoElements::ePseudo_NotPseudoElement) {
    if (pseudoType != nsCSSPseudoElements::ePseudo_before &&
        pseudoType != nsCSSPseudoElements::ePseudo_after) {
      return nsnull;
    }

    NS_ASSERTION((pseudoType == nsCSSPseudoElements::ePseudo_before &&
                  aElement->Tag() == nsGkAtoms::mozgeneratedcontentbefore) ||
                 (pseudoType == nsCSSPseudoElements::ePseudo_after &&
                  aElement->Tag() == nsGkAtoms::mozgeneratedcontentafter),
                 "Unexpected aElement coming through");

    
    
    aElement = aElement->GetParent()->AsElement();
  }

  ElementTransitions *et =
      GetElementTransitions(aElement, pseudoType, PR_FALSE);
  if (!et &&
      disp->mTransitionPropertyCount == 1 &&
      disp->mTransitions[0].GetDelay() == 0.0f &&
      disp->mTransitions[0].GetDuration() == 0.0f) {
    return nsnull;
  }      


  if (aNewStyleContext->PresContext()->IsProcessingAnimationStyleChange()) {
    return nsnull;
  }
  
  if (aNewStyleContext->GetParent() &&
      aNewStyleContext->GetParent()->HasPseudoElementData()) {
    
    
    
    return nsnull;
  }

  
  
  
  
  PRBool startedAny = PR_FALSE;
  nsCSSPropertySet whichStarted;
  for (PRUint32 i = disp->mTransitionPropertyCount; i-- != 0; ) {
    const nsTransition& t = disp->mTransitions[i];
    
    
    if (t.GetDelay() != 0.0f || t.GetDuration() != 0.0f) {
      
      
      
      
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

  
  
  
  
  
  if (et) {
    PRBool checkProperties =
      disp->mTransitions[0].GetProperty() != eCSSPropertyExtra_all_properties;
    nsCSSPropertySet allTransitionProperties;
    if (checkProperties) {
      for (PRUint32 i = disp->mTransitionPropertyCount; i-- != 0; ) {
        const nsTransition& t = disp->mTransitions[i];
        
        
        nsCSSProperty property = t.GetProperty();
        if (property == eCSSPropertyExtra_no_properties ||
            property == eCSSProperty_UNKNOWN) {
          
        } else if (property == eCSSPropertyExtra_all_properties) {
          for (nsCSSProperty p = nsCSSProperty(0); 
               p < eCSSProperty_COUNT_no_shorthands;
               p = nsCSSProperty(p + 1)) {
            allTransitionProperties.AddProperty(p);
          }
        } else if (nsCSSProps::IsShorthand(property)) {
          CSSPROPS_FOR_SHORTHAND_SUBPROPERTIES(subprop, property) {
            allTransitionProperties.AddProperty(*subprop);
          }
        } else {
          allTransitionProperties.AddProperty(property);
        }
      }
    }

    nsTArray<ElementPropertyTransition> &pts = et->mPropertyTransitions;
    PRUint32 i = pts.Length();
    NS_ABORT_IF_FALSE(i != 0, "empty transitions list?");
    nsStyleAnimation::Value currentValue;
    do {
      --i;
      ElementPropertyTransition &pt = pts[i];
          
      if ((checkProperties &&
           !allTransitionProperties.HasProperty(pt.mProperty)) ||
          
          
          !TransExtractComputedValue(pt.mProperty, aNewStyleContext,
                                     currentValue) ||
          currentValue != pt.mEndValue) {
        
        pts.RemoveElementAt(i);
      }
    } while (i != 0);

    if (pts.IsEmpty()) {
      et->Destroy();
      et = nsnull;
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
                       dom::Element *aElement,
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
    TransExtractComputedValue(aProperty, aOldStyleContext, pt.mStartValue) &&
    TransExtractComputedValue(aProperty, aNewStyleContext, pt.mEndValue) &&
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
      
    }
    return;
  }

  
  
  
  
  double durationFraction = 1.0;

  
  
  
  if (currentIndex != nsTArray<ElementPropertyTransition>::NoIndex) {
    const ElementPropertyTransition &oldPT =
      aElementTransitions->mPropertyTransitions[currentIndex];

    if (oldPT.mEndValue == pt.mEndValue) {
      
      
      
      
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

    if (!oldPT.IsRemovedSentinel() &&
        nsStyleAnimation::ComputeDistance(aProperty, oldPT.mEndValue,
                                          pt.mEndValue, remainingDistance)) {
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

  nsRestyleHint hint =
    aNewStyleContext->GetPseudoType() ==
      nsCSSPseudoElements::ePseudo_NotPseudoElement ?
    eRestyle_Self : eRestyle_Subtree;
  presContext->PresShell()->RestyleForAnimation(aElement, hint);

  *aStartedAny = PR_TRUE;
  aWhichStarted->AddProperty(aProperty);
}

ElementTransitions*
nsTransitionManager::GetElementTransitions(dom::Element *aElement,
                                           nsCSSPseudoElements::Type aPseudoType,
                                           PRBool aCreateIfNeeded)
{
  if (!aCreateIfNeeded && PR_CLIST_IS_EMPTY(&mElementTransitions)) {
    
    return nsnull;
  }

  nsIAtom *propName;
  if (aPseudoType == nsCSSPseudoElements::ePseudo_NotPseudoElement) {
    propName = nsGkAtoms::transitionsProperty;
  } else if (aPseudoType == nsCSSPseudoElements::ePseudo_before) {
    propName = nsGkAtoms::transitionsOfBeforeProperty;
  } else if (aPseudoType == nsCSSPseudoElements::ePseudo_after) {
    propName = nsGkAtoms::transitionsOfAfterProperty;
  } else {
    NS_ASSERTION(!aCreateIfNeeded,
                 "should never try to create transitions for pseudo "
                 "other than :before or :after");
    return nsnull;
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





NS_IMPL_ISUPPORTS1(nsTransitionManager, nsIStyleRuleProcessor)





nsresult
nsTransitionManager::WalkTransitionRule(RuleProcessorData* aData,
                                        nsCSSPseudoElements::Type aPseudoType)
{
  ElementTransitions *et =
    GetElementTransitions(aData->mElement, aPseudoType, PR_FALSE);
  if (!et) {
    return NS_OK;
  }

  if (aData->mPresContext->IsProcessingRestyles() &&
      !aData->mPresContext->IsProcessingAnimationStyleChange()) {
    
    
    
    

    
    
    if (et) {
      nsRestyleHint hint =
        aPseudoType == nsCSSPseudoElements::ePseudo_NotPseudoElement ?
        eRestyle_Self : eRestyle_Subtree;
      mPresContext->PresShell()->RestyleForAnimation(aData->mElement, hint);
    }
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

nsRestyleHint
nsTransitionManager::HasStateDependentStyle(StateRuleProcessorData* aData)
{
  return nsRestyleHint(0);
}

PRBool
nsTransitionManager::HasDocumentStateDependentStyle(StateRuleProcessorData* aData)
{
  return PR_FALSE;
}

nsRestyleHint
nsTransitionManager::HasAttributeDependentStyle(AttributeRuleProcessorData* aData)
{
  return nsRestyleHint(0);
}

NS_IMETHODIMP
nsTransitionManager::MediumFeaturesChanged(nsPresContext* aPresContext,
                                           PRBool* aRulesChanged)
{
  *aRulesChanged = PR_FALSE;
  return NS_OK;
}

struct TransitionEventInfo {
  nsCOMPtr<nsIContent> mElement;
  nsTransitionEvent mEvent;

  TransitionEventInfo(nsIContent *aElement, nsCSSProperty aProperty,
                      TimeDuration aDuration)
    : mElement(aElement),
      mEvent(PR_TRUE, NS_TRANSITION_END,
             NS_ConvertUTF8toUTF16(nsCSSProps::GetStringValue(aProperty)),
             aDuration.ToSeconds())
  {
  }

  
  
  TransitionEventInfo(const TransitionEventInfo &aOther)
    : mElement(aOther.mElement),
      mEvent(PR_TRUE, NS_TRANSITION_END,
             aOther.mEvent.propertyName, aOther.mEvent.elapsedTime)
  {
  }
};

 void
nsTransitionManager::WillRefresh(mozilla::TimeStamp aTime)
{
  NS_ABORT_IF_FALSE(mPresContext,
                    "refresh driver should not notify additional observers "
                    "after pres context has been destroyed");

  nsTArray<TransitionEventInfo> events;

  
  
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
        if (pt.IsRemovedSentinel()) {
          
          
          et->mPropertyTransitions.RemoveElementAt(i);
        } else if (pt.mStartTime + pt.mDuration <= aTime) {
          

          
          
          
          if (et->mElementProperty == nsGkAtoms::transitionsProperty) {
            nsCSSProperty prop = pt.mProperty;
            if (nsCSSProps::PropHasFlags(prop, CSS_PROPERTY_REPORT_OTHER_NAME))
            {
              prop = nsCSSProps::OtherNameFor(prop);
            }
            events.AppendElement(
              TransitionEventInfo(et->mElement, prop, pt.mDuration));
          }

          
          
          
          
          
          
          
          pt.SetRemovedSentinel();
        }
      } while (i != 0);

      
      
      NS_ASSERTION(et->mElementProperty == nsGkAtoms::transitionsProperty ||
                   et->mElementProperty == nsGkAtoms::transitionsOfBeforeProperty ||
                   et->mElementProperty == nsGkAtoms::transitionsOfAfterProperty,
                   "Unexpected element property; might restyle too much");
      nsRestyleHint hint = et->mElementProperty == nsGkAtoms::transitionsProperty ?
        eRestyle_Self : eRestyle_Subtree;
      mPresContext->PresShell()->RestyleForAnimation(et->mElement, hint);

      if (et->mPropertyTransitions.IsEmpty()) {
        et->Destroy();
        
        et = nsnull;
      }
    }
  }

  
  TransitionsRemoved();

  for (PRUint32 i = 0, i_end = events.Length(); i < i_end; ++i) {
    TransitionEventInfo &info = events[i];
    nsEventDispatcher::Dispatch(info.mElement, mPresContext, &info.mEvent);

    if (!mPresContext) {
      break;
    }
  }
}

void
nsTransitionManager::TransitionsRemoved()
{
  
  
  if (PR_CLIST_IS_EMPTY(&mElementTransitions)) {
    mPresContext->RefreshDriver()->RemoveRefreshObserver(this, Flush_Style);
  }
}
