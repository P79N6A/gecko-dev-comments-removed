




#ifndef nsNodeUtils_h___
#define nsNodeUtils_h___

#include "nsIContent.h"          
#include "nsIMutationObserver.h" 

struct CharacterDataChangeInfo;
struct JSContext;
class JSObject;
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
                                  int32_t aNameSpaceID,
                                  nsIAtom* aAttribute,
                                  int32_t aModType);

  







  static void AttributeChanged(mozilla::dom::Element* aElement,
                               int32_t aNameSpaceID,
                               nsIAtom* aAttribute,
                               int32_t aModType);
  






  static void AttributeSetToCurrentValue(mozilla::dom::Element* aElement,
                                         int32_t aNameSpaceID,
                                         nsIAtom* aAttribute);

  






  static void ContentAppended(nsIContent* aContainer,
                              nsIContent* aFirstNewContent,
                              int32_t aNewIndexInContainer);

  






  static void ContentInserted(nsINode* aContainer,
                              nsIContent* aChild,
                              int32_t aIndexInContainer);
  






  static void ContentRemoved(nsINode* aContainer,
                             nsIContent* aChild,
                             int32_t aIndexInContainer,
                             nsIContent* aPreviousSibling);
  




  static inline void ParentChainChanged(nsIContent *aContent)
  {
    nsINode::nsSlots* slots = aContent->GetExistingSlots();
    if (slots && !slots->mMutationObservers.IsEmpty()) {
      NS_OBSERVER_ARRAY_NOTIFY_OBSERVERS(slots->mMutationObservers,
                                         nsIMutationObserver,
                                         ParentChainChanged,
                                         (aContent));
    }
  }

  



  static void LastRelease(nsINode* aNode);

  

















  static nsresult Clone(nsINode *aNode, bool aDeep,
                        nsNodeInfoManager *aNewNodeInfoManager,
                        nsCOMArray<nsINode> &aNodesWithProperties,
                        nsINode **aResult)
  {
    return CloneAndAdopt(aNode, true, aDeep, aNewNodeInfoManager, nullptr,
                         nullptr, aNodesWithProperties, nullptr, aResult);
  }

  


  static nsresult Clone(nsINode *aNode, bool aDeep, nsINode **aResult)
  {
    nsCOMArray<nsINode> dummyNodeWithProperties;
    return CloneAndAdopt(aNode, true, aDeep, nullptr, nullptr, nullptr,
                         dummyNodeWithProperties, aNode->GetParent(), aResult);
  }

  


















  static nsresult Adopt(nsINode *aNode, nsNodeInfoManager *aNewNodeInfoManager,
                        JSContext *aCx, JSObject *aNewScope,
                        nsCOMArray<nsINode> &aNodesWithProperties)
  {
    nsCOMPtr<nsINode> node;
    nsresult rv = CloneAndAdopt(aNode, false, true, aNewNodeInfoManager,
                                aCx, aNewScope, aNodesWithProperties,
                                nullptr, getter_AddRefs(node));

    nsMutationGuard::DidMutate();

    return rv;
  }

  














  static nsresult CallUserDataHandlers(nsCOMArray<nsINode> &aNodesWithProperties,
                                       nsIDocument *aOwnerDocument,
                                       uint16_t aOperation, bool aCloned);

  






  static void TraverseUserData(nsINode* aNode,
                               nsCycleCollectionTraversalCallback &aCb);

  








  static nsresult CloneNodeImpl(nsINode *aNode, bool aDeep,
                                bool aCallUserDataHandlers,
                                nsINode **aResult);

  




  static void UnlinkUserData(nsINode *aNode);

private:
  






























  static nsresult CloneAndAdopt(nsINode *aNode, bool aClone, bool aDeep,
                                nsNodeInfoManager *aNewNodeInfoManager,
                                JSContext *aCx, JSObject *aNewScope,
                                nsCOMArray<nsINode> &aNodesWithProperties,
                                nsINode *aParent, nsINode **aResult);
};

#endif 
