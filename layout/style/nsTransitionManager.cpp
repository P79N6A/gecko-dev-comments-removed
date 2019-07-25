







































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
#include "gfxColor.h"
#include "nsCSSPropertySet.h"
#include "nsStyleAnimation.h"
#include "nsEventDispatcher.h"
#include "nsGUIEvent.h"
#include "mozilla/dom/Element.h"

using mozilla::TimeStamp;
using mozilla::TimeDuration;

namespace dom = mozilla::dom;
namespace css = mozilla::css;





struct ElementPropertyTransition
{
  nsCSSProperty mProperty;
  nsStyleAnimation::Value mStartValue, mEndValue;
  TimeStamp mStartTime; 

  
  TimeDuration mDuration;
  css::ComputedTimingFunction mTimingFunction;

  
  
  
  
  
  nsStyleAnimation::Value mStartForReversingTest;
  
  
  
  
  
  
  
  
  
  double mReversePortion;

  
  
  
  double ValuePortionFor(TimeStamp aRefreshTime) const;

  PRBool IsRemovedSentinel() const
  {
    return mStartTime.IsNull();
  }

  void SetRemovedSentinel()
  {
    
    mStartTime = TimeStamp();
  }
};

double
ElementPropertyTransition::ValuePortionFor(TimeStamp aRefreshTime) const
{
  
  
  
  double duration = mDuration.ToSeconds();
  NS_ABORT_IF_FALSE(duration >= 0.0, "negative duration forbidden");
  double timePortion;
  if (duration == 0.0) {
    
    
    if (aRefreshTime >= mStartTime) {
      timePortion = 1.0;
    } else {
      timePortion = 0.0;
    }
  } else {
    timePortion = (aRefreshTime - mStartTime).ToSeconds() / duration;
    if (timePortion < 0.0)
      timePortion = 0.0; 
    if (timePortion > 1.0)
      timePortion = 1.0; 
  }

  return mTimingFunction.GetValue(timePortion);
}

struct ElementTransitions : public mozilla::css::CommonElementAnimationData
{
  ElementTransitions(dom::Element *aElement, nsIAtom *aElementProperty,
                     nsTransitionManager *aTransitionManager)
    : CommonElementAnimationData(aElement, aElementProperty,
                                 aTransitionManager)
  {
  }

  void EnsureStyleRuleFor(TimeStamp aRefreshTime);


  
  nsTArray<ElementPropertyTransition> mPropertyTransitions;

  
  
  
  
  
  
  nsRefPtr<css::AnimValuesStyleRule> mStyleRule;
  
