







#ifndef nsLineBox_h___
#define nsLineBox_h___

#include "mozilla/Attributes.h"
#include "mozilla/Likely.h"

#include "nsILineIterator.h"
#include "nsIFrame.h"
#include <algorithm>

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

  nsIFrame* mFloat;                     

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
  nsFloatCacheList() : mHead(nullptr) { }
#endif
  ~nsFloatCacheList();

  bool IsEmpty() const {
    return nullptr == mHead;
  }

  bool NotEmpty() const {
    return nullptr != mHead;
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
  nsFloatCacheFreeList() : mTail(nullptr) { }
  ~nsFloatCacheFreeList() { }
#endif

  
  bool IsEmpty() const {
    return nullptr == mHead;
  }

  nsFloatCache* Head() const {
    return mHead;
  }

  nsFloatCache* Tail() const {
    return mTail;
  }
  
  bool NotEmpty() const {
    return nullptr != mHead;
  }

  void DeleteAll();

  
  
  void Append(nsFloatCacheList& aList);

  void Append(nsFloatCache* aFloatCache);

  void Remove(nsFloatCache* aElement);

  
  
  nsFloatCache* Alloc(nsIFrame* aFloat);
  
protected:
  nsFloatCache* mTail;

  friend class nsFloatCacheList;
};



#define LINE_MAX_BREAK_TYPE  ((1 << 4) - 1)
#define LINE_MAX_CHILD_COUNT INT32_MAX









nsLineBox* NS_NewLineBox(nsIPresShell* aPresShell, nsIFrame* aFrame,
                         bool aIsBlock);




nsLineBox* NS_NewLineBox(nsIPresShell* aPresShell, nsLineBox* aFromLine,
                         nsIFrame* aFrame, int32_t aCount);

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







class nsLineBox MOZ_FINAL : public nsLineLink {
private:
  nsLineBox(nsIFrame* aFrame, int32_t aCount, bool aIsBlock);
  ~nsLineBox();
  
  
  
  void* operator new(size_t sz, nsIPresShell* aPresShell) CPP_THROW_NEW;
  void operator delete(void* aPtr, size_t sz) MOZ_DELETE;

public:
  
  friend nsLineBox* NS_NewLineBox(nsIPresShell* aPresShell, nsIFrame* aFrame,
                                  bool aIsBlock);
  friend nsLineBox* NS_NewLineBox(nsIPresShell* aPresShell, nsLineBox* aFromLine,
                                  nsIFrame* aFrame, int32_t aCount);
  void Destroy(nsIPresShell* aPresShell);

  
  bool IsBlock() const {
    return mFlags.mBlock;
  }
  bool IsInline() const {
    return 0 == mFlags.mBlock;
  }

  
  void MarkDirty() {
    mFlags.mDirty = 1;
  }
  void ClearDirty() {
    mFlags.mDirty = 0;
  }
  bool IsDirty() const {
    return mFlags.mDirty;
  }

  
  void MarkPreviousMarginDirty() {
    mFlags.mPreviousMarginDirty = 1;
  }
  void ClearPreviousMarginDirty() {
    mFlags.mPreviousMarginDirty = 0;
  }
  bool IsPreviousMarginDirty() const {
    return mFlags.mPreviousMarginDirty;
  }

  
  void SetHasClearance() {
    mFlags.mHasClearance = 1;
  }
  void ClearHasClearance() {
    mFlags.mHasClearance = 0;
  }
  bool HasClearance() const {
    return mFlags.mHasClearance;
  }

  
  void SetLineIsImpactedByFloat(bool aValue) {
    mFlags.mImpactedByFloat = aValue;
  }
  bool IsImpactedByFloat() const {
    return mFlags.mImpactedByFloat;
  }

  
  void SetLineWrapped(bool aOn) {
    mFlags.mLineWrapped = aOn;
  }
  bool IsLineWrapped() const {
    return mFlags.mLineWrapped;
  }

  
  void SetInvalidateTextRuns(bool aOn) {
    mFlags.mInvalidateTextRuns = aOn;
  }
  bool GetInvalidateTextRuns() const {
    return mFlags.mInvalidateTextRuns;
  }

  
  void DisableResizeReflowOptimization() {
    mFlags.mResizeReflowOptimizationDisabled = true;
  }
  void EnableResizeReflowOptimization() {
    mFlags.mResizeReflowOptimizationDisabled = false;
  }
  bool ResizeReflowOptimizationDisabled() const {
    return mFlags.mResizeReflowOptimizationDisabled;
  }

  
  void SetHasBullet() {
    mFlags.mHasBullet = true;
    InvalidateCachedIsEmpty();
  }
  void ClearHasBullet() {
    mFlags.mHasBullet = false;
    InvalidateCachedIsEmpty();
  }
  bool HasBullet() const {
    return mFlags.mHasBullet;
  }

  
  void SetHadFloatPushed() {
    mFlags.mHadFloatPushed = true;
  }
  void ClearHadFloatPushed() {
    mFlags.mHadFloatPushed = false;
  }
  bool HadFloatPushed() const {
    return mFlags.mHadFloatPushed;
  }

private:
  
