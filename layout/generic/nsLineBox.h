









































#ifndef nsLineBox_h___
#define nsLineBox_h___

#include "nsPlaceholderFrame.h"
#include "nsILineIterator.h"

class nsLineBox;
class nsFloatCache;
class nsFloatCacheList;
class nsFloatCacheFreeList;



class nsFloatCache {
public:
  nsFloatCache();
#ifdef NS_BUILD_REFCNT_LOGGING
  ~nsFloatCache();
#else
  ~nsFloatCache() { }
#endif

  nsFloatCache* Next() const { return mNext; }

  nsPlaceholderFrame* mPlaceholder;     

protected:
  nsFloatCache* mNext;

  friend class nsFloatCacheList;
  friend class nsFloatCacheFreeList;
};



class nsFloatCacheList {
public:
#ifdef NS_BUILD_REFCNT_LOGGING
  nsFloatCacheList();
#else
  nsFloatCacheList() : mHead(nsnull) { }
#endif
  ~nsFloatCacheList();

  PRBool IsEmpty() const {
    return nsnull == mHead;
  }

  PRBool NotEmpty() const {
    return nsnull != mHead;
  }

  nsFloatCache* Head() const {
    return mHead;
  }

  nsFloatCache* Tail() const;

  void DeleteAll();

  nsFloatCache* Find(nsIFrame* aOutOfFlowFrame);

  
  
  void Remove(nsFloatCache* aElement) { RemoveAndReturnPrev(aElement); }
  
  
  
  void Append(nsFloatCacheFreeList& aList);

protected:
  nsFloatCache* mHead;

  
  
  
  nsFloatCache* RemoveAndReturnPrev(nsFloatCache* aElement);
  
  friend class nsFloatCacheFreeList;
};




class nsFloatCacheFreeList : private nsFloatCacheList {
public:
#ifdef NS_BUILD_REFCNT_LOGGING
  nsFloatCacheFreeList();
  ~nsFloatCacheFreeList();
#else
  nsFloatCacheFreeList() : mTail(nsnull) { }
  ~nsFloatCacheFreeList() { }
#endif

  
  PRBool IsEmpty() const {
    return nsnull == mHead;
  }

  nsFloatCache* Head() const {
    return mHead;
  }

  nsFloatCache* Tail() const {
    return mTail;
  }
  
  PRBool NotEmpty() const {
    return nsnull != mHead;
  }

  void DeleteAll();

  
  
  void Append(nsFloatCacheList& aList);

  void Append(nsFloatCache* aFloatCache);

  void Remove(nsFloatCache* aElement);

  
  
  nsFloatCache* Alloc();
  
protected:
  nsFloatCache* mTail;

  friend class nsFloatCacheList;
};



#define LINE_MAX_BREAK_TYPE  ((1 << 4) - 1)
#define LINE_MAX_CHILD_COUNT ((1 << 20) - 1)

#if NS_STYLE_CLEAR_LAST_VALUE > 15
need to rearrange the mBits bitfield;
#endif


nsLineBox* NS_NewLineBox(nsIPresShell* aPresShell, nsIFrame* aFrame,
                         PRInt32 aCount, PRBool aIsBlock);

class nsLineList;




class nsLineList_iterator;
class nsLineList_const_iterator;
class nsLineList_reverse_iterator;
class nsLineList_const_reverse_iterator;








class nsLineLink {

  public:
    friend class nsLineList;
    friend class nsLineList_iterator;
    friend class nsLineList_reverse_iterator;
    friend class nsLineList_const_iterator;
    friend class nsLineList_const_reverse_iterator;

  private:
    nsLineLink *_mNext; 
    nsLineLink *_mPrev; 

};







class nsLineBox : public nsLineLink {
private:
  nsLineBox(nsIFrame* aFrame, PRInt32 aCount, PRBool aIsBlock);
  ~nsLineBox();
  
  
  
  void* operator new(size_t sz, nsIPresShell* aPresShell) CPP_THROW_NEW;
  void operator delete(void* aPtr, size_t sz);

public:
  
  friend nsLineBox* NS_NewLineBox(nsIPresShell* aPresShell, nsIFrame* aFrame,
                                  PRInt32 aCount, PRBool aIsBlock);

