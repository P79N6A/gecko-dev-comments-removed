




































#ifndef nsIEditorSupport_h__
#define nsIEditorSupport_h__
#include "nsISupports.h"

class nsIDOMNode;





#define NS_IEDITORSUPPORT_IID \
{/* 89b999b0-c529-11d2-86da-000064657374*/ \
0x89b999b0, 0xc529, 0x11d2, \
{0x86, 0xda, 0x0, 0x0, 0x64, 0x65, 0x73, 0x74} }




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
                           PRBool       aNodeToKeepIsFirst)=0;

  static nsresult GetChildOffset(nsIDOMNode *aChild, nsIDOMNode *aParent, PRInt32 &aOffset);
  


};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIEditorSupport, NS_IEDITORSUPPORT_IID)

#endif 

