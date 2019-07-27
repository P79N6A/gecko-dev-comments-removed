




#ifndef __selectionstate_h__
#define __selectionstate_h__

#include "nsCOMPtr.h"
#include "nsIDOMNode.h"
#include "nsINode.h"
#include "nsTArray.h"
#include "nscore.h"

class nsCycleCollectionTraversalCallback;
class nsIDOMCharacterData;
class nsIDOMRange;
class nsISelection;
class nsRange;
namespace mozilla {
namespace dom {
class Selection;
class Text;
}
}








struct nsRangeStore MOZ_FINAL
{
  nsRangeStore();

private:
  
  ~nsRangeStore();

public:
  void StoreRange(nsRange* aRange);
  already_AddRefed<nsRange> GetRange();

  NS_INLINE_DECL_REFCOUNTING(nsRangeStore)
        
  nsCOMPtr<nsINode> startNode;
  int32_t           startOffset;
  nsCOMPtr<nsINode> endNode;
  int32_t           endOffset;
};

class nsSelectionState
{
  public:
      
    nsSelectionState();
    ~nsSelectionState();

    void DoTraverse(nsCycleCollectionTraversalCallback &cb);
    void DoUnlink() { MakeEmpty(); }
  
    void     SaveSelection(mozilla::dom::Selection *aSel);
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
    
    
    
    
    
    
    nsresult SelAdjCreateNode(nsINode* aParent, int32_t aPosition);
    nsresult SelAdjCreateNode(nsIDOMNode *aParent, int32_t aPosition);
    nsresult SelAdjInsertNode(nsINode* aParent, int32_t aPosition);
    nsresult SelAdjInsertNode(nsIDOMNode *aParent, int32_t aPosition);
    void     SelAdjDeleteNode(nsINode* aNode);
    void     SelAdjDeleteNode(nsIDOMNode *aNode);
    nsresult SelAdjSplitNode(nsINode* aOldRightNode, int32_t aOffset,
                             nsINode* aNewLeftNode);
    nsresult SelAdjSplitNode(nsIDOMNode *aOldRightNode, int32_t aOffset, nsIDOMNode *aNewLeftNode);
    nsresult SelAdjJoinNodes(nsINode* aLeftNode,
                             nsINode* aRightNode,
                             nsINode* aParent,
                             int32_t aOffset,
                             int32_t aOldLeftNodeLength);
    nsresult SelAdjJoinNodes(nsIDOMNode *aLeftNode, 
                             nsIDOMNode *aRightNode, 
                             nsIDOMNode *aParent, 
                             int32_t aOffset,
                             int32_t aOldLeftNodeLength);
    void     SelAdjInsertText(mozilla::dom::Text& aTextNode, int32_t aOffset,
                              const nsAString &aString);
    nsresult SelAdjDeleteText(nsIContent* aTextNode, int32_t aOffset,
                              int32_t aLength);
    nsresult SelAdjDeleteText(nsIDOMCharacterData *aTextNode, int32_t aOffset, int32_t aLength);
    
    
    nsresult WillReplaceContainer();
    nsresult DidReplaceContainer(mozilla::dom::Element* aOriginalNode,
                                 mozilla::dom::Element* aNewNode);
    nsresult WillRemoveContainer();
    nsresult DidRemoveContainer(nsINode* aNode, nsINode* aParent,
                                int32_t aOffset, uint32_t aNodeOrigLen);
    nsresult DidRemoveContainer(nsIDOMNode *aNode, nsIDOMNode *aParent, int32_t aOffset, uint32_t aNodeOrigLen);
    nsresult WillInsertContainer();
    nsresult DidInsertContainer();
    void WillMoveNode();
    void DidMoveNode(nsINode* aOldParent, int32_t aOldOffset,
                     nsINode* aNewParent, int32_t aNewOffset);
  protected:    
    nsTArray<nsRefPtr<nsRangeStore> > mArray;
    bool mLock;
};







class MOZ_STACK_CLASS nsAutoTrackDOMPoint
{
  private:
    nsRangeUpdater &mRU;
    
