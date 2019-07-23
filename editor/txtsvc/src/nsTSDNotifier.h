




































#ifndef nsTSDNotifier_h__
#define nsTSDNotifier_h__

#include "nsCOMPtr.h"
#include "nsIEditActionListener.h"

class nsTextServicesDocument;

class nsTSDNotifier : public nsIEditActionListener
{
private:

  nsTextServicesDocument *mDoc;

public:

  

  nsTSDNotifier(nsTextServicesDocument *aDoc);

  

  virtual ~nsTSDNotifier();

  
  NS_DECL_ISUPPORTS

  
  NS_IMETHOD WillInsertNode(nsIDOMNode *aNode,
                            nsIDOMNode *aParent,
                            PRInt32      aPosition);
  NS_IMETHOD DidInsertNode(nsIDOMNode *aNode,
                           nsIDOMNode *aParent,
                           PRInt32     aPosition,
                           nsresult    aResult);

  NS_IMETHOD WillDeleteNode(nsIDOMNode *aChild);
  NS_IMETHOD DidDeleteNode(nsIDOMNode *aChild, nsresult aResult);

  NS_IMETHOD WillSplitNode(nsIDOMNode * aExistingRightNode,
                           PRInt32      aOffset);
  NS_IMETHOD DidSplitNode(nsIDOMNode *aExistingRightNode,
                          PRInt32     aOffset,
                          nsIDOMNode *aNewLeftNode,
                          nsresult    aResult);

  NS_IMETHOD WillJoinNodes(nsIDOMNode  *aLeftNode,
                           nsIDOMNode  *aRightNode,
                           nsIDOMNode  *aParent);
  NS_IMETHOD DidJoinNodes(nsIDOMNode  *aLeftNode,
                          nsIDOMNode  *aRightNode,
                          nsIDOMNode  *aParent,
                          nsresult     aResult);
  
  NS_IMETHOD WillCreateNode(const nsAString& aTag, nsIDOMNode *aParent, PRInt32 aPosition);
  NS_IMETHOD DidCreateNode(const nsAString& aTag, nsIDOMNode *aNode, nsIDOMNode *aParent, PRInt32 aPosition, nsresult aResult);
  NS_IMETHOD WillInsertText(nsIDOMCharacterData *aTextNode, PRInt32 aOffset, const nsAString &aString);
  NS_IMETHOD DidInsertText(nsIDOMCharacterData *aTextNode, PRInt32 aOffset, const nsAString &aString, nsresult aResult);
  NS_IMETHOD WillDeleteText(nsIDOMCharacterData *aTextNode, PRInt32 aOffset, PRInt32 aLength);
  NS_IMETHOD DidDeleteText(nsIDOMCharacterData *aTextNode, PRInt32 aOffset, PRInt32 aLength, nsresult aResult);
  NS_IMETHOD WillDeleteSelection(nsISelection *aSelection);
  NS_IMETHOD DidDeleteSelection(nsISelection *aSelection);
};

#endif 
