





































#ifndef nsCoreUtils_h_
#define nsCoreUtils_h_

#include "nsAccessibilityAtoms.h"

#include "nsIDOMNode.h"
#include "nsIContent.h"
#include "nsIBoxObject.h"
#include "nsITreeBoxObject.h"
#include "nsITreeColumns.h"

#include "nsIFrame.h"
#include "nsIDocShellTreeItem.h"
#include "nsIArray.h"
#include "nsIMutableArray.h"
#include "nsPoint.h"

class nsCoreUtils
{
public:
  



  static PRBool HasClickListener(nsIContent *aContent);

  








  static void DispatchClickEvent(nsITreeBoxObject *aTreeBoxObj,
                                 PRInt32 aRowIndex, nsITreeColumn *aColumn,
                                 const nsCString& aPseudoElt = EmptyCString());

  






  static PRBool DispatchMouseEvent(PRUint32 aEventType,
                                   nsIPresShell *aPresShell,
                                   nsIContent *aContent);

  










  static void DispatchMouseEvent(PRUint32 aEventType, PRInt32 aX, PRInt32 aY,
                                 nsIContent *aContent, nsIFrame *aFrame,
                                 nsIPresShell *aPresShell,
                                 nsIWidget *aRootWidget);

  





  static PRUint32 GetAccessKeyFor(nsIContent *aContent);

  








  static already_AddRefed<nsIDOMElement> GetDOMElementFor(nsIDOMNode *aNode);

  


  static already_AddRefed<nsIDOMNode> GetDOMNodeFromDOMPoint(nsIDOMNode *aNode,
                                                             PRUint32 aOffset);
  








  static nsIContent *GetRoleContent(nsIDOMNode *aDOMNode);

  










   static PRBool IsAncestorOf(nsINode *aPossibleAncestorNode,
                              nsINode *aPossibleDescendantNode);

  




   static PRBool AreSiblings(nsINode *aNode1, nsINode *aNode2);

  










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

  



  static PRBool GetUIntAttr(nsIContent *aContent, nsIAtom *aAttr,
                            PRInt32 *aUInt);

  





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

  


  static void
    GetTreeBoxObject(nsIDOMNode* aDOMNode, nsITreeBoxObject** aBoxObject);

  


  static already_AddRefed<nsITreeColumn>
    GetFirstSensibleColumn(nsITreeBoxObject *aTree);

  


  static already_AddRefed<nsITreeColumn>
    GetLastSensibleColumn(nsITreeBoxObject *aTree);

  


  static PRUint32 GetSensibleColumnCount(nsITreeBoxObject *aTree);

  


  static already_AddRefed<nsITreeColumn>
    GetSensibleColumnAt(nsITreeBoxObject *aTree, PRUint32 aIndex);

  


  static already_AddRefed<nsITreeColumn>
    GetNextSensibleColumn(nsITreeColumn *aColumn);

  


  static already_AddRefed<nsITreeColumn>
    GetPreviousSensibleColumn(nsITreeColumn *aColumn);

  


  static PRBool IsColumnHidden(nsITreeColumn *aColumn);

  


  static PRBool IsHTMLTableHeader(nsIContent *aContent)
  {
    return aContent->NodeInfo()->Equals(nsAccessibilityAtoms::th) ||
      aContent->HasAttr(kNameSpaceID_None, nsAccessibilityAtoms::scope);
  }

  






  static void GeneratePopupTree(nsIDOMNode *aNode, PRBool aIsAnon = PR_FALSE);
};




















#define NS_DECL_RUNNABLEMETHOD_HELPER(ClassType, Method)                       \
  void Revoke()                                                                \
  {                                                                            \
    NS_IF_RELEASE(mObj);                                                       \
  }                                                                            \
                                                                               \
protected:                                                                     \
  virtual ~nsRunnableMethod_##Method()                                         \
  {                                                                            \
    NS_IF_RELEASE(mObj);                                                       \
  }                                                                            \
                                                                               \
