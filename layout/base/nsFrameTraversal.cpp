



































#include "nsCOMPtr.h"
#include "nsGkAtoms.h"

#include "nsFrameTraversal.h"
#include "nsFrameList.h"
#include "nsPlaceholderFrame.h"


class nsFrameIterator: public nsIBidirectionalEnumerator
{
public:
  NS_DECL_ISUPPORTS

  NS_IMETHOD First();

  NS_IMETHOD Last();
  
  NS_IMETHOD Next();

  NS_IMETHOD Prev();

  NS_IMETHOD CurrentItem(nsISupports **aItem);

  NS_IMETHOD IsDone();

  nsFrameIterator(nsPresContext* aPresContext, nsIFrame *aStart,
                  nsIteratorType aType, PRBool aLockScroll, PRBool aFollowOOFs);

protected:
  void      setCurrent(nsIFrame *aFrame){mCurrent = aFrame;}
  nsIFrame *getCurrent(){return mCurrent;}
  void      setStart(nsIFrame *aFrame){mStart = aFrame;}
  nsIFrame *getStart(){return mStart;}
  nsIFrame *getLast(){return mLast;}
  void      setLast(nsIFrame *aFrame){mLast = aFrame;}
  PRInt8    getOffEdge(){return mOffEdge;}
  void      setOffEdge(PRInt8 aOffEdge){mOffEdge = aOffEdge;}
  void      SetLockInScrollView(PRBool aLockScroll){mLockScroll = aLockScroll;}

  















  
  
  nsIFrame* GetParentFrame(nsIFrame* aFrame);

  nsIFrame* GetFirstChild(nsIFrame* aFrame);
  nsIFrame* GetLastChild(nsIFrame* aFrame);

  nsIFrame* GetNextSibling(nsIFrame* aFrame);
  nsIFrame* GetPrevSibling(nsIFrame* aFrame);

  





  
  virtual nsIFrame* GetFirstChildInner(nsIFrame* aFrame);
  virtual nsIFrame* GetLastChildInner(nsIFrame* aFrame);  

  virtual nsIFrame* GetNextSiblingInner(nsIFrame* aFrame);
  virtual nsIFrame* GetPrevSiblingInner(nsIFrame* aFrame);

  nsIFrame* GetPlaceholderFrame(nsIFrame* aFrame);
  PRBool    IsPopupFrame(nsIFrame* aFrame);

  nsPresContext* mPresContext;
  PRPackedBool mLockScroll;
  PRPackedBool mFollowOOFs;
  nsIteratorType mType;

private:
  nsIFrame *mStart;
  nsIFrame *mCurrent;
  nsIFrame *mLast; 
  PRInt8    mOffEdge; 
};




class nsVisualIterator: public nsFrameIterator
{
public:
  nsVisualIterator(nsPresContext* aPresContext, nsIFrame *aStart,
                   nsIteratorType aType, PRBool aLockScroll, PRBool aFollowOOFs) :
  nsFrameIterator(aPresContext, aStart, aType, aLockScroll, aFollowOOFs) {}

protected:
  nsIFrame* GetFirstChildInner(nsIFrame* aFrame);
  nsIFrame* GetLastChildInner(nsIFrame* aFrame);  
  
  nsIFrame* GetNextSiblingInner(nsIFrame* aFrame);
  nsIFrame* GetPrevSiblingInner(nsIFrame* aFrame);  
};



nsresult NS_CreateFrameTraversal(nsIFrameTraversal** aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  *aResult = nsnull;

  nsCOMPtr<nsIFrameTraversal> t(new nsFrameTraversal());
  if (!t)
    return NS_ERROR_OUT_OF_MEMORY;

  *aResult = t;
  NS_ADDREF(*aResult);

  return NS_OK;
}

nsresult
NS_NewFrameTraversal(nsIBidirectionalEnumerator **aEnumerator,
                     nsPresContext* aPresContext,
                     nsIFrame *aStart,
                     nsIteratorType aType,
                     PRBool aVisual,
                     PRBool aLockInScrollView,
                     PRBool aFollowOOFs)
{
  if (!aEnumerator || !aStart)
    return NS_ERROR_NULL_POINTER;
  nsFrameIterator *trav;
  if (aVisual) {
    trav = new nsVisualIterator(aPresContext, aStart, aType,
                                aLockInScrollView, aFollowOOFs);
  } else {
    trav = new nsFrameIterator(aPresContext, aStart, aType,
                               aLockInScrollView, aFollowOOFs);
  }
  if (!trav)
    return NS_ERROR_OUT_OF_MEMORY;
  *aEnumerator = NS_STATIC_CAST(nsIBidirectionalEnumerator*, trav);
  NS_ADDREF(trav);
  return NS_OK;
}


nsFrameTraversal::nsFrameTraversal()
{
}

nsFrameTraversal::~nsFrameTraversal()
{
}

NS_IMPL_ISUPPORTS1(nsFrameTraversal,nsIFrameTraversal)