  static const uint32_t kMinChildCountForHashtable = 200;

  





  void StealHashTableFrom(nsLineBox* aFromLine, uint32_t aFromLineNewCount);

  



  void NoteFramesMovedFrom(nsLineBox* aFromLine);

  void SwitchToHashtable()
  {
    MOZ_ASSERT(!mFlags.mHasHashedFrames);
    uint32_t count = GetChildCount();
    mFlags.mHasHashedFrames = 1;
    uint32_t minLength = std::max(kMinChildCountForHashtable,
                                  uint32_t(PL_DHASH_DEFAULT_INITIAL_LENGTH));
    mFrames = new nsTHashtable< nsPtrHashKey<nsIFrame> >(std::max(count, minLength));
    for (nsIFrame* f = mFirstChild; count-- > 0; f = f->GetNextSibling()) {
      mFrames->PutEntry(f);
    }
  }
  void SwitchToCounter() {
    MOZ_ASSERT(mFlags.mHasHashedFrames);
    uint32_t count = GetChildCount();
    delete mFrames;
    mFlags.mHasHashedFrames = 0;
    mChildCount = count;
  }

public:
  int32_t GetChildCount() const {
    return MOZ_UNLIKELY(mFlags.mHasHashedFrames) ? mFrames->Count() : mChildCount;
  }

  


  void NoteFrameAdded(nsIFrame* aFrame) {
    if (MOZ_UNLIKELY(mFlags.mHasHashedFrames)) {
      mFrames->PutEntry(aFrame);
    } else {
      if (++mChildCount >= kMinChildCountForHashtable) {
        SwitchToHashtable();
      }
    }
  }

  


  void NoteFrameRemoved(nsIFrame* aFrame) {
    MOZ_ASSERT(GetChildCount() > 0);
    if (MOZ_UNLIKELY(mFlags.mHasHashedFrames)) {
      mFrames->RemoveEntry(aFrame);
      if (mFrames->Count() < kMinChildCountForHashtable) {
        SwitchToCounter();
      }
    } else {
      --mChildCount;
    }
  }

  
  
  
  
  bool HasBreakBefore() const {
    return IsBlock() && NS_STYLE_CLEAR_NONE != mFlags.mBreakType;
  }
  void SetBreakTypeBefore(uint8_t aBreakType) {
    NS_ASSERTION(IsBlock(), "Only blocks have break-before");
    NS_ASSERTION(aBreakType == NS_STYLE_CLEAR_NONE ||
                 aBreakType == NS_STYLE_CLEAR_LEFT ||
                 aBreakType == NS_STYLE_CLEAR_RIGHT ||
                 aBreakType == NS_STYLE_CLEAR_BOTH,
                 "Only float break types are allowed before a line");
    mFlags.mBreakType = aBreakType;
  }
  uint8_t GetBreakTypeBefore() const {
    return IsBlock() ? mFlags.mBreakType : NS_STYLE_CLEAR_NONE;
  }

  bool HasBreakAfter() const {
    return !IsBlock() && NS_STYLE_CLEAR_NONE != mFlags.mBreakType;
  }
  void SetBreakTypeAfter(uint8_t aBreakType) {
    NS_ASSERTION(!IsBlock(), "Only inlines have break-after");
    NS_ASSERTION(aBreakType <= LINE_MAX_BREAK_TYPE, "bad break type");
    mFlags.mBreakType = aBreakType;
  }
  bool HasFloatBreakAfter() const {
    return !IsBlock() && (NS_STYLE_CLEAR_LEFT == mFlags.mBreakType ||
                          NS_STYLE_CLEAR_RIGHT == mFlags.mBreakType ||
                          NS_STYLE_CLEAR_BOTH == mFlags.mBreakType);
  }
  uint8_t GetBreakTypeAfter() const {
    return !IsBlock() ? mFlags.mBreakType : NS_STYLE_CLEAR_NONE;
  }

  
  nsCollapsingMargin GetCarriedOutBEndMargin() const;
  
