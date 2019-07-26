








#ifndef nsRange_h___
#define nsRange_h___

#include "nsIDOMRange.h"
#include "nsCOMPtr.h"
#include "nsINode.h"
#include "nsIDocument.h"
#include "nsIDOMNode.h"
#include "prmon.h"
#include "nsStubMutationObserver.h"
#include "nsWrapperCache.h"
#include "mozilla/Attributes.h"

class nsClientRect;
class nsClientRectList;
class nsIDOMDocumentFragment;

namespace mozilla {
class ErrorResult;
namespace dom {
class DocumentFragment;
}
}

class nsRange MOZ_FINAL : public nsIDOMRange,
                          public nsStubMutationObserver,
                          public nsWrapperCache
{
  typedef mozilla::ErrorResult ErrorResult;

public:
  nsRange(nsINode* aNode)
    : mRoot(nullptr)
    , mStartOffset(0)
    , mEndOffset(0)
    , mIsPositioned(false)
    , mIsDetached(false)
    , mMaySpanAnonymousSubtrees(false)
    , mInSelection(false)
    , mStartOffsetWasIncremented(false)
    , mEndOffsetWasIncremented(false)
#ifdef DEBUG
    , mAssertNextInsertOrAppendIndex(-1)
    , mAssertNextInsertOrAppendNode(nullptr)
#endif
  {
    SetIsDOMBinding();
    MOZ_ASSERT(aNode, "range isn't in a document!");
    mOwner = aNode->OwnerDoc();
  }
  virtual ~nsRange();

  static nsresult CreateRange(nsIDOMNode* aStartParent, int32_t aStartOffset,
                              nsIDOMNode* aEndParent, int32_t aEndOffset,
                              nsRange** aRange);
  static nsresult CreateRange(nsIDOMNode* aStartParent, int32_t aStartOffset,
                              nsIDOMNode* aEndParent, int32_t aEndOffset,
                              nsIDOMRange** aRange);

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_AMBIGUOUS(nsRange, nsIDOMRange)

  
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

  int32_t StartOffset() const
  {
    return mStartOffset;
  }

  int32_t EndOffset() const
  {
    return mEndOffset;
  }
  
  bool IsPositioned() const
  {
    return mIsPositioned;
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
    if (mInSelection == aInSelection) {
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
  nsresult SetStart(nsINode* aParent, int32_t aOffset);
  nsresult SetEnd(nsINode* aParent, int32_t aOffset);
  already_AddRefed<nsRange> CloneRange() const;

  nsresult Set(nsINode* aStartParent, int32_t aStartOffset,
               nsINode* aEndParent, int32_t aEndOffset)
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

  
  bool Collapsed() const
  {
    return mIsPositioned && mStartParent == mEndParent &&
           mStartOffset == mEndOffset;
  }
  already_AddRefed<mozilla::dom::DocumentFragment>
  CreateContextualFragment(const nsAString& aString, ErrorResult& aError);
  already_AddRefed<mozilla::dom::DocumentFragment>
  CloneContents(ErrorResult& aErr);
  int16_t CompareBoundaryPoints(uint16_t aHow, nsRange& aOther,
                                ErrorResult& aErr);
  int16_t ComparePoint(nsINode& aParent, uint32_t aOffset, ErrorResult& aErr);
  void DeleteContents(ErrorResult& aRv);
  already_AddRefed<mozilla::dom::DocumentFragment>
    ExtractContents(ErrorResult& aErr);
  nsINode* GetCommonAncestorContainer(ErrorResult& aRv) const;
  nsINode* GetStartContainer(ErrorResult& aRv) const;
  uint32_t GetStartOffset(ErrorResult& aRv) const;
  nsINode* GetEndContainer(ErrorResult& aRv) const;
  uint32_t GetEndOffset(ErrorResult& aRv) const;
  void InsertNode(nsINode& aNode, ErrorResult& aErr);
  bool IntersectsNode(nsINode& aNode, ErrorResult& aRv);
  bool IsPointInRange(nsINode& aParent, uint32_t aOffset, ErrorResult& aErr);
  void SelectNode(nsINode& aNode, ErrorResult& aErr);
  void SelectNodeContents(nsINode& aNode, ErrorResult& aErr);
  void SetEnd(nsINode& aNode, uint32_t aOffset, ErrorResult& aErr);
  void SetEndAfter(nsINode& aNode, ErrorResult& aErr);
  void SetEndBefore(nsINode& aNode, ErrorResult& aErr);
  void SetStart(nsINode& aNode, uint32_t aOffset, ErrorResult& aErr);
  void SetStartAfter(nsINode& aNode, ErrorResult& aErr);
  void SetStartBefore(nsINode& aNode, ErrorResult& aErr);
  void SurroundContents(nsINode& aNode, ErrorResult& aErr);
  already_AddRefed<nsClientRect> GetBoundingClientRect();
  already_AddRefed<nsClientRectList> GetClientRects();

  nsINode* GetParentObject() const { return mOwner; }
  virtual JSObject* WrapObject(JSContext* cx, JSObject* scope) MOZ_OVERRIDE MOZ_FINAL;

private:
  
  nsRange(const nsRange&);
  nsRange& operator=(const nsRange&);

  





  nsresult CutContents(mozilla::dom::DocumentFragment** frag);

  static nsresult CloneParentsBetween(nsIDOMNode *aAncestor,
                                      nsIDOMNode *aNode,
                                      nsIDOMNode **aClosestAncestor,
                                      nsIDOMNode **aFarthestAncestor);

public:







  static nsresult CompareNodeToRange(nsINode* aNode, nsRange* aRange,
                                     bool *outNodeBefore,
                                     bool *outNodeAfter);

  static bool IsNodeSelected(nsINode* aNode, uint32_t aStartOffset,
                             uint32_t aEndOffset);

  typedef nsTHashtable<nsPtrHashKey<nsRange> > RangeHashTable;
protected:
  void RegisterCommonAncestor(nsINode* aNode);
  void UnregisterCommonAncestor(nsINode* aNode);
  nsINode* IsValidBoundary(nsINode* aNode);

  
  
  
  
  void DoSetRange(nsINode* aStartN, int32_t aStartOffset,
                  nsINode* aEndN, int32_t aEndOffset,
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

  nsCOMPtr<nsIDocument> mOwner;
  nsCOMPtr<nsINode> mRoot;
  nsCOMPtr<nsINode> mStartParent;
  nsCOMPtr<nsINode> mEndParent;
  int32_t mStartOffset;
  int32_t mEndOffset;

  bool mIsPositioned;
  bool mIsDetached;
  bool mMaySpanAnonymousSubtrees;
  bool mInSelection;
  bool mStartOffsetWasIncremented;
  bool mEndOffsetWasIncremented;
#ifdef DEBUG
  int32_t  mAssertNextInsertOrAppendIndex;
  nsINode* mAssertNextInsertOrAppendNode;
#endif
};

inline nsISupports*
ToCanonicalSupports(nsRange* aRange)
{
  return static_cast<nsIDOMRange*>(aRange);
}

inline nsISupports*
ToSupports(nsRange* aRange)
{
  return static_cast<nsIDOMRange*>(aRange);
}

#endif 
