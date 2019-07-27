





#ifndef nsEditorUtils_h__
#define nsEditorUtils_h__


#include "nsCOMPtr.h"
#include "nsDebug.h"
#include "nsEditor.h"
#include "nsIDOMNode.h"
#include "nsIEditor.h"
#include "nscore.h"

class nsIAtom;
class nsIContentIterator;
class nsIDOMDocument;
class nsRange;
template <class E> class nsCOMArray;
namespace mozilla {
namespace dom {
template <class T> class OwningNonNull;
class Selection;
}
}





class MOZ_STACK_CLASS nsAutoPlaceHolderBatch
{
  private:
    nsCOMPtr<nsIEditor> mEd;
  public:
    nsAutoPlaceHolderBatch( nsIEditor *aEd, nsIAtom *atom) : mEd(do_QueryInterface(aEd)) 
                   { if (mEd) mEd->BeginPlaceHolderTransaction(atom); }
    ~nsAutoPlaceHolderBatch() { if (mEd) mEd->EndPlaceHolderTransaction(); }
};






class MOZ_STACK_CLASS nsAutoEditBatch : public nsAutoPlaceHolderBatch
{
  public:
    explicit nsAutoEditBatch( nsIEditor *aEd) : nsAutoPlaceHolderBatch(aEd,nullptr)  {}
    ~nsAutoEditBatch() {}
};





class MOZ_STACK_CLASS nsAutoSelectionReset
{
  private:
    
    nsRefPtr<mozilla::dom::Selection> mSel;
    nsEditor *mEd;  

  public:
    
    nsAutoSelectionReset(mozilla::dom::Selection* aSel, nsEditor* aEd);
    
    
    ~nsAutoSelectionReset();

    
    void Abort();
};




class MOZ_STACK_CLASS nsAutoRules
{
  public:
  
  nsAutoRules(nsEditor *ed, EditAction action,
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






class MOZ_STACK_CLASS nsAutoTxnsConserveSelection
{
  public:
  
  explicit nsAutoTxnsConserveSelection(nsEditor *ed) : mEd(ed), mOldState(true)
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




class MOZ_STACK_CLASS nsAutoUpdateViewBatch
{
  public:
  
  explicit nsAutoUpdateViewBatch(nsEditor *ed) : mEd(ed)
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
    virtual bool operator()(nsIDOMNode* aNode) const = 0;
    bool operator()(nsINode* aNode) const
    {
      return operator()(GetAsDOMNode(aNode));
    }
};

class MOZ_STACK_CLASS nsDOMIterator
{
  public:
    explicit nsDOMIterator(nsRange& aRange);
    explicit nsDOMIterator(nsINode& aNode);
    virtual ~nsDOMIterator();

    void AppendList(const nsBoolDomIterFunctor& functor,
                    nsTArray<mozilla::dom::OwningNonNull<nsINode>>& arrayOfNodes) const;
    void AppendList(const nsBoolDomIterFunctor& functor,
                    nsCOMArray<nsIDOMNode>& arrayOfNodes) const;
  protected:
    nsCOMPtr<nsIContentIterator> mIter;

    
    nsDOMIterator();
};

class MOZ_STACK_CLASS nsDOMSubtreeIterator : public nsDOMIterator
{
  public:
    explicit nsDOMSubtreeIterator(nsRange& aRange);
    virtual ~nsDOMSubtreeIterator();
};

class nsTrivialFunctor : public nsBoolDomIterFunctor
{
  public:
    
    virtual bool operator()(nsIDOMNode* aNode) const
    {
      return true;
    }
};





struct MOZ_STACK_CLASS DOMPoint
{
  nsCOMPtr<nsINode> node;
  int32_t offset;

  DOMPoint() : node(nullptr), offset(-1) {}
  DOMPoint(nsINode* aNode, int32_t aOffset)
    : node(aNode)
    , offset(aOffset)
  {}
  DOMPoint(nsIDOMNode* aNode, int32_t aOffset)
    : node(do_QueryInterface(aNode))
    , offset(aOffset)
  {}

  void SetPoint(nsINode* aNode, int32_t aOffset)
  {
    node = aNode;
    offset = aOffset;
  }
  void SetPoint(nsIDOMNode* aNode, int32_t aOffset)
  {
    node = do_QueryInterface(aNode);
    offset = aOffset;
  }
};


class nsEditorUtils
{
  public:
    static bool IsDescendantOf(nsINode* aNode, nsINode* aParent, int32_t* aOffset = 0);
    static bool IsDescendantOf(nsIDOMNode *aNode, nsIDOMNode *aParent, int32_t *aOffset = 0);
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
