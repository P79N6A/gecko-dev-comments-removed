





































#ifndef nsCoreUtils_h_
#define nsCoreUtils_h_

#include "nsAccessibilityAtoms.h"

#include "nsIDOMDocumentXBL.h"
#include "nsIDOMNode.h"
#include "nsIContent.h"
#include "nsIBoxObject.h"
#include "nsITreeBoxObject.h"
#include "nsITreeColumns.h"

#include "nsIFrame.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDOMCSSStyleDeclaration.h"
#include "nsIDOMDOMStringList.h"
#include "nsIMutableArray.h"
#include "nsPoint.h"
#include "nsTArray.h"




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

  







  static nsIContent* GetDOMElementFor(nsIContent *aContent);

  


  static nsINode *GetDOMNodeFromDOMPoint(nsINode *aNode, PRUint32 aOffset);

  








  static nsIContent* GetRoleContent(nsINode *aNode);

  












   static PRBool IsAncestorOf(nsINode *aPossibleAncestorNode,
                              nsINode *aPossibleDescendantNode,
                              nsINode *aRootNode = nsnull);

  










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

  




  static nsIntPoint GetScreenCoordsForWindow(nsINode *aNode);

  


  static already_AddRefed<nsIDocShellTreeItem>
    GetDocShellTreeItemFor(nsINode *aNode);

  


  static PRBool IsRootDocument(nsIDocument *aDocument);

  


  static PRBool IsContentDocument(nsIDocument *aDocument);

  


  static PRBool IsErrorPage(nsIDocument *aDocument);

  





  static PRBool IsCorrectFrameType(nsIFrame* aFrame, nsIAtom* aAtom);

  


  static nsIPresShell *GetPresShellFor(nsINode *aNode)
  {
    nsIDocument *document = aNode->GetOwnerDoc();
    return document ? document->GetShell() : nsnull;
  }
  static already_AddRefed<nsIWeakReference> GetWeakShellFor(nsINode *aNode)
  {
    nsCOMPtr<nsIWeakReference> weakShell =
      do_GetWeakReference(GetPresShellFor(aNode));
    return weakShell.forget();
  }

  


  static already_AddRefed<nsIDOMNode>
    GetDOMNodeForContainer(nsIDocShellTreeItem *aContainer);

  





  static PRBool GetID(nsIContent *aContent, nsAString& aID);

  



  static PRBool GetUIntAttr(nsIContent *aContent, nsIAtom *aAttr,
                            PRInt32 *aUInt);

  





  static PRBool IsXLink(nsIContent *aContent);

  






  static void GetLanguageFor(nsIContent *aContent, nsIContent *aRootContent,
                             nsAString& aLanguage);

  


  static already_AddRefed<nsIDOMCSSStyleDeclaration>
    GetComputedStyleDeclaration(const nsAString& aPseudoElt,
                                nsIContent *aContent);

  


  static already_AddRefed<nsIBoxObject>
    GetTreeBodyBoxObject(nsITreeBoxObject *aTreeBoxObj);

  


  static already_AddRefed<nsITreeBoxObject>
    GetTreeBoxObject(nsIContent* aContent);

  


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
};





class nsAccessibleDOMStringList : public nsIDOMDOMStringList
{
public:
  nsAccessibleDOMStringList() {};
  virtual ~nsAccessibleDOMStringList() {};

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMDOMSTRINGLIST

  PRBool Add(const nsAString& aName) {
    return mNames.AppendElement(aName) != nsnull;
  }

private:
  nsTArray<nsString> mNames;
};






class IDRefsIterator
{
public:
  IDRefsIterator(nsIContent* aContent, nsIAtom* aIDRefsAttr);

  


  const nsDependentSubstring NextID();

  


  nsIContent* NextElem();

  


  nsIContent* GetElem(const nsDependentSubstring& aID);

private:
  nsString mIDs;
  nsAString::index_type mCurrIdx;

  nsIDocument* mDocument;
  nsCOMPtr<nsIDOMDocumentXBL> mXBLDocument;
  nsCOMPtr<nsIDOMElement> mBindingParent;
};

#endif

