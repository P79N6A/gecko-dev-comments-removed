










































#include "nsRuleNode.h"

class nsRuleWalker {
public:
  nsRuleNode* CurrentNode() { return mCurrent; }
  void SetCurrentNode(nsRuleNode* aNode) {
    NS_ASSERTION(aNode, "Must have node here!");
    mCurrent = aNode;
  }

  void Forward(nsIStyleRule* aRule) { 
    mCurrent = mCurrent->Transition(aRule, mLevel, mImportance);
    mCheckForImportantRules =
      mCheckForImportantRules && !aRule->GetImportantRule();
    NS_POSTCONDITION(mCurrent, "Transition messed up");
  }

  void Reset() { mCurrent = mRoot; }

  PRBool AtRoot() { return mCurrent == mRoot; }

  void SetLevel(PRUint8 aLevel, PRBool aImportance,
                PRBool aCheckForImportantRules) {
    NS_ASSERTION(!aCheckForImportantRules || !aImportance,
                 "Shouldn't be checking for important rules while walking "
                 "important rules");
    mLevel = aLevel;
    mImportance = aImportance;
    mCheckForImportantRules = aCheckForImportantRules;
  }
  PRUint8 GetLevel() const { return mLevel; }
  PRBool GetImportance() const { return mImportance; }
  PRBool GetCheckForImportantRules() const { return mCheckForImportantRules; }

private:
  nsRuleNode* mCurrent; 
  nsRuleNode* mRoot; 
  PRUint8 mLevel; 
  PRPackedBool mImportance;
  PRPackedBool mCheckForImportantRules; 
                                        
                                        

public:
  nsRuleWalker(nsRuleNode* aRoot) :mCurrent(aRoot), mRoot(aRoot) {
    NS_ASSERTION(mCurrent, "Caller screwed up and gave us null node");
    MOZ_COUNT_CTOR(nsRuleWalker);
  }
  ~nsRuleWalker() { MOZ_COUNT_DTOR(nsRuleWalker); }
};