  bool SetCarriedOutBEndMargin(nsCollapsingMargin aValue);

  
  bool HasFloats() const {
    return (IsInline() && mInlineData) && mInlineData->mFloats.NotEmpty();
  }
  nsFloatCache* GetFirstFloat();
  void FreeFloats(nsFloatCacheFreeList& aFreeList);
  void AppendFloats(nsFloatCacheFreeList& aFreeList);
  bool RemoveFloat(nsIFrame* aFrame);

  
  
  
  
  void SetOverflowAreas(const nsOverflowAreas& aOverflowAreas);
  nsRect GetOverflowArea(nsOverflowType aType) {
    return mData ? mData->mOverflowAreas.Overflow(aType) : GetPhysicalBounds();
  }
  nsOverflowAreas GetOverflowAreas() {
    if (mData) {
      return mData->mOverflowAreas;
    }
    nsRect bounds = GetPhysicalBounds();
    return nsOverflowAreas(bounds, bounds);
  }
  nsRect GetVisualOverflowArea()
    { return GetOverflowArea(eVisualOverflow); }
  nsRect GetScrollableOverflowArea()
    { return GetOverflowArea(eScrollableOverflow); }

  void SlideBy(nscoord aDBCoord, nscoord aContainerWidth) {
    NS_ASSERTION(aContainerWidth == mContainerWidth || mContainerWidth == -1,
                 "container width doesn't match");
    mContainerWidth = aContainerWidth;
    mBounds.BStart(mWritingMode) += aDBCoord;
    if (mData) {
      NS_FOR_FRAME_OVERFLOW_TYPES(otype) {
        mData->mOverflowAreas.Overflow(otype).y += aDBCoord;
      }
    }
  }

  void IndentBy(nscoord aDICoord, nscoord aContainerWidth) {
    NS_ASSERTION(aContainerWidth == mContainerWidth || mContainerWidth == -1,
                 "container width doesn't match");
    mContainerWidth = aContainerWidth;
    mBounds.IStart(mWritingMode) += aDICoord;
  }

  void ExpandBy(nscoord aDISize, nscoord aContainerWidth) {
    NS_ASSERTION(aContainerWidth == mContainerWidth || mContainerWidth == -1,
                 "container width doesn't match");
    mContainerWidth = aContainerWidth;
    mBounds.ISize(mWritingMode) += aDISize;
  }

  







  nscoord GetLogicalAscent() const { return mAscent; }
  void SetLogicalAscent(nscoord aAscent) { mAscent = aAscent; }

  nscoord BStart() const {
    return mBounds.BStart(mWritingMode);
  }
  nscoord BSize() const {
    return mBounds.BSize(mWritingMode);
  }
  nscoord BEnd() const {
    return mBounds.BEnd(mWritingMode);
  }
  nscoord IStart() const {
    return mBounds.IStart(mWritingMode);
  }
  nscoord ISize() const {
    return mBounds.ISize(mWritingMode);
  }
  nscoord IEnd() const {
    return mBounds.IEnd(mWritingMode);
  }
  void SetBoundsEmpty() {
    mBounds.IStart(mWritingMode) = 0;
    mBounds.ISize(mWritingMode) = 0;
    mBounds.BStart(mWritingMode) = 0;
    mBounds.BSize(mWritingMode) = 0;
  }

  static void DeleteLineList(nsPresContext* aPresContext, nsLineList& aLines,
                             nsIFrame* aDestructRoot, nsFrameList* aFrames);

  
  
  
  
  
  
  static bool RFindLineContaining(nsIFrame* aFrame,
                                    const nsLineList_iterator& aBegin,
                                    nsLineList_iterator& aEnd,
                                    nsIFrame* aLastFrameBeforeEnd,
                                    int32_t* aFrameIndexInLine);

#ifdef DEBUG_FRAME_DUMP
  char* StateToString(char* aBuf, int32_t aBufSize) const;

  void List(FILE* out, int32_t aIndent, uint32_t aFlags = 0) const;
  void List(FILE* out = stderr, const char* aPrefix = "", uint32_t aFlags = 0) const;
  nsIFrame* LastChild() const;
#endif

private:
  int32_t IndexOf(nsIFrame* aFrame) const;
public:

  bool Contains(nsIFrame* aFrame) const {
    return MOZ_UNLIKELY(mFlags.mHasHashedFrames) ? mFrames->Contains(aFrame)
                                                : IndexOf(aFrame) >= 0;
  }

  
  bool IsEmpty() const;

  
  
  
  bool CachedIsEmpty();

  void InvalidateCachedIsEmpty() {
    mFlags.mEmptyCacheValid = false;
  }

  
  bool IsValidCachedIsEmpty() {
    return mFlags.mEmptyCacheValid;
  }

#ifdef DEBUG
  static int32_t GetCtorCount();
#endif

