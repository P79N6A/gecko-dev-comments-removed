









#ifndef nsRuleProcessorData_h_
#define nsRuleProcessorData_h_

#include "nsChangeHint.h"
#include "nsCompatibility.h"
#include "nsCSSPseudoElements.h"
#include "nsRuleWalker.h"
#include "nsNthIndexCache.h"
#include "nsILoadContext.h"
#include "nsIDocument.h"
#include "mozilla/AutoRestore.h"
#include "mozilla/BloomFilter.h"
#include "mozilla/EventStates.h"
#include "mozilla/GuardObjects.h"

class nsIAtom;
class nsIContent;
class nsICSSPseudoComparator;
struct TreeMatchContext;






class MOZ_STACK_CLASS AncestorFilter {
  friend struct TreeMatchContext;
 public:
  
  void PushAncestor(mozilla::dom::Element *aElement);
  void PopAncestor();

  

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













struct MOZ_STACK_CLASS TreeMatchContext {
  
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

  





  void InitAncestors(mozilla::dom::Element *aElement);

  



  void InitStyleScopes(mozilla::dom::Element* aElement);

  void PushStyleScope(mozilla::dom::Element* aElement)
  {
    NS_PRECONDITION(aElement, "aElement must not be null");
    if (aElement->IsScopedStyleRoot()) {
      mStyleScopes.AppendElement(aElement);
    }
  }

  void PopStyleScope(mozilla::dom::Element* aElement)
  {
    NS_PRECONDITION(aElement, "aElement must not be null");
    if (mStyleScopes.SafeLastElement(nullptr) == aElement) {
      mStyleScopes.TruncateLength(mStyleScopes.Length() - 1);
    }
  }
 
  bool PopStyleScopeForSelectorMatching(mozilla::dom::Element* aElement)
  {
    NS_ASSERTION(mForScopedStyle, "only call PopStyleScopeForSelectorMatching "
                                  "when mForScopedStyle is true");

    if (!mCurrentStyleScope) {
      return false;
    }
    if (mCurrentStyleScope == aElement) {
      mCurrentStyleScope = nullptr;
    }
    return true;
  }

#ifdef DEBUG
  void AssertHasAllStyleScopes(mozilla::dom::Element* aElement) const;
#endif

  bool SetStyleScopeForSelectorMatching(mozilla::dom::Element* aSubject,
                                        mozilla::dom::Element* aScope)
  {
#ifdef DEBUG
    AssertHasAllStyleScopes(aSubject);
#endif

    mForScopedStyle = !!aScope;
    if (!aScope) {
      
      
      mCurrentStyleScope = nullptr;
      return true;
    }
    if (aScope == aSubject) {
      
      
      
      
      
      mCurrentStyleScope = nullptr;
      return true;
    }
    if (mStyleScopes.Contains(aScope)) {
      
      
      
      mCurrentStyleScope = aScope;
      return true;
    }
    
    
    mCurrentStyleScope = nullptr;
    return false;
  }