NS_IMETHODIMP 
 nsFrameTraversal::NewFrameTraversal(nsIBidirectionalEnumerator **aEnumerator,
                                     nsPresContext* aPresContext,
                                     nsIFrame *aStart,
                                     PRInt32 aType,
                                     PRBool aVisual,
                                     PRBool aLockInScrollView,
                                     PRBool aFollowOOFs)
{
  return NS_NewFrameTraversal(aEnumerator, aPresContext, aStart,
                              NS_STATIC_CAST(nsIteratorType, aType),
                              aVisual, aLockInScrollView, aFollowOOFs);  
}



NS_IMPL_ISUPPORTS2(nsFrameIterator, nsIEnumerator, nsIBidirectionalEnumerator)

nsFrameIterator::nsFrameIterator(nsPresContext* aPresContext, nsIFrame *aStart,
                                 nsIteratorType aType, PRBool aLockInScrollView,
                                 PRBool aFollowOOFs)
{
  mOffEdge = 0;
  mPresContext = aPresContext;
  if (aFollowOOFs && aStart)
    aStart = nsPlaceholderFrame::GetRealFrameFor(aStart);
  setStart(aStart);
  setCurrent(aStart);
  setLast(aStart);
  mType = aType;
  SetLockInScrollView(aLockInScrollView);
  mFollowOOFs = aFollowOOFs;
}



NS_IMETHODIMP
nsFrameIterator::CurrentItem(nsISupports **aItem)
{
  if (!aItem)
    return NS_ERROR_NULL_POINTER;
  *aItem = mCurrent;
  if (mOffEdge)
    return NS_ENUMERATOR_FALSE;
  return NS_OK;
}



NS_IMETHODIMP
nsFrameIterator::IsDone()
{
  if (mOffEdge != 0)
    return NS_OK;
  return NS_ENUMERATOR_FALSE;
}



NS_IMETHODIMP
nsFrameIterator::First()
{
  mCurrent = mStart;
  return NS_OK;
}

static PRBool
IsRootFrame(nsIFrame* aFrame)
{
  nsIAtom* atom = aFrame->GetType();
  return (atom == nsGkAtoms::canvasFrame) ||
         (atom == nsGkAtoms::rootFrame);
}

NS_IMETHODIMP
nsFrameIterator::Last()
{
  nsIFrame* result;
  nsIFrame* parent = getCurrent();
  while (!IsRootFrame(parent) && (result = GetParentFrame(parent)))
    parent = result;
  
  while ((result = GetLastChild(parent))) {
    parent = result;
  }
  
  setCurrent(parent);
  if (!parent)
    setOffEdge(1);
  return NS_OK;
}

