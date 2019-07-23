




































#ifndef __selectionstate_h__
#define __selectionstate_h__

#include "nsCOMPtr.h"
#include "nsVoidArray.h"
#include "nsIDOMNode.h"
#include "nsIDOMRange.h"

class nsIDOMCharacterData;
class nsISelection;








struct nsRangeStore 
{
  nsRangeStore();
  ~nsRangeStore();
  nsresult StoreRange(nsIDOMRange *aRange);
  nsresult GetRange(nsCOMPtr<nsIDOMRange> *outRange);
        
  nsCOMPtr<nsIDOMNode> startNode;
  PRInt32              startOffset;
  nsCOMPtr<nsIDOMNode> endNode;
  PRInt32              endOffset;
  
};

class nsSelectionState
{
  public:
      
    nsSelectionState();
    ~nsSelectionState();
  
    nsresult SaveSelection(nsISelection *aSel);
    nsresult RestoreSelection(nsISelection *aSel);
    PRBool   IsCollapsed();
    PRBool   IsEqual(nsSelectionState *aSelState);
    void     MakeEmpty();
    PRBool   IsEmpty();
  protected:    
    nsVoidArray mArray;
    
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
    
    
    
    
    
    
    nsresult SelAdjCreateNode(nsIDOMNode *aParent, PRInt32 aPosition);
    nsresult SelAdjInsertNode(nsIDOMNode *aParent, PRInt32 aPosition);
    nsresult SelAdjDeleteNode(nsIDOMNode *aNode);
    nsresult SelAdjSplitNode(nsIDOMNode *aOldRightNode, PRInt32 aOffset, nsIDOMNode *aNewLeftNode);
    nsresult SelAdjJoinNodes(nsIDOMNode *aLeftNode, 
                             nsIDOMNode *aRightNode, 
                             nsIDOMNode *aParent, 
                             PRInt32 aOffset,
                             PRInt32 aOldLeftNodeLength);
    nsresult SelAdjInsertText(nsIDOMCharacterData *aTextNode, PRInt32 aOffset, const nsAString &aString);
    nsresult SelAdjDeleteText(nsIDOMCharacterData *aTextNode, PRInt32 aOffset, PRInt32 aLength);
    
    
    nsresult WillReplaceContainer();
    nsresult DidReplaceContainer(nsIDOMNode *aOriginalNode, nsIDOMNode *aNewNode);
    nsresult WillRemoveContainer();
    nsresult DidRemoveContainer(nsIDOMNode *aNode, nsIDOMNode *aParent, PRInt32 aOffset, PRUint32 aNodeOrigLen);
    nsresult WillInsertContainer();
    nsresult DidInsertContainer();
    nsresult WillMoveNode();
    nsresult DidMoveNode(nsIDOMNode *aOldParent, PRInt32 aOldOffset, nsIDOMNode *aNewParent, PRInt32 aNewOffset);
  protected:    
    nsVoidArray mArray;
    PRBool mLock;
};







class nsAutoTrackDOMPoint
{
  private:
    nsRangeUpdater &mRU;
    nsCOMPtr<nsIDOMNode> *mNode;
    PRInt32 *mOffset;
    nsRangeStore mRangeItem;
  public:
    nsAutoTrackDOMPoint(nsRangeUpdater &aRangeUpdater, nsCOMPtr<nsIDOMNode> *aNode, PRInt32 *aOffset) :
    mRU(aRangeUpdater)
    ,mNode(aNode)
    ,mOffset(aOffset)
    {
      mRangeItem.startNode = *mNode;
      mRangeItem.endNode = *mNode;
      mRangeItem.startOffset = *mOffset;
      mRangeItem.endOffset = *mOffset;
      mRU.RegisterRangeItem(&mRangeItem);
    }
    
    ~nsAutoTrackDOMPoint()
    {
      mRU.DropRangeItem(&mRangeItem);
      *mNode  = mRangeItem.startNode;
      *mOffset = mRangeItem.startOffset;
    }
};








class nsAutoReplaceContainerSelNotify
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







class nsAutoRemoveContainerSelNotify
{
  private:
    nsRangeUpdater &mRU;
    nsIDOMNode *mNode;
    nsIDOMNode *mParent;
    PRInt32    mOffset;
    PRUint32   mNodeOrigLen;

  public:
    nsAutoRemoveContainerSelNotify(nsRangeUpdater &aRangeUpdater, 
                                   nsIDOMNode *aNode, 
                                   nsIDOMNode *aParent, 
                                   PRInt32 aOffset, 
                                   PRUint32 aNodeOrigLen) :
    mRU(aRangeUpdater)
    ,mNode(aNode)
    ,mParent(aParent)
    ,mOffset(aOffset)
    ,mNodeOrigLen(aNodeOrigLen)
    {
      mRU.WillRemoveContainer();
    }
    
    ~nsAutoRemoveContainerSelNotify()
    {
      mRU.DidRemoveContainer(mNode, mParent, mOffset, mNodeOrigLen);
    }
};






class nsAutoInsertContainerSelNotify
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







class nsAutoMoveNodeSelNotify
{
  private:
    nsRangeUpdater &mRU;
    nsIDOMNode *mOldParent;
    nsIDOMNode *mNewParent;
    PRInt32    mOldOffset;
    PRInt32    mNewOffset;

  public:
    nsAutoMoveNodeSelNotify(nsRangeUpdater &aRangeUpdater, 
                            nsIDOMNode *aOldParent, 
                            PRInt32 aOldOffset, 
                            nsIDOMNode *aNewParent, 
                            PRInt32 aNewOffset) :
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