  void Destroy(nsIPresShell* aPresShell);

  
  PRBool IsBlock() const {
    return mFlags.mBlock;
  }
  PRBool IsInline() const {
    return 0 == mFlags.mBlock;
  }

  
  void MarkDirty() {
    mFlags.mDirty = 1;
  }
  void ClearDirty() {
    mFlags.mDirty = 0;
  }
  PRBool IsDirty() const {
    return mFlags.mDirty;
  }

  
  void MarkPreviousMarginDirty() {
    mFlags.mPreviousMarginDirty = 1;
  }
  void ClearPreviousMarginDirty() {
    mFlags.mPreviousMarginDirty = 0;
  }
  PRBool IsPreviousMarginDirty() const {
    return mFlags.mPreviousMarginDirty;
  }

  
  void SetHasClearance() {
    mFlags.mHasClearance = 1;
  }
  void ClearHasClearance() {
    mFlags.mHasClearance = 0;
  }
  PRBool HasClearance() const {
    return mFlags.mHasClearance;
  }

  
  void SetLineIsImpactedByFloat(PRBool aValue) {
    NS_ASSERTION((PR_FALSE==aValue || PR_TRUE==aValue), "somebody is playing fast and loose with bools and bits!");
    mFlags.mImpactedByFloat = aValue;
  }
  PRBool IsImpactedByFloat() const {
    return mFlags.mImpactedByFloat;
  }

  
  void SetLineWrapped(PRBool aOn) {
    NS_ASSERTION((PR_FALSE==aOn || PR_TRUE==aOn), "somebody is playing fast and loose with bools and bits!");
    mFlags.mLineWrapped = aOn;
  }
  PRBool IsLineWrapped() const {
    return mFlags.mLineWrapped;
  }

  
  void SetInvalidateTextRuns(PRBool aOn) {
    NS_ASSERTION((PR_FALSE==aOn || PR_TRUE==aOn), "somebody is playing fast and loose with bools and bits!");
    mFlags.mInvalidateTextRuns = aOn;
  }
  PRBool GetInvalidateTextRuns() const {
    return mFlags.mInvalidateTextRuns;
  }

  
  void DisableResizeReflowOptimization() {
    mFlags.mResizeReflowOptimizationDisabled = PR_TRUE;
  }
  void EnableResizeReflowOptimization() {
    mFlags.mResizeReflowOptimizationDisabled = PR_FALSE;
  }
  PRBool ResizeReflowOptimizationDisabled() const {
    return mFlags.mResizeReflowOptimizationDisabled;
  }

  
  void SetHasBullet() {
    mFlags.mHasBullet = PR_TRUE;
    InvalidateCachedIsEmpty();
  }
  void ClearHasBullet() {
    mFlags.mHasBullet = PR_FALSE;
    InvalidateCachedIsEmpty();
  }
  PRBool HasBullet() const {
    return mFlags.mHasBullet;
  }
  
  
  
  PRInt32 GetChildCount() const {
    return (PRInt32) mFlags.mChildCount;
  }
  void SetChildCount(PRInt32 aNewCount) {
    if (aNewCount < 0) {
      NS_WARNING("negative child count");
      aNewCount = 0;
    }
    if (aNewCount > LINE_MAX_CHILD_COUNT) {
      aNewCount = LINE_MAX_CHILD_COUNT;
    }
    mFlags.mChildCount = aNewCount;
  }

  
  
  
  
  PRBool HasBreakBefore() const {
    return IsBlock() && NS_STYLE_CLEAR_NONE != mFlags.mBreakType;
  }
  void SetBreakTypeBefore(PRUint8 aBreakType) {
    NS_ASSERTION(IsBlock(), "Only blocks have break-before");
    NS_ASSERTION(aBreakType <= NS_STYLE_CLEAR_LEFT_AND_RIGHT,
                 "Only float break types are allowed before a line");
    mFlags.mBreakType = aBreakType;
  }
  PRUint8 GetBreakTypeBefore() const {
    return IsBlock() ? mFlags.mBreakType : NS_STYLE_CLEAR_NONE;
  }

  PRBool HasBreakAfter() const {
    return !IsBlock() && NS_STYLE_CLEAR_NONE != mFlags.mBreakType;
  }
  void SetBreakTypeAfter(PRUint8 aBreakType) {
    NS_ASSERTION(!IsBlock(), "Only inlines have break-after");
    NS_ASSERTION(aBreakType <= LINE_MAX_BREAK_TYPE, "bad break type");
    mFlags.mBreakType = aBreakType;
  }
  PRBool HasFloatBreakAfter() const {
    return !IsBlock() && (NS_STYLE_CLEAR_LEFT == mFlags.mBreakType ||
                          NS_STYLE_CLEAR_RIGHT == mFlags.mBreakType ||
                          NS_STYLE_CLEAR_LEFT_AND_RIGHT == mFlags.mBreakType);
  }
  PRUint8 GetBreakTypeAfter() const {
    return !IsBlock() ? mFlags.mBreakType : NS_STYLE_CLEAR_NONE;
  }

  
  nsCollapsingMargin GetCarriedOutBottomMargin() const;
  
  PRBool SetCarriedOutBottomMargin(nsCollapsingMargin aValue);

  
  PRBool HasFloats() const {
    return (IsInline() && mInlineData) && mInlineData->mFloats.NotEmpty();
  }
  nsFloatCache* GetFirstFloat();
  void FreeFloats(nsFloatCacheFreeList& aFreeList);
  void AppendFloats(nsFloatCacheFreeList& aFreeList);
  PRBool RemoveFloat(nsIFrame* aFrame);

  
  
  
  
  void SetCombinedArea(const nsRect& aCombinedArea);
  nsRect GetCombinedArea() {
    return mData ? mData->mCombinedArea : mBounds;
  }
  PRBool CombinedAreaIntersects(const nsRect& aDamageRect) {
    nsRect* ca = (mData ? &mData->mCombinedArea : &mBounds);
    return !((ca->YMost() <= aDamageRect.y) ||
             (ca->y >= aDamageRect.YMost()));
  }

