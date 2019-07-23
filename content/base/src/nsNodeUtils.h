




































#ifndef nsNodeUtils_h___
#define nsNodeUtils_h___

#include "nsDOMAttributeMap.h"
#include "nsIDOMNode.h"
#include "nsIMutationObserver.h"

struct JSContext;
struct JSObject;
class nsINode;
class nsNodeInfoManager;
class nsIVariant;
class nsIDOMUserDataHandler;
template<class E> class nsCOMArray;
class nsCycleCollectionTraversalCallback;
struct CharacterDataChangeInfo;

class nsNodeUtils
{
public:
  





  static void CharacterDataWillChange(nsIContent* aContent,
                                      CharacterDataChangeInfo* aInfo);

  





  static void CharacterDataChanged(nsIContent* aContent,
                                   CharacterDataChangeInfo* aInfo);

  








  static void AttributeChanged(nsIContent* aContent,
                               PRInt32 aNameSpaceID,
                               nsIAtom* aAttribute,
                               PRInt32 aModType,
                               PRUint32 aStateMask);

  





  static void ContentAppended(nsIContent* aContainer,
                              PRInt32 aNewIndexInContainer);

  






  static void ContentInserted(nsINode* aContainer,
                              nsIContent* aChild,
                              PRInt32 aIndexInContainer);
  






  static void ContentRemoved(nsINode* aContainer,
                             nsIContent* aChild,
                             PRInt32 aIndexInContainer);
  




  static void ParentChainChanged(nsIContent *aContent);

  



  static void LastRelease(nsINode* aNode);

  

















  static nsresult Clone(nsINode *aNode, PRBool aDeep,
                        nsNodeInfoManager *aNewNodeInfoManager,
                        nsCOMArray<nsINode> &aNodesWithProperties,
                        nsIDOMNode **aResult)
  {
    return CloneAndAdopt(aNode, PR_TRUE, aDeep, aNewNodeInfoManager, nsnull,
                         nsnull, nsnull, aNodesWithProperties, nsnull,
                         aResult);
  }

  



















  static nsresult Adopt(nsINode *aNode, nsNodeInfoManager *aNewNodeInfoManager,
                        JSContext *aCx, JSObject *aOldScope,
                        JSObject *aNewScope,
                        nsCOMArray<nsINode> &aNodesWithProperties)
  {
    nsCOMPtr<nsIDOMNode> dummy;
    return CloneAndAdopt(aNode, PR_FALSE, PR_TRUE, aNewNodeInfoManager, aCx,
                         aOldScope, aNewScope, aNodesWithProperties,
                         nsnull, getter_AddRefs(dummy));
  }

  














  static nsresult SetUserData(nsINode *aNode, const nsAString &aKey,
                              nsIVariant *aData,
                              nsIDOMUserDataHandler *aHandler,
                              nsIVariant **aResult);

  









  static nsresult GetUserData(nsINode *aNode, const nsAString &aKey,
                              nsIVariant **aResult);

  














  static nsresult CallUserDataHandlers(nsCOMArray<nsINode> &aNodesWithProperties,
                                       nsIDocument *aOwnerDocument,
                                       PRUint16 aOperation, PRBool aCloned);

  






  static void TraverseUserData(nsINode* aNode,
                               nsCycleCollectionTraversalCallback &aCb);

  







  static nsresult CloneNodeImpl(nsINode *aNode, PRBool aDeep,
                                nsIDOMNode **aResult);

  




  static void UnlinkUserData(nsINode *aNode);

private:
  friend PLDHashOperator
    AdoptFunc(nsAttrHashKey::KeyType aKey, nsIDOMNode *aData, void* aUserArg);

  






























  static nsresult CloneAndAdopt(nsINode *aNode, PRBool aClone, PRBool aDeep,
                                nsNodeInfoManager *aNewNodeInfoManager,
                                JSContext *aCx, JSObject *aOldScope,
                                JSObject *aNewScope,
                                nsCOMArray<nsINode> &aNodesWithProperties,
                                nsINode *aParent, nsIDOMNode **aResult);
};

#endif 