  TimeStamp mStyleRuleRefreshTime;
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

void
ElementTransitions::EnsureStyleRuleFor(TimeStamp aRefreshTime)
{
  if (!mStyleRule || mStyleRuleRefreshTime != aRefreshTime) {
    mStyleRule = new css::AnimValuesStyleRule();
    mStyleRuleRefreshTime = aRefreshTime;

    for (PRUint32 i = 0, i_end = mPropertyTransitions.Length(); i < i_end; ++i)
    {
      ElementPropertyTransition &pt = mPropertyTransitions[i];
      if (pt.IsRemovedSentinel()) {
        continue;
      }

      nsStyleAnimation::Value *val = mStyleRule->AddEmptyValue(pt.mProperty);

      double valuePortion = pt.ValuePortionFor(aRefreshTime);
#ifdef DEBUG
      PRBool ok =
#endif
        nsStyleAnimation::Interpolate(pt.mProperty,
                                      pt.mStartValue, pt.mEndValue,
                                      valuePortion, *val);
      NS_ABORT_IF_FALSE(ok, "could not interpolate values");
    }
  }
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
          
          
          !ExtractComputedValueForTransition(pt.mProperty, aNewStyleContext,
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

  
  
  
  
  
  
  
  
  
  
  
  
  
  

  nsRefPtr<css::AnimValuesStyleRule> coverRule = new css::AnimValuesStyleRule;
  if (!coverRule) {
    NS_WARNING("out of memory");
    return nsnull;
  }
  
  nsTArray<ElementPropertyTransition> &pts = et->mPropertyTransitions;
  for (PRUint32 i = 0, i_end = pts.Length(); i < i_end; ++i) {
    ElementPropertyTransition &pt = pts[i];
    if (whichStarted.HasProperty(pt.mProperty)) {
      coverRule->AddValue(pt.mProperty, pt.mStartValue);
    }
  }

  return coverRule.forget();
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
  PRBool haveValues =
    ExtractComputedValueForTransition(aProperty, aOldStyleContext,
                                      pt.mStartValue) &&
    ExtractComputedValueForTransition(aProperty, aNewStyleContext,
                                      pt.mEndValue);
  PRBool shouldAnimate =
    haveValues &&
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
    nsTArray<ElementPropertyTransition> &pts =
      aElementTransitions->mPropertyTransitions;
    if (currentIndex != nsTArray<ElementPropertyTransition>::NoIndex &&
        (!haveValues || pts[currentIndex].mEndValue != pt.mEndValue)) {
      
      
      
      
      
      
      
      
      
      pts.RemoveElementAt(currentIndex);
      if (pts.IsEmpty()) {
        aElementTransitions->Destroy();
        
        aElementTransitions = nsnull;
      }
      
    }
    return;
  }

  TimeStamp mostRecentRefresh =
    presContext->RefreshDriver()->MostRecentRefresh();

  const nsTimingFunction &tf = aTransition.GetTimingFunction();
  float delay = aTransition.GetDelay();
  float duration = aTransition.GetDuration();
  if (duration < 0.0) {
    
    duration = 0.0;
  }
  pt.mStartForReversingTest = pt.mStartValue;
  pt.mReversePortion = 1.0;

  
  
  if (currentIndex != nsTArray<ElementPropertyTransition>::NoIndex) {
    const ElementPropertyTransition &oldPT =
      aElementTransitions->mPropertyTransitions[currentIndex];

    if (oldPT.mEndValue == pt.mEndValue) {
      
      
      
      
      return;
    }

    
    
    if (!oldPT.IsRemovedSentinel() &&
        oldPT.mStartForReversingTest == pt.mEndValue) {
      
      
      double valuePortion =
        oldPT.ValuePortionFor(mostRecentRefresh) * oldPT.mReversePortion +
        (1.0 - oldPT.mReversePortion); 
      
      
      
      
      if (valuePortion < 0.0)
        valuePortion = -valuePortion;
      
      
      
      
      
      if (valuePortion > 1.0)
        valuePortion = 1.0;

      
      
      
      if (delay < 0.0f)
        delay *= valuePortion;
      duration *= valuePortion;

      pt.mStartForReversingTest = oldPT.mEndValue;
      pt.mReversePortion = valuePortion;
    }
  }

  pt.mProperty = aProperty;
  pt.mStartTime = mostRecentRefresh + TimeDuration::FromMilliseconds(delay);
  pt.mDuration = TimeDuration::FromMilliseconds(duration);
  pt.mTimingFunction.Init(tf);

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
  if (!aCreateIfNeeded && PR_CLIST_IS_EMPTY(&mElementData)) {
    
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

    AddElementData(et);
  }

  return et;
}





void
nsTransitionManager::WalkTransitionRule(RuleProcessorData* aData,
                                        nsCSSPseudoElements::Type aPseudoType)
{
  ElementTransitions *et =
    GetElementTransitions(aData->mElement, aPseudoType, PR_FALSE);
  if (!et) {
    return;
  }

  if (aData->mPresContext->IsProcessingRestyles() &&
      !aData->mPresContext->IsProcessingAnimationStyleChange()) {
    
    
    
    

    
    
    if (et) {
      nsRestyleHint hint =
        aPseudoType == nsCSSPseudoElements::ePseudo_NotPseudoElement ?
        eRestyle_Self : eRestyle_Subtree;
      mPresContext->PresShell()->RestyleForAnimation(aData->mElement, hint);
    }
    return;
  }

  et->EnsureStyleRuleFor(
    aData->mPresContext->RefreshDriver()->MostRecentRefresh());

  aData->mRuleWalker->Forward(et->mStyleRule);
}

 void
nsTransitionManager::RulesMatching(ElementRuleProcessorData* aData)
{
  NS_ABORT_IF_FALSE(aData->mPresContext == mPresContext,
                    "pres context mismatch");
  WalkTransitionRule(aData,
                     nsCSSPseudoElements::ePseudo_NotPseudoElement);
}

 void
nsTransitionManager::RulesMatching(PseudoElementRuleProcessorData* aData)
{
  NS_ABORT_IF_FALSE(aData->mPresContext == mPresContext,
                    "pres context mismatch");

  
  
  
  WalkTransitionRule(aData, aData->mPseudoType);
}

 void
nsTransitionManager::RulesMatching(AnonBoxRuleProcessorData* aData)
{
}

#ifdef MOZ_XUL
 void
nsTransitionManager::RulesMatching(XULTreeRuleProcessorData* aData)
{
}
#endif

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
  if (!mPresContext->GetPresShell()) {
    
    
    
    
    RemoveAllElementData();
    return;
  }

  nsTArray<TransitionEventInfo> events;

  
  
  {
    PRCList *next = PR_LIST_HEAD(&mElementData);
    while (next != &mElementData) {
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

  
  ElementDataRemoved();

  for (PRUint32 i = 0, i_end = events.Length(); i < i_end; ++i) {
    TransitionEventInfo &info = events[i];
    nsEventDispatcher::Dispatch(info.mElement, mPresContext, &info.mEvent);

    if (!mPresContext) {
      break;
    }
  }
}
