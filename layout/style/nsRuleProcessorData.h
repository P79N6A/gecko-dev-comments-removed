










































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











struct TreeMatchContext {
  
  
  
  const PRBool mForStyling;

  
  
  PRBool mHaveRelevantLink;

  
  
  nsRuleWalker::VisitedHandlingType mVisitedHandling;

  
  nsIDocument* const mDocument;

  
  
  nsIContent* mScopedRoot;

  
  
  
  const PRPackedBool mIsHTMLDocument;

  
  
  const nsCompatibility mCompatMode;

  
  nsNthIndexCache mNthIndexCache;

  TreeMatchContext(PRBool aForStyling,
                   nsRuleWalker::VisitedHandlingType aVisitedHandling,
                   nsIDocument* aDocument)
    : mForStyling(aForStyling)
    , mHaveRelevantLink(PR_FALSE)
    , mVisitedHandling(aVisitedHandling)
    , mDocument(aDocument)
    , mScopedRoot(nsnull)
    , mIsHTMLDocument(aDocument->IsHTML())
    , mCompatMode(aDocument->GetCompatibilityMode())
  {
  }
};




struct RuleProcessorData : public TreeMatchContext {
  RuleProcessorData(nsPresContext* aPresContext,
                    mozilla::dom::Element* aElement, 
                    nsRuleWalker* aRuleWalker,
                    PRBool aForStyling);
  
  
  ~RuleProcessorData();

  
  static RuleProcessorData* Create(nsPresContext* aPresContext,
                                   mozilla::dom::Element* aElement, 
                                   nsRuleWalker* aRuleWalker)
  {
    
    
    
    if (NS_LIKELY(aPresContext)) {
      return new (aPresContext) RuleProcessorData(aPresContext, aElement,
                                                  aRuleWalker, PR_FALSE);
    }

    return new RuleProcessorData(aPresContext, aElement, aRuleWalker, PR_FALSE);
  }
  
  void Destroy() {
    nsPresContext * pc = mPresContext;
    if (NS_LIKELY(pc)) {
      this->~RuleProcessorData();
      pc->FreeToShell(sizeof(RuleProcessorData), this);
      return;
    }
    delete this;
  }

  
  void* operator new(size_t sz, RuleProcessorData* aSlot) CPP_THROW_NEW {
    return aSlot;
  }
private:
  void* operator new(size_t sz, nsPresContext* aContext) CPP_THROW_NEW {
    return aContext->AllocateFromShell(sz);
  }
  void* operator new(size_t sz) CPP_THROW_NEW {
    return ::operator new(sz);
  }
public:
  
  
  void ResetForVisitedMatching() {
    mHaveRelevantLink = PR_FALSE;
    mRuleWalker->ResetForVisitedMatching();
    mVisitedHandling = mRuleWalker->VisitedHandling();
  }
  
  nsEventStates DocumentState();

  nsPresContext*    mPresContext;
  mozilla::dom::Element* mElement;       
  nsIContent*       mParentContent; 
  nsRuleWalker*     mRuleWalker; 
  
  
  
  
  RuleProcessorData* mPreviousSiblingData;
  RuleProcessorData* mParentData;
};

struct ElementRuleProcessorData : public RuleProcessorData {
  ElementRuleProcessorData(nsPresContext* aPresContext,
                           mozilla::dom::Element* aElement, 
                           nsRuleWalker* aRuleWalker,
                           PRBool aForStyling)
  : RuleProcessorData(aPresContext, aElement, aRuleWalker, aForStyling)
  {
    NS_PRECONDITION(aPresContext, "null pointer");
    NS_PRECONDITION(aRuleWalker, "null pointer");
  }
};

struct PseudoElementRuleProcessorData : public RuleProcessorData {
  PseudoElementRuleProcessorData(nsPresContext* aPresContext,
                                 mozilla::dom::Element* aParentElement,
                                 nsRuleWalker* aRuleWalker,
                                 nsCSSPseudoElements::Type aPseudoType)
    : RuleProcessorData(aPresContext, aParentElement, aRuleWalker, PR_TRUE),
      mPseudoType(aPseudoType)
  {
    NS_PRECONDITION(aPresContext, "null pointer");
    NS_PRECONDITION(aPseudoType <
                      nsCSSPseudoElements::ePseudo_PseudoElementCount,
                    "null pointer");
    NS_PRECONDITION(aRuleWalker, "null pointer");
  }

  nsCSSPseudoElements::Type mPseudoType;
};

struct AnonBoxRuleProcessorData {
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
struct XULTreeRuleProcessorData : public RuleProcessorData {
  XULTreeRuleProcessorData(nsPresContext* aPresContext,
                           mozilla::dom::Element* aParentElement,
                           nsRuleWalker* aRuleWalker,
                           nsIAtom* aPseudoTag,
                           nsICSSPseudoComparator* aComparator)
    : RuleProcessorData(aPresContext, aParentElement, aRuleWalker, PR_TRUE),
      mPseudoTag(aPseudoTag),
      mComparator(aComparator)
  {
    NS_PRECONDITION(aPresContext, "null pointer");
    NS_PRECONDITION(aPseudoTag, "null pointer");
    NS_PRECONDITION(aRuleWalker, "null pointer");
    NS_PRECONDITION(aComparator, "must have a comparator");
  }

  nsIAtom*                 mPseudoTag;
  nsICSSPseudoComparator*  mComparator;
};
#endif

struct StateRuleProcessorData : public RuleProcessorData {
  StateRuleProcessorData(nsPresContext* aPresContext,
                         mozilla::dom::Element* aElement,
                         nsEventStates aStateMask)
    : RuleProcessorData(aPresContext, aElement, nsnull, PR_FALSE),
      mStateMask(aStateMask)
  {
    NS_PRECONDITION(aPresContext, "null pointer");
  }
  const nsEventStates mStateMask; 
                                  
};

struct AttributeRuleProcessorData : public RuleProcessorData {
  AttributeRuleProcessorData(nsPresContext* aPresContext,
                             mozilla::dom::Element* aElement,
                             nsIAtom* aAttribute,
                             PRInt32 aModType,
                             PRBool aAttrHasChanged)
    : RuleProcessorData(aPresContext, aElement, nsnull, PR_FALSE),
      mAttribute(aAttribute),
      mModType(aModType),
      mAttrHasChanged(aAttrHasChanged)
  {
    NS_PRECONDITION(aPresContext, "null pointer");
  }
  nsIAtom* mAttribute; 
  PRInt32 mModType;    
  PRBool mAttrHasChanged; 
};

#endif 
