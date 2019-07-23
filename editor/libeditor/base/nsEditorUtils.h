





































#ifndef nsEditorUtils_h__
#define nsEditorUtils_h__


#include "nsCOMPtr.h"
#include "nsIDOMNode.h"
#include "nsISelection.h"
#include "nsIEditor.h"
#include "nsIAtom.h"
#include "nsVoidArray.h"
#include "nsEditor.h"
#include "nsIContentIterator.h"
#include "nsCOMArray.h"

class nsPlaintextEditor;





class nsAutoPlaceHolderBatch
{
  private:
    nsCOMPtr<nsIEditor> mEd;
  public:
    nsAutoPlaceHolderBatch( nsIEditor *aEd, nsIAtom *atom) : mEd(do_QueryInterface(aEd)) 
                   { if (mEd) mEd->BeginPlaceHolderTransaction(atom); }
    ~nsAutoPlaceHolderBatch() { if (mEd) mEd->EndPlaceHolderTransaction(); }
};






class nsAutoEditBatch : public nsAutoPlaceHolderBatch
{
  public:
    nsAutoEditBatch( nsIEditor *aEd) : nsAutoPlaceHolderBatch(aEd,nsnull)  {}
    ~nsAutoEditBatch() {}
};





class nsAutoSelectionReset
{
  private:
    
    nsCOMPtr<nsISelection> mSel;
    nsEditor *mEd;  

  public:
    
    nsAutoSelectionReset(nsISelection *aSel, nsEditor *aEd);
    
    
    ~nsAutoSelectionReset();

    
    void Abort();
};




class nsAutoRules
{
  public:
  
  nsAutoRules(nsEditor *ed, PRInt32 action, nsIEditor::EDirection aDirection) : 
         mEd(ed), mDoNothing(PR_FALSE)
  { 
    if (mEd && !mEd->mAction) 
    {
      mEd->StartOperation(action, aDirection);
    }
    else mDoNothing = PR_TRUE; 
  }
  ~nsAutoRules() 
  {
    if (mEd && !mDoNothing) 
    {
      mEd->EndOperation();
    }
  }
  
  protected:
  nsEditor *mEd;
  PRBool mDoNothing;
};






class nsAutoTxnsConserveSelection
{
  public:
  
  nsAutoTxnsConserveSelection(nsEditor *ed) : mEd(ed), mOldState(PR_TRUE)
  {
    if (mEd) 
    {
      mOldState = mEd->GetShouldTxnSetSelection();
      mEd->SetShouldTxnSetSelection(PR_FALSE);
    }
  }
  
  ~nsAutoTxnsConserveSelection() 
  {
    if (mEd) 
    {
      mEd->SetShouldTxnSetSelection(mOldState);
    }
  }
  
  protected:
  nsEditor *mEd;
  PRBool mOldState;
};




class nsAutoUpdateViewBatch
{
  public:
  
  nsAutoUpdateViewBatch(nsEditor *ed) : mEd(ed)
  {
    NS_ASSERTION(mEd, "null mEd pointer!");

    if (mEd) 
      mEd->BeginUpdateViewBatch();
  }
  
  ~nsAutoUpdateViewBatch() 
  {
    if (mEd) 
      mEd->EndUpdateViewBatch();
  }
  
  protected:
  nsEditor *mEd;
};





class nsDomIterFunctor 
{
  public:
    virtual void* operator()(nsIDOMNode* aNode)=0;
};

class nsBoolDomIterFunctor 
{
  public:
    virtual PRBool operator()(nsIDOMNode* aNode)=0;
};

class nsDOMIterator
{
  public:
    nsDOMIterator();
    virtual ~nsDOMIterator();
    
    nsresult Init(nsIDOMRange* aRange);
    nsresult Init(nsIDOMNode* aNode);
    void ForEach(nsDomIterFunctor& functor) const;
    nsresult AppendList(nsBoolDomIterFunctor& functor,
                        nsCOMArray<nsIDOMNode>& arrayOfNodes) const;
  protected:
    nsCOMPtr<nsIContentIterator> mIter;
};

class nsDOMSubtreeIterator : public nsDOMIterator
{
  public:
    nsDOMSubtreeIterator();
    virtual ~nsDOMSubtreeIterator();

    nsresult Init(nsIDOMRange* aRange);
    nsresult Init(nsIDOMNode* aNode);
};

class nsTrivialFunctor : public nsBoolDomIterFunctor
{
  public:
    virtual PRBool operator()(nsIDOMNode* aNode)  
    {
      return PR_TRUE;
    }
};





struct DOMPoint
{
  nsCOMPtr<nsIDOMNode> node;
  PRInt32 offset;
  
  DOMPoint() : node(0),offset(0) {}
  DOMPoint(nsIDOMNode *aNode, PRInt32 aOffset) : 
                 node(aNode),offset(aOffset) {}
  void SetPoint(nsIDOMNode *aNode, PRInt32 aOffset)
  {
    node = aNode; offset = aOffset;
  }
  void GetPoint(nsCOMPtr<nsIDOMNode> &aNode, PRInt32 &aOffset)
  {
    aNode = node; aOffset = offset;
  }
};


class nsEditorUtils
{
  public:
    static PRBool IsDescendantOf(nsIDOMNode *aNode, nsIDOMNode *aParent, PRInt32 *aOffset = 0);
    static PRBool IsLeafNode(nsIDOMNode *aNode);
};


class nsIDragSession;
class nsITransferable;
class nsIDOMEvent;
class nsISimpleEnumerator;

class nsEditorHookUtils
{
  public:
    static PRBool   DoAllowDragHook(nsIDOMDocument *aDoc, nsIDOMEvent *aEvent);
    static PRBool   DoDragHook(nsIDOMDocument *aDoc, nsIDOMEvent *aEvent,
                                    nsITransferable *aTrans);
    static PRBool   DoAllowDropHook(nsIDOMDocument *aDoc, nsIDOMEvent *aEvent,
                                    nsIDragSession *aSession);
    static PRBool   DoInsertionHook(nsIDOMDocument *aDoc, nsIDOMEvent *aEvent,
                                    nsITransferable *aTrans);
  private:
    static nsresult GetHookEnumeratorFromDocument(nsIDOMDocument *aDoc,
                                                  nsISimpleEnumerator **aEnumerator);
};

#endif 