  void SlideBy(nscoord aDY) {
    mBounds.y += aDY;
    if (mData) {
      mData->mCombinedArea.y += aDY;
    }
  }

  







  nscoord GetAscent() const { return mAscent; }
  void SetAscent(nscoord aAscent) { mAscent = aAscent; }

  nscoord GetHeight() const {
    return mBounds.height;
  }

  static void DeleteLineList(nsPresContext* aPresContext, nsLineList& aLines);

  
  
  static nsLineBox* FindLineContaining(nsLineList& aLines, nsIFrame* aFrame,
                                       PRInt32* aFrameIndexInLine);

  
  
  
  static PRBool RFindLineContaining(nsIFrame* aFrame,
                                    const nsLineList_iterator& aBegin,
                                    nsLineList_iterator& aEnd,
                                    PRInt32* aFrameIndexInLine);

#ifdef DEBUG
  char* StateToString(char* aBuf, PRInt32 aBufSize) const;

  void List(FILE* out, PRInt32 aIndent) const;
#endif

  nsIFrame* LastChild() const;

  PRBool IsLastChild(nsIFrame* aFrame) const;

  PRInt32 IndexOf(nsIFrame* aFrame) const;

  PRBool Contains(nsIFrame* aFrame) const {
    return IndexOf(aFrame) >= 0;
  }

  
  PRBool IsEmpty() const;

  
  
  
  PRBool CachedIsEmpty();

  void InvalidateCachedIsEmpty() {
    mFlags.mEmptyCacheValid = PR_FALSE;
  }

  
  PRBool IsValidCachedIsEmpty() {
    return mFlags.mEmptyCacheValid;
  }

#ifdef DEBUG
  static PRInt32 GetCtorCount();
#endif

  nsIFrame* mFirstChild;

  nsRect mBounds;

  struct FlagBits {
    PRUint32 mDirty : 1;
    PRUint32 mPreviousMarginDirty : 1;
    PRUint32 mHasClearance : 1;
    PRUint32 mBlock : 1;
    PRUint32 mImpactedByFloat : 1;
    PRUint32 mLineWrapped: 1;
    PRUint32 mInvalidateTextRuns : 1;
    PRUint32 mResizeReflowOptimizationDisabled: 1;  
    PRUint32 mEmptyCacheValid: 1;
    PRUint32 mEmptyCacheState: 1;
    
    
    PRUint32 mHasBullet : 1;
    PRUint32 mBreakType : 4;

    PRUint32 mChildCount : 17;
  };

  struct ExtraData {
    ExtraData(const nsRect& aBounds) : mCombinedArea(aBounds) {
    }
    nsRect mCombinedArea;
  };

  struct ExtraBlockData : public ExtraData {
    ExtraBlockData(const nsRect& aBounds)
      : ExtraData(aBounds),
        mCarriedOutBottomMargin()
    {
    }
    nsCollapsingMargin mCarriedOutBottomMargin;
  };

  struct ExtraInlineData : public ExtraData {
    ExtraInlineData(const nsRect& aBounds) : ExtraData(aBounds) {
    }
    nsFloatCacheList mFloats;
  };

protected:
  nscoord mAscent;           
  union {
    PRUint32 mAllFlags;
    FlagBits mFlags;
  };

  union {
    ExtraData* mData;
    ExtraBlockData* mBlockData;
    ExtraInlineData* mInlineData;
  };

  void Cleanup();
  void MaybeFreeData();
};







 
class nsLineList_iterator {
  public:
    friend class nsLineList;
    friend class nsLineList_reverse_iterator;
    friend class nsLineList_const_iterator;
    friend class nsLineList_const_reverse_iterator;

    typedef nsLineList_iterator         iterator_self_type;
    typedef nsLineList_reverse_iterator iterator_reverse_type;

    typedef nsLineBox&                  reference;
    typedef const nsLineBox&            const_reference;

    typedef nsLineBox*                  pointer;
    typedef const nsLineBox*            const_pointer;

    typedef PRUint32                    size_type;
    typedef PRInt32                     difference_type;

    typedef nsLineLink                  link_type;

#ifdef NS_DEBUG
    nsLineList_iterator() { memset(&mCurrent, 0xcd, sizeof(mCurrent)); }
#else
    
#endif
    

    inline iterator_self_type&
        operator=(const iterator_self_type& aOther);
    inline iterator_self_type&
        operator=(const iterator_reverse_type& aOther);

    iterator_self_type& operator++()
    {
      mCurrent = mCurrent->_mNext;
      return *this;
    }

    iterator_self_type operator++(int)
    {
      iterator_self_type rv(*this);
      mCurrent = mCurrent->_mNext;
      return rv;
    }

    iterator_self_type& operator--()
    {
      mCurrent = mCurrent->_mPrev;
      return *this;
    }

    iterator_self_type operator--(int)
    {
      iterator_self_type rv(*this);
      mCurrent = mCurrent->_mPrev;
      return rv;
    }

    reference operator*()
    {
      NS_ABORT_IF_FALSE(mCurrent != mListLink, "running past end");
      return *static_cast<pointer>(mCurrent);
    }

