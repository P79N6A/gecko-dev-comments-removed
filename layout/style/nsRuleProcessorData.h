










































#ifndef nsRuleProcessorData_h_
#define nsRuleProcessorData_h_

#include "nsPresContext.h" 
#include "nsString.h"
#include "nsChangeHint.h"
#include "nsIContent.h"
#include "nsCSSPseudoElements.h"
#include "nsRuleWalker.h"
#include "nsNthIndexCache.h"

class nsIStyleSheet;
class nsIAtom;
class nsICSSPseudoComparator;
class nsAttrValue;













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

  
  
  
  const bool mForStyling;

 private:
  
  
  
  
  
  bool mHaveRelevantLink;

  
  
  nsRuleWalker::VisitedHandlingType mVisitedHandling;

 public:
  
  nsIDocument* const mDocument;

  
  
  nsIContent* mScopedRoot;

  
  
  
  const bool mIsHTMLDocument;

  
  
  const nsCompatibility mCompatMode;

  
  nsNthIndexCache mNthIndexCache;

  
  TreeMatchContext(bool aForStyling,
                   nsRuleWalker::VisitedHandlingType aVisitedHandling,
                   nsIDocument* aDocument)
    : mForStyling(aForStyling)
    , mHaveRelevantLink(false)
    , mVisitedHandling(aVisitedHandling)
    , mDocument(aDocument)
    , mScopedRoot(nsnull)
    , mIsHTMLDocument(aDocument->IsHTML())
    , mCompatMode(aDocument->GetCompatibilityMode())
  {
  }
};




struct NS_STACK_CLASS RuleProcessorData  {
  RuleProcessorData(nsPresContext* aPresContext,
                    mozilla::dom::Element* aElement, 
                    nsRuleWalker* aRuleWalker,
                    TreeMatchContext& aTreeMatchContext)
    : mPresContext(aPresContext)
    , mElement(aElement)
    , mRuleWalker(aRuleWalker)
    , mTreeMatchContext(aTreeMatchContext)
  {
    NS_ASSERTION(aElement, "null element leaked into SelectorMatches");
    NS_ASSERTION(aElement->OwnerDoc(), "Document-less node here?");
    NS_PRECONDITION(aTreeMatchContext.mForStyling == !!aRuleWalker,
                    "Should be styling if and only if we have a rule walker");
  }
  
  nsPresContext* const mPresContext;
  mozilla::dom::Element* const mElement; 
  nsRuleWalker* const mRuleWalker; 
  TreeMatchContext& mTreeMatchContext;
};

struct NS_STACK_CLASS ElementRuleProcessorData : public RuleProcessorData {
  ElementRuleProcessorData(nsPresContext* aPresContext,
                           mozilla::dom::Element* aElement, 
                           nsRuleWalker* aRuleWalker,
                           TreeMatchContext& aTreeMatchContext)
  : RuleProcessorData(aPresContext, aElement, aRuleWalker, aTreeMatchContext)
  {
    NS_PRECONDITION(aPresContext, "null pointer");
    NS_PRECONDITION(aRuleWalker, "null pointer");
    NS_PRECONDITION(aTreeMatchContext.mForStyling, "Styling here!");
  }
};

struct NS_STACK_CLASS PseudoElementRuleProcessorData : public RuleProcessorData {
  PseudoElementRuleProcessorData(nsPresContext* aPresContext,
                                 mozilla::dom::Element* aParentElement,
                                 nsRuleWalker* aRuleWalker,
                                 nsCSSPseudoElements::Type aPseudoType,
                                 TreeMatchContext& aTreeMatchContext)
    : RuleProcessorData(aPresContext, aParentElement, aRuleWalker,
                        aTreeMatchContext),
      mPseudoType(aPseudoType)
  {
    NS_PRECONDITION(aPresContext, "null pointer");
    NS_PRECONDITION(aPseudoType <
                      nsCSSPseudoElements::ePseudo_PseudoElementCount,
                    "null pointer");
    NS_PRECONDITION(aRuleWalker, "null pointer");
    NS_PRECONDITION(aTreeMatchContext.mForStyling, "Styling here!");
  }

  nsCSSPseudoElements::Type mPseudoType;
};

struct NS_STACK_CLASS AnonBoxRuleProcessorData {
  AnonBoxRuleProcessorData(nsPresContext* aPresContext,
                           nsIAtom* aPseudoTag,
                           nsRuleWalker* aRuleWalker)
    : mPresContext(aPresContext),
      mPseudoTag(aPseudoTag),
      mRuleWalker(aRuleWalker)
  {
    NS_PRECONDITION(mPresContext, "Must have prescontext");
    NS_PRECONDITION(aPseudoTag, "Must have pseudo tag");
    NS_PRECONDITION(aRuleWalker, "Must have rule walker");
  }

  nsPresContext* mPresContext;
  nsIAtom* mPseudoTag;
  nsRuleWalker* mRuleWalker;
};

#ifdef MOZ_XUL
struct NS_STACK_CLASS XULTreeRuleProcessorData : public RuleProcessorData {
  XULTreeRuleProcessorData(nsPresContext* aPresContext,
                           mozilla::dom::Element* aParentElement,
                           nsRuleWalker* aRuleWalker,
                           nsIAtom* aPseudoTag,
                           nsICSSPseudoComparator* aComparator,
                           TreeMatchContext& aTreeMatchContext)
    : RuleProcessorData(aPresContext, aParentElement, aRuleWalker,
                        aTreeMatchContext),
      mPseudoTag(aPseudoTag),
      mComparator(aComparator)
  {
    NS_PRECONDITION(aPresContext, "null pointer");
    NS_PRECONDITION(aPseudoTag, "null pointer");
    NS_PRECONDITION(aRuleWalker, "null pointer");
    NS_PRECONDITION(aComparator, "must have a comparator");
    NS_PRECONDITION(aTreeMatchContext.mForStyling, "Styling here!");
  }

  nsIAtom*                 mPseudoTag;
  nsICSSPseudoComparator*  mComparator;
};
#endif

struct NS_STACK_CLASS StateRuleProcessorData : public RuleProcessorData {
  StateRuleProcessorData(nsPresContext* aPresContext,
                         mozilla::dom::Element* aElement,
                         nsEventStates aStateMask,
                         TreeMatchContext& aTreeMatchContext)
    : RuleProcessorData(aPresContext, aElement, nsnull, aTreeMatchContext),
      mStateMask(aStateMask)
  {
    NS_PRECONDITION(aPresContext, "null pointer");
    NS_PRECONDITION(!aTreeMatchContext.mForStyling, "Not styling here!");
  }
  const nsEventStates mStateMask; 
                                  
};

struct NS_STACK_CLASS AttributeRuleProcessorData : public RuleProcessorData {
  AttributeRuleProcessorData(nsPresContext* aPresContext,
                             mozilla::dom::Element* aElement,
                             nsIAtom* aAttribute,
                             PRInt32 aModType,
                             bool aAttrHasChanged,
                             TreeMatchContext& aTreeMatchContext)
    : RuleProcessorData(aPresContext, aElement, nsnull, aTreeMatchContext),
      mAttribute(aAttribute),
      mModType(aModType),
      mAttrHasChanged(aAttrHasChanged)
  {
    NS_PRECONDITION(aPresContext, "null pointer");
    NS_PRECONDITION(!aTreeMatchContext.mForStyling, "Not styling here!");
  }
  nsIAtom* mAttribute; 
  PRInt32 mModType;    
  bool mAttrHasChanged; 
};

#endif 
