






































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

  nsFrameList(nsIFrame* aFirstFrame, nsIFrame* aLastFrame) :
    mFirstChild(aFirstFrame)
  {
    NS_ASSERTION(aLastFrame == LastChild(), "wrong last frame");

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

  void Clear() { SetFrames(nsnull); }

  void SetFrames(nsFrameList& aFrameList) {
    NS_PRECONDITION(!mFirstChild, "Losing frames");
    mFirstChild = aFrameList.FirstChild();
    aFrameList.Clear();
  }

  class Slice;

  



  void AppendFrames(nsIFrame* aParent, nsIFrame* aFrameList) {
    InsertFrames(aParent, LastChild(), aFrameList);
  }

  




  Slice AppendFrames(nsIFrame* aParent, nsFrameList& aFrameList) {
    NS_PRECONDITION(!aFrameList.IsEmpty(), "Unexpected empty list");
    nsIFrame* firstNewFrame = aFrameList.FirstChild();
    AppendFrames(aParent, firstNewFrame);
    aFrameList.Clear();
    return Slice(*this, firstNewFrame, nsnull);
  }

  

  inline void AppendFrame(nsIFrame* aParent, nsIFrame* aFrame);

  







  PRBool RemoveFrame(nsIFrame* aFrame, nsIFrame* aPrevSiblingHint = nsnull);

  




  nsFrameList RemoveFramesAfter(nsIFrame* aAfterFrame);

  





  PRBool RemoveFirstChild();

  







  PRBool DestroyFrame(nsIFrame* aFrame, nsIFrame* aPrevSiblingHint = nsnull);

  






  inline void InsertFrame(nsIFrame* aParent, nsIFrame* aPrevSibling,
                          nsIFrame* aNewFrame);

  




  void InsertFrames(nsIFrame* aParent,
                    nsIFrame* aPrevSibling,
                    nsIFrame* aFrameList);

  







  inline Slice InsertFrames(nsIFrame* aParent, nsIFrame* aPrevSibling,
                            nsFrameList& aFrameList);

  class FrameLinkEnumerator;

  



  nsFrameList ExtractHead(FrameLinkEnumerator& aLink);

  



  nsFrameList ExtractTail(FrameLinkEnumerator& aLink);

  





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

  



  nsIFrame* OnlyChild() const {
    if (FirstChild() == LastChild()) {
      return FirstChild();
    }
    return nsnull;
  }

#ifdef IBMBIDI
  



  nsIFrame* GetPrevVisualFor(nsIFrame* aFrame) const;

  



  nsIFrame* GetNextVisualFor(nsIFrame* aFrame) const;
#endif 

#ifdef DEBUG
  void List(FILE* out) const;
#endif

  static nsresult Init();
  static void Shutdown() { delete sEmptyList; }
  static const nsFrameList& EmptyList() { return *sEmptyList; }

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

    PRBool AtEnd() const {
      
      
      
      return !mFrame || mFrame == mEnd;
    }

    


    inline void Next();

    



    nsIFrame* get() const {
      NS_PRECONDITION(!AtEnd(), "Enumerator is at end");
      return mFrame;
    }

    



    Enumerator GetUnlimitedEnumerator() const {
      return Enumerator(*this, nsnull);
    }

#ifdef DEBUG
    const nsFrameList& List() const { return mSlice.mList; }
#endif

  protected:
    Enumerator(const Enumerator& aOther, const nsIFrame* const aNewEnd):
#ifdef DEBUG
      mSlice(aOther.mSlice),
#endif
      mFrame(aOther.mFrame),
      mEnd(aNewEnd)
    {}

#ifdef DEBUG
    

    const Slice mSlice;
#endif
    nsIFrame* mFrame; 
    const nsIFrame* const mEnd; 
                                
  };

  








  class FrameLinkEnumerator : private Enumerator {
  public:
    friend class nsFrameList;

    FrameLinkEnumerator(const nsFrameList& aList) :
      Enumerator(aList),
      mPrev(nsnull)
    {}

    FrameLinkEnumerator(const FrameLinkEnumerator& aOther) :
      Enumerator(aOther),
      mPrev(aOther.mPrev)
    {}

    void operator=(const FrameLinkEnumerator& aOther) {
      NS_PRECONDITION(&List() == &aOther.List(), "Different lists?");
      mFrame = aOther.mFrame;
      mPrev = aOther.mPrev;
    }

    void Next() {
      mPrev = mFrame;
      Enumerator::Next();
    }

    PRBool AtEnd() const { return Enumerator::AtEnd(); }

    nsIFrame* PrevFrame() const { return mPrev; }
    nsIFrame* NextFrame() const { return mFrame; }

  protected:
    nsIFrame* mPrev;
  };

private:
#ifdef DEBUG
  void CheckForLoops();
#endif

  static const nsFrameList* sEmptyList;

protected:
  nsIFrame* mFirstChild;
};

#endif 