  bool IsWithinStyleScopeForSelectorMatching() const
  {
    NS_ASSERTION(mForScopedStyle, "only call IsWithinScopeForSelectorMatching "
                                  "when mForScopedStyle is true");
    return mCurrentStyleScope;
  }

  
  class MOZ_STACK_CLASS AutoAncestorPusher {
  public:
    explicit AutoAncestorPusher(TreeMatchContext& aTreeMatchContext
                                MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : mPushedAncestor(false)
      , mPushedStyleScope(false)
      , mTreeMatchContext(aTreeMatchContext)
      , mElement(nullptr)
    {
      MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    void PushAncestorAndStyleScope(mozilla::dom::Element* aElement) {
      MOZ_ASSERT(!mElement);
      if (aElement) {
        mElement = aElement;
        mPushedAncestor = true;
        mPushedStyleScope = true;
        mTreeMatchContext.mAncestorFilter.PushAncestor(aElement);
        mTreeMatchContext.PushStyleScope(aElement);
      }
    }

    void PushAncestorAndStyleScope(nsIContent* aContent) {
      if (aContent && aContent->IsElement()) {
        PushAncestorAndStyleScope(aContent->AsElement());
      }
    }

    void PushStyleScope(mozilla::dom::Element* aElement) {
      MOZ_ASSERT(!mElement);
      if (aElement) {
        mElement = aElement;
        mPushedStyleScope = true;
        mTreeMatchContext.PushStyleScope(aElement);
      }
    }

    void PushStyleScope(nsIContent* aContent) {
      if (aContent && aContent->IsElement()) {
        PushStyleScope(aContent->AsElement());
      }
    }

    ~AutoAncestorPusher() {
      if (mPushedAncestor) {
        mTreeMatchContext.mAncestorFilter.PopAncestor();
      }
      if (mPushedStyleScope) {
        mTreeMatchContext.PopStyleScope(mElement);
      }
    }

  private:
    bool mPushedAncestor;
    bool mPushedStyleScope;
    TreeMatchContext& mTreeMatchContext;
    mozilla::dom::Element* mElement;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
  };

  








  class MOZ_STACK_CLASS AutoParentDisplayBasedStyleFixupSkipper {
  public:
    explicit AutoParentDisplayBasedStyleFixupSkipper(TreeMatchContext& aTreeMatchContext,
                                                     bool aSkipParentDisplayBasedStyleFixup = true
                                                     MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : mAutoRestorer(aTreeMatchContext.mSkippingParentDisplayBasedStyleFixup)
    {
      MOZ_GUARD_OBJECT_NOTIFIER_INIT;
      if (aSkipParentDisplayBasedStyleFixup) {
        aTreeMatchContext.mSkippingParentDisplayBasedStyleFixup = true;
      }
    }

  private:
    mozilla::AutoRestore<bool> mAutoRestorer;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
  };

  
  
  
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

  
  
  
  
  bool mSkippingParentDisplayBasedStyleFixup;

  
  
  bool mForScopedStyle;

  enum MatchVisited {
    eNeverMatchVisited,
    eMatchVisitedDefault
  };

  
  
  nsAutoTArray<mozilla::dom::Element*, 1> mStyleScopes;

  
  mozilla::dom::Element* mCurrentStyleScope;

  
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
    , mIsHTMLDocument(aDocument->IsHTMLDocument())
    , mCompatMode(aDocument->GetCompatibilityMode())
    , mUsingPrivateBrowsing(false)
    , mSkippingParentDisplayBasedStyleFixup(false)
    , mForScopedStyle(false)
    , mCurrentStyleScope(nullptr)
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

struct MOZ_STACK_CLASS RuleProcessorData {
  RuleProcessorData(nsPresContext* aPresContext,
                    nsRuleWalker* aRuleWalker)
    : mPresContext(aPresContext),
      mRuleWalker(aRuleWalker),
      mScope(nullptr)
  {
    NS_PRECONDITION(mPresContext, "Must have prescontext");
  }

  nsPresContext* const mPresContext;
  nsRuleWalker* const mRuleWalker; 
  mozilla::dom::Element* mScope;
};

struct MOZ_STACK_CLASS ElementDependentRuleProcessorData :
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

struct MOZ_STACK_CLASS ElementRuleProcessorData :
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

struct MOZ_STACK_CLASS PseudoElementRuleProcessorData :
                          public ElementDependentRuleProcessorData {
  PseudoElementRuleProcessorData(nsPresContext* aPresContext,
                                 mozilla::dom::Element* aParentElement,
                                 nsRuleWalker* aRuleWalker,
                                 nsCSSPseudoElements::Type aPseudoType,
                                 TreeMatchContext& aTreeMatchContext,
                                 mozilla::dom::Element* aPseudoElement)
    : ElementDependentRuleProcessorData(aPresContext, aParentElement, aRuleWalker,
                                        aTreeMatchContext),
      mPseudoType(aPseudoType),
      mPseudoElement(aPseudoElement)
  {
    NS_PRECONDITION(aPseudoType <
                      nsCSSPseudoElements::ePseudo_PseudoElementCount,
                    "invalid aPseudoType value");
    NS_PRECONDITION(aTreeMatchContext.mForStyling, "Styling here!");
    NS_PRECONDITION(aRuleWalker, "Must have rule walker");
  }

  nsCSSPseudoElements::Type mPseudoType;
  mozilla::dom::Element* const mPseudoElement; 
};

struct MOZ_STACK_CLASS AnonBoxRuleProcessorData : public RuleProcessorData {
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
struct MOZ_STACK_CLASS XULTreeRuleProcessorData :
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

struct MOZ_STACK_CLASS StateRuleProcessorData :
                          public ElementDependentRuleProcessorData {
  StateRuleProcessorData(nsPresContext* aPresContext,
                         mozilla::dom::Element* aElement,
                         mozilla::EventStates aStateMask,
                         TreeMatchContext& aTreeMatchContext)
    : ElementDependentRuleProcessorData(aPresContext, aElement, nullptr,
                                        aTreeMatchContext),
      mStateMask(aStateMask)
  {
    NS_PRECONDITION(!aTreeMatchContext.mForStyling, "Not styling here!");
  }
  
  
  const mozilla::EventStates mStateMask;
};

struct MOZ_STACK_CLASS PseudoElementStateRuleProcessorData :
                          public StateRuleProcessorData {
  PseudoElementStateRuleProcessorData(nsPresContext* aPresContext,
                                      mozilla::dom::Element* aElement,
                                      mozilla::EventStates aStateMask,
                                      nsCSSPseudoElements::Type aPseudoType,
                                      TreeMatchContext& aTreeMatchContext,
                                      mozilla::dom::Element* aPseudoElement)
    : StateRuleProcessorData(aPresContext, aElement, aStateMask,
                             aTreeMatchContext),
      mPseudoType(aPseudoType),
      mPseudoElement(aPseudoElement)
  {
    NS_PRECONDITION(!aTreeMatchContext.mForStyling, "Not styling here!");
  }

  
  
  
  nsCSSPseudoElements::Type mPseudoType;
  mozilla::dom::Element* const mPseudoElement; 
};

struct MOZ_STACK_CLASS AttributeRuleProcessorData :
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
