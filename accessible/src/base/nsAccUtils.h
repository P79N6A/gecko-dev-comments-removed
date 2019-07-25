





































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

  







   static nsAccessible * GetAncestorWithRole(nsAccessible *aDescendant,
                                             PRUint32 aRole);

  





  static nsAccessible* GetSelectableContainer(nsAccessible* aAccessible,
                                              PRUint64 aState);

  


  static nsAccessible *GetMultiSelectableContainer(nsINode *aNode);

  



  static PRBool IsARIASelected(nsAccessible *aAccessible);

  






  static nsHyperTextAccessible*
    GetTextAccessibleFromSelection(nsISelection* aSelection);

  










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

  


  static inline PRUint64 To64State(PRUint32 aState1, PRUint32 aState2)
  {
    return static_cast<PRUint64>(aState1) +
        (static_cast<PRUint64>(aState2) << 31);
  }

  


  static inline void To32States(PRUint64 aState64,
                                PRUint32* aState1, PRUint32* aState2)
  {
    *aState1 = aState64 & 0x7fffffff;
    if (aState2)
      *aState2 = static_cast<PRUint32>(aState64 >> 31);
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