    pointer operator->()
    {
      NS_ABORT_IF_FALSE(mCurrent != mListLink, "running past end");
      return static_cast<pointer>(mCurrent);
    }

    pointer get()
    {
      NS_ABORT_IF_FALSE(mCurrent != mListLink, "running past end");
      return static_cast<pointer>(mCurrent);
    }

    operator pointer()
    {
      NS_ABORT_IF_FALSE(mCurrent != mListLink, "running past end");
      return static_cast<pointer>(mCurrent);
    }

    const_reference operator*() const
    {
      NS_ABORT_IF_FALSE(mCurrent != mListLink, "running past end");
      return *static_cast<const_pointer>(mCurrent);
    }

    const_pointer operator->() const
    {
      NS_ABORT_IF_FALSE(mCurrent != mListLink, "running past end");
      return static_cast<const_pointer>(mCurrent);
    }

#ifndef __MWERKS__
    operator const_pointer() const
    {
      NS_ABORT_IF_FALSE(mCurrent != mListLink, "running past end");
      return static_cast<const_pointer>(mCurrent);
    }
#endif 

    iterator_self_type next()
    {
      iterator_self_type copy(*this);
      return ++copy;
    }

    const iterator_self_type next() const
    {
      iterator_self_type copy(*this);
      return ++copy;
    }

    iterator_self_type prev()
    {
      iterator_self_type copy(*this);
      return --copy;
    }

    const iterator_self_type prev() const
    {
      iterator_self_type copy(*this);
      return --copy;
    }

    
    
    PRBool operator==(const iterator_self_type aOther) const
    {
      NS_ABORT_IF_FALSE(mListLink == aOther.mListLink, "comparing iterators over different lists");
      return mCurrent == aOther.mCurrent;
    }
    PRBool operator!=(const iterator_self_type aOther) const
    {
      NS_ABORT_IF_FALSE(mListLink == aOther.mListLink, "comparing iterators over different lists");
      return mCurrent != aOther.mCurrent;
    }
    PRBool operator==(const iterator_self_type aOther)
    {
      NS_ABORT_IF_FALSE(mListLink == aOther.mListLink, "comparing iterators over different lists");
      return mCurrent == aOther.mCurrent;
    }
    PRBool operator!=(const iterator_self_type aOther)
    {
      NS_ABORT_IF_FALSE(mListLink == aOther.mListLink, "comparing iterators over different lists");
      return mCurrent != aOther.mCurrent;
    }

  private:
    link_type *mCurrent;
#ifdef DEBUG
    link_type *mListLink; 
#endif
};

class nsLineList_reverse_iterator {

  public:

    friend class nsLineList;
    friend class nsLineList_iterator;
    friend class nsLineList_const_iterator;
    friend class nsLineList_const_reverse_iterator;

    typedef nsLineList_reverse_iterator iterator_self_type;
    typedef nsLineList_iterator         iterator_reverse_type;

    typedef nsLineBox&                  reference;
    typedef const nsLineBox&            const_reference;

    typedef nsLineBox*                  pointer;
    typedef const nsLineBox*            const_pointer;

    typedef PRUint32                    size_type;
    typedef PRInt32                     difference_type;

    typedef nsLineLink                  link_type;

#ifdef NS_DEBUG
    nsLineList_reverse_iterator() { memset(&mCurrent, 0xcd, sizeof(mCurrent)); }
#else
    
#endif
    

    inline iterator_self_type&
        operator=(const iterator_reverse_type& aOther);
    inline iterator_self_type&
        operator=(const iterator_self_type& aOther);

    iterator_self_type& operator++()
    {
      mCurrent = mCurrent->_mPrev;
      return *this;
    }

    iterator_self_type operator++(int)
    {
      iterator_self_type rv(*this);
      mCurrent = mCurrent->_mPrev;
      return rv;
    }

    iterator_self_type& operator--()
    {
      mCurrent = mCurrent->_mNext;
      return *this;
    }

    iterator_self_type operator--(int)
    {
      iterator_self_type rv(*this);
      mCurrent = mCurrent->_mNext;
      return rv;
    }

    reference operator*()
    {
      NS_ABORT_IF_FALSE(mCurrent != mListLink, "running past end");
      return *static_cast<pointer>(mCurrent);
    }

    pointer operator->()
    {
      NS_ABORT_IF_FALSE(mCurrent != mListLink, "running past end");
      return static_cast<pointer>(mCurrent);
    }

    pointer get()
    {
      NS_ABORT_IF_FALSE(mCurrent != mListLink, "running past end");
      return static_cast<pointer>(mCurrent);
    }

    operator pointer()
    {
      NS_ABORT_IF_FALSE(mCurrent != mListLink, "running past end");
      return static_cast<pointer>(mCurrent);
    }

    const_reference operator*() const
    {
      NS_ABORT_IF_FALSE(mCurrent != mListLink, "running past end");
      return *static_cast<const_pointer>(mCurrent);
    }