NS_IMETHODIMP
nsFrameIterator::Next()
{
  
  nsIFrame *result = nsnull;
  nsIFrame *parent = getCurrent();
  if (!parent)
    parent = getLast();

  if (mType == eLeaf) {
    
    while ((result = GetFirstChild(parent))) {
      parent = result;
    }
  } else if (mType == ePreOrder) {
    result = GetFirstChild(parent);
    if (result)
      parent = result;
  }

  if (parent != getCurrent()) {
    result = parent;
  } else {
    while (parent) {
      result = GetNextSibling(parent);
      if (result) {
        if (mType != ePreOrder) {
          parent = result;
          while ((result = GetFirstChild(parent))) {
            parent = result;
          }
          result = parent;
        }
        break;
      }
      else {
        result = GetParentFrame(parent);
        if (!result || IsRootFrame(result) ||
            (mLockScroll && result->GetType() == nsGkAtoms::scrollFrame)) {
          result = nsnull;
          break;
        }
        if (mType == ePostOrder)
          break;
        parent = result;
      }
    }
  }

  setCurrent(result);
  if (!result) {
    setOffEdge(1);
    setLast(parent);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsFrameIterator::Prev()
{
  
  nsIFrame *result = nsnull;
  nsIFrame *parent = getCurrent();
  if (!parent)
    parent = getLast();

  if (mType == eLeaf) {
    
    while ((result = GetLastChild(parent))) {
      parent = result;
    }
  } else if (mType == ePostOrder) {
    result = GetLastChild(parent);
    if (result)
      parent = result;
  }
  
  if (parent != getCurrent()) {
    result = parent;
  } else {
    while (parent) {
      result = GetPrevSibling(parent);
      if (result) {
        if (mType != ePostOrder) {
          parent = result;
          while ((result = GetLastChild(parent))) {
            parent = result;
          }
          result = parent;
        }
        break;
      } else {
        result = GetParentFrame(parent);
        if (!result || IsRootFrame(result) ||
            (mLockScroll && result->GetType() == nsGkAtoms::scrollFrame)) {
          result = nsnull;
          break;
        }
        if (mType == ePreOrder)
          break;
        parent = result;
      }
    }
  }

  setCurrent(result);
  if (!result) {
    setOffEdge(-1);
    setLast(parent);
  }
  return NS_OK;
}

nsIFrame*
nsFrameIterator::GetParentFrame(nsIFrame* aFrame)
{
  if (mFollowOOFs)
    aFrame = GetPlaceholderFrame(aFrame);
  if (aFrame)
    return aFrame->GetParent();
  
  return nsnull;
}

nsIFrame*
nsFrameIterator::GetFirstChild(nsIFrame* aFrame)
{
  nsIFrame* result = GetFirstChildInner(aFrame);
  if (result && mFollowOOFs) {
    result = nsPlaceholderFrame::GetRealFrameFor(result);
    
    if (result && IsPopupFrame(result))
      result = GetNextSibling(result);
  }
  return result;
}

nsIFrame*
nsFrameIterator::GetLastChild(nsIFrame* aFrame)
{
  nsIFrame* result = GetLastChildInner(aFrame);
  if (result && mFollowOOFs) {
    result = nsPlaceholderFrame::GetRealFrameFor(result);
    
    if (result && IsPopupFrame(result))
      result = GetNextSibling(result);
  }
  return result;
}

nsIFrame*
nsFrameIterator::GetNextSibling(nsIFrame* aFrame)
{
  nsIFrame* result = nsnull;
  if (mFollowOOFs)
    aFrame = GetPlaceholderFrame(aFrame);
  if (aFrame) {
    result = GetNextSiblingInner(aFrame);
    if (result && mFollowOOFs)
      result = nsPlaceholderFrame::GetRealFrameFor(result);
  }

  if (result && mFollowOOFs && IsPopupFrame(result))
    result = GetNextSibling(result);

  return result;
}

nsIFrame*
nsFrameIterator::GetPrevSibling(nsIFrame* aFrame)
{
  nsIFrame* result = nsnull;
  if (mFollowOOFs)
    aFrame = GetPlaceholderFrame(aFrame);
  if (aFrame) {
    result = GetPrevSiblingInner(aFrame);
    if (result && mFollowOOFs)
      result = nsPlaceholderFrame::GetRealFrameFor(result);
  }

  if (result && mFollowOOFs && IsPopupFrame(result))
    result = GetPrevSibling(result);

  return result;
}

nsIFrame*
nsFrameIterator::GetFirstChildInner(nsIFrame* aFrame) {
  return aFrame->GetFirstChild(nsnull);
}

nsIFrame*
nsFrameIterator::GetLastChildInner(nsIFrame* aFrame) {
  nsIFrame* child = aFrame->GetFirstChild(nsnull);
  if (!child)
    return nsnull;
  nsFrameList list(child);
  return list.LastChild();
}

nsIFrame*
nsFrameIterator::GetNextSiblingInner(nsIFrame* aFrame) {
  return aFrame->GetNextSibling();
}

nsIFrame*
nsFrameIterator::GetPrevSiblingInner(nsIFrame* aFrame) {
  nsIFrame* parent = GetParentFrame(aFrame);
  if (!parent)
    return nsnull;
  nsFrameList list(parent->GetFirstChild(nsnull));
  return list.GetPrevSiblingFor(aFrame);
}


nsIFrame*
nsFrameIterator::GetPlaceholderFrame(nsIFrame* aFrame)
{
  nsIFrame* result = aFrame;
  nsIPresShell *presShell = mPresContext->GetPresShell();
  if (presShell) {
    nsIFrame* placeholder = 0;
    presShell->GetPlaceholderFrameFor(aFrame, &placeholder);
    if (placeholder)
      result = placeholder;
  }

  if (result != aFrame)
    result = GetPlaceholderFrame(result);

  return result;
}

PRBool
nsFrameIterator::IsPopupFrame(nsIFrame* aFrame)
{
  return (aFrame->GetStyleDisplay()->mDisplay == NS_STYLE_DISPLAY_POPUP);
}



nsIFrame*
nsVisualIterator::GetFirstChildInner(nsIFrame* aFrame) {
  nsIFrame* child = aFrame->GetFirstChild(nsnull);
  if (!child)
    return nsnull;
  nsFrameList list(child);
  return list.GetNextVisualFor(nsnull);
}

nsIFrame*
nsVisualIterator::GetLastChildInner(nsIFrame* aFrame) {
  nsIFrame* child = aFrame->GetFirstChild(nsnull);
  if (!child)
    return nsnull;
  nsFrameList list(child);
  return list.GetPrevVisualFor(nsnull);
}

nsIFrame*
nsVisualIterator::GetNextSiblingInner(nsIFrame* aFrame) {
  nsIFrame* parent = GetParentFrame(aFrame);
  if (!parent)
    return nsnull;
  nsFrameList list(parent->GetFirstChild(nsnull));
  return list.GetNextVisualFor(aFrame);
}

nsIFrame*
nsVisualIterator::GetPrevSiblingInner(nsIFrame* aFrame) {
  nsIFrame* parent = GetParentFrame(aFrame);
  if (!parent)
    return nsnull;
  nsFrameList list(parent->GetFirstChild(nsnull));
  return list.GetPrevVisualFor(aFrame);
}
