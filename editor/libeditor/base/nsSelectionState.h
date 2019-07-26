




#ifndef __selectionstate_h__
#define __selectionstate_h__

#include "nsCOMPtr.h"
#include "nsIDOMNode.h"
#include "nsINode.h"
#include "nsTArray.h"
#include "nscore.h"
#include "prtypes.h"

class nsCycleCollectionTraversalCallback;
class nsIDOMCharacterData;
class nsIDOMRange;
class nsISelection;
class nsRange;
namespace mozilla {
class Selection;
}








struct nsRangeStore 
{
  nsRangeStore();
  ~nsRangeStore();
  nsresult StoreRange(nsIDOMRange *aRange);
  nsresult GetRange(nsRange** outRange);

  NS_INLINE_DECL_REFCOUNTING(nsRangeStore)
        
  nsCOMPtr<nsIDOMNode> startNode;
  int32_t              startOffset;
  nsCOMPtr<nsIDOMNode> endNode;
  int32_t              endOffset;
  
};

class nsSelectionState
{
  public:
      
    nsSelectionState();
    ~nsSelectionState();

    void DoTraverse(nsCycleCollectionTraversalCallback &cb);
    void DoUnlink() { MakeEmpty(); }
  
    void     SaveSelection(mozilla::Selection *aSel);
    nsresult RestoreSelection(nsISelection *aSel);
    bool     IsCollapsed();
    bool     IsEqual(nsSelectionState *aSelState);
    void     MakeEmpty();
    bool     IsEmpty();
  protected:    
    nsTArray<nsRefPtr<nsRangeStore> > mArray;
    
    friend class nsRangeUpdater;
};

class nsRangeUpdater
{
  public:    
  
    nsRangeUpdater();
    ~nsRangeUpdater();
  
    void RegisterRangeItem(nsRangeStore *aRangeItem);
    void DropRangeItem(nsRangeStore *aRangeItem);
    nsresult RegisterSelectionState(nsSelectionState &aSelState);
    nsresult DropSelectionState(nsSelectionState &aSelState);
    
    
    
    
    
    
    nsresult SelAdjCreateNode(nsIDOMNode *aParent, int32_t aPosition);
    nsresult SelAdjInsertNode(nsIDOMNode *aParent, int32_t aPosition);
    void     SelAdjDeleteNode(nsIDOMNode *aNode);
    nsresult SelAdjSplitNode(nsIDOMNode *aOldRightNode, int32_t aOffset, nsIDOMNode *aNewLeftNode);
    nsresult SelAdjJoinNodes(nsIDOMNode *aLeftNode, 
                             nsIDOMNode *aRightNode, 
                             nsIDOMNode *aParent, 
                             int32_t aOffset,
                             int32_t aOldLeftNodeLength);
    nsresult SelAdjInsertText(nsIDOMCharacterData *aTextNode, int32_t aOffset, const nsAString &aString);
    nsresult SelAdjDeleteText(nsIDOMCharacterData *aTextNode, int32_t aOffset, int32_t aLength);
    
    
    nsresult WillReplaceContainer();
    nsresult DidReplaceContainer(nsIDOMNode *aOriginalNode, nsIDOMNode *aNewNode);
    nsresult WillRemoveContainer();
    nsresult DidRemoveContainer(nsIDOMNode *aNode, nsIDOMNode *aParent, int32_t aOffset, uint32_t aNodeOrigLen);
    nsresult WillInsertContainer();
    nsresult DidInsertContainer();
    nsresult WillMoveNode();
    nsresult DidMoveNode(nsIDOMNode *aOldParent, int32_t aOldOffset, nsIDOMNode *aNewParent, int32_t aNewOffset);
  protected:    
    nsTArray<nsRefPtr<nsRangeStore> > mArray;
    bool mLock;
};