private:                                                                       \
  ClassType *mObj;                                                             \


#define NS_DECL_RUNNABLEMETHOD(ClassType, Method)                              \
class nsRunnableMethod_##Method : public nsRunnable                            \
{                                                                              \
public:                                                                        \
  nsRunnableMethod_##Method(ClassType *aObj) : mObj(aObj)                      \
  {                                                                            \
    NS_IF_ADDREF(mObj);                                                        \
  }                                                                            \
                                                                               \
  NS_IMETHODIMP Run()                                                          \
  {                                                                            \
    if (!mObj)                                                                 \
      return NS_OK;                                                            \
    (mObj-> Method)();                                                         \
    return NS_OK;                                                              \
  }                                                                            \
                                                                               \
  NS_DECL_RUNNABLEMETHOD_HELPER(ClassType, Method)                             \
                                                                               \
};

#define NS_DECL_RUNNABLEMETHOD_ARG1(ClassType, Method, Arg1Type)               \
class nsRunnableMethod_##Method : public nsRunnable                            \
{                                                                              \
public:                                                                        \
  nsRunnableMethod_##Method(ClassType *aObj, Arg1Type aArg1) :                 \
    mObj(aObj), mArg1(aArg1)                                                   \
  {                                                                            \
    NS_IF_ADDREF(mObj);                                                        \
  }                                                                            \
                                                                               \
  NS_IMETHODIMP Run()                                                          \
  {                                                                            \
    if (!mObj)                                                                 \
      return NS_OK;                                                            \
    (mObj-> Method)(mArg1);                                                    \
    return NS_OK;                                                              \
  }                                                                            \
                                                                               \
  NS_DECL_RUNNABLEMETHOD_HELPER(ClassType, Method)                             \
  Arg1Type mArg1;                                                              \
};

#define NS_DECL_RUNNABLEMETHOD_ARG2(ClassType, Method, Arg1Type, Arg2Type)     \
class nsRunnableMethod_##Method : public nsRunnable                            \
{                                                                              \
public:                                                                        \
                                                                               \
  nsRunnableMethod_##Method(ClassType *aObj,                                   \
                            Arg1Type aArg1, Arg2Type aArg2) :                  \
    mObj(aObj), mArg1(aArg1), mArg2(aArg2)                                     \
  {                                                                            \
    NS_IF_ADDREF(mObj);                                                        \
  }                                                                            \
                                                                               \
  NS_IMETHODIMP Run()                                                          \
  {                                                                            \
    if (!mObj)                                                                 \
      return NS_OK;                                                            \
    (mObj-> Method)(mArg1, mArg2);                                             \
    return NS_OK;                                                              \
  }                                                                            \
                                                                               \
  NS_DECL_RUNNABLEMETHOD_HELPER(ClassType, Method)                             \
  Arg1Type mArg1;                                                              \
  Arg2Type mArg2;                                                              \
};

#define NS_DISPATCH_RUNNABLEMETHOD(Method, Obj)                                \
{                                                                              \
  nsCOMPtr<nsIRunnable> runnable =                                             \
    new nsRunnableMethod_##Method(Obj);                                        \
  if (runnable)                                                                \
    NS_DispatchToMainThread(runnable);                                         \
}

#define NS_DISPATCH_RUNNABLEMETHOD_ARG1(Method, Obj, Arg1)                     \
{                                                                              \
  nsCOMPtr<nsIRunnable> runnable =                                             \
    new nsRunnableMethod_##Method(Obj, Arg1);                                  \
  if (runnable)                                                                \
    NS_DispatchToMainThread(runnable);                                         \
}

#define NS_DISPATCH_RUNNABLEMETHOD_ARG2(Method, Obj, Arg1, Arg2)               \
{                                                                              \
  nsCOMPtr<nsIRunnable> runnable =                                             \
    new nsRunnableMethod_##Method(Obj, Arg1, Arg2);                            \
  if (runnable)                                                                \
    NS_DispatchToMainThread(runnable);                                         \
}

#endif