    nsCOMPtr<nsINode>* mNode;
    nsCOMPtr<nsIDOMNode>* mDOMNode;
    int32_t* mOffset;
    nsRefPtr<nsRangeStore> mRangeItem;
  public:
    nsAutoTrackDOMPoint(nsRangeUpdater &aRangeUpdater,
                        nsCOMPtr<nsINode>* aNode, int32_t* aOffset)
      : mRU(aRangeUpdater)
      , mNode(aNode)
      , mDOMNode(nullptr)
      , mOffset(aOffset)
    {
      mRangeItem = new nsRangeStore();
      mRangeItem->startNode = *mNode;
      mRangeItem->endNode = *mNode;
      mRangeItem->startOffset = *mOffset;
      mRangeItem->endOffset = *mOffset;
      mRU.RegisterRangeItem(mRangeItem);
    }

    nsAutoTrackDOMPoint(nsRangeUpdater &aRangeUpdater,
                        nsCOMPtr<nsIDOMNode>* aNode, int32_t* aOffset)
      : mRU(aRangeUpdater)
      , mNode(nullptr)
      , mDOMNode(aNode)
      , mOffset(aOffset)
    {
      mRangeItem = new nsRangeStore();
      mRangeItem->startNode = do_QueryInterface(*mDOMNode);
      mRangeItem->endNode = do_QueryInterface(*mDOMNode);
      mRangeItem->startOffset = *mOffset;
      mRangeItem->endOffset = *mOffset;
      mRU.RegisterRangeItem(mRangeItem);
    }

    ~nsAutoTrackDOMPoint()
    {
      mRU.DropRangeItem(mRangeItem);
      if (mNode) {
        *mNode = mRangeItem->startNode;
      } else {
        *mDOMNode = GetAsDOMNode(mRangeItem->startNode);
      }
      *mOffset = mRangeItem->startOffset;
    }
};








namespace mozilla {
namespace dom {
class MOZ_STACK_CLASS AutoReplaceContainerSelNotify
{
  private:
    nsRangeUpdater &mRU;
    Element* mOriginalElement;
    Element* mNewElement;

  public:
    AutoReplaceContainerSelNotify(nsRangeUpdater& aRangeUpdater,
                                  Element* aOriginalElement,
                                  Element* aNewElement)
      : mRU(aRangeUpdater)
      , mOriginalElement(aOriginalElement)
      , mNewElement(aNewElement)
    {
      mRU.WillReplaceContainer();
    }

    ~AutoReplaceContainerSelNotify()
    {
      mRU.DidReplaceContainer(mOriginalElement, mNewElement);
    }
};
}
}







class MOZ_STACK_CLASS nsAutoRemoveContainerSelNotify
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






class MOZ_STACK_CLASS nsAutoInsertContainerSelNotify
{
  private:
    nsRangeUpdater &mRU;

  public:
    explicit nsAutoInsertContainerSelNotify(nsRangeUpdater &aRangeUpdater) :
    mRU(aRangeUpdater)
    {
      mRU.WillInsertContainer();
    }
    
    ~nsAutoInsertContainerSelNotify()
    {
      mRU.DidInsertContainer();
    }
};







class MOZ_STACK_CLASS nsAutoMoveNodeSelNotify
{
  private:
    nsRangeUpdater &mRU;
    nsINode* mOldParent;
    nsINode* mNewParent;
    int32_t    mOldOffset;
    int32_t    mNewOffset;

  public:
    nsAutoMoveNodeSelNotify(nsRangeUpdater &aRangeUpdater, 
                            nsINode* aOldParent,
                            int32_t aOldOffset, 
                            nsINode* aNewParent,
                            int32_t aNewOffset)
      : mRU(aRangeUpdater)
      , mOldParent(aOldParent)
      , mNewParent(aNewParent)
      , mOldOffset(aOldOffset)
      , mNewOffset(aNewOffset)
    {
      MOZ_ASSERT(aOldParent);
      MOZ_ASSERT(aNewParent);
      mRU.WillMoveNode();
    }
    
    ~nsAutoMoveNodeSelNotify()
    {
      mRU.DidMoveNode(mOldParent, mOldOffset, mNewParent, mNewOffset);
    }
};

#endif


