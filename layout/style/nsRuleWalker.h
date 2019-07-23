










































#include "nsRuleNode.h"

class nsRuleWalker {
public:
  nsRuleNode* GetCurrentNode() { return mCurrent; }
  void SetCurrentNode(nsRuleNode* aNode) { mCurrent = aNode; }

  void Forward(nsIStyleRule* aRule) { 
    nsRuleNode* next;
    mCurrent->Transition(aRule, &next);
    mCurrent = next;
  }

  void Back() {
    if (mCurrent != mRoot)
      mCurrent = mCurrent->GetParent();
  }

  void Reset() { mCurrent = mRoot; }

  PRBool AtRoot() { return mCurrent == mRoot; }

private:
  nsRuleNode* mCurrent; 
  nsRuleNode* mRoot; 

public:
  nsRuleWalker(nsRuleNode* aRoot) :mCurrent(aRoot), mRoot(aRoot) { MOZ_COUNT_CTOR(nsRuleWalker); }
  ~nsRuleWalker() { MOZ_COUNT_DTOR(nsRuleWalker); }
};
