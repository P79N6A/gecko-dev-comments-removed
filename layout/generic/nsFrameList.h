




#ifndef nsFrameList_h___
#define nsFrameList_h___

#include "nscore.h"
#include "nsTraceRefcnt.h"
#include <stdio.h> 
#include "nsDebug.h"
#include "nsTArray.h"

class nsIFrame;
namespace mozilla {
namespace layout {
  class FrameChildList;
  enum FrameChildListID {
      
      kPrincipalList                = 0x1,
      kPopupList                    = 0x2,
      kCaptionList                  = 0x4,
      kColGroupList                 = 0x8,
      kSelectPopupList              = 0x10,
      kAbsoluteList                 = 0x20,
      kFixedList                    = 0x40,
      kOverflowList                 = 0x80,
      kOverflowContainersList       = 0x100,
      kExcessOverflowContainersList = 0x200,
      kOverflowOutOfFlowList        = 0x400,
      kFloatList                    = 0x800,
      kBulletList                   = 0x1000,
      kPushedFloatsList             = 0x2000,
      
      
      kNoReflowPrincipalList        = 0x4000
  };
}
}







class nsFrameList {
public:
  nsFrameList() :
    mFirstChild(nullptr), mLastChild(nullptr)
  {
    MOZ_COUNT_CTOR(nsFrameList);
  }

  nsFrameList(nsIFrame* aFirstFrame, nsIFrame* aLastFrame) :
    mFirstChild(aFirstFrame), mLastChild(aLastFrame)
  {
    MOZ_COUNT_CTOR(nsFrameList);
    VerifyList();
  }

  nsFrameList(const nsFrameList& aOther) :
    mFirstChild(aOther.mFirstChild), mLastChild(aOther.mLastChild)
  {
    MOZ_COUNT_CTOR(nsFrameList);
  }

  ~nsFrameList() {
    MOZ_COUNT_DTOR(nsFrameList);
    
  }

  



  void DestroyFrames();

  



  void DestroyFramesFrom(nsIFrame* aDestructRoot);

  




  void Destroy();

  




  void DestroyFrom(nsIFrame* aDestructRoot);

  void Clear() { mFirstChild = mLastChild = nullptr; }

  void SetFrames(nsIFrame* aFrameList);

  void SetFrames(nsFrameList& aFrameList) {
    NS_PRECONDITION(!mFirstChild, "Losing frames");

    mFirstChild = aFrameList.FirstChild();
    mLastChild = aFrameList.LastChild();
    aFrameList.Clear();
  }

  class Slice;

  




  Slice AppendFrames(nsIFrame* aParent, nsFrameList& aFrameList) {
    return InsertFrames(aParent, LastChild(), aFrameList);
  }


  



  void AppendFrame(nsIFrame* aParent, nsIFrame* aFrame) {
    nsFrameList temp(aFrame, aFrame);
    AppendFrames(aParent, temp);
  }

  




  void RemoveFrame(nsIFrame* aFrame);

  





  bool RemoveFrameIfPresent(nsIFrame* aFrame);

  





  nsFrameList RemoveFramesAfter(nsIFrame* aAfterFrame);

  



  nsIFrame* RemoveFirstChild();

  


















  inline bool StartRemoveFrame(nsIFrame* aFrame);

  






  inline bool ContinueRemoveFrame(nsIFrame* aFrame);

  



  void DestroyFrame(nsIFrame* aFrame);

  




  bool DestroyFrameIfPresent(nsIFrame* aFrame);

  





  void InsertFrame(nsIFrame* aParent, nsIFrame* aPrevSibling,
                   nsIFrame* aFrame) {
    nsFrameList temp(aFrame, aFrame);
    InsertFrames(aParent, aPrevSibling, temp);
  }


  





  Slice InsertFrames(nsIFrame* aParent, nsIFrame* aPrevSibling,
                     nsFrameList& aFrameList);

  class FrameLinkEnumerator;

  




  nsFrameList ExtractHead(FrameLinkEnumerator& aLink);

  




  nsFrameList ExtractTail(FrameLinkEnumerator& aLink);

  nsIFrame* FirstChild() const {
    return mFirstChild;
  }

  nsIFrame* LastChild() const {
    return mLastChild;
  }

  nsIFrame* FrameAt(int32_t aIndex) const;
  int32_t IndexOf(nsIFrame* aFrame) const;

  bool IsEmpty() const {
    return nullptr == mFirstChild;
  }

  bool NotEmpty() const {
    return nullptr != mFirstChild;
  }

  bool ContainsFrame(const nsIFrame* aFrame) const;

  int32_t GetLength() const;

  



  nsIFrame* OnlyChild() const {
    if (FirstChild() == LastChild()) {
      return FirstChild();
    }
    return nullptr;
  }

  



  void ApplySetParent(nsIFrame* aParent) const;

  




  inline void AppendIfNonempty(nsTArray<mozilla::layout::FrameChildList>* aLists,
                               mozilla::layout::FrameChildListID aListID) const;

#ifdef IBMBIDI
  



  nsIFrame* GetPrevVisualFor(nsIFrame* aFrame) const;

  



  nsIFrame* GetNextVisualFor(nsIFrame* aFrame) const;
#endif 

#ifdef DEBUG
  void List(FILE* out) const;
#endif

  static void Init();
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
      mEnd(nullptr)
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

    bool AtEnd() const {
      
      
      
      return !mFrame || mFrame == mEnd;
    }

    


    inline void Next();

    



    nsIFrame* get() const {
      NS_PRECONDITION(!AtEnd(), "Enumerator is at end");
      return mFrame;
    }

    



    Enumerator GetUnlimitedEnumerator() const {
      return Enumerator(*this, nullptr);
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
      mPrev(nullptr)
    {}

    FrameLinkEnumerator(const FrameLinkEnumerator& aOther) :
      Enumerator(aOther),
      mPrev(aOther.mPrev)
    {}

    


    inline FrameLinkEnumerator(const nsFrameList& aList, nsIFrame* aPrevFrame);

    void operator=(const FrameLinkEnumerator& aOther) {
      NS_PRECONDITION(&List() == &aOther.List(), "Different lists?");
      mFrame = aOther.mFrame;
      mPrev = aOther.mPrev;
    }

    void Next() {
      mPrev = mFrame;
      Enumerator::Next();
    }

    bool AtEnd() const { return Enumerator::AtEnd(); }

    nsIFrame* PrevFrame() const { return mPrev; }
    nsIFrame* NextFrame() const { return mFrame; }

  protected:
    nsIFrame* mPrev;
  };

private:
#ifdef DEBUG_FRAME_LIST
  void VerifyList() const;
#else
  void VerifyList() const {}
#endif

  static const nsFrameList* sEmptyList;

protected:
  





  static void UnhookFrameFromSiblings(nsIFrame* aFrame);

  nsIFrame* mFirstChild;
  nsIFrame* mLastChild;
};

#endif 
