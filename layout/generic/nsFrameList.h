




#ifndef nsFrameList_h___
#define nsFrameList_h___

#include <stdio.h> 
#include "nsDebug.h"
#include "nsTArrayForwardDeclare.h"
#include "mozilla/ReverseIterator.h"

#if defined(DEBUG) || defined(MOZ_DUMP_PAINTING)


#define DEBUG_FRAME_DUMP 1
#endif

class nsContainerFrame;
class nsIFrame;
class nsIPresShell;
class nsPresContext;

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
  }

  nsFrameList(nsIFrame* aFirstFrame, nsIFrame* aLastFrame) :
    mFirstChild(aFirstFrame), mLastChild(aLastFrame)
  {
    VerifyList();
  }

  nsFrameList(const nsFrameList& aOther) :
    mFirstChild(aOther.mFirstChild), mLastChild(aOther.mLastChild)
  {
  }

  


  void* operator new(size_t sz, nsIPresShell* aPresShell) CPP_THROW_NEW;

  



  void Delete(nsIPresShell* aPresShell);

  



  void DestroyFrames();

  



  void DestroyFramesFrom(nsIFrame* aDestructRoot);

  void Clear() { mFirstChild = mLastChild = nullptr; }

  void SetFrames(nsIFrame* aFrameList);

  void SetFrames(nsFrameList& aFrameList) {
    NS_PRECONDITION(!mFirstChild, "Losing frames");

    mFirstChild = aFrameList.FirstChild();
    mLastChild = aFrameList.LastChild();
    aFrameList.Clear();
  }

  class Slice;

  




  Slice AppendFrames(nsContainerFrame* aParent, nsFrameList& aFrameList) {
    return InsertFrames(aParent, LastChild(), aFrameList);
  }


  



  void AppendFrame(nsContainerFrame* aParent, nsIFrame* aFrame) {
    nsFrameList temp(aFrame, aFrame);
    AppendFrames(aParent, temp);
  }

  




  void RemoveFrame(nsIFrame* aFrame);

  





  nsFrameList RemoveFramesAfter(nsIFrame* aAfterFrame);

  



  nsIFrame* RemoveFirstChild();

  


















  inline bool StartRemoveFrame(nsIFrame* aFrame);

  






  inline bool ContinueRemoveFrame(nsIFrame* aFrame);

  



  void DestroyFrame(nsIFrame* aFrame);

  





  void InsertFrame(nsContainerFrame* aParent, nsIFrame* aPrevSibling,
                   nsIFrame* aFrame) {
    nsFrameList temp(aFrame, aFrame);
    InsertFrames(aParent, aPrevSibling, temp);
  }


  





  Slice InsertFrames(nsContainerFrame* aParent, nsIFrame* aPrevSibling,
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

  



  void ApplySetParent(nsContainerFrame* aParent) const;

  




  inline void AppendIfNonempty(nsTArray<mozilla::layout::FrameChildList>* aLists,
                               mozilla::layout::FrameChildListID aListID) const;

  



  nsIFrame* GetPrevVisualFor(nsIFrame* aFrame) const;

  



  nsIFrame* GetNextVisualFor(nsIFrame* aFrame) const;

#ifdef DEBUG_FRAME_DUMP
  void List(FILE* out) const;
#endif

  static inline const nsFrameList& EmptyList();

  class Enumerator;

  


  class Slice {
    friend class Enumerator;

  public:
    
    
    MOZ_IMPLICIT Slice(const nsFrameList& aList) :
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
    explicit Enumerator(const Slice& aSlice) :
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

    explicit FrameLinkEnumerator(const nsFrameList& aList) :
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

    inline void Next();

    bool AtEnd() const { return Enumerator::AtEnd(); }

    nsIFrame* PrevFrame() const { return mPrev; }
    nsIFrame* NextFrame() const { return mFrame; }

  protected:
    nsIFrame* mPrev;
  };

  class Iterator
  {
  public:
    Iterator(const nsFrameList& aList, nsIFrame* aCurrent)
      : mList(aList)
      , mCurrent(aCurrent)
    {}

    Iterator(const Iterator& aOther)
      : mList(aOther.mList)
      , mCurrent(aOther.mCurrent)
    {}

    nsIFrame* operator*() const { return mCurrent; }

    
    
    Iterator& operator++();
    Iterator& operator--();

    Iterator operator++(int) { auto ret = *this; ++*this; return ret; }
    Iterator operator--(int) { auto ret = *this; --*this; return ret; }

    friend bool operator==(const Iterator& aIter1, const Iterator& aIter2);
    friend bool operator!=(const Iterator& aIter1, const Iterator& aIter2);

  private:
    const nsFrameList& mList;
    nsIFrame* mCurrent;
  };

  typedef Iterator iterator;
  typedef Iterator const_iterator;
  typedef mozilla::ReverseIterator<Iterator> reverse_iterator;
  typedef mozilla::ReverseIterator<Iterator> const_reverse_iterator;

  iterator begin() const { return iterator(*this, mFirstChild); }
  const_iterator cbegin() const { return begin(); }
  iterator end() const { return iterator(*this, nullptr); }
  const_iterator cend() const { return end(); }
  reverse_iterator rbegin() const { return reverse_iterator(end()); }
  const_reverse_iterator crbegin() const { return rbegin(); }
  reverse_iterator rend() const { return reverse_iterator(begin()); }
  const_reverse_iterator crend() const { return rend(); }

private:
  void operator delete(void*) = delete;

#ifdef DEBUG_FRAME_LIST
  void VerifyList() const;
#else
  void VerifyList() const {}
#endif

protected:
  





  static void UnhookFrameFromSiblings(nsIFrame* aFrame);

  nsIFrame* mFirstChild;
  nsIFrame* mLastChild;
};

inline bool
operator==(const nsFrameList::Iterator& aIter1,
           const nsFrameList::Iterator& aIter2)
{
  MOZ_ASSERT(&aIter1.mList == &aIter2.mList,
             "must not compare iterator from different list");
  return aIter1.mCurrent == aIter2.mCurrent;
}

inline bool
operator!=(const nsFrameList::Iterator& aIter1,
           const nsFrameList::Iterator& aIter2)
{
  MOZ_ASSERT(&aIter1.mList == &aIter2.mList,
             "Must not compare iterator from different list");
  return aIter1.mCurrent != aIter2.mCurrent;
}

namespace mozilla {
namespace layout {






class AutoFrameListPtr {
public:
  AutoFrameListPtr(nsPresContext* aPresContext, nsFrameList* aFrameList)
    : mPresContext(aPresContext), mFrameList(aFrameList) {}
  ~AutoFrameListPtr();
  operator nsFrameList*() const { return mFrameList; }
  nsFrameList* operator->() const { return mFrameList; }
private:
  nsPresContext* mPresContext;
  nsFrameList* mFrameList;
};

namespace detail {
union AlignedFrameListBytes {
  void* ptr;
  char bytes[sizeof(nsFrameList)];
};
extern const AlignedFrameListBytes gEmptyFrameListBytes;
}
}
}

 inline const nsFrameList&
nsFrameList::EmptyList()
{
  return *reinterpret_cast<const nsFrameList*>(&mozilla::layout::detail::gEmptyFrameListBytes);
}

#endif 
