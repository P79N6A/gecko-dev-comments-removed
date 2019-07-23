






































#ifndef nsGenConList_h___
#define nsGenConList_h___

#include "nsIContent.h"
#include "nsStyleStruct.h"
#include "nsStyleContext.h"
#include "prclist.h"
#include "nsIDOMCharacterData.h"
#include "nsCSSPseudoElements.h"

struct nsGenConNode : public PRCList {
  
  
  
  
  nsIContent* mParentContent;
  
  nsIAtom*    mPseudoType;

  
  
  
  const PRInt32 mContentIndex;

  
  
  nsCOMPtr<nsIDOMCharacterData> mText;

  static nsIAtom* ToGeneratedContentType(nsIAtom* aPseudoType)
  {
    if (aPseudoType == nsCSSPseudoElements::before ||
        aPseudoType == nsCSSPseudoElements::after)
      return aPseudoType;
    return nsnull;
  }
  
  nsGenConNode(nsIContent* aParentContent, nsStyleContext* aStyleContext,
               PRInt32 aContentIndex)
    : mParentContent(aParentContent)
    , mPseudoType(ToGeneratedContentType(aStyleContext->GetPseudoType()))
    , mContentIndex(aContentIndex)
  {
    NS_ASSERTION(aContentIndex <
                 PRInt32(aStyleContext->GetStyleContent()->ContentCount()),
                 "index out of range");
    
    
    NS_ASSERTION(aContentIndex < 0 || mPseudoType,
                 "not :before/:after generated content and not counter change");
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
    return static_cast<nsGenConNode*>(PR_NEXT_LINK(aNode));
  }
  static nsGenConNode* Prev(nsGenConNode* aNode) {
    return static_cast<nsGenConNode*>(PR_PREV_LINK(aNode));
  }
  void Insert(nsGenConNode* aNode);
  



  PRBool DestroyNodesFor(nsIContent* aParentContent, nsIAtom* aPseudo);

  
  static PRBool NodeAfter(const nsGenConNode* aNode1,
                          const nsGenConNode* aNode2);

  void Remove(nsGenConNode* aNode) { PR_REMOVE_LINK(aNode); mSize--; }
  PRBool IsLast(nsGenConNode* aNode) { return (Next(aNode) == mFirstNode); }
};

#endif 
