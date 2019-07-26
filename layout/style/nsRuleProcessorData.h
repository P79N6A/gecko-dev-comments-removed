









#ifndef nsRuleProcessorData_h_
#define nsRuleProcessorData_h_

#include "nsPresContext.h" 
#include "nsString.h"
#include "nsChangeHint.h"
#include "nsIContent.h"
#include "nsCSSPseudoElements.h"
#include "nsRuleWalker.h"
#include "nsNthIndexCache.h"
#include "nsILoadContext.h"
#include "mozilla/BloomFilter.h"
#include "mozilla/GuardObjects.h"

class nsIStyleSheet;
class nsIAtom;
class nsICSSPseudoComparator;
class nsAttrValue;






class NS_STACK_CLASS AncestorFilter {
 public:
  




  void Init(mozilla::dom::Element *aElement);

  
  void PushAncestor(mozilla::dom::Element *aElement);
  void PopAncestor();

  
  class NS_STACK_CLASS AutoAncestorPusher {
  public:
    AutoAncestorPusher(bool aDoPush,
                       AncestorFilter &aFilter,
                       mozilla::dom::Element *aElement
                       MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : mPushed(aDoPush && aElement), mFilter(aFilter)
    {
      MOZ_GUARD_OBJECT_NOTIFIER_INIT;
      if (mPushed) {
        mFilter.PushAncestor(aElement);
      }
    }
    ~AutoAncestorPusher() {
      if (mPushed) {
        mFilter.PopAncestor();
      }
    }

  private:
    bool mPushed;
    AncestorFilter &mFilter;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
  };

  

  template<size_t hashListLength>
    bool MightHaveMatchingAncestor(const uint32_t* aHashes) const
  {
    MOZ_ASSERT(mFilter);
    for (size_t i = 0; i < hashListLength && aHashes[i]; ++i) {
      if (!mFilter->mightContain(aHashes[i])) {
        return false;
      }
    }

    return true;
  }

  bool HasFilter() const { return mFilter; }

#ifdef DEBUG
  void AssertHasAllAncestors(mozilla::dom::Element *aElement) const;
#endif
  
 private:
  
  
  
  
  
  typedef mozilla::BloomFilter<12, nsIAtom> Filter;
  nsAutoPtr<Filter> mFilter;

  
  nsTArray<uint32_t> mPopTargets;

  
  
  
  nsTArray<uint32_t> mHashes;

  
#ifdef DEBUG
  nsTArray<mozilla::dom::Element*> mElements;
#endif
};













struct NS_STACK_CLASS TreeMatchContext {
  
  void ResetForVisitedMatching() {
    NS_PRECONDITION(mForStyling, "Why is this being called?");
    mHaveRelevantLink = false;
    mVisitedHandling = nsRuleWalker::eRelevantLinkVisited;
  }
  
  void ResetForUnvisitedMatching() {
    NS_PRECONDITION(mForStyling, "Why is this being called?");
    mHaveRelevantLink = false;
    mVisitedHandling = nsRuleWalker::eRelevantLinkUnvisited;
  }

  void SetHaveRelevantLink() { mHaveRelevantLink = true; }
  bool HaveRelevantLink() const { return mHaveRelevantLink; }

  nsRuleWalker::VisitedHandlingType VisitedHandling() const
  {
    return mVisitedHandling;
  }

  void AddScopeElement(mozilla::dom::Element* aElement) {
    NS_PRECONDITION(mHaveSpecifiedScope,
                    "Should be set before calling AddScopeElement()");
    mScopes.AppendElement(aElement);
  }
  bool IsScopeElement(mozilla::dom::Element* aElement) const {
    return mScopes.Contains(aElement);
  }
  void SetHasSpecifiedScope() {
    mHaveSpecifiedScope = true;
  }
  bool HasSpecifiedScope() const {
    return mHaveSpecifiedScope;
  }

  
  
  
  const bool mForStyling;

 private:
  
  
  
  
  
  bool mHaveRelevantLink;

  
  
  bool mHaveSpecifiedScope;

  
  
