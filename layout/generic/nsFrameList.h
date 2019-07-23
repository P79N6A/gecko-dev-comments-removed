






































#ifndef nsFrameList_h___
#define nsFrameList_h___

#include "nscore.h"
#include "nsTraceRefcnt.h"
#include <stdio.h> 
#include "nsDebug.h"

class nsIFrame;




class nsFrameList {
public:
  nsFrameList() :
    mFirstChild(nsnull)
  {
    MOZ_COUNT_CTOR(nsFrameList);
  }

  
  nsFrameList(nsIFrame* aHead) :
    mFirstChild(aHead)
  {
    MOZ_COUNT_CTOR(nsFrameList);
#ifdef DEBUG
    CheckForLoops();
#endif
  }

  nsFrameList(const nsFrameList& aOther) :
    mFirstChild(aOther.mFirstChild)
  {
    MOZ_COUNT_CTOR(nsFrameList);
  }

  ~nsFrameList() {
    MOZ_COUNT_DTOR(nsFrameList);
    
  }

  void DestroyFrames();

  
  void Destroy();

  void SetFrames(nsIFrame* aFrameList) {
    mFirstChild = aFrameList;
#ifdef DEBUG
    CheckForLoops();
#endif
  }

  class Slice;

  



  void AppendFrames(nsIFrame* aParent, nsIFrame* aFrameList);

  




  Slice AppendFrames(nsIFrame* aParent, nsFrameList& aFrameList) {
    NS_PRECONDITION(!aFrameList.IsEmpty(), "Unexpected empty list");
    nsIFrame* firstNewFrame = aFrameList.FirstChild();
    AppendFrames(aParent, firstNewFrame);
    aFrameList.mFirstChild = nsnull;
    return Slice(*this, firstNewFrame, nsnull);
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

  







  inline Slice InsertFrames(nsIFrame* aParent, nsIFrame* aPrevSibling,
                            nsFrameList& aFrameList);

  PRBool Split(nsIFrame* aAfterFrame, nsIFrame** aNextFrameResult);

  





  void SortByContentOrder();

  nsIFrame* FirstChild() const {
    return mFirstChild;
  }

  nsIFrame* LastChild() const;

  nsIFrame* FrameAt(PRInt32 aIndex) const;
  PRInt32 IndexOf(nsIFrame* aFrame) const;

  PRBool IsEmpty() const {
    return nsnull == mFirstChild;
  }

  PRBool NotEmpty() const {
    return nsnull != mFirstChild;
  }

  PRBool ContainsFrame(const nsIFrame* aFrame) const;
  PRBool ContainsFrameBefore(const nsIFrame* aFrame, const nsIFrame* aEnd) const;

  PRInt32 GetLength() const;

  nsIFrame* GetPrevSiblingFor(nsIFrame* aFrame) const;

#ifdef IBMBIDI
  



  nsIFrame* GetPrevVisualFor(nsIFrame* aFrame) const;

  



  nsIFrame* GetNextVisualFor(nsIFrame* aFrame) const;
#endif 

#ifdef DEBUG
  void List(FILE* out) const;
#endif

  class Enumerator;

  


  class Slice {
    friend class Enumerator;

  public:
    
    
    Slice(const nsFrameList& aList) :
#ifdef DEBUG
      mList(aList),
#endif
      mStart(aList.FirstChild()),
      mEnd(nsnull)
    {}

    Slice(const nsFrameList& aList, nsIFrame* aStart, nsIFrame* aEnd) :
#ifdef DEBUG
      mList(aList),
#endif
      mStart(aStart),
      mEnd(aEnd)
    {}

    Slice(const Slice& aOther) :
#ifdef DEBUG
      mList(aOther.mList),
#endif
      mStart(aOther.mStart),
      mEnd(aOther.mEnd)
    {}

  private:
#ifdef DEBUG
    const nsFrameList& mList;
#endif
    nsIFrame* const mStart; 
    const nsIFrame* const mEnd; 
                                
  };

  class Enumerator {
  public:
    Enumerator(const Slice& aSlice) :
#ifdef DEBUG
      mSlice(aSlice),
#endif
      mFrame(aSlice.mStart),
      mEnd(aSlice.mEnd)
    {}

    Enumerator(const Enumerator& aOther) :
#ifdef DEBUG
      mSlice(aOther.mSlice),
#endif
      mFrame(aOther.mFrame),
      mEnd(aOther.mEnd)
    {}

    PRBool AtEnd() const { return mFrame == mEnd; }

    


    inline void Next();

    nsIFrame* get() const { return mFrame; }

  private:
#ifdef DEBUG
    const Slice& mSlice;
#endif
    nsIFrame* mFrame; 
    const nsIFrame* const mEnd; 
                                
  };

private:
#ifdef DEBUG
  void CheckForLoops();
#endif
  
protected:
  nsIFrame* mFirstChild;
};

#endif 
