








































#ifndef nsRange_h___
#define nsRange_h___

#include "nsIRange.h"
#include "nsIDOMRange.h"
#include "nsIRangeUtils.h"
#include "nsIDOMNSRange.h"
#include "nsCOMPtr.h"
#include "nsIDOMDocumentFragment.h"
#include "nsIContent.h"
#include "nsIDOMNode.h"
#include "prmon.h"
#include "nsStubMutationObserver.h"

class nsVoidArray;



class nsRangeUtils : public nsIRangeUtils
{
public:
  NS_DECL_ISUPPORTS

  
  NS_IMETHOD_(PRInt32) ComparePoints(nsIDOMNode* aParent1, PRInt32 aOffset1,
                                     nsIDOMNode* aParent2, PRInt32 aOffset2);
                               
  NS_IMETHOD CompareNodeToRange(nsIContent* aNode, 
                                nsIDOMRange* aRange,
                                PRBool *outNodeBefore,
                                PRBool *outNodeAfter);
};



class nsRange : public nsIRange,
                public nsIDOMRange,
                public nsIDOMNSRange,
                public nsStubMutationObserver
{
public:
  nsRange()
  {
  }
  virtual ~nsRange();

  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIDOMRANGE

  
  NS_DECL_NSIDOMNSRANGE
  
  
  virtual nsINode* GetCommonAncestor();
  virtual void Reset();
  
  
  virtual void CharacterDataChanged(nsIDocument* aDocument,
                                    nsIContent* aContent,
                                    CharacterDataChangeInfo* aChangeInfo);
  virtual void ContentInserted(nsIDocument* aDocument,
                               nsIContent* aContainer,
                               nsIContent* aChild,
                               PRInt32 aIndexInContainer);
  virtual void ContentRemoved(nsIDocument* aDocument,
                              nsIContent* aContainer,
                              nsIContent* aChild,
                              PRInt32 aIndexInContainer);
  virtual void NodeWillBeDestroyed(const nsINode* aNode);

private:
  
  nsRange(const nsRange&);
  nsRange& operator=(const nsRange&);

  nsINode* IsValidBoundary(nsINode* aNode);
 
public:







  static nsresult CompareNodeToRange(nsIContent* aNode, nsIDOMRange* aRange,
                                     PRBool *outNodeBefore,
                                     PRBool *outNodeAfter);
  static nsresult CompareNodeToRange(nsIContent* aNode, nsIRange* aRange,
                                     PRBool *outNodeBefore,
                                     PRBool *outNodeAfter);

protected:
  void DoSetRange(nsINode* aStartN, PRInt32 aStartOffset,
                  nsINode* aEndN, PRInt32 aEndOffset,
                  nsINode* aRoot);
};


nsresult NS_NewRange(nsIDOMRange** aInstancePtrResult);


nsresult NS_NewRangeUtils(nsIRangeUtils** aInstancePtrResult);

#endif 
