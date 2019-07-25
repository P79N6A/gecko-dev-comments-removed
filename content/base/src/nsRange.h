








































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
                public nsIDOMNSRange,
                public nsStubMutationObserver
{
public:
  nsRange()
  {
  }
  virtual ~nsRange();

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsRange, nsIRange)

  
  NS_DECL_NSIDOMRANGE

  
  NS_DECL_NSIDOMNSRANGE
  
  
  virtual nsINode* GetCommonAncestor() const;
  virtual void Reset();
  virtual nsresult SetStart(nsINode* aParent, PRInt32 aOffset);
  virtual nsresult SetEnd(nsINode* aParent, PRInt32 aOffset);
  virtual nsresult CloneRange(nsIRange** aNewRange) const;

  
  NS_DECL_NSIMUTATIONOBSERVER_CHARACTERDATACHANGED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTINSERTED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTREMOVED
  NS_DECL_NSIMUTATIONOBSERVER_PARENTCHAINCHANGED

private:
  
  nsRange(const nsRange&);
  nsRange& operator=(const nsRange&);

  nsINode* IsValidBoundary(nsINode* aNode);
 
  





  nsresult CutContents(nsIDOMDocumentFragment** frag);

  


  nsresult DoCloneRange(nsIRange** aNewRange) const;

  static nsresult CloneParentsBetween(nsIDOMNode *aAncestor,
                                      nsIDOMNode *aNode,
                                      nsIDOMNode **aClosestAncestor,
                                      nsIDOMNode **aFarthestAncestor);

public:







  static nsresult CompareNodeToRange(nsINode* aNode, nsIDOMRange* aRange,
                                     PRBool *outNodeBefore,
                                     PRBool *outNodeAfter);
  static nsresult CompareNodeToRange(nsINode* aNode, nsIRange* aRange,
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
