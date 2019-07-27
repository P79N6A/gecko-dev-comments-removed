









#ifndef nsRuleWalker_h_
#define nsRuleWalker_h_

#include "nsRuleNode.h"
#include "nsIStyleRule.h"
#include "StyleRule.h"
#include "nsQueryObject.h"

class nsRuleWalker {
public:
  nsRuleNode* CurrentNode() { return mCurrent; }
  void SetCurrentNode(nsRuleNode* aNode) {
    NS_ASSERTION(aNode, "Must have node here!");
    mCurrent = aNode;
  }

  nsPresContext* PresContext() const { return mRoot->PresContext(); }

protected:
  void DoForward(nsIStyleRule* aRule) {
    mCurrent = mCurrent->Transition(aRule, mLevel, mImportance);
    NS_POSTCONDITION(mCurrent, "Transition messed up");
  }

public:
  void Forward(nsIStyleRule* aRule) {
    NS_PRECONDITION(!nsRefPtr<mozilla::css::StyleRule>(do_QueryObject(aRule)),
                    "Calling the wrong Forward() overload");
    DoForward(aRule);
  }
  void Forward(mozilla::css::StyleRule* aRule) {
    DoForward(aRule);
    mCheckForImportantRules =
      mCheckForImportantRules && !aRule->GetImportantRule();
  }
  
  
  
  void ForwardOnPossiblyCSSRule(nsIStyleRule* aRule) {
    DoForward(aRule);
  }

  void Reset() { mCurrent = mRoot; }

  bool AtRoot() { return mCurrent == mRoot; }

  void SetLevel(uint8_t aLevel, bool aImportance,
                bool aCheckForImportantRules) {
    NS_ASSERTION(!aCheckForImportantRules || !aImportance,
                 "Shouldn't be checking for important rules while walking "
                 "important rules");
    mLevel = aLevel;
    mImportance = aImportance;
    mCheckForImportantRules = aCheckForImportantRules;
  }
  uint8_t GetLevel() const { return mLevel; }
  bool GetImportance() const { return mImportance; }
  bool GetCheckForImportantRules() const { return mCheckForImportantRules; }

  bool AuthorStyleDisabled() const { return mAuthorStyleDisabled; }

  
  
  enum VisitedHandlingType {
    
    eRelevantLinkUnvisited,
    
    
    eRelevantLinkVisited,
    
    
    
    eLinksVisitedOrUnvisited
  };

private:
  nsRuleNode* mCurrent; 
  nsRuleNode* mRoot; 
  uint8_t mLevel; 
  bool mImportance;
  bool mCheckForImportantRules; 
                                
                                
  bool mAuthorStyleDisabled;

public:
  nsRuleWalker(nsRuleNode* aRoot, bool aAuthorStyleDisabled)
    : mCurrent(aRoot)
    , mRoot(aRoot)
    , mAuthorStyleDisabled(aAuthorStyleDisabled)
  {
    NS_ASSERTION(mCurrent, "Caller screwed up and gave us null node");
    MOZ_COUNT_CTOR(nsRuleWalker);
  }
  ~nsRuleWalker() { MOZ_COUNT_DTOR(nsRuleWalker); }
};

#endif 
