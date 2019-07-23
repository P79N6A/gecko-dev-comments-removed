










































#include "nsRuleNode.h"

class nsRuleWalker {
public:
  nsRuleNode* GetCurrentNode() { return mCurrent; }
  void SetCurrentNode(nsRuleNode* aNode) { mCurrent = aNode; }

  void Forward(nsIStyleRule* aRule) { 
    if (mCurrent) { 
      mCurrent = mCurrent->Transition(aRule, mLevel, mImportance);
    }
  }

  void Reset() { mCurrent = mRoot; }

  PRBool AtRoot() { return mCurrent == mRoot; }

  void SetLevel(PRUint8 aLevel, PRBool aImportance) {
    mLevel = aLevel;
    mImportance = aImportance;
  }
  PRUint8 GetLevel() const { return mLevel; }
  PRBool GetImportance() const { return mImportance; }

private:
  nsRuleNode* mCurrent; 
  nsRuleNode* mRoot; 
  PRUint8 mLevel; 
  PRPackedBool mImportance;

public:
  nsRuleWalker(nsRuleNode* aRoot) :mCurrent(aRoot), mRoot(aRoot) { MOZ_COUNT_CTOR(nsRuleWalker); }
  ~nsRuleWalker() { MOZ_COUNT_DTOR(nsRuleWalker); }
};
