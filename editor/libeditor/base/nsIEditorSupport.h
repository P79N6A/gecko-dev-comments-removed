




#ifndef nsIEditorSupport_h__
#define nsIEditorSupport_h__
#include "nsISupports.h"

class nsIDOMNode;





#define NS_IEDITORSUPPORT_IID \
{/* c4cbcda8-58ec-4f03-9c99-5e46b6828b7a*/ \
0xc4cbcda8, 0x58ec, 0x4f03, \
{0x0c, 0x99, 0x5e, 0x46, 0xb6, 0x82, 0x8b, 0x7a} }




class nsIEditorSupport  : public nsISupports {

public:

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IEDITORSUPPORT_IID)

  






  NS_IMETHOD SplitNodeImpl(nsIDOMNode * aExistingRightNode,
                           PRInt32      aOffset,
                           nsIDOMNode * aNewLeftNode,
                           nsIDOMNode * aParent)=0;

  








  NS_IMETHOD JoinNodesImpl(nsIDOMNode *aNodeToKeep,
                           nsIDOMNode  *aNodeToJoin,
                           nsIDOMNode  *aParent,
                           bool         aNodeToKeepIsFirst)=0;

  static PRInt32 GetChildOffset(nsIDOMNode* aChild, nsIDOMNode* aParent);
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIEditorSupport, NS_IEDITORSUPPORT_IID)

#endif 

