





#ifndef FrameChildList_h_
#define FrameChildList_h_

#include "nsFrameList.h"
#include "nsTArray.h"

class nsIFrame;

namespace mozilla {
namespace layout {



#ifdef DEBUG_FRAME_DUMP
extern const char* ChildListName(FrameChildListID aListID);
#endif

class FrameChildListIDs {
friend class FrameChildListIterator;
 public:
  FrameChildListIDs() : mIDs(0) {}
  FrameChildListIDs(const FrameChildListIDs& aOther) : mIDs(aOther.mIDs) {}
  MOZ_IMPLICIT FrameChildListIDs(FrameChildListID aListID) : mIDs(aListID) {}

  FrameChildListIDs operator|(FrameChildListIDs aOther) const {
    return FrameChildListIDs(mIDs | aOther.mIDs);
  }
  FrameChildListIDs& operator|=(FrameChildListIDs aOther) {
    mIDs |= aOther.mIDs;
    return *this;
  }
  bool operator==(FrameChildListIDs aOther) const {
    return mIDs == aOther.mIDs;
  }
  bool operator!=(FrameChildListIDs aOther) const {
    return !(*this == aOther);
  }
  bool Contains(FrameChildListIDs aOther) const {
    return (mIDs & aOther.mIDs) == aOther.mIDs;
  }

 protected:
  explicit FrameChildListIDs(uint32_t aIDs) : mIDs(aIDs) {}
  uint32_t mIDs;
};

class FrameChildList {
 public:
  FrameChildList(const nsFrameList& aList, FrameChildListID aID)
    : mList(aList), mID(aID) {}
  nsFrameList mList;
  FrameChildListID mID;
};




class MOZ_STACK_CLASS FrameChildListArrayIterator {
 public:
  explicit FrameChildListArrayIterator(const nsTArray<FrameChildList>& aLists)
    : mLists(aLists), mCurrentIndex(0) {}
  bool IsDone() const { return mCurrentIndex >= mLists.Length(); }
  FrameChildListID CurrentID() const {
    NS_ASSERTION(!IsDone(), "CurrentID(): iterator at end");
    return mLists[mCurrentIndex].mID;
  }
  const nsFrameList& CurrentList() const {
    NS_ASSERTION(!IsDone(), "CurrentList(): iterator at end");
    return mLists[mCurrentIndex].mList;
  }
  void Next() {
    NS_ASSERTION(!IsDone(), "Next(): iterator at end");
    ++mCurrentIndex;
  }

protected:
  const nsTArray<FrameChildList>& mLists;
  uint32_t mCurrentIndex;
};




class MOZ_STACK_CLASS FrameChildListIterator
  : public FrameChildListArrayIterator {
 public:
  explicit FrameChildListIterator(const nsIFrame* aFrame);

protected:
  nsAutoTArray<FrameChildList,4> mLists;
};

inline mozilla::layout::FrameChildListIDs
operator|(mozilla::layout::FrameChildListID aLeftOp,
          mozilla::layout::FrameChildListID aRightOp)
{
  return mozilla::layout::FrameChildListIDs(aLeftOp) |
         mozilla::layout::FrameChildListIDs(aRightOp);
}

inline mozilla::layout::FrameChildListIDs
operator|(mozilla::layout::FrameChildListID aLeftOp,
          mozilla::layout::FrameChildListIDs aRightOp)
{
  return mozilla::layout::FrameChildListIDs(aLeftOp) | aRightOp;
}

} 
} 

inline void nsFrameList::AppendIfNonempty(
         nsTArray<mozilla::layout::FrameChildList>* aLists,
         mozilla::layout::FrameChildListID aListID) const
{
  if (NotEmpty()) {
    aLists->AppendElement(mozilla::layout::FrameChildList(*this, aListID));
  }
}

#endif 