  nsRuleWalker::VisitedHandlingType mVisitedHandling;

  
  nsAutoTArray<mozilla::dom::Element*, 1> mScopes;
 public:
  
  nsIDocument* const mDocument;

  
  
  nsIContent* mScopedRoot;

  
  
  
  const bool mIsHTMLDocument;

  
  
  const nsCompatibility mCompatMode;

  
  nsNthIndexCache mNthIndexCache;

  
  AncestorFilter mAncestorFilter;

  
  bool mUsingPrivateBrowsing;

  enum MatchVisited {
    eNeverMatchVisited,
    eMatchVisitedDefault
  };

  
  TreeMatchContext(bool aForStyling,
                   nsRuleWalker::VisitedHandlingType aVisitedHandling,
                   nsIDocument* aDocument,
                   MatchVisited aMatchVisited = eMatchVisitedDefault)
    : mForStyling(aForStyling)
    , mHaveRelevantLink(false)
    , mHaveSpecifiedScope(false)
    , mVisitedHandling(aVisitedHandling)
    , mDocument(aDocument)
    , mScopedRoot(nullptr)
    , mIsHTMLDocument(aDocument->IsHTML())
    , mCompatMode(aDocument->GetCompatibilityMode())
    , mUsingPrivateBrowsing(false)
  {
    if (aMatchVisited != eNeverMatchVisited) {
      nsCOMPtr<nsISupports> container = mDocument->GetContainer();
      if (container) {
        nsCOMPtr<nsILoadContext> loadContext = do_QueryInterface(container);
        NS_ASSERTION(loadContext, "Couldn't get loadContext from container; assuming no private browsing.");
        if (loadContext) {
          mUsingPrivateBrowsing = loadContext->UsePrivateBrowsing();
        }
      }
    }
  }
};

struct NS_STACK_CLASS RuleProcessorData {
  RuleProcessorData(nsPresContext* aPresContext,
                    nsRuleWalker* aRuleWalker)
    : mPresContext(aPresContext),
      mRuleWalker(aRuleWalker)
  {
    NS_PRECONDITION(mPresContext, "Must have prescontext");
  }

  nsPresContext* const mPresContext;
  nsRuleWalker* const mRuleWalker; 
};

struct NS_STACK_CLASS ElementDependentRuleProcessorData :
                          public RuleProcessorData {
  ElementDependentRuleProcessorData(nsPresContext* aPresContext,
                                    mozilla::dom::Element* aElement,
                                    nsRuleWalker* aRuleWalker,
                                    TreeMatchContext& aTreeMatchContext)
    : RuleProcessorData(aPresContext, aRuleWalker)
    , mElement(aElement)
    , mTreeMatchContext(aTreeMatchContext)
  {
    NS_ASSERTION(aElement, "null element leaked into SelectorMatches");
    NS_ASSERTION(aElement->OwnerDoc(), "Document-less node here?");
    NS_PRECONDITION(aTreeMatchContext.mForStyling == !!aRuleWalker,
                    "Should be styling if and only if we have a rule walker");
  }
  
  mozilla::dom::Element* const mElement; 
  TreeMatchContext& mTreeMatchContext;
};

struct NS_STACK_CLASS ElementRuleProcessorData :
                          public ElementDependentRuleProcessorData {
  ElementRuleProcessorData(nsPresContext* aPresContext,
                           mozilla::dom::Element* aElement, 
                           nsRuleWalker* aRuleWalker,
                           TreeMatchContext& aTreeMatchContext)
    : ElementDependentRuleProcessorData(aPresContext, aElement, aRuleWalker,
                                        aTreeMatchContext)
  {
    NS_PRECONDITION(aTreeMatchContext.mForStyling, "Styling here!");
    NS_PRECONDITION(aRuleWalker, "Must have rule walker");
  }
};

struct NS_STACK_CLASS PseudoElementRuleProcessorData :
                          public ElementDependentRuleProcessorData {
  PseudoElementRuleProcessorData(nsPresContext* aPresContext,
                                 mozilla::dom::Element* aParentElement,
                                 nsRuleWalker* aRuleWalker,
                                 nsCSSPseudoElements::Type aPseudoType,
                                 TreeMatchContext& aTreeMatchContext)
    : ElementDependentRuleProcessorData(aPresContext, aParentElement, aRuleWalker,
                                        aTreeMatchContext),
      mPseudoType(aPseudoType)
  {
    NS_PRECONDITION(aPseudoType <
                      nsCSSPseudoElements::ePseudo_PseudoElementCount,
                    "invalid aPseudoType value");
    NS_PRECONDITION(aTreeMatchContext.mForStyling, "Styling here!");
    NS_PRECONDITION(aRuleWalker, "Must have rule walker");
  }

  nsCSSPseudoElements::Type mPseudoType;
};

struct NS_STACK_CLASS AnonBoxRuleProcessorData : public RuleProcessorData {
  AnonBoxRuleProcessorData(nsPresContext* aPresContext,
                           nsIAtom* aPseudoTag,
                           nsRuleWalker* aRuleWalker)
    : RuleProcessorData(aPresContext, aRuleWalker),
      mPseudoTag(aPseudoTag)
  {
    NS_PRECONDITION(aPseudoTag, "Must have pseudo tag");
    NS_PRECONDITION(aRuleWalker, "Must have rule walker");
  }