    const_pointer operator->() const
    {
      NS_ABORT_IF_FALSE(mCurrent != mListLink, "running past end");
      return static_cast<const_pointer>(mCurrent);
    }

#ifndef __MWERKS__
    operator const_pointer() const
    {
      NS_ABORT_IF_FALSE(mCurrent != mListLink, "running past end");
      return static_cast<const_pointer>(mCurrent);
    }
#endif 

    
    
    PRBool operator==(const iterator_self_type aOther) const
    {
      NS_ASSERTION(mListLink == aOther.mListLink, "comparing iterators over different lists");
      return mCurrent == aOther.mCurrent;
    }
    PRBool operator!=(const iterator_self_type aOther) const
    {
      NS_ASSERTION(mListLink == aOther.mListLink, "comparing iterators over different lists");
      return mCurrent != aOther.mCurrent;
    }
    PRBool operator==(const iterator_self_type aOther)
    {
      NS_ASSERTION(mListLink == aOther.mListLink, "comparing iterators over different lists");
      return mCurrent == aOther.mCurrent;
    }
    PRBool operator!=(const iterator_self_type aOther)
    {
      NS_ASSERTION(mListLink == aOther.mListLink, "comparing iterators over different lists");
      return mCurrent != aOther.mCurrent;
    }

  private:
    link_type *mCurrent;
#ifdef DEBUG
    link_type *mListLink; 
#endif
};

class nsLineList_const_iterator {
  public:

    friend class nsLineList;
    friend class nsLineList_iterator;
    friend class nsLineList_reverse_iterator;
    friend class nsLineList_const_reverse_iterator;

    typedef nsLineList_const_iterator           iterator_self_type;
    typedef nsLineList_const_reverse_iterator   iterator_reverse_type;
    typedef nsLineList_iterator                 iterator_nonconst_type;
    typedef nsLineList_reverse_iterator         iterator_nonconst_reverse_type;

    typedef nsLineBox&                  reference;
    typedef const nsLineBox&            const_reference;

    typedef nsLineBox*                  pointer;
    typedef const nsLineBox*            const_pointer;

    typedef PRUint32                    size_type;
    typedef PRInt32                     difference_type;

    typedef nsLineLink                  link_type;

#ifdef DEBUG
    nsLineList_const_iterator() { memset(&mCurrent, 0xcd, sizeof(mCurrent)); }
#else
    
#endif
    

    inline iterator_self_type&
        operator=(const iterator_nonconst_type& aOther);
    inline iterator_self_type&
        operator=(const iterator_nonconst_reverse_type& aOther);
    inline iterator_self_type&
        operator=(const iterator_self_type& aOther);
    inline iterator_self_type&
        operator=(const iterator_reverse_type& aOther);

    iterator_self_type& operator++()
    {
      mCurrent = mCurrent->_mNext;
      return *this;
    }

    iterator_self_type operator++(int)
    {
      iterator_self_type rv(*this);
      mCurrent = mCurrent->_mNext;
      return rv;
    }

    iterator_self_type& operator--()
    {
      mCurrent = mCurrent->_mPrev;
      return *this;
    }

    iterator_self_type operator--(int)
    {
      iterator_self_type rv(*this);
      mCurrent = mCurrent->_mPrev;
      return rv;
    }

    const_reference operator*() const
    {
      NS_ABORT_IF_FALSE(mCurrent != mListLink, "running past end");
      return *static_cast<const_pointer>(mCurrent);
    }

    const_pointer operator->() const
    {
      NS_ABORT_IF_FALSE(mCurrent != mListLink, "running past end");
      return static_cast<const_pointer>(mCurrent);
    }

    const_pointer get() const
    {
      NS_ABORT_IF_FALSE(mCurrent != mListLink, "running past end");
      return static_cast<const_pointer>(mCurrent);
    }

#ifndef __MWERKS__
    operator const_pointer() const
    {
      NS_ABORT_IF_FALSE(mCurrent != mListLink, "running past end");
      return static_cast<const_pointer>(mCurrent);
    }
#endif 

    const iterator_self_type next() const
    {
      iterator_self_type copy(*this);
      return ++copy;
    }

    const iterator_self_type prev() const
    {
      iterator_self_type copy(*this);
      return --copy;
    }

    
    
    PRBool operator==(const iterator_self_type aOther) const
    {
      NS_ASSERTION(mListLink == aOther.mListLink, "comparing iterators over different lists");
      return mCurrent == aOther.mCurrent;
    }
    PRBool operator!=(const iterator_self_type aOther) const
    {
      NS_ASSERTION(mListLink == aOther.mListLink, "comparing iterators over different lists");
      return mCurrent != aOther.mCurrent;
    }
    PRBool operator==(const iterator_self_type aOther)
    {
      NS_ASSERTION(mListLink == aOther.mListLink, "comparing iterators over different lists");
      return mCurrent == aOther.mCurrent;
    }
    PRBool operator!=(const iterator_self_type aOther)
    {
      NS_ASSERTION(mListLink == aOther.mListLink, "comparing iterators over different lists");
      return mCurrent != aOther.mCurrent;
    }

  private:
    const link_type *mCurrent;
#ifdef DEBUG
    const link_type *mListLink; 
#endif
};

class nsLineList_const_reverse_iterator {
  public:

