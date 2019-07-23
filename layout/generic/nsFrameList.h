






































#ifndef nsFrameList_h___
#define nsFrameList_h___

#include "nsIFrame.h"





class nsFrameList {
public:
  nsFrameList() {
    mFirstChild = nsnull;
  }

  nsFrameList(nsIFrame* aHead) {
    mFirstChild = aHead;
#ifdef DEBUG
    CheckForLoops();
#endif
  }

  ~nsFrameList() {
  }

  void DestroyFrames();

  void SetFrames(nsIFrame* aFrameList) {
    mFirstChild = aFrameList;
#ifdef DEBUG
    CheckForLoops();
#endif
  }

  void AppendFrames(nsIFrame* aParent, nsIFrame* aFrameList);

  void AppendFrames(nsIFrame* aParent, nsFrameList& aFrameList) {
    AppendFrames(aParent, aFrameList.mFirstChild);
    aFrameList.mFirstChild = nsnull;
  }

  void AppendFrame(nsIFrame* aParent, nsIFrame* aFrame);

  
  
  
  
  
  PRBool RemoveFrame(nsIFrame* aFrame, nsIFrame* aPrevSiblingHint = nsnull);

  
  
  
  PRBool RemoveFirstChild();

  
  
  
  PRBool DestroyFrame(nsIFrame* aFrame);

  void InsertFrame(nsIFrame* aParent,
                   nsIFrame* aPrevSibling,
                   nsIFrame* aNewFrame);

  void InsertFrames(nsIFrame* aParent,
                    nsIFrame* aPrevSibling,
                    nsIFrame* aFrameList);

  void InsertFrames(nsIFrame* aParent, nsIFrame* aPrevSibling,
                    nsFrameList& aFrameList) {
    InsertFrames(aParent, aPrevSibling, aFrameList.FirstChild());
    aFrameList.mFirstChild = nsnull;
  }

  PRBool Split(nsIFrame* aAfterFrame, nsIFrame** aNextFrameResult);

  





  void SortByContentOrder();

  nsIFrame* FirstChild() const {
    return mFirstChild;
  }

  nsIFrame* LastChild() const;

  nsIFrame* FrameAt(PRInt32 aIndex) const;

  PRBool IsEmpty() const {
    return nsnull == mFirstChild;
  }

  PRBool NotEmpty() const {
    return nsnull != mFirstChild;
  }

  PRBool ContainsFrame(const nsIFrame* aFrame) const;

  PRInt32 GetLength() const;

  nsIFrame* GetPrevSiblingFor(nsIFrame* aFrame) const;

#ifdef IBMBIDI
  



  nsIFrame* GetPrevVisualFor(nsIFrame* aFrame) const;

  



  nsIFrame* GetNextVisualFor(nsIFrame* aFrame) const;
#endif 

  void VerifyParent(nsIFrame* aParent) const;

#ifdef NS_DEBUG
  void List(FILE* out) const;
#endif

private:
#ifdef DEBUG
  void CheckForLoops();
#endif
  
protected:
  nsIFrame* mFirstChild;
};

#endif 
