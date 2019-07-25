








































#ifndef nsRange_h___
#define nsRange_h___

#include "nsIDOMRange.h"
#include "nsIRangeUtils.h"
#include "nsCOMPtr.h"
#include "nsIDOMDocumentFragment.h"
#include "nsIContent.h"
#include "nsIDOMNode.h"
#include "prmon.h"
#include "nsStubMutationObserver.h"

class nsRange : public nsIDOMRange,
                public nsStubMutationObserver
{
public:
  nsRange()
    : mRoot(nsnull)
    , mStartOffset(0)
    , mEndOffset(0)
    , mIsPositioned(false)
    , mIsDetached(false)
    , mMaySpanAnonymousSubtrees(false)
    , mInSelection(false)
  {}
  virtual ~nsRange();

  static nsresult CreateRange(nsIDOMNode* aStartParent, PRInt32 aStartOffset,
                              nsIDOMNode* aEndParent, PRInt32 aEndOffset,
                              nsRange** aRange);
  static nsresult CreateRange(nsIDOMNode* aStartParent, PRInt32 aStartOffset,
                              nsIDOMNode* aEndParent, PRInt32 aEndOffset,
                              nsIDOMRange** aRange);

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsRange, nsIDOMRange)

  
  NS_DECL_NSIDOMRANGE
  
  nsINode* GetRoot() const
  {
    return mRoot;
  }

  nsINode* GetStartParent() const
  {
    return mStartParent;
  }

  nsINode* GetEndParent() const
  {
    return mEndParent;
  }

  PRInt32 StartOffset() const
  {
    return mStartOffset;
  }

  PRInt32 EndOffset() const
  {
    return mEndOffset;
  }
  
  bool IsPositioned() const
  {
    return mIsPositioned;
  }

  bool IsDetached() const
  {
    return mIsDetached;
  }
  
  bool Collapsed() const
  {
    return mIsPositioned && mStartParent == mEndParent &&
           mStartOffset == mEndOffset;
  }

  void SetMaySpanAnonymousSubtrees(bool aMaySpanAnonymousSubtrees)
  {
    mMaySpanAnonymousSubtrees = aMaySpanAnonymousSubtrees;
  }
  
  



  bool IsInSelection() const
  {
    return mInSelection;
  }

  


  void SetInSelection(bool aInSelection)
  {
    if (mInSelection == aInSelection || mIsDetached) {
      return;
    }
    mInSelection = aInSelection;
    nsINode* commonAncestor = GetCommonAncestor();
    NS_ASSERTION(commonAncestor, "unexpected disconnected nodes");
    if (mInSelection) {
      RegisterCommonAncestor(commonAncestor);
    } else {
      UnregisterCommonAncestor(commonAncestor);
    }
  }

  nsINode* GetCommonAncestor() const;
  void Reset();
  nsresult SetStart(nsINode* aParent, PRInt32 aOffset);
  nsresult SetEnd(nsINode* aParent, PRInt32 aOffset);
  nsresult CloneRange(nsRange** aNewRange) const;

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

  static nsresult CloneParentsBetween(nsIDOMNode *aAncestor,
                                      nsIDOMNode *aNode,
                                      nsIDOMNode **aClosestAncestor,
                                      nsIDOMNode **aFarthestAncestor);

public:







  static nsresult CompareNodeToRange(nsINode* aNode, nsRange* aRange,
                                     bool *outNodeBefore,
                                     bool *outNodeAfter);

  static bool IsNodeSelected(nsINode* aNode, PRUint32 aStartOffset,
                             PRUint32 aEndOffset);

  typedef nsTHashtable<nsPtrHashKey<nsRange> > RangeHashTable;
protected:
  void RegisterCommonAncestor(nsINode* aNode);
  void UnregisterCommonAncestor(nsINode* aNode);
  nsINode* IsValidBoundary(nsINode* aNode);

  
  
  
  
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
  
  nsCOMPtr<nsINode> mRoot;
  nsCOMPtr<nsINode> mStartParent;
  nsCOMPtr<nsINode> mEndParent;
  PRInt32 mStartOffset;
  PRInt32 mEndOffset;

  bool mIsPositioned;
  bool mIsDetached;
  bool mMaySpanAnonymousSubtrees;
  bool mInSelection;
};


class nsRangeUtils : public nsIRangeUtils
{
public:
  NS_DECL_ISUPPORTS

  
  NS_IMETHOD_(PRInt32) ComparePoints(nsIDOMNode* aParent1, PRInt32 aOffset1,
                                     nsIDOMNode* aParent2, PRInt32 aOffset2);
                               
  NS_IMETHOD CompareNodeToRange(nsIContent* aNode, 
                                nsRange* aRange,
                                bool *outNodeBefore,
                                bool *outNodeAfter);
};


nsresult NS_NewRangeUtils(nsIRangeUtils** aInstancePtrResult);

#endif 