    friend class nsLineList;
    friend class nsLineList_iterator;
    friend class nsLineList_reverse_iterator;
    friend class nsLineList_const_iterator;

    typedef nsLineList_const_reverse_iterator   iterator_self_type;
    typedef nsLineList_const_iterator           iterator_reverse_type;
    typedef nsLineList_iterator                 iterator_nonconst_reverse_type;
    typedef nsLineList_reverse_iterator         iterator_nonconst_type;

    typedef nsLineBox&                  reference;
    typedef const nsLineBox&            const_reference;

    typedef nsLineBox*                  pointer;
    typedef const nsLineBox*            const_pointer;

    typedef PRUint32                    size_type;
    typedef PRInt32                     difference_type;

    typedef nsLineLink                  link_type;

#ifdef DEBUG
    nsLineList_const_reverse_iterator() { memset(&mCurrent, 0xcd, sizeof(mCurrent)); }
#else
    
#endif
    

    inline iterator_self_type&
        operator=(const iterator_nonconst_type& aOther);
    inline iterator_self_type&
        operator=(const iterator_nonconst_reverse_type& aOther);
    inline iterator_self_type&
        operator=(const iterator_self_type& aOther);
    inline iterator_self_type&
        operator=(const iterator_reverse_type& aOther);

    iterator_self_type& operator++()
    {
      mCurrent = mCurrent->_mPrev;
      return *this;
    }

    iterator_self_type operator++(int)
    {
      iterator_self_type rv(*this);
      mCurrent = mCurrent->_mPrev;
      return rv;
    }

    iterator_self_type& operator--()
    {
      mCurrent = mCurrent->_mNext;
      return *this;
    }

    iterator_self_type operator--(int)
    {
      iterator_self_type rv(*this);
      mCurrent = mCurrent->_mNext;
      return rv;
    }

    const_reference operator*() const
    {
      NS_ABORT_IF_FALSE(mCurrent != mListLink, "running past end");
      return *static_cast<const_pointer>(mCurrent);
    }

    const_pointer operator->() const
    {
      NS_ABORT_IF_FALSE(mCurrent != mListLink, "running past end");
      return static_cast<const_pointer>(mCurrent);
    }

    const_pointer get() const
    {
      NS_ABORT_IF_FALSE(mCurrent != mListLink, "running past end");
      return static_cast<const_pointer>(mCurrent);
    }

#ifndef __MWERKS__
    operator const_pointer() const
    {
      NS_ABORT_IF_FALSE(mCurrent != mListLink, "running past end");
      return static_cast<const_pointer>(mCurrent);
    }
#endif 

    
    
    PRBool operator==(const iterator_self_type aOther) const
    {
      NS_ASSERTION(mListLink == aOther.mListLink, "comparing iterators over different lists");
      return mCurrent == aOther.mCurrent;
    }
    PRBool operator!=(const iterator_self_type aOther) const
    {
      NS_ASSERTION(mListLink == aOther.mListLink, "comparing iterators over different lists");
      return mCurrent != aOther.mCurrent;
    }
    PRBool operator==(const iterator_self_type aOther)
    {
      NS_ASSERTION(mListLink == aOther.mListLink, "comparing iterators over different lists");
      return mCurrent == aOther.mCurrent;
    }
    PRBool operator!=(const iterator_self_type aOther)
    {
      NS_ASSERTION(mListLink == aOther.mListLink, "comparing iterators over different lists");
      return mCurrent != aOther.mCurrent;
    }


    const link_type *mCurrent;
#ifdef DEBUG
    const link_type *mListLink; 
#endif
};

class nsLineList {

  public:

  friend class nsLineList_iterator;
  friend class nsLineList_reverse_iterator;
  friend class nsLineList_const_iterator;
  friend class nsLineList_const_reverse_iterator;

  typedef PRUint32                    size_type;
  typedef PRInt32                     difference_type;

  typedef nsLineLink                  link_type;

  private:
    link_type mLink;

  public:
    typedef nsLineList                  self_type;

    typedef nsLineBox&                  reference;
    typedef const nsLineBox&            const_reference;

    typedef nsLineBox*                  pointer;
    typedef const nsLineBox*            const_pointer;

    typedef nsLineList_iterator         iterator;
    typedef nsLineList_reverse_iterator reverse_iterator;
    typedef nsLineList_const_iterator   const_iterator;
    typedef nsLineList_const_reverse_iterator const_reverse_iterator;

    nsLineList()
    {
      clear();
    }

    const_iterator begin() const
    {
      const_iterator rv;
      rv.mCurrent = mLink._mNext;
#ifdef DEBUG
      rv.mListLink = &mLink;
#endif
      return rv;
    }

    iterator begin()
    {
      iterator rv;
      rv.mCurrent = mLink._mNext;
#ifdef DEBUG
      rv.mListLink = &mLink;
#endif
      return rv;
    }

    iterator begin(nsLineBox* aLine)
    {
      iterator rv;
      rv.mCurrent = aLine;
#ifdef DEBUG
      rv.mListLink = &mLink;
#endif
      return rv;
    }

