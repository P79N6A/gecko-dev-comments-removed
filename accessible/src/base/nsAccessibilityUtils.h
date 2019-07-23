





































#ifndef nsAccessibilityUtils_h_
#define nsAccessibilityUtils_h_

#include "nsAccessibilityAtoms.h"
#include "nsIAccessible.h"
#include "nsARIAMap.h"

#include "nsIDOMNode.h"
#include "nsIPersistentProperties2.h"
#include "nsIContent.h"
#include "nsIFrame.h"
#include "nsIDocShellTreeItem.h"
#include "nsPoint.h"
#include "nsIAccessibleDocument.h"

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

  



  static PRBool HasListener(nsIContent *aContent, const nsAString& aEventType);

  





  static PRUint32 GetAccessKeyFor(nsIContent *aContent);

  


  static nsresult FireAccEvent(PRUint32 aEventType, nsIAccessible *aAccessible,
                               PRBool aIsAsynch = PR_FALSE);

  






   static PRBool IsAncestorOf(nsIDOMNode *aPossibleAncestorNode,
                              nsIDOMNode *aPossibleDescendantNode);

  





   static already_AddRefed<nsIAccessible>
     GetAncestorWithRole(nsIAccessible *aDescendant, PRUint32 aRole);

  










  static nsresult ScrollSubstringTo(nsIFrame *aFrame,
                                    nsIDOMNode *aStartNode, PRInt32 aStartIndex,
                                    nsIDOMNode *aEndNode, PRInt32 aEndIndex,
                                    PRUint32 aScrollType);

  



  static void ConvertScrollTypeToPercents(PRUint32 aScrollType,
                                          PRInt16 *aVPercent,
                                          PRInt16 *aHPercent);

  




  static nsIntPoint GetScreenCoordsForWindow(nsIDOMNode *aNode);

  


  static already_AddRefed<nsIDocShellTreeItem>
    GetDocShellTreeItemFor(nsIDOMNode *aNode);

  





  static PRBool GetID(nsIContent *aContent, nsAString& aID);

  





  static PRUint32 GetAriaPropTypes(nsIContent *aContent, nsIWeakReference *aWeakShell = nsnull);

  









  static PRBool HasAriaProperty(nsIContent *aContent, nsIWeakReference *aWeakShell,
                                EAriaProperty aProperty,
                                PRUint32 aCheckFlags = 0);

  










  static PRBool GetAriaProperty(nsIContent *aContent, nsIWeakReference *aWeakShell,
                                EAriaProperty aProperty, nsAString& aValue, 
                                PRUint32 aCheckFlags = 0);

  












  static nsIContent *FindNeighbourPointingToNode(nsIContent *aForNode,
                                                 EAriaProperty aAriaProperty,
                                                 nsIAtom *aTagName = nsnull,
                                                 nsIAtom *aRelationAttr = nsnull,
                                                 PRUint32 aAncestorLevelsToSearch = 5);

  













  static nsIContent *FindDescendantPointingToID(const nsString *aId,
                                                nsIContent *aLookContent,
                                                EAriaProperty aAriaProperty,
                                                nsIAtom *aRelationAttr = nsnull,
                                                nsIContent *aExcludeContent = nsnull,
                                                nsIAtom *aTagType = nsAccessibilityAtoms::label);

  
  static nsIContent *FindDescendantPointingToIDImpl(nsCString& aIdWithSpaces,
                                                    nsIContent *aLookContent,
                                                    EAriaProperty aAriaProperty,
                                                    PRUint32 aAriaPropTypes,
                                                    nsIAtom *aRelationAttr = nsnull,
                                                    nsIContent *aExcludeContent = nsnull,
                                                    nsIAtom *aTagType = nsAccessibilityAtoms::label);
};

#endif