  nsIAtom* mPseudoTag;
};

#ifdef MOZ_XUL
struct NS_STACK_CLASS XULTreeRuleProcessorData :
                          public ElementDependentRuleProcessorData {
  XULTreeRuleProcessorData(nsPresContext* aPresContext,
                           mozilla::dom::Element* aParentElement,
                           nsRuleWalker* aRuleWalker,
                           nsIAtom* aPseudoTag,
                           nsICSSPseudoComparator* aComparator,
                           TreeMatchContext& aTreeMatchContext)
    : ElementDependentRuleProcessorData(aPresContext, aParentElement,
                                        aRuleWalker, aTreeMatchContext),
      mPseudoTag(aPseudoTag),
      mComparator(aComparator)
  {
    NS_PRECONDITION(aPseudoTag, "null pointer");
    NS_PRECONDITION(aRuleWalker, "Must have rule walker");
    NS_PRECONDITION(aComparator, "must have a comparator");
    NS_PRECONDITION(aTreeMatchContext.mForStyling, "Styling here!");
  }

  nsIAtom*                 mPseudoTag;
  nsICSSPseudoComparator*  mComparator;
};
#endif

struct NS_STACK_CLASS StateRuleProcessorData :
                          public ElementDependentRuleProcessorData {
  StateRuleProcessorData(nsPresContext* aPresContext,
                         mozilla::dom::Element* aElement,
                         nsEventStates aStateMask,
                         TreeMatchContext& aTreeMatchContext)
    : ElementDependentRuleProcessorData(aPresContext, aElement, nullptr,
                                        aTreeMatchContext),
      mStateMask(aStateMask)
  {
    NS_PRECONDITION(!aTreeMatchContext.mForStyling, "Not styling here!");
  }
  const nsEventStates mStateMask; 
                                  
};

struct NS_STACK_CLASS AttributeRuleProcessorData :
                          public ElementDependentRuleProcessorData {
  AttributeRuleProcessorData(nsPresContext* aPresContext,
                             mozilla::dom::Element* aElement,
                             nsIAtom* aAttribute,
                             int32_t aModType,
                             bool aAttrHasChanged,
                             TreeMatchContext& aTreeMatchContext)
    : ElementDependentRuleProcessorData(aPresContext, aElement, nullptr,
                                        aTreeMatchContext),
      mAttribute(aAttribute),
      mModType(aModType),
      mAttrHasChanged(aAttrHasChanged)
  {
    NS_PRECONDITION(!aTreeMatchContext.mForStyling, "Not styling here!");
  }
  nsIAtom* mAttribute; 
  int32_t mModType;    
  bool mAttrHasChanged; 
};

#endif 
