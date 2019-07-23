





































#ifndef nsCoreUtils_h_
#define nsCoreUtils_h_

#include "nsAccessibilityAtoms.h"
#include "nsIAccessible.h"
#include "nsIAccessNode.h"
#include "nsARIAMap.h"

#include "nsIDOMNode.h"
#include "nsIPersistentProperties2.h"
#include "nsIContent.h"
#include "nsIFrame.h"
#include "nsIDocShellTreeItem.h"
#include "nsPoint.h"
#include "nsIAccessibleDocument.h"

class nsCoreUtils
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

  



  static PRBool HasListener(nsIContent *aContent, const nsAString& aEventType);

  






  static PRBool DispatchMouseEvent(PRUint32 aEventType,
                                   nsIPresShell *aPresShell,
                                   nsIContent *aContent);

  





  static PRUint32 GetAccessKeyFor(nsIContent *aContent);

  


  static nsresult FireAccEvent(PRUint32 aEventType, nsIAccessible *aAccessible,
                               PRBool aIsAsynch = PR_FALSE);

  








  static already_AddRefed<nsIDOMElement> GetDOMElementFor(nsIDOMNode *aNode);

  






   static PRBool IsAncestorOf(nsIDOMNode *aPossibleAncestorNode,
                              nsIDOMNode *aPossibleDescendantNode);

  



   static PRBool AreSiblings(nsIDOMNode *aDOMNode1,
                             nsIDOMNode *aDOMNode2);

  





   static already_AddRefed<nsIAccessible>
     GetAncestorWithRole(nsIAccessible *aDescendant, PRUint32 aRole);

   







   static void
     GetARIATreeItemParent(nsIAccessible *aStartTreeItem,
                           nsIContent *aStartTreeItemContent,
                           nsIAccessible **aTreeItemParent);

  










  static nsresult ScrollSubstringTo(nsIFrame *aFrame,
                                    nsIDOMNode *aStartNode, PRInt32 aStartIndex,
                                    nsIDOMNode *aEndNode, PRInt32 aEndIndex,
                                    PRUint32 aScrollType);

  










  static nsresult ScrollSubstringTo(nsIFrame *aFrame,
                                    nsIDOMNode *aStartNode, PRInt32 aStartIndex,
                                    nsIDOMNode *aEndNode, PRInt32 aEndIndex,
                                    PRInt16 aVPercent, PRInt16 aHPercent);

  







  static void ScrollFrameToPoint(nsIFrame *aScrollableFrame,
                                 nsIFrame *aFrame, const nsIntPoint& aPoint);

  



  static void ConvertScrollTypeToPercents(PRUint32 aScrollType,
                                          PRInt16 *aVPercent,
                                          PRInt16 *aHPercent);

  










  static nsresult ConvertToScreenCoords(PRInt32 aX, PRInt32 aY,
                                        PRUint32 aCoordinateType,
                                        nsIAccessNode *aAccessNode,
                                        nsIntPoint *aCoords);

  










  static nsresult ConvertScreenCoordsTo(PRInt32 *aX, PRInt32 *aY,
                                        PRUint32 aCoordinateType,
                                        nsIAccessNode *aAccessNode);

  




  static nsIntPoint GetScreenCoordsForWindow(nsIDOMNode *aNode);

  




  static nsIntPoint GetScreenCoordsForWindow(nsIAccessNode *aAccessNode);

  




  static nsIntPoint GetScreenCoordsForParent(nsIAccessNode *aAccessNode);

  


  static already_AddRefed<nsIDocShellTreeItem>
    GetDocShellTreeItemFor(nsIDOMNode *aNode);

  


  static nsIFrame* GetFrameFor(nsIDOMElement *aElm);

  





  static PRBool GetID(nsIContent *aContent, nsAString& aID);

  





  static PRBool IsXLink(nsIContent *aContent);

  






  static void GetLanguageFor(nsIContent *aContent, nsIContent *aRootContent,
                             nsAString& aLanguage);

  





   static nsRoleMapEntry* GetRoleMapEntry(nsIDOMNode *aNode);

  













  static nsIContent *FindNeighbourPointingToNode(nsIContent *aForNode,
                                                 nsIAtom **aRelationAttrs, 
                                                 PRUint32 aAttrNum,
                                                 nsIAtom *aTagName = nsnull,
                                                 PRUint32 aAncestorLevelsToSearch = 5);

  



  static nsIContent *FindNeighbourPointingToNode(nsIContent *aForNode,
                                                 nsIAtom *aRelationAttr, 
                                                 nsIAtom *aTagName = nsnull,
                                                 PRUint32 aAncestorLevelsToSearch = 5);

  














  static nsIContent *FindDescendantPointingToID(const nsString *aId,
                                                nsIContent *aLookContent,
                                                nsIAtom **aRelationAttrs,
                                                PRUint32 aAttrNum = 1,
                                                nsIContent *aExcludeContent = nsnull,
                                                nsIAtom *aTagType = nsAccessibilityAtoms::label);

  



  static nsIContent *FindDescendantPointingToID(const nsString *aId,
                                                nsIContent *aLookContent,
                                                nsIAtom *aRelationAttr,
                                                nsIContent *aExcludeContent = nsnull,
                                                nsIAtom *aTagType = nsAccessibilityAtoms::label);

  
  static nsIContent *FindDescendantPointingToIDImpl(nsCString& aIdWithSpaces,
                                                    nsIContent *aLookContent,
                                                    nsIAtom **aRelationAttrs,
                                                    PRUint32 aAttrNum = 1,
                                                    nsIContent *aExcludeContent = nsnull,
                                                    nsIAtom *aTagType = nsAccessibilityAtoms::label);
  
  
  static PRBool IsARIAPropForObjectAttr(nsIAtom *aAtom);


  





  static void GetLiveContainerAttributes(nsIPersistentProperties *aAttributes,
                                         nsIContent *aStartContent, nsIContent *aTopContent);
};

#endif

