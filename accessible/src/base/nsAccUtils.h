





































#ifndef nsAccUtils_h_
#define nsAccUtils_h_

#include "nsIAccessible.h"
#include "nsIAccessNode.h"
#include "nsIAccessibleDocument.h"
#include "nsIAccessibleRole.h"
#include "nsIAccessibleText.h"
#include "nsIAccessibleTable.h"

#include "nsARIAMap.h"
#include "nsAccessibilityService.h"
#include "nsCoreUtils.h"

#include "nsIContent.h"
#include "nsIDocShell.h"
#include "nsIDOMNode.h"
#include "nsIPersistentProperties2.h"
#include "nsIPresShell.h"
#include "nsPoint.h"

class nsAccessNode;
class nsAccessible;
class nsHyperTextAccessible;
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

  


  static void SetAccGroupAttrs(nsIPersistentProperties *aAttributes,
                               PRInt32 aLevel, PRInt32 aSetSize,
                               PRInt32 aPosInSet);

  


  static PRInt32 GetDefaultLevel(nsAccessible *aAcc);

  



  static PRInt32 GetARIAOrDefaultLevel(nsAccessible *aAccessible);

  



  static void GetPositionAndSizeForXULSelectControlItem(nsIContent *aContent,
                                                        PRInt32 *aPosInSet,
                                                        PRInt32 *aSetSize);

  



  static void GetPositionAndSizeForXULContainerItem(nsIContent *aContent,
                                                    PRInt32 *aPosInSet,
                                                    PRInt32 *aSetSize);

  


  static PRInt32 GetLevelForXULContainerItem(nsIContent *aContent);

  






  static void SetLiveContainerAttributes(nsIPersistentProperties *aAttributes,
                                         nsIContent *aStartContent,
                                         nsIContent *aTopContent);

  






  static PRBool HasDefinedARIAToken(nsIContent *aContent, nsIAtom *aAtom);

  


  static nsDocAccessible *GetDocAccessibleFor(nsIWeakReference *aWeakShell)
  {
    nsCOMPtr<nsIPresShell> presShell(do_QueryReferent(aWeakShell));
    return presShell ?
      GetAccService()->GetDocAccessible(presShell->GetDocument()) : nsnull;
  }

  


  static nsDocAccessible *GetDocAccessibleFor(nsINode *aNode)
  {
    nsIPresShell *presShell = nsCoreUtils::GetPresShellFor(aNode);
    return presShell ?
      GetAccService()->GetDocAccessible(presShell->GetDocument()) : nsnull;
  }

  


  static nsDocAccessible *GetDocAccessibleFor(nsIDocShellTreeItem *aContainer)
  {
    nsCOMPtr<nsIDocShell> docShell(do_QueryInterface(aContainer));
    nsCOMPtr<nsIPresShell> presShell;
    docShell->GetPresShell(getter_AddRefs(presShell));
    return presShell ?
      GetAccService()->GetDocAccessible(presShell->GetDocument()) : nsnull;
  }

  


  static PRBool HasAccessibleChildren(nsINode *aNode);

  







   static nsAccessible * GetAncestorWithRole(nsAccessible *aDescendant,
                                             PRUint32 aRole);

  





  static nsAccessible *GetSelectableContainer(nsAccessible *aAccessible,
                                              PRUint32 aState);

  


  static nsAccessible *GetMultiSelectableContainer(nsINode *aNode);

  



  static PRBool IsARIASelected(nsAccessible *aAccessible);

  







  static already_AddRefed<nsHyperTextAccessible>
    GetTextAccessibleFromSelection(nsISelection *aSelection,
                                   nsINode **aNode = nsnull);

  










  static nsresult ConvertToScreenCoords(PRInt32 aX, PRInt32 aY,
                                        PRUint32 aCoordinateType,
                                        nsAccessNode *aAccessNode,
                                        nsIntPoint *aCoords);

  










  static nsresult ConvertScreenCoordsTo(PRInt32 *aX, PRInt32 *aY,
                                        PRUint32 aCoordinateType,
                                        nsAccessNode *aAccessNode);

  




  static nsIntPoint GetScreenCoordsForWindow(nsAccessNode *aAccessNode);

  




  static nsIntPoint GetScreenCoordsForParent(nsAccessNode *aAccessNode);

  







  static nsRoleMapEntry *GetRoleMapEntry(nsINode *aNode);

  


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

  


  static PRUint32 ExtendedState(nsIAccessible *aAcc)
  {
    PRUint32 state = 0;
    PRUint32 extstate = 0;
    if (aAcc)
      aAcc->GetState(&state, &extstate);

    return extstate;
  }

  






  static PRUint8 GetAttributeCharacteristics(nsIAtom* aAtom);

  








  static PRBool GetLiveAttrValue(PRUint32 aRule, nsAString& aValue);

#ifdef DEBUG_A11Y
  



  static PRBool IsTextInterfaceSupportCorrect(nsAccessible *aAccessible);
#endif

  


  static PRBool IsText(nsIAccessible *aAcc)
  {
    PRUint32 role = Role(aAcc);
    return role == nsIAccessibleRole::ROLE_TEXT_LEAF ||
           role == nsIAccessibleRole::ROLE_STATICTEXT;
  }

  


  static PRUint32 TextLength(nsAccessible *aAccessible);

  


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