  nsIFrame* mFirstChild;

  mozilla::WritingMode mWritingMode;
  nscoord mContainerWidth;
 private:
  mozilla::LogicalRect mBounds;
 public:
  const mozilla::LogicalRect& GetBounds() { return mBounds; }
  nsRect GetPhysicalBounds() const
  {
    if (mBounds.IsEmpty()) {
      return nsRect(0, 0, 0, 0);
    }

    NS_ASSERTION(mContainerWidth != -1, "mContainerWidth not initialized");
    return mBounds.GetPhysicalRect(mWritingMode, mContainerWidth);
  }
  void SetBounds(mozilla::WritingMode aWritingMode,
                 nscoord aIStart, nscoord aBStart,
                 nscoord aISize, nscoord aBSize,
                 nscoord aContainerWidth)
  {
    mWritingMode = aWritingMode;
    mContainerWidth = aContainerWidth;
    mBounds = mozilla::LogicalRect(aWritingMode, aIStart, aBStart,
                                   aISize, aBSize);
  }
  void SetBounds(mozilla::WritingMode aWritingMode,
                 nsRect aRect, nscoord aContainerWidth)
  {
    mWritingMode = aWritingMode;
    mContainerWidth = aContainerWidth;
    mBounds = mozilla::LogicalRect(aWritingMode, aRect, aContainerWidth);
  }

  
  union {
    nsTHashtable< nsPtrHashKey<nsIFrame> >* mFrames;
    uint32_t mChildCount;
  };

  struct FlagBits {
    uint32_t mDirty : 1;
    uint32_t mPreviousMarginDirty : 1;
    uint32_t mHasClearance : 1;
    uint32_t mBlock : 1;
    uint32_t mImpactedByFloat : 1;
    uint32_t mLineWrapped: 1;
    uint32_t mInvalidateTextRuns : 1;
    uint32_t mResizeReflowOptimizationDisabled: 1;  
    uint32_t mEmptyCacheValid: 1;
    uint32_t mEmptyCacheState: 1;
    
    
    uint32_t mHasBullet : 1;
    
    
    uint32_t mHadFloatPushed : 1;
    uint32_t mHasHashedFrames: 1;
    uint32_t mBreakType : 4;
  };

  struct ExtraData {
    explicit ExtraData(const nsRect& aBounds) : mOverflowAreas(aBounds, aBounds) {
    }
    nsOverflowAreas mOverflowAreas;
  };

  struct ExtraBlockData : public ExtraData {
    explicit ExtraBlockData(const nsRect& aBounds)
      : ExtraData(aBounds),
        mCarriedOutBEndMargin()
    {
    }
    nsCollapsingMargin mCarriedOutBEndMargin;
  };

  struct ExtraInlineData : public ExtraData {
    explicit ExtraInlineData(const nsRect& aBounds) : ExtraData(aBounds) {
    }
    nsFloatCacheList mFloats;
  };

protected:
  nscoord mAscent;           
  union {
    uint32_t mAllFlags;
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

    typedef uint32_t                    size_type;
    typedef int32_t                     difference_type;

    typedef nsLineLink                  link_type;

#ifdef DEBUG
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

    
    
    bool operator==(const iterator_self_type aOther) const
    {
      NS_ABORT_IF_FALSE(mListLink == aOther.mListLink, "comparing iterators over different lists");
      return mCurrent == aOther.mCurrent;
    }
    bool operator!=(const iterator_self_type aOther) const
    {
      NS_ABORT_IF_FALSE(mListLink == aOther.mListLink, "comparing iterators over different lists");
      return mCurrent != aOther.mCurrent;
    }
    bool operator==(const iterator_self_type aOther)
    {
      NS_ABORT_IF_FALSE(mListLink == aOther.mListLink, "comparing iterators over different lists");
      return mCurrent == aOther.mCurrent;
    }
    bool operator!=(const iterator_self_type aOther)
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

    typedef uint32_t                    size_type;
    typedef int32_t                     difference_type;

    typedef nsLineLink                  link_type;

#ifdef DEBUG
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

    
    
    bool operator==(const iterator_self_type aOther) const
    {
      NS_ASSERTION(mListLink == aOther.mListLink, "comparing iterators over different lists");
      return mCurrent == aOther.mCurrent;
    }
    bool operator!=(const iterator_self_type aOther) const
    {
      NS_ASSERTION(mListLink == aOther.mListLink, "comparing iterators over different lists");
      return mCurrent != aOther.mCurrent;
    }
    bool operator==(const iterator_self_type aOther)
    {
      NS_ASSERTION(mListLink == aOther.mListLink, "comparing iterators over different lists");
      return mCurrent == aOther.mCurrent;
    }
    bool operator!=(const iterator_self_type aOther)
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

