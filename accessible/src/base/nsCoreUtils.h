





































#ifndef nsCoreUtils_h_
#define nsCoreUtils_h_

#include "nsAccessibilityAtoms.h"

#include "nsIDOMNode.h"
#include "nsIContent.h"
#include "nsIBoxObject.h"
#include "nsITreeBoxObject.h"

#include "nsIFrame.h"
#include "nsIDocShellTreeItem.h"
#include "nsIArray.h"
#include "nsIMutableArray.h"
#include "nsPoint.h"

class nsCoreUtils
{
public:
  



  static PRBool HasListener(nsIContent *aContent, const nsAString& aEventType);

  






  static PRBool DispatchMouseEvent(PRUint32 aEventType,
                                   nsIPresShell *aPresShell,
                                   nsIContent *aContent);

  





  static PRUint32 GetAccessKeyFor(nsIContent *aContent);

  








  static already_AddRefed<nsIDOMElement> GetDOMElementFor(nsIDOMNode *aNode);

  


  static already_AddRefed<nsIDOMNode> GetDOMNodeFromDOMPoint(nsIDOMNode *aNode,
                                                             PRUint32 aOffset);
  








  static nsIContent *GetRoleContent(nsIDOMNode *aDOMNode);

  






   static PRBool IsAncestorOf(nsIDOMNode *aPossibleAncestorNode,
                              nsIDOMNode *aPossibleDescendantNode);

  



   static PRBool AreSiblings(nsIDOMNode *aDOMNode1,
                             nsIDOMNode *aDOMNode2);

  










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

  




  static nsIntPoint GetScreenCoordsForWindow(nsIDOMNode *aNode);

  


  static already_AddRefed<nsIDocShellTreeItem>
    GetDocShellTreeItemFor(nsIDOMNode *aNode);

  


  static nsIFrame* GetFrameFor(nsIDOMElement *aElm);

  





  static PRBool IsCorrectFrameType(nsIFrame* aFrame, nsIAtom* aAtom);

  


  static already_AddRefed<nsIPresShell> GetPresShellFor(nsIDOMNode *aNode);

  


  static already_AddRefed<nsIDOMNode>
    GetDOMNodeForContainer(nsIDocShellTreeItem *aContainer);

  





  static PRBool GetID(nsIContent *aContent, nsAString& aID);

  





  static PRBool IsXLink(nsIContent *aContent);

  






  static void GetLanguageFor(nsIContent *aContent, nsIContent *aRootContent,
                             nsAString& aLanguage);

  







  static void GetElementsByIDRefsAttr(nsIContent *aContent, nsIAtom *aAttr,
                                      nsIArray **aRefElements);

  







  static void GetElementsHavingIDRefsAttr(nsIContent *aRootContent,
                                          nsIContent *aContent,
                                          nsIAtom *aIDRefsAttr,
                                          nsIArray **aElements);

  


  static void GetElementsHavingIDRefsAttrImpl(nsIContent *aRootContent,
                                              nsCString& aIdWithSpaces,
                                              nsIAtom *aIDRefsAttr,
                                              nsIMutableArray *aElements);

  


  static void GetComputedStyleDeclaration(const nsAString& aPseudoElt,
                                          nsIDOMNode *aNode,
                                          nsIDOMCSSStyleDeclaration **aCssDecl);

  













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

  


  static nsIContent *GetLabelContent(nsIContent *aForNode);

  


  static nsIContent *GetHTMLLabelContent(nsIContent *aForNode);

  


  static already_AddRefed<nsIBoxObject>
    GetTreeBodyBoxObject(nsITreeBoxObject *aTreeBoxObj);

  


  static PRBool IsHTMLTableHeader(nsIContent *aContent)
  {
    return aContent->NodeInfo()->Equals(nsAccessibilityAtoms::th) ||
      aContent->HasAttr(kNameSpaceID_None, nsAccessibilityAtoms::scope);
  }
};

#endif

