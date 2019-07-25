






































#ifndef nsGenConList_h___
#define nsGenConList_h___

#include "nsIFrame.h"
#include "nsStyleStruct.h"
#include "prclist.h"
#include "nsIDOMCharacterData.h"
#include "nsCSSPseudoElements.h"

class nsGenConList;

struct nsGenConNode : public PRCList {
  
  
  
  
  nsIFrame* mPseudoFrame;

  
  
  
  const PRInt32 mContentIndex;

  
  
  nsCOMPtr<nsIDOMCharacterData> mText;

  nsGenConNode(PRInt32 aContentIndex)
    : mPseudoFrame(nsnull)
    , mContentIndex(aContentIndex)
  {
  }

  











  virtual PRBool InitTextFrame(nsGenConList* aList, nsIFrame* aPseudoFrame,
                               nsIFrame* aTextFrame)
  {
    mPseudoFrame = aPseudoFrame;
    CheckFrameAssertions();
    return PR_FALSE;
  }

  virtual ~nsGenConNode() {} 

protected:
  void CheckFrameAssertions() {
    NS_ASSERTION(mContentIndex <
                   PRInt32(mPseudoFrame->GetStyleContent()->ContentCount()),
                 "index out of range");
      
      

    NS_ASSERTION(mContentIndex < 0 ||
                 mPseudoFrame->GetStyleContext()->GetPseudo() ==
                   nsCSSPseudoElements::before ||
                 mPseudoFrame->GetStyleContext()->GetPseudo() ==
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
  PRUint32 mSize;
public:
  nsGenConList() : mFirstNode(nsnull), mSize(0) {}
  ~nsGenConList() { Clear(); }
  void Clear();
  static nsGenConNode* Next(nsGenConNode* aNode) {
    return static_cast<nsGenConNode*>(PR_NEXT_LINK(aNode));
  }
  static nsGenConNode* Prev(nsGenConNode* aNode) {
    return static_cast<nsGenConNode*>(PR_PREV_LINK(aNode));
  }
  void Insert(nsGenConNode* aNode);
  
  PRBool DestroyNodesFor(nsIFrame* aFrame); 

  
  static PRBool NodeAfter(const nsGenConNode* aNode1,
                          const nsGenConNode* aNode2);

  void Remove(nsGenConNode* aNode) { PR_REMOVE_LINK(aNode); mSize--; }
  PRBool IsLast(nsGenConNode* aNode) { return (Next(aNode) == mFirstNode); }
};

#endif 
