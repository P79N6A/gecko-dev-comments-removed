






#ifndef nsGenConList_h___
#define nsGenConList_h___

#include "nsIFrame.h"
#include "nsStyleStruct.h"
#include "prclist.h"
#include "nsCSSPseudoElements.h"
#include "nsTextNode.h"

class nsGenConList;

struct nsGenConNode : public PRCList {
  
  
  
  
  nsIFrame* mPseudoFrame;

  
  
  
  const int32_t mContentIndex;

  
  
  nsRefPtr<nsTextNode> mText;

  explicit nsGenConNode(int32_t aContentIndex)
    : mPseudoFrame(nullptr)
    , mContentIndex(aContentIndex)
  {
  }

  











  virtual bool InitTextFrame(nsGenConList* aList, nsIFrame* aPseudoFrame,
                               nsIFrame* aTextFrame)
  {
    mPseudoFrame = aPseudoFrame;
    CheckFrameAssertions();
    return false;
  }

  virtual ~nsGenConNode() {} 

protected:
  void CheckFrameAssertions() {
    NS_ASSERTION(mContentIndex <
                   int32_t(mPseudoFrame->StyleContent()->ContentCount()),
                 "index out of range");
      
      

    NS_ASSERTION(mContentIndex < 0 ||
                 mPseudoFrame->StyleContext()->GetPseudo() ==
                   nsCSSPseudoElements::before ||
                 mPseudoFrame->StyleContext()->GetPseudo() ==
                   nsCSSPseudoElements::after,
                 "not :before/:after generated content and not counter change");
    NS_ASSERTION(mContentIndex < 0 ||
                 mPseudoFrame->GetStateBits() & NS_FRAME_GENERATED_CONTENT,
                 "not generated content and not counter change");
  }
};

class nsGenConList {
protected:
  nsGenConNode* mFirstNode;
  uint32_t mSize;
public:
  nsGenConList() : mFirstNode(nullptr), mSize(0) {}
  ~nsGenConList() { Clear(); }
  void Clear();
  static nsGenConNode* Next(nsGenConNode* aNode) {
    return static_cast<nsGenConNode*>(PR_NEXT_LINK(aNode));
  }
  static nsGenConNode* Prev(nsGenConNode* aNode) {
    return static_cast<nsGenConNode*>(PR_PREV_LINK(aNode));
  }
  void Insert(nsGenConNode* aNode);
  
  bool DestroyNodesFor(nsIFrame* aFrame); 

  
  static bool NodeAfter(const nsGenConNode* aNode1,
                          const nsGenConNode* aNode2);

  void Remove(nsGenConNode* aNode) { PR_REMOVE_LINK(aNode); mSize--; }
  bool IsLast(nsGenConNode* aNode) { return (Next(aNode) == mFirstNode); }
};

#endif 
