




































#ifndef nsNodeUtils_h___
#define nsNodeUtils_h___

#include "nsINode.h"

struct CharacterDataChangeInfo;
struct JSContext;
struct JSObject;
class nsIVariant;
class nsIDOMNode;
class nsIDOMUserDataHandler;
template<class E> class nsCOMArray;
class nsCycleCollectionTraversalCallback;

class nsNodeUtils
{
public:
  





  static void CharacterDataWillChange(nsIContent* aContent,
                                      CharacterDataChangeInfo* aInfo);

  





  static void CharacterDataChanged(nsIContent* aContent,
                                   CharacterDataChangeInfo* aInfo);

  







  static void AttributeWillChange(mozilla::dom::Element* aElement,
                                  PRInt32 aNameSpaceID,
                                  nsIAtom* aAttribute,
                                  PRInt32 aModType);

  







  static void AttributeChanged(mozilla::dom::Element* aElement,
                               PRInt32 aNameSpaceID,
                               nsIAtom* aAttribute,
                               PRInt32 aModType);

  






  static void ContentAppended(nsIContent* aContainer,
                              nsIContent* aFirstNewContent,
                              PRInt32 aNewIndexInContainer);

  






  static void ContentInserted(nsINode* aContainer,
                              nsIContent* aChild,
                              PRInt32 aIndexInContainer);
  






  static void ContentRemoved(nsINode* aContainer,
                             nsIContent* aChild,
                             PRInt32 aIndexInContainer,
                             nsIContent* aPreviousSibling);
  





  static void AttributeChildRemoved(nsINode* aAttribute, nsIContent* aChild);
  




  static void ParentChainChanged(nsIContent *aContent);

  



  static void LastRelease(nsINode* aNode);

  

















  static nsresult Clone(nsINode *aNode, bool aDeep,
                        nsNodeInfoManager *aNewNodeInfoManager,
                        nsCOMArray<nsINode> &aNodesWithProperties,
                        nsIDOMNode **aResult)
  {
    return CloneAndAdopt(aNode, PR_TRUE, aDeep, aNewNodeInfoManager, nsnull,
                         nsnull, aNodesWithProperties, aResult);
  }

  


















  static nsresult Adopt(nsINode *aNode, nsNodeInfoManager *aNewNodeInfoManager,
                        JSContext *aCx, JSObject *aNewScope,
                        nsCOMArray<nsINode> &aNodesWithProperties)
  {
    nsresult rv = CloneAndAdopt(aNode, PR_FALSE, PR_TRUE, aNewNodeInfoManager,
                                aCx, aNewScope, aNodesWithProperties,
                                nsnull);

    nsMutationGuard::DidMutate();

    return rv;
  }

  














  static nsresult CallUserDataHandlers(nsCOMArray<nsINode> &aNodesWithProperties,
                                       nsIDocument *aOwnerDocument,
                                       PRUint16 aOperation, bool aCloned);

  






  static void TraverseUserData(nsINode* aNode,
                               nsCycleCollectionTraversalCallback &aCb);

  








  static nsresult CloneNodeImpl(nsINode *aNode, bool aDeep,
                                bool aCallUserDataHandlers,
                                nsIDOMNode **aResult);

  




  static void UnlinkUserData(nsINode *aNode);

private:
  



























  static nsresult CloneAndAdopt(nsINode *aNode, bool aClone, bool aDeep,
                                nsNodeInfoManager *aNewNodeInfoManager,
                                JSContext *aCx, JSObject *aNewScope,
                                nsCOMArray<nsINode> &aNodesWithProperties,
                                nsIDOMNode **aResult)
  {
    NS_ASSERTION(!aClone == !aResult,
                 "aResult must be null when adopting and non-null when "
                 "cloning");

    nsCOMPtr<nsINode> clone;
    nsresult rv = CloneAndAdopt(aNode, aClone, aDeep, aNewNodeInfoManager,
                                aCx, aNewScope, aNodesWithProperties,
                                nsnull, getter_AddRefs(clone));
    NS_ENSURE_SUCCESS(rv, rv);

    return clone ? CallQueryInterface(clone, aResult) : NS_OK;
  }

  








  static nsresult CloneAndAdopt(nsINode *aNode, bool aClone, bool aDeep,
                                nsNodeInfoManager *aNewNodeInfoManager,
                                JSContext *aCx, JSObject *aNewScope,
                                nsCOMArray<nsINode> &aNodesWithProperties,
                                nsINode *aParent, nsINode **aResult);
};

#endif 
