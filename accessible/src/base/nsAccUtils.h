





































#ifndef nsAccUtils_h_
#define nsAccUtils_h_

#include "nsIAccessible.h"
#include "nsIAccessNode.h"
#include "nsIAccessibleDocument.h"
#include "nsIAccessibleRole.h"
#include "nsIAccessibleText.h"
#include "nsIAccessibleTable.h"
#include "nsARIAMap.h"

#include "nsIDOMNode.h"
#include "nsIPersistentProperties2.h"
#include "nsIContent.h"
#include "nsPoint.h"

class nsAccessNode;
class nsAccessible;
class nsHTMLTableAccessible;
class nsDocAccessible;
#ifdef MOZ_XUL
class nsXULTreeAccessible;
#endif

class nsAccUtils
{
public:
  






  static void GetAccAttr(nsIPersistentProperties *aAttributes,
                         nsIAtom *aAttrName,
                         nsAString& aAttrValue);

  






  static void SetAccAttr(nsIPersistentProperties *aAttributes,
                         nsIAtom *aAttrName,
                         const nsAString& aAttrValue);

  


  static void GetAccGroupAttrs(nsIPersistentProperties *aAttributes,
                               PRInt32 *aLevel,
                               PRInt32 *aPosInSet,
                               PRInt32 *aSetSize);

  


  static PRBool HasAccGroupAttrs(nsIPersistentProperties *aAttributes);

  


  static void SetAccGroupAttrs(nsIPersistentProperties *aAttributes,
                               PRInt32 aLevel,
                               PRInt32 aPosInSet,
                               PRInt32 aSetSize);

  






  static void SetAccAttrsForXULSelectControlItem(nsIDOMNode *aNode,
                                                 nsIPersistentProperties *aAttributes);

  






  static void SetAccAttrsForXULContainerItem(nsIDOMNode *aNode,
                                             nsIPersistentProperties *aAttributes);

  






  static void SetLiveContainerAttributes(nsIPersistentProperties *aAttributes,
                                         nsIContent *aStartContent,
                                         nsIContent *aTopContent);

  






  static PRBool HasDefinedARIAToken(nsIContent *aContent, nsIAtom *aAtom);

  


  static nsresult FireAccEvent(PRUint32 aEventType, nsIAccessible *aAccessible,
                               PRBool aIsAsynch = PR_FALSE);

  


  static PRBool HasAccessibleChildren(nsIDOMNode *aNode);

  





   static already_AddRefed<nsIAccessible>
     GetAncestorWithRole(nsIAccessible *aDescendant, PRUint32 aRole);

   







   static void
     GetARIATreeItemParent(nsIAccessible *aStartTreeItem,
                           nsIContent *aStartTreeItemContent,
                           nsIAccessible **aTreeItemParent);

  



  static PRBool IsARIASelected(nsIAccessible *aAccessible);

  







  static already_AddRefed<nsIAccessibleText>
    GetTextAccessibleFromSelection(nsISelection *aSelection,
                                   nsIDOMNode **aNode = nsnull);

  










  static nsresult ConvertToScreenCoords(PRInt32 aX, PRInt32 aY,
                                        PRUint32 aCoordinateType,
                                        nsIAccessNode *aAccessNode,
                                        nsIntPoint *aCoords);

  










  static nsresult ConvertScreenCoordsTo(PRInt32 *aX, PRInt32 *aY,
                                        PRUint32 aCoordinateType,
                                        nsIAccessNode *aAccessNode);

  




  static nsIntPoint GetScreenCoordsForWindow(nsIAccessNode *aAccessNode);

  




  static nsIntPoint GetScreenCoordsForParent(nsIAccessNode *aAccessNode);

  





  static nsRoleMapEntry* GetRoleMapEntry(nsIDOMNode *aNode);

  


  static PRUint32 Role(nsIAccessible *aAcc)
  {
    PRUint32 role = nsIAccessibleRole::ROLE_NOTHING;
    if (aAcc)
      aAcc->GetRole(&role);

    return role;
  }

  


  static PRUint32 RoleInternal(nsIAccessible *aAcc);

  


  static PRUint32 State(nsIAccessible *aAcc)
  {
    PRUint32 state = 0;
    if (aAcc)
      aAcc->GetState(&state, nsnull);

    return state;
  }

  






