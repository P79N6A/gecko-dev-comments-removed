










































#ifndef nsIStyleRuleProcessor_h___
#define nsIStyleRuleProcessor_h___

#include <stdio.h>

#include "nsISupports.h"
#include "nsPresContext.h" 
#include "nsILinkHandler.h"
#include "nsString.h"
#include "nsChangeHint.h"

class nsIStyleSheet;
class nsPresContext;
class nsIContent;
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

  void* operator new(size_t sz, nsPresContext* aContext) CPP_THROW_NEW {
    return aContext->AllocateFromShell(sz);
  }
  void Destroy(nsPresContext* aContext) {
    this->~RuleProcessorData();
    aContext->FreeToShell(sizeof(RuleProcessorData), this);
  }

  const nsString* GetLang();

  nsPresContext*   mPresContext;
  nsIContent*       mContent;       
  nsIContent*       mParentContent; 
  nsRuleWalker*     mRuleWalker; 
  nsIContent*       mScopedRoot;    
  
  nsIAtom*          mContentTag;    
  nsIAtom*          mContentID;     
  PRPackedBool      mIsHTMLContent; 
  PRPackedBool      mIsLink;        
  PRPackedBool      mHasAttributes; 
  nsCompatibility   mCompatMode;    
  nsLinkState       mLinkState;     
  PRInt32           mEventState;    
  PRInt32           mNameSpaceID;   
  const nsAttrValue* mClasses;      
  
  
  
  RuleProcessorData* mPreviousSiblingData;
  RuleProcessorData* mParentData;

protected:
  nsAutoString *mLanguage; 
};

struct ElementRuleProcessorData : public RuleProcessorData {
  ElementRuleProcessorData(nsPresContext* aPresContext,
                           nsIContent* aContent, 
                           nsRuleWalker* aRuleWalker)
  : RuleProcessorData(aPresContext,aContent,aRuleWalker)
  {
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
    NS_PRECONDITION(aContent, "null pointer");
  }
  const PRInt32 mStateMask; 
                            
};

struct AttributeRuleProcessorData : public RuleProcessorData {
  AttributeRuleProcessorData(nsPresContext* aPresContext,
                             nsIContent* aContent,
                             PRInt32 aNameSpaceID,
                             nsIAtom* aAttribute,
                             PRInt32 aModType,
                             PRUint32 aStateMask)
    : RuleProcessorData(aPresContext, aContent, nsnull),
      mNameSpaceID(aNameSpaceID),
      mAttribute(aAttribute),
      mModType(aModType),
      mStateMask(aStateMask)
  {
    NS_PRECONDITION(aContent, "null pointer");
  }
  
  PRInt32  mNameSpaceID;
  nsIAtom* mAttribute;

  PRInt32  mModType;     
  PRUint32 mStateMask;   
};



#define NS_ISTYLE_RULE_PROCESSOR_IID     \
{0x015575fe, 0x7b6c, 0x11d3, {0xba, 0x05, 0x00, 0x10, 0x83, 0x02, 0x3c, 0x2b}}









class nsIStyleRuleProcessor : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISTYLE_RULE_PROCESSOR_IID)

  
  
  typedef PRBool (* PR_CALLBACK EnumFunc)(nsIStyleRuleProcessor*, void*);

  





  NS_IMETHOD RulesMatching(ElementRuleProcessorData* aData) = 0;

  



  NS_IMETHOD RulesMatching(PseudoRuleProcessorData* aData) = 0;

  







  NS_IMETHOD HasStateDependentStyle(StateRuleProcessorData* aData,
                                    nsReStyleHint* aResult) = 0;

  





  NS_IMETHOD HasAttributeDependentStyle(AttributeRuleProcessorData* aData,
                                        nsReStyleHint* aResult) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIStyleRuleProcessor,
                              NS_ISTYLE_RULE_PROCESSOR_IID)

#endif 