    const_iterator end() const
    {
      const_iterator rv;
      rv.mCurrent = &mLink;
#ifdef DEBUG
      rv.mListLink = &mLink;
#endif
      return rv;
    }

    iterator end()
    {
      iterator rv;
      rv.mCurrent = &mLink;
#ifdef DEBUG
      rv.mListLink = &mLink;
#endif
      return rv;
    }

    const_reverse_iterator rbegin() const
    {
      const_reverse_iterator rv;
      rv.mCurrent = mLink._mPrev;
#ifdef DEBUG
      rv.mListLink = &mLink;
#endif
      return rv;
    }

    reverse_iterator rbegin()
    {
      reverse_iterator rv;
      rv.mCurrent = mLink._mPrev;
#ifdef DEBUG
      rv.mListLink = &mLink;
#endif
      return rv;
    }

    const_reverse_iterator rend() const
    {
      const_reverse_iterator rv;
      rv.mCurrent = &mLink;
#ifdef DEBUG
      rv.mListLink = &mLink;
#endif
      return rv;
    }

    reverse_iterator rend()
    {
      reverse_iterator rv;
      rv.mCurrent = &mLink;
#ifdef DEBUG
      rv.mListLink = &mLink;
#endif
      return rv;
    }

    PRBool empty() const
    {
      return mLink._mNext == &mLink;
    }

    
    size_type size() const
    {
      size_type count = 0;
      for (const link_type *cur = mLink._mNext;
           cur != &mLink;
           cur = cur->_mNext)
      {
        ++count;
      }
      return count;
    }

    pointer front()
    {
      NS_ASSERTION(!empty(), "no element to return");
      return static_cast<pointer>(mLink._mNext);
    }

    const_pointer front() const
    {
      NS_ASSERTION(!empty(), "no element to return");
      return static_cast<const_pointer>(mLink._mNext);
    }

    pointer back()
    {
      NS_ASSERTION(!empty(), "no element to return");
      return static_cast<pointer>(mLink._mPrev);
    }

    const_pointer back() const
    {
      NS_ASSERTION(!empty(), "no element to return");
      return static_cast<const_pointer>(mLink._mPrev);
    }

    void push_front(pointer aNew)
    {
      aNew->_mNext = mLink._mNext;
      mLink._mNext->_mPrev = aNew;
      aNew->_mPrev = &mLink;
      mLink._mNext = aNew;
    }

    void pop_front()
        
    {
      NS_ASSERTION(!empty(), "no element to pop");
      link_type *newFirst = mLink._mNext->_mNext;
      newFirst->_mPrev = &mLink;
      
      
      mLink._mNext = newFirst;
    }

    void push_back(pointer aNew)
    {
      aNew->_mPrev = mLink._mPrev;
      mLink._mPrev->_mNext = aNew;
      aNew->_mNext = &mLink;
      mLink._mPrev = aNew;
    }

    void pop_back()
        
    {
      NS_ASSERTION(!empty(), "no element to pop");
      link_type *newLast = mLink._mPrev->_mPrev;
      newLast->_mNext = &mLink;
      
      
      mLink._mPrev = newLast;
    }

    
    iterator before_insert(iterator position, pointer x)
    {
      
      x->_mPrev = position.mCurrent->_mPrev;
      x->_mNext = position.mCurrent;
      position.mCurrent->_mPrev->_mNext = x;
      position.mCurrent->_mPrev = x;
      return --position;
    }

    
    iterator after_insert(iterator position, pointer x)
    {
      
      x->_mNext = position.mCurrent->_mNext;
      x->_mPrev = position.mCurrent;
      position.mCurrent->_mNext->_mPrev = x;
      position.mCurrent->_mNext = x;
      return ++position;
    }

    
    iterator erase(iterator position)
        
    {
      position->_mPrev->_mNext = position->_mNext;
      position->_mNext->_mPrev = position->_mPrev;
      return ++position;
    }

    void swap(self_type& y)
    {
      link_type tmp(y.mLink);
      y.mLink = mLink;
      mLink = tmp;
    }

    void clear()
        
    {
      mLink._mNext = &mLink;
      mLink._mPrev = &mLink;
    }

    
    void splice(iterator position, self_type& x)
    {
      
      position.mCurrent->_mPrev->_mNext = x.mLink._mNext;
      x.mLink._mNext->_mPrev = position.mCurrent->_mPrev;
      x.mLink._mPrev->_mNext = position.mCurrent;
      position.mCurrent->_mPrev = x.mLink._mPrev;
      x.clear();
    }

    
    
    void splice(iterator position, self_type& x, iterator i)
    {
      NS_ASSERTION(!x.empty(), "Can't insert from empty list.");
      NS_ASSERTION(position != i && position.mCurrent != i->_mNext,
                   "We don't check for this case.");

      
      i->_mPrev->_mNext = i->_mNext;
      i->_mNext->_mPrev = i->_mPrev;

      
      
      i->_mPrev = position.mCurrent->_mPrev;
      position.mCurrent->_mPrev->_mNext = i.get();

      
      i->_mNext = position.mCurrent;
      position.mCurrent->_mPrev = i.get();
    }

    
    
