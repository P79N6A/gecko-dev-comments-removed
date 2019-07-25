




































#ifndef __inDeepTreeWalker_h___
#define __inDeepTreeWalker_h___

#include "inIDeepTreeWalker.h"

#include "nsCOMPtr.h"
#include "nsIDOMNode.h"
#include "nsTArray.h"

class inIDOMUtils;



struct DeepTreeStackItem
{
  nsCOMPtr<nsIDOMNode> node;
  nsCOMPtr<nsIDOMNodeList> kids;
  PRUint32 lastIndex; 
                      
};



class inDeepTreeWalker : public inIDeepTreeWalker
{
public:
	NS_DECL_ISUPPORTS
	NS_DECL_NSIDOMTREEWALKER
	NS_DECL_INIDEEPTREEWALKER

  inDeepTreeWalker();
  virtual ~inDeepTreeWalker();

protected:
  void PushNode(nsIDOMNode* aNode);

  PRBool mShowAnonymousContent;
  PRBool mShowSubDocuments;
  nsCOMPtr<nsIDOMNode> mRoot;
  nsCOMPtr<nsIDOMNode> mCurrentNode;
  PRUint32 mWhatToShow;
  
  nsAutoTArray<DeepTreeStackItem, 8> mStack;
  nsCOMPtr<inIDOMUtils> mDOMUtils;
};


#define IN_DEEPTREEWALKER_CID \
{ 0xbfcb82c2, 0x5611, 0x4318, { 0x90, 0xd6, 0xba, 0xf4, 0xa7, 0x86, 0x42, 0x52 } }

#endif 
