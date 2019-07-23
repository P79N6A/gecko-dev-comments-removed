

























































#include "nsCOMPtr.h"
#include "nsCOMArray.h"
#include "nsTArray.h"
#include "nsIContent.h"
#include "nsIXULTemplateResult.h"
#include "nsIXULTemplateQueryProcessor.h"
#include "nsIXULSortService.h"
#include "nsCycleCollectionParticipant.h"

enum nsSortState_direction {
  nsSortState_descending,
  nsSortState_ascending,
  nsSortState_natural
};
  

struct nsSortState
{
  PRBool initialized;
  PRBool invertSort;
  PRBool inbetweenSeparatorSort;
  PRBool sortStaticsLast;
  PRBool isContainerRDFSeq;

  nsSortState_direction direction;
  nsAutoString sort;
  nsCOMArray<nsIAtom> sortKeys;

  nsCOMPtr<nsIXULTemplateQueryProcessor> processor;
  nsCOMPtr<nsIContent> lastContainer;
  PRBool lastWasFirst, lastWasLast;

  nsSortState()
    : initialized(PR_FALSE)
  {
  }
  void Traverse(nsCycleCollectionTraversalCallback &cb) const
  {
    cb.NoteXPCOMChild(processor);
    cb.NoteXPCOMChild(lastContainer);
  }
};


struct contentSortInfo {
  nsCOMPtr<nsIContent> content;
  nsCOMPtr<nsIContent> parent;
  nsCOMPtr<nsIXULTemplateResult> result;
  void swap(contentSortInfo& other)
  {
    content.swap(other.content);
    parent.swap(other.parent);
    result.swap(other.result);
  }
};






class XULSortServiceImpl : public nsIXULSortService
{
protected:
  XULSortServiceImpl(void) {};
  virtual ~XULSortServiceImpl(void) {};

  friend nsresult NS_NewXULSortService(nsIXULSortService** mgr);

private:

public:
  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIXULSORTSERVICE

  


  void
  SetSortHints(nsIContent *aNode, nsSortState* aSortState);

  





  void
  SetSortColumnHints(nsIContent *content,
                     const nsAString &sortResource,
                     const nsAString &sortDirection);

  







  nsresult
  GetItemsToSort(nsIContent *aContainer,
                 nsSortState* aSortState,
                 nsTArray<contentSortInfo>& aSortItems);

  


  nsresult
  GetTemplateItemsToSort(nsIContent* aContainer,
                         nsIXULTemplateBuilder* aBuilder,
                         nsSortState* aSortState,
                         nsTArray<contentSortInfo>& aSortItems);

  


  nsresult
  SortContainer(nsIContent *aContainer, nsSortState* aSortState);

  



  nsresult
  InvertSortInfo(nsTArray<contentSortInfo>& aData,
                 PRInt32 aStart, PRInt32 aNumItems);

  









  static nsresult
  InitializeSortState(nsIContent* aRootElement,
                      nsIContent* aContainer,
                      const nsAString& aSortKey,
                      const nsAString& aSortDirection,
                      nsSortState* aSortState);
};