    void splice(iterator position, self_type& x, iterator first,
                iterator last)
    {
      NS_ASSERTION(!x.empty(), "Can't insert from empty list.");

      if (first == last)
        return;

      --last; 
      
      first->_mPrev->_mNext = last->_mNext;
      last->_mNext->_mPrev = first->_mPrev;

      
      
      first->_mPrev = position.mCurrent->_mPrev;
      position.mCurrent->_mPrev->_mNext = first.get();

      
      last->_mNext = position.mCurrent;
      position.mCurrent->_mPrev = last.get();
    }

};





#ifdef DEBUG

  
  
#define ASSIGN_FROM(other_)          \
  mCurrent = other_.mCurrent;        \
  mListLink = other_.mListLink;      \
  return *this;

#else 

#define ASSIGN_FROM(other_)          \
  mCurrent = other_.mCurrent;        \
  return *this;

#endif 

inline
nsLineList_iterator&
nsLineList_iterator::operator=(const nsLineList_iterator& aOther)
{
  ASSIGN_FROM(aOther)
}

inline
nsLineList_iterator&
nsLineList_iterator::operator=(const nsLineList_reverse_iterator& aOther)
{
  ASSIGN_FROM(aOther)
}

inline
nsLineList_reverse_iterator&
nsLineList_reverse_iterator::operator=(const nsLineList_iterator& aOther)
{
  ASSIGN_FROM(aOther)
}

inline
nsLineList_reverse_iterator&
nsLineList_reverse_iterator::operator=(const nsLineList_reverse_iterator& aOther)
{
  ASSIGN_FROM(aOther)
}

inline
nsLineList_const_iterator&
nsLineList_const_iterator::operator=(const nsLineList_iterator& aOther)
{
  ASSIGN_FROM(aOther)
}

inline
nsLineList_const_iterator&
nsLineList_const_iterator::operator=(const nsLineList_reverse_iterator& aOther)
{
  ASSIGN_FROM(aOther)
}

inline
nsLineList_const_iterator&
nsLineList_const_iterator::operator=(const nsLineList_const_iterator& aOther)
{
  ASSIGN_FROM(aOther)
}

inline
nsLineList_const_iterator&
nsLineList_const_iterator::operator=(const nsLineList_const_reverse_iterator& aOther)
{
  ASSIGN_FROM(aOther)
}

inline
nsLineList_const_reverse_iterator&
nsLineList_const_reverse_iterator::operator=(const nsLineList_iterator& aOther)
{
  ASSIGN_FROM(aOther)
}

inline
nsLineList_const_reverse_iterator&
nsLineList_const_reverse_iterator::operator=(const nsLineList_reverse_iterator& aOther)
{
  ASSIGN_FROM(aOther)
}

inline
nsLineList_const_reverse_iterator&
nsLineList_const_reverse_iterator::operator=(const nsLineList_const_iterator& aOther)
{
  ASSIGN_FROM(aOther)
}

inline
nsLineList_const_reverse_iterator&
nsLineList_const_reverse_iterator::operator=(const nsLineList_const_reverse_iterator& aOther)
{
  ASSIGN_FROM(aOther)
}




class NS_FINAL_CLASS nsLineIterator : public nsILineIterator
{
public:
  nsLineIterator();
  ~nsLineIterator();

  virtual void DisposeLineIterator();

  virtual PRInt32 GetNumLines();
  virtual PRBool GetDirection();
  NS_IMETHOD GetLine(PRInt32 aLineNumber,
                     nsIFrame** aFirstFrameOnLine,
                     PRInt32* aNumFramesOnLine,
                     nsRect& aLineBounds,
                     PRUint32* aLineFlags);
  virtual PRInt32 FindLineContaining(nsIFrame* aFrame);
  virtual PRInt32 FindLineAt(nscoord aY);
  NS_IMETHOD FindFrameAt(PRInt32 aLineNumber,
                         nscoord aX,
                         nsIFrame** aFrameFound,
                         PRBool* aXIsBeforeFirstFrame,
                         PRBool* aXIsAfterLastFrame);

  NS_IMETHOD GetNextSiblingOnLine(nsIFrame*& aFrame, PRInt32 aLineNumber);
#ifdef IBMBIDI
  NS_IMETHOD CheckLineOrder(PRInt32                  aLine,
                            PRBool                   *aIsReordered,
                            nsIFrame                 **aFirstVisual,
                            nsIFrame                 **aLastVisual);
#endif
  nsresult Init(nsLineList& aLines, PRBool aRightToLeft);

private:
  nsLineBox* PrevLine() {
    if (0 == mIndex) {
      return nsnull;
    }
    return mLines[--mIndex];
  }

  nsLineBox* NextLine() {
    if (mIndex >= mNumLines - 1) {
      return nsnull;
    }
    return mLines[++mIndex];
  }

  nsLineBox* LineAt(PRInt32 aIndex) {
    if ((aIndex < 0) || (aIndex >= mNumLines)) {
      return nsnull;
    }
    return mLines[aIndex];
  }

  nsLineBox** mLines;
  PRInt32 mIndex;
  PRInt32 mNumLines;
  PRPackedBool mRightToLeft;
};

#endif 
