






































#ifndef nsGenConList_h___
#define nsGenConList_h___

#include "nsIFrame.h"
#include "nsStyleStruct.h"
#include "prclist.h"
#include "nsIDOMCharacterData.h"
#include "nsCSSPseudoElements.h"

struct nsGenConNode : public PRCList {
  
  
  
  
  nsIFrame* const mPseudoFrame;

  
  
  
  const PRInt32 mContentIndex;

  
  
  nsCOMPtr<nsIDOMCharacterData> mText;

  nsGenConNode(nsIFrame* aPseudoFrame, PRInt32 aContentIndex)
    : mPseudoFrame(aPseudoFrame)
    , mContentIndex(aContentIndex)
  {
    NS_ASSERTION(aContentIndex <
                   PRInt32(aPseudoFrame->GetStyleContent()->ContentCount()),
                 "index out of range");
    
    

    NS_ASSERTION(aContentIndex < 0 ||
                 aPseudoFrame->GetStyleContext()->GetPseudoType() ==
                   nsCSSPseudoElements::before ||
                 aPseudoFrame->GetStyleContext()->GetPseudoType() ==
                   nsCSSPseudoElements::after,
                 "not :before/:after generated content and not counter change");
    NS_ASSERTION(aContentIndex < 0 ||
                 aPseudoFrame->GetStateBits() & NS_FRAME_GENERATED_CONTENT,
                 "not generated content and not counter change");
  }

  virtual ~nsGenConNode() {} 
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
    return NS_STATIC_CAST(nsGenConNode*, PR_NEXT_LINK(aNode));
  }
  static nsGenConNode* Prev(nsGenConNode* aNode) {
    return NS_STATIC_CAST(nsGenConNode*, PR_PREV_LINK(aNode));
  }
  void Insert(nsGenConNode* aNode);
  
  PRBool DestroyNodesFor(nsIFrame* aFrame); 

  
  static PRBool NodeAfter(const nsGenConNode* aNode1,
                          const nsGenConNode* aNode2);

  void Remove(nsGenConNode* aNode) { PR_REMOVE_LINK(aNode); mSize--; }
  PRBool IsLast(nsGenConNode* aNode) { return (Next(aNode) == mFirstNode); }
};

#endif 