class NS_STACK_CLASS nsAutoTrackDOMPoint
{
  private:
    nsRangeUpdater &mRU;
    nsCOMPtr<nsIDOMNode> *mNode;
    int32_t *mOffset;
    nsRefPtr<nsRangeStore> mRangeItem;
  public:
    nsAutoTrackDOMPoint(nsRangeUpdater &aRangeUpdater, nsCOMPtr<nsIDOMNode> *aNode, int32_t *aOffset) :
    mRU(aRangeUpdater)
    ,mNode(aNode)
    ,mOffset(aOffset)
    {
      mRangeItem = new nsRangeStore();
      mRangeItem->startNode = *mNode;
      mRangeItem->endNode = *mNode;
      mRangeItem->startOffset = *mOffset;
      mRangeItem->endOffset = *mOffset;
      mRU.RegisterRangeItem(mRangeItem);
    }
    
    ~nsAutoTrackDOMPoint()
    {
      mRU.DropRangeItem(mRangeItem);
      *mNode  = mRangeItem->startNode;
      *mOffset = mRangeItem->startOffset;
    }
};








class NS_STACK_CLASS nsAutoReplaceContainerSelNotify
{
  private:
    nsRangeUpdater &mRU;
    nsIDOMNode *mOriginalNode;
    nsIDOMNode *mNewNode;

  public:
    nsAutoReplaceContainerSelNotify(nsRangeUpdater &aRangeUpdater, nsIDOMNode *aOriginalNode, nsIDOMNode *aNewNode) :
    mRU(aRangeUpdater)
    ,mOriginalNode(aOriginalNode)
    ,mNewNode(aNewNode)
    {
      mRU.WillReplaceContainer();
    }
    
    ~nsAutoReplaceContainerSelNotify()
    {
      mRU.DidReplaceContainer(mOriginalNode, mNewNode);
    }
};







class NS_STACK_CLASS nsAutoRemoveContainerSelNotify
{
  private:
    nsRangeUpdater &mRU;
    nsIDOMNode *mNode;
    nsIDOMNode *mParent;
    int32_t    mOffset;
    uint32_t   mNodeOrigLen;

  public:
    nsAutoRemoveContainerSelNotify(nsRangeUpdater& aRangeUpdater,
                                   nsINode* aNode,
                                   nsINode* aParent,
                                   int32_t aOffset,
                                   uint32_t aNodeOrigLen)
      : mRU(aRangeUpdater)
      , mNode(aNode->AsDOMNode())
      , mParent(aParent->AsDOMNode())
      , mOffset(aOffset)
      , mNodeOrigLen(aNodeOrigLen)
    {
      mRU.WillRemoveContainer();
    }
    
    ~nsAutoRemoveContainerSelNotify()
    {
      mRU.DidRemoveContainer(mNode, mParent, mOffset, mNodeOrigLen);
    }
};






class NS_STACK_CLASS nsAutoInsertContainerSelNotify
{
  private:
    nsRangeUpdater &mRU;

  public:
    nsAutoInsertContainerSelNotify(nsRangeUpdater &aRangeUpdater) :
    mRU(aRangeUpdater)
    {
      mRU.WillInsertContainer();
    }
    
    ~nsAutoInsertContainerSelNotify()
    {
      mRU.DidInsertContainer();
    }
};







class NS_STACK_CLASS nsAutoMoveNodeSelNotify
{
  private:
    nsRangeUpdater &mRU;
    nsIDOMNode *mOldParent;
    nsIDOMNode *mNewParent;
    int32_t    mOldOffset;
    int32_t    mNewOffset;

  public:
    nsAutoMoveNodeSelNotify(nsRangeUpdater &aRangeUpdater, 
                            nsIDOMNode *aOldParent, 
                            int32_t aOldOffset, 
                            nsIDOMNode *aNewParent, 
                            int32_t aNewOffset) :
    mRU(aRangeUpdater)
    ,mOldParent(aOldParent)
    ,mNewParent(aNewParent)
    ,mOldOffset(aOldOffset)
    ,mNewOffset(aNewOffset)
    {
      mRU.WillMoveNode();
    }
    
    ~nsAutoMoveNodeSelNotify()
    {
      mRU.DidMoveNode(mOldParent, mOldOffset, mNewParent, mNewOffset);
    }
};

#endif