  static PRUint8 GetAttributeCharacteristics(nsIAtom* aAtom);

  








  static PRBool GetLiveAttrValue(PRUint32 aRule, nsAString& aValue);

  


  template<class DestinationType, class SourceType> static inline
    already_AddRefed<DestinationType> QueryObject(SourceType *aObject)
  {
    DestinationType* object = nsnull;
    if (aObject)
      CallQueryInterface(aObject, &object);

    return object;
  }
  template<class DestinationType, class SourceType> static inline
    already_AddRefed<DestinationType> QueryObject(nsCOMPtr<SourceType>& aObject)
  {
    DestinationType* object = nsnull;
    if (aObject)
      CallQueryInterface(aObject, &object);

    return object;
  }

  


  static already_AddRefed<nsAccessNode>
    QueryAccessNode(nsIAccessible *aAccessible)
  {
    nsAccessNode* accessNode = nsnull;
    if (aAccessible)
      CallQueryInterface(aAccessible, &accessNode);

    return accessNode;
  }

  


  static already_AddRefed<nsAccessNode>
    QueryAccessNode(nsIAccessNode *aAccessNode)
  {
    nsAccessNode* accessNode = nsnull;
    if (aAccessNode)
      CallQueryInterface(aAccessNode, &accessNode);
    
    return accessNode;
  }

  


  static already_AddRefed<nsAccessNode>
    QueryAccessNode(nsIAccessibleDocument *aAccessibleDocument)
  {
    nsAccessNode* accessNode = nsnull;
    if (aAccessibleDocument)
      CallQueryInterface(aAccessibleDocument, &accessNode);
    
    return accessNode;
  }

  


  static already_AddRefed<nsAccessible>
    QueryAccessible(nsIAccessible *aAccessible);

  


  static already_AddRefed<nsAccessible>
    QueryAccessible(nsIAccessNode *aAccessNode);

  


  static already_AddRefed<nsHTMLTableAccessible>
    QueryAccessibleTable(nsIAccessibleTable *aAccessibleTable);

  


  static already_AddRefed<nsDocAccessible>
    QueryAccessibleDocument(nsIAccessible *aAccessible);

  


  static already_AddRefed<nsDocAccessible>
    QueryAccessibleDocument(nsIAccessibleDocument *aAccessibleDocument);

#ifdef MOZ_XUL
  


  static already_AddRefed<nsXULTreeAccessible>
    QueryAccessibleTree(nsIAccessible *aAccessible);
#endif

#ifdef DEBUG_A11Y
  



  static PRBool IsTextInterfaceSupportCorrect(nsIAccessible *aAccessible);
#endif

  


  static PRBool IsText(nsIAccessible *aAcc)
  {
    PRUint32 role = Role(aAcc);
    return role == nsIAccessibleRole::ROLE_TEXT_LEAF ||
           role == nsIAccessibleRole::ROLE_STATICTEXT;
  }

  


  static PRInt32 TextLength(nsIAccessible *aAccessible);

  


  static PRBool IsEmbeddedObject(nsIAccessible *aAcc)
  {
    PRUint32 role = Role(aAcc);
    return role != nsIAccessibleRole::ROLE_TEXT_LEAF &&
           role != nsIAccessibleRole::ROLE_WHITESPACE &&
           role != nsIAccessibleRole::ROLE_STATICTEXT;
  }

  


  static inline PRBool IsLeaf(nsIAccessible *aAcc)
  {
    PRInt32 numChildren = 0;
    aAcc->GetChildCount(&numChildren);
    return numChildren == 0;
  }

  



  static PRBool MustPrune(nsIAccessible *aAccessible);

  



  static PRBool IsNodeRelevant(nsIDOMNode *aNode);

  


  static already_AddRefed<nsIAccessible> GetMultiSelectFor(nsIDOMNode *aNode);

  


  enum {
    
    eRowHeaderCells,
    
    eColumnHeaderCells
  };

  










  static nsresult GetHeaderCellsFor(nsIAccessibleTable *aTable,
                                    nsIAccessibleTableCell *aCell,
                                    PRInt32 aRowOrColHeaderCells,
                                    nsIArray **aCells);
};

#endif
