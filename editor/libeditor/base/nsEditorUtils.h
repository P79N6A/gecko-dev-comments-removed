





#ifndef nsEditorUtils_h__
#define nsEditorUtils_h__


#include "nsCOMPtr.h"
#include "nsDebug.h"
#include "nsEditor.h"
#include "nsIDOMNode.h"
#include "nsIEditor.h"
#include "nscore.h"
#include "prtypes.h"

class nsIAtom;
class nsIContentIterator;
class nsIDOMDocument;
class nsIDOMRange;
class nsISelection;
template <class E> class nsCOMArray;





class NS_STACK_CLASS nsAutoPlaceHolderBatch
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





class NS_STACK_CLASS nsAutoSelectionReset
{
  private:
    
    nsRefPtr<mozilla::Selection> mSel;
    nsEditor *mEd;  

  public:
    
    nsAutoSelectionReset(mozilla::Selection* aSel, nsEditor* aEd);
    
    
    ~nsAutoSelectionReset();

    
    void Abort();
};




class NS_STACK_CLASS nsAutoRules
{
  public:
  
  nsAutoRules(nsEditor *ed, nsEditor::OperationID action,
              nsIEditor::EDirection aDirection) :
         mEd(ed), mDoNothing(false)
  { 
    if (mEd && !mEd->mAction) 
    {
      mEd->StartOperation(action, aDirection);
    }
    else mDoNothing = true; 
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
  bool mDoNothing;
};






class NS_STACK_CLASS nsAutoTxnsConserveSelection
{
  public:
  
  nsAutoTxnsConserveSelection(nsEditor *ed) : mEd(ed), mOldState(true)
  {
    if (mEd) 
    {
      mOldState = mEd->GetShouldTxnSetSelection();
      mEd->SetShouldTxnSetSelection(false);
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
  bool mOldState;
};




class NS_STACK_CLASS nsAutoUpdateViewBatch
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





class nsBoolDomIterFunctor 
{
  public:
    virtual bool operator()(nsIDOMNode* aNode)=0;
};

class NS_STACK_CLASS nsDOMIterator
{
  public:
    nsDOMIterator();
    virtual ~nsDOMIterator();
    
    nsresult Init(nsIDOMRange* aRange);
    nsresult Init(nsIDOMNode* aNode);
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
};

class nsTrivialFunctor : public nsBoolDomIterFunctor
{
  public:
    virtual bool operator()(nsIDOMNode* aNode)  
    {
      return true;
    }
};





struct NS_STACK_CLASS DOMPoint
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
    static bool IsDescendantOf(nsIDOMNode *aNode, nsIDOMNode *aParent, PRInt32 *aOffset = 0);
    static bool IsLeafNode(nsIDOMNode *aNode);
};


class nsIDOMEvent;
class nsISimpleEnumerator;
class nsITransferable;

class nsEditorHookUtils
{
  public:
    static bool     DoInsertionHook(nsIDOMDocument *aDoc, nsIDOMEvent *aEvent,
                                    nsITransferable *aTrans);
  private:
    static nsresult GetHookEnumeratorFromDocument(nsIDOMDocument *aDoc,
                                                  nsISimpleEnumerator **aEnumerator);
};

#endif 