    typedef uint32_t                    size_type;
    typedef int32_t                     difference_type;

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

    
    
    bool operator==(const iterator_self_type aOther) const
    {
      NS_ASSERTION(mListLink == aOther.mListLink, "comparing iterators over different lists");
      return mCurrent == aOther.mCurrent;
    }
    bool operator!=(const iterator_self_type aOther) const
    {
      NS_ASSERTION(mListLink == aOther.mListLink, "comparing iterators over different lists");
      return mCurrent != aOther.mCurrent;
    }
    bool operator==(const iterator_self_type aOther)
    {
      NS_ASSERTION(mListLink == aOther.mListLink, "comparing iterators over different lists");
      return mCurrent == aOther.mCurrent;
    }
    bool operator!=(const iterator_self_type aOther)
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

    typedef uint32_t                    size_type;
    typedef int32_t                     difference_type;

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

    
    
    bool operator==(const iterator_self_type aOther) const
    {
      NS_ASSERTION(mListLink == aOther.mListLink, "comparing iterators over different lists");
      return mCurrent == aOther.mCurrent;
    }
    bool operator!=(const iterator_self_type aOther) const
    {
      NS_ASSERTION(mListLink == aOther.mListLink, "comparing iterators over different lists");
      return mCurrent != aOther.mCurrent;
    }
    bool operator==(const iterator_self_type aOther)
    {
      NS_ASSERTION(mListLink == aOther.mListLink, "comparing iterators over different lists");
      return mCurrent == aOther.mCurrent;
    }
    bool operator!=(const iterator_self_type aOther)
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

  typedef uint32_t                    size_type;
  typedef int32_t                     difference_type;

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
      MOZ_COUNT_CTOR(nsLineList);
      clear();
    }

    ~nsLineList()
    {
      MOZ_COUNT_DTOR(nsLineList);
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

    reverse_iterator rbegin(nsLineBox* aLine)
    {
      reverse_iterator rv;
      rv.mCurrent = aLine;
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

    bool empty() const
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

      if (!empty()) {
        mLink._mNext->_mPrev = &mLink;
        mLink._mPrev->_mNext = &mLink;
      }

      if (!y.empty()) {
        y.mLink._mNext->_mPrev = &y.mLink;
        y.mLink._mPrev->_mNext = &y.mLink;
      }
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




class nsLineIterator MOZ_FINAL : public nsILineIterator
{
public:
  nsLineIterator();
  ~nsLineIterator();

  virtual void DisposeLineIterator() MOZ_OVERRIDE;

  virtual int32_t GetNumLines() MOZ_OVERRIDE;
  virtual bool GetDirection() MOZ_OVERRIDE;
  NS_IMETHOD GetLine(int32_t aLineNumber,
                     nsIFrame** aFirstFrameOnLine,
                     int32_t* aNumFramesOnLine,
                     nsRect& aLineBounds,
                     uint32_t* aLineFlags) MOZ_OVERRIDE;
  virtual int32_t FindLineContaining(nsIFrame* aFrame, int32_t aStartLine = 0) MOZ_OVERRIDE;
  NS_IMETHOD FindFrameAt(int32_t aLineNumber,
                         nscoord aX,
                         nsIFrame** aFrameFound,
                         bool* aXIsBeforeFirstFrame,
                         bool* aXIsAfterLastFrame) MOZ_OVERRIDE;

  NS_IMETHOD GetNextSiblingOnLine(nsIFrame*& aFrame, int32_t aLineNumber) MOZ_OVERRIDE;
  NS_IMETHOD CheckLineOrder(int32_t                  aLine,
                            bool                     *aIsReordered,
                            nsIFrame                 **aFirstVisual,
                            nsIFrame                 **aLastVisual) MOZ_OVERRIDE;
  nsresult Init(nsLineList& aLines, bool aRightToLeft);

private:
  nsLineBox* PrevLine() {
    if (0 == mIndex) {
      return nullptr;
    }
    return mLines[--mIndex];
  }

  nsLineBox* NextLine() {
    if (mIndex >= mNumLines - 1) {
      return nullptr;
    }
    return mLines[++mIndex];
  }

  nsLineBox* LineAt(int32_t aIndex) {
    if ((aIndex < 0) || (aIndex >= mNumLines)) {
      return nullptr;
    }
    return mLines[aIndex];
  }

  nsLineBox** mLines;
  int32_t mIndex;
  int32_t mNumLines;
  bool mRightToLeft;
};

#endif 
