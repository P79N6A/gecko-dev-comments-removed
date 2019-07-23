










































#ifndef nsRuleProcessorData_h_
#define nsRuleProcessorData_h_

#include "nsPresContext.h" 
#include "nsString.h"
#include "nsChangeHint.h"
#include "nsIContent.h"

class nsIStyleSheet;
class nsPresContext;
class nsIAtom;
class nsICSSPseudoComparator;
class nsRuleWalker;
class nsAttrValue;




struct RuleProcessorData {
  RuleProcessorData(nsPresContext* aPresContext,
                    nsIContent* aContent, 
                    nsRuleWalker* aRuleWalker,
                    nsCompatibility* aCompat = nsnull);
  
  
  ~RuleProcessorData();

  
  static RuleProcessorData* Create(nsPresContext* aPresContext,
                                   nsIContent* aContent, 
                                   nsRuleWalker* aRuleWalker,
                                   nsCompatibility aCompat)
  {
    if (NS_LIKELY(aPresContext)) {
      return new (aPresContext) RuleProcessorData(aPresContext, aContent,
                                                  aRuleWalker, &aCompat);
    }

    return new RuleProcessorData(aPresContext, aContent, aRuleWalker,
                                 &aCompat);
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
  const nsString* GetLang();
  PRUint32 ContentState();
  PRBool IsLink();
  nsLinkState LinkState() {
    NS_ASSERTION(mGotLinkInfo && mIsLink, "Why am I being called?");
    return mLinkState;
  }

  
  
  
  
  
  
  PRInt32 GetNthIndex(PRBool aIsOfType, PRBool aIsFromEnd,
                      PRBool aCheckEdgeOnly);

  nsPresContext*    mPresContext;
  nsIContent*       mContent;       
  nsIContent*       mParentContent; 
  nsRuleWalker*     mRuleWalker; 
  nsIContent*       mScopedRoot;    
  
  nsIAtom*          mContentTag;    
  nsIAtom*          mContentID;     
  PRPackedBool      mIsHTMLContent; 
  PRPackedBool      mIsHTML;        
  PRPackedBool      mHasAttributes; 
  nsCompatibility   mCompatMode;    
  PRInt32           mNameSpaceID;   
  const nsAttrValue* mClasses;      
  
  
  
  RuleProcessorData* mPreviousSiblingData;
  RuleProcessorData* mParentData;

private:
  nsString *mLanguage; 

  
  
  
  
  
  
  PRInt32 mNthIndices[2][2];

  PRInt32 mContentState;  
  nsLinkState mLinkState; 
  PRPackedBool mIsLink;   
                          
  PRPackedBool mGotContentState;
  PRPackedBool mGotLinkInfo; 
                             
};

struct ElementRuleProcessorData : public RuleProcessorData {
  ElementRuleProcessorData(nsPresContext* aPresContext,
                           nsIContent* aContent, 
                           nsRuleWalker* aRuleWalker)
  : RuleProcessorData(aPresContext,aContent,aRuleWalker)
  {
    NS_PRECONDITION(aPresContext, "null pointer");
    NS_PRECONDITION(aContent, "null pointer");
    NS_PRECONDITION(aRuleWalker, "null pointer");
  }
};

struct PseudoRuleProcessorData : public RuleProcessorData {
  PseudoRuleProcessorData(nsPresContext* aPresContext,
                          nsIContent* aParentContent,
                          nsIAtom* aPseudoTag,
                          nsICSSPseudoComparator* aComparator,
                          nsRuleWalker* aRuleWalker)
  : RuleProcessorData(aPresContext, aParentContent, aRuleWalker)
  {
    NS_PRECONDITION(aPresContext, "null pointer");
    NS_PRECONDITION(aPseudoTag, "null pointer");
    NS_PRECONDITION(aRuleWalker, "null pointer");
    mPseudoTag = aPseudoTag;
    mComparator = aComparator;
  }

  nsIAtom*                 mPseudoTag;
  nsICSSPseudoComparator*  mComparator;
};

struct StateRuleProcessorData : public RuleProcessorData {
  StateRuleProcessorData(nsPresContext* aPresContext,
                         nsIContent* aContent,
                         PRInt32 aStateMask)
    : RuleProcessorData(aPresContext, aContent, nsnull),
      mStateMask(aStateMask)
  {
    NS_PRECONDITION(aPresContext, "null pointer");
    NS_PRECONDITION(aContent, "null pointer");
  }
  const PRInt32 mStateMask; 
                            
};

struct AttributeRuleProcessorData : public RuleProcessorData {
  AttributeRuleProcessorData(nsPresContext* aPresContext,
                             nsIContent* aContent,
                             nsIAtom* aAttribute,
                             PRInt32 aModType,
                             PRUint32 aStateMask)
    : RuleProcessorData(aPresContext, aContent, nsnull),
      mAttribute(aAttribute),
      mModType(aModType),
      mStateMask(aStateMask)
  {
    NS_PRECONDITION(aPresContext, "null pointer");
    NS_PRECONDITION(aContent, "null pointer");
  }
  nsIAtom* mAttribute; 
  PRInt32 mModType;    
  PRUint32 mStateMask; 
};

#endif 
