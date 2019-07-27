



#ifndef __inDeepTreeWalker_h___
#define __inDeepTreeWalker_h___

#include "inIDeepTreeWalker.h"

#include "nsCOMPtr.h"
#include "nsIDOMNode.h"
#include "nsTArray.h"

class nsINodeList;
class inIDOMUtils;

class inDeepTreeWalker : public inIDeepTreeWalker
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_INIDEEPTREEWALKER

  inDeepTreeWalker();

  nsresult SetCurrentNode(nsIDOMNode* aCurrentNode,
                          nsINodeList* aSiblings);
protected:
  virtual ~inDeepTreeWalker();

  already_AddRefed<nsIDOMNode> GetParent();
  nsresult EdgeChild(nsIDOMNode** _retval, bool aReverse);

  bool mShowAnonymousContent;
  bool mShowSubDocuments;
  bool mShowDocumentsAsNodes;

  
  
  nsCOMPtr<nsIDOMNode> mRoot;
  nsCOMPtr<nsIDOMNode> mCurrentNode;
  nsCOMPtr<inIDOMUtils> mDOMUtils;

  
  
  
  
  
  
  
  
  
  nsCOMPtr<nsINodeList> mSiblings;

  
  int32_t mCurrentIndex;

  
  uint32_t mWhatToShow;
};


#define IN_DEEPTREEWALKER_CID \
{ 0xbfcb82c2, 0x5611, 0x4318, { 0x90, 0xd6, 0xba, 0xf4, 0xa7, 0x86, 0x42, 0x52 } }

#endif 
