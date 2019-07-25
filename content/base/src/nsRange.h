








































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
                                bool *outNodeBefore,
                                bool *outNodeAfter);
};



class nsRange : public nsIRange,
                public nsIDOMNSRange,
                public nsStubMutationObserver
{
public:
  nsRange(){}
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

  nsresult Set(nsINode* aStartParent, PRInt32 aStartOffset,
               nsINode* aEndParent, PRInt32 aEndOffset)
  {
    
    
    nsresult rv = SetStart(aStartParent, aStartOffset);
    NS_ENSURE_SUCCESS(rv, rv);

    return SetEnd(aEndParent, aEndOffset);
  }

  NS_IMETHOD GetUsedFontFaces(nsIDOMFontFaceList** aResult);

  
  NS_DECL_NSIMUTATIONOBSERVER_CHARACTERDATACHANGED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTINSERTED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTREMOVED
  NS_DECL_NSIMUTATIONOBSERVER_PARENTCHAINCHANGED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTAPPENDED

private:
  
  nsRange(const nsRange&);
  nsRange& operator=(const nsRange&);

  





  nsresult CutContents(nsIDOMDocumentFragment** frag);

  


  nsresult DoCloneRange(nsIRange** aNewRange) const;

  static nsresult CloneParentsBetween(nsIDOMNode *aAncestor,
                                      nsIDOMNode *aNode,
                                      nsIDOMNode **aClosestAncestor,
                                      nsIDOMNode **aFarthestAncestor);

public:







  static nsresult CompareNodeToRange(nsINode* aNode, nsIDOMRange* aRange,
                                     bool *outNodeBefore,
                                     bool *outNodeAfter);
  static nsresult CompareNodeToRange(nsINode* aNode, nsIRange* aRange,
                                     bool *outNodeBefore,
                                     bool *outNodeAfter);

  static bool IsNodeSelected(nsINode* aNode, PRUint32 aStartOffset,
                             PRUint32 aEndOffset);

protected:
  
  
  
  
  void DoSetRange(nsINode* aStartN, PRInt32 aStartOffset,
                  nsINode* aEndN, PRInt32 aEndOffset,
                  nsINode* aRoot, bool aNotInsertedYet = false);

  







  nsINode* GetRegisteredCommonAncestor();

  struct NS_STACK_CLASS AutoInvalidateSelection
  {
    AutoInvalidateSelection(nsRange* aRange) : mRange(aRange)
    {
#ifdef DEBUG
      mWasInSelection = mRange->IsInSelection();
#endif
      if (!mRange->IsInSelection() || mIsNested) {
        return;
      }
      mIsNested = true;
      NS_ASSERTION(!mRange->IsDetached(), "detached range in selection");
      mCommonAncestor = mRange->GetRegisteredCommonAncestor();
    }
    ~AutoInvalidateSelection();
    nsRange* mRange;
    nsRefPtr<nsINode> mCommonAncestor;
#ifdef DEBUG
    bool mWasInSelection;
#endif
    static bool mIsNested;
  };
  
};


nsresult NS_NewRange(nsIDOMRange** aInstancePtrResult);


nsresult NS_NewRangeUtils(nsIRangeUtils** aInstancePtrResult);

#endif 
