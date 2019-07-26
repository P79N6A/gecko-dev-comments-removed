




#ifndef nsCoreUtils_h_
#define nsCoreUtils_h_


#include "nsIContent.h"
#include "nsIBoxObject.h"
#include "nsIPresShell.h"

#include "nsIDOMDOMStringList.h"
#include "nsPoint.h"
#include "nsTArray.h"

class nsRange;
class nsIFrame;
class nsIDocShell;
class nsITreeColumn;
class nsITreeBoxObject;
class nsIWidget;




class nsCoreUtils
{
public:
  



  static bool HasClickListener(nsIContent *aContent);

  








  static void DispatchClickEvent(nsITreeBoxObject *aTreeBoxObj,
                                 int32_t aRowIndex, nsITreeColumn *aColumn,
                                 const nsCString& aPseudoElt = EmptyCString());

  






  static bool DispatchMouseEvent(uint32_t aEventType,
                                   nsIPresShell *aPresShell,
                                   nsIContent *aContent);

  










  static void DispatchMouseEvent(uint32_t aEventType, int32_t aX, int32_t aY,
                                 nsIContent *aContent, nsIFrame *aFrame,
                                 nsIPresShell *aPresShell,
                                 nsIWidget *aRootWidget);

  





  static uint32_t GetAccessKeyFor(nsIContent *aContent);

  







  static nsIContent* GetDOMElementFor(nsIContent *aContent);

  


  static nsINode *GetDOMNodeFromDOMPoint(nsINode *aNode, uint32_t aOffset);

  








  static nsIContent* GetRoleContent(nsINode *aNode);

  












   static bool IsAncestorOf(nsINode *aPossibleAncestorNode,
                              nsINode *aPossibleDescendantNode,
                              nsINode *aRootNode = nullptr);

  







  static nsresult ScrollSubstringTo(nsIFrame* aFrame, nsRange* aRange,
                                    uint32_t aScrollType);

  







  static nsresult ScrollSubstringTo(nsIFrame* aFrame, nsRange* aRange,
                                    nsIPresShell::ScrollAxis aVertical,
                                    nsIPresShell::ScrollAxis aHorizontal);

  







  static void ScrollFrameToPoint(nsIFrame *aScrollableFrame,
                                 nsIFrame *aFrame, const nsIntPoint& aPoint);

  



  static void ConvertScrollTypeToPercents(uint32_t aScrollType,
                                          nsIPresShell::ScrollAxis *aVertical,
                                          nsIPresShell::ScrollAxis *aHorizontal);

  





  static nsIntPoint GetScreenCoordsForWindow(nsINode *aNode);

  


  static already_AddRefed<nsIDocShell> GetDocShellFor(nsINode *aNode);

  


  static bool IsRootDocument(nsIDocument *aDocument);

  


  static bool IsContentDocument(nsIDocument *aDocument);

  


  static bool IsTabDocument(nsIDocument* aDocumentNode);

  


  static bool IsErrorPage(nsIDocument *aDocument);

  


  static nsIPresShell *GetPresShellFor(nsINode *aNode)
  {
    return aNode->OwnerDoc()->GetShell();
  }

  





  static bool GetID(nsIContent *aContent, nsAString& aID);

  



  static bool GetUIntAttr(nsIContent *aContent, nsIAtom *aAttr,
                            int32_t *aUInt);

  






  static void GetLanguageFor(nsIContent *aContent, nsIContent *aRootContent,
                             nsAString& aLanguage);

  


  static already_AddRefed<nsIBoxObject>
    GetTreeBodyBoxObject(nsITreeBoxObject *aTreeBoxObj);

  


  static already_AddRefed<nsITreeBoxObject>
    GetTreeBoxObject(nsIContent* aContent);

  


  static already_AddRefed<nsITreeColumn>
    GetFirstSensibleColumn(nsITreeBoxObject *aTree);

  


  static uint32_t GetSensibleColumnCount(nsITreeBoxObject *aTree);

  


  static already_AddRefed<nsITreeColumn>
    GetSensibleColumnAt(nsITreeBoxObject *aTree, uint32_t aIndex);

  


  static already_AddRefed<nsITreeColumn>
    GetNextSensibleColumn(nsITreeColumn *aColumn);

  


  static already_AddRefed<nsITreeColumn>
    GetPreviousSensibleColumn(nsITreeColumn *aColumn);

  


  static bool IsColumnHidden(nsITreeColumn *aColumn);

  


  static void ScrollTo(nsIPresShell* aPresShell, nsIContent* aContent,
                       uint32_t aScrollType);

  


  static bool IsHTMLTableHeader(nsIContent *aContent)
  {
    return aContent->NodeInfo()->Equals(nsGkAtoms::th) ||
      aContent->HasAttr(kNameSpaceID_None, nsGkAtoms::scope);
  }

};





class nsAccessibleDOMStringList : public nsIDOMDOMStringList
{
public:
  nsAccessibleDOMStringList() {}
  virtual ~nsAccessibleDOMStringList() {}

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMDOMSTRINGLIST

  bool Add(const nsAString& aName) {
    return mNames.AppendElement(aName) != nullptr;
  }

private:
  nsTArray<nsString> mNames;
};

#endif

