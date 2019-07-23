










































#include "nsRuleNode.h"

class nsRuleWalker {
public:
  nsRuleNode* GetCurrentNode() { return mCurrent; }
  void SetCurrentNode(nsRuleNode* aNode) { mCurrent = aNode; }

  void Forward(nsIStyleRule* aRule) { 
    if (mCurrent) { 
      mCurrent = mCurrent->Transition(aRule, mLevel, mImportance);
      mCheckForImportantRules =
        mCheckForImportantRules && !aRule->GetImportantRule();
    }
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
  nsRuleWalker(nsRuleNode* aRoot) :mCurrent(aRoot), mRoot(aRoot) { MOZ_COUNT_CTOR(nsRuleWalker); }
  ~nsRuleWalker() { MOZ_COUNT_DTOR(nsRuleWalker); }
};
