



#ifndef NSFRAMEITERATOR_H
#define NSFRAMEITERATOR_H

#include "prtypes.h"

class nsPresContext;
class nsIFrame;

enum nsIteratorType {
  eLeaf,
  ePreOrder,
  ePostOrder
};

class nsFrameIterator
{
public:
  nsFrameIterator(nsPresContext* aPresContext, nsIFrame *aStart,
                  nsIteratorType aType, uint32_t aFlags);

  ~nsFrameIterator() {}

  void First();
  void Next();
  nsIFrame* CurrentItem();
  bool IsDone();

  void Last();
  void Prev();

  enum FrameIteratorFlags {
    FLAG_NONE = 0,
    FLAG_LOCK_SCROLL = 1 << 1,
    FLAG_FOLLOW_OUT_OF_FLOW = 1 << 2,
    FLAG_VISUAL = 1 << 3
  };
protected:
  void      setCurrent(nsIFrame *aFrame){mCurrent = aFrame;}
  nsIFrame *getCurrent(){return mCurrent;}
  void      setStart(nsIFrame *aFrame){mStart = aFrame;}
  nsIFrame *getStart(){return mStart;}
  nsIFrame *getLast(){return mLast;}
  void      setLast(nsIFrame *aFrame){mLast = aFrame;}
  int8_t    getOffEdge(){return mOffEdge;}
  void      setOffEdge(int8_t aOffEdge){mOffEdge = aOffEdge;}
  void      SetLockInScrollView(bool aLockScroll){mLockScroll = aLockScroll;}

  

















  nsIFrame* GetParentFrame(nsIFrame* aFrame);
  
  nsIFrame* GetParentFrameNotPopup(nsIFrame* aFrame);

  nsIFrame* GetFirstChild(nsIFrame* aFrame);
  nsIFrame* GetLastChild(nsIFrame* aFrame);

  nsIFrame* GetNextSibling(nsIFrame* aFrame);
  nsIFrame* GetPrevSibling(nsIFrame* aFrame);

  






  nsIFrame* GetFirstChildInner(nsIFrame* aFrame);
  nsIFrame* GetLastChildInner(nsIFrame* aFrame);

  nsIFrame* GetNextSiblingInner(nsIFrame* aFrame);
  nsIFrame* GetPrevSiblingInner(nsIFrame* aFrame);

  nsIFrame* GetPlaceholderFrame(nsIFrame* aFrame);
  bool      IsPopupFrame(nsIFrame* aFrame);

  nsPresContext* mPresContext;
  nsIFrame *mStart;
  nsIFrame *mCurrent;
  nsIFrame *mLast; 
  int8_t    mOffEdge; 
  nsIteratorType mType;
  bool mLockScroll;
  bool mFollowOOFs;
  bool mVisual;
};

#endif 
