






#include "nsFloatManager.h"
#include "nsIPresShell.h"
#include "nsMemory.h"
#include "nsHTMLReflowState.h"
#include "nsBlockDebugFlags.h"
#include "nsError.h"
#include <algorithm>

using namespace mozilla;

int32_t nsFloatManager::sCachedFloatManagerCount = 0;
void* nsFloatManager::sCachedFloatManagers[NS_FLOAT_MANAGER_CACHE_SIZE];




static void*
PSArenaAllocCB(size_t aSize, void* aClosure)
{
  return static_cast<nsIPresShell*>(aClosure)->AllocateMisc(aSize);
}


static void
PSArenaFreeCB(size_t aSize, void* aPtr, void* aClosure)
{
  static_cast<nsIPresShell*>(aClosure)->FreeMisc(aSize, aPtr);
}




nsFloatManager::nsFloatManager(nsIPresShell* aPresShell,
                               mozilla::WritingMode aWM)
  :
#ifdef DEBUG
    mWritingMode(aWM),
#endif
    mLineLeft(0), mBlockStart(0),
    mFloatDamage(PSArenaAllocCB, PSArenaFreeCB, aPresShell),
    mPushedLeftFloatPastBreak(false),
    mPushedRightFloatPastBreak(false),
    mSplitLeftFloatAcrossBreak(false),
    mSplitRightFloatAcrossBreak(false)
{
  MOZ_COUNT_CTOR(nsFloatManager);
}

nsFloatManager::~nsFloatManager()
{
  MOZ_COUNT_DTOR(nsFloatManager);
}


void* nsFloatManager::operator new(size_t aSize) CPP_THROW_NEW
{
  if (sCachedFloatManagerCount > 0) {
    
    
    return sCachedFloatManagers[--sCachedFloatManagerCount];
  }

  
  
  return moz_xmalloc(aSize);
}

void
nsFloatManager::operator delete(void* aPtr, size_t aSize)
{
  if (!aPtr)
    return;
  
  
  

  if (sCachedFloatManagerCount < NS_FLOAT_MANAGER_CACHE_SIZE &&
      sCachedFloatManagerCount >= 0) {
    
    

    sCachedFloatManagers[sCachedFloatManagerCount++] = aPtr;
    return;
  }

  
  
  free(aPtr);
}



void nsFloatManager::Shutdown()
{
  
  

  int32_t i;

  for (i = 0; i < sCachedFloatManagerCount; i++) {
    void* floatManager = sCachedFloatManagers[i];
    if (floatManager)
      free(floatManager);
  }

  
  sCachedFloatManagerCount = -1;
}

#define CHECK_BLOCK_DIR(aWM) \
  NS_ASSERTION(aWM.GetBlockDir() == mWritingMode.value.GetBlockDir(), \
  "incompatible writing modes")

nsFlowAreaRect
nsFloatManager::GetFlowArea(WritingMode aWM, nscoord aBOffset,
                            BandInfoType aInfoType, nscoord aBSize,
                            LogicalRect aContentArea, SavedState* aState,
                            nscoord aContainerWidth) const
{
  CHECK_BLOCK_DIR(aWM);
  NS_ASSERTION(aBSize >= 0, "unexpected max block size");
  NS_ASSERTION(aContentArea.ISize(aWM) >= 0,
               "unexpected content area inline size");

  nscoord blockStart = aBOffset + mBlockStart;
  if (blockStart < nscoord_MIN) {
    NS_WARNING("bad value");
    blockStart = nscoord_MIN;
  }

  
  uint32_t floatCount;
  if (aState) {
    
    floatCount = aState->mFloatInfoCount;
    MOZ_ASSERT(floatCount <= mFloats.Length(), "bad state");
  } else {
    
    floatCount = mFloats.Length();
  }

  
  
  if (floatCount == 0 ||
      (mFloats[floatCount-1].mLeftBEnd <= blockStart &&
       mFloats[floatCount-1].mRightBEnd <= blockStart)) {
    return nsFlowAreaRect(aWM, aContentArea.IStart(aWM), aBOffset,
                          aContentArea.ISize(aWM), aBSize, false);
  }

  nscoord blockEnd;
  if (aBSize == nscoord_MAX) {
    
    
    NS_WARN_IF_FALSE(aInfoType == BAND_FROM_POINT,
                     "bad height");
    blockEnd = nscoord_MAX;
  } else {
    blockEnd = blockStart + aBSize;
    if (blockEnd < blockStart || blockEnd > nscoord_MAX) {
      NS_WARNING("bad value");
      blockEnd = nscoord_MAX;
    }
  }
  nscoord lineLeft = mLineLeft + aContentArea.LineLeft(aWM, aContainerWidth);
  nscoord lineRight = mLineLeft + aContentArea.LineRight(aWM, aContainerWidth);
  if (lineRight < lineLeft) {
    NS_WARNING("bad value");
    lineRight = lineLeft;
  }

  
  
  bool haveFloats = false;
  for (uint32_t i = floatCount; i > 0; --i) {
    const FloatInfo &fi = mFloats[i-1];
    if (fi.mLeftBEnd <= blockStart && fi.mRightBEnd <= blockStart) {
      
      break;
    }
    if (fi.IsEmpty()) {
      
      
      
      continue;
    }

    nscoord floatBStart = fi.BStart();
    nscoord floatBEnd = fi.BEnd();
    if (blockStart < floatBStart && aInfoType == BAND_FROM_POINT) {
      
      if (floatBStart < blockEnd) {
        blockEnd = floatBStart;
      }
    }
    
    
    
    
    
    else if (blockStart < floatBEnd &&
             (floatBStart < blockEnd ||
              (floatBStart == blockEnd && blockStart == blockEnd))) {
      

      
      if (floatBEnd < blockEnd && aInfoType == BAND_FROM_POINT) {
        blockEnd = floatBEnd;
      }

      
      if (fi.mFrame->StyleDisplay()->mFloats == NS_STYLE_FLOAT_LEFT) {
        
        nscoord lineRightEdge = fi.LineRight();
        if (lineRightEdge > lineLeft) {
          lineLeft = lineRightEdge;
          
          
          
          
          haveFloats = true;
        }
      } else {
        
        nscoord lineLeftEdge = fi.LineLeft();
        if (lineLeftEdge < lineRight) {
          lineRight = lineLeftEdge;
          
          haveFloats = true;
        }
      }
    }
  }

  nscoord blockSize = (blockEnd == nscoord_MAX) ?
                       nscoord_MAX : (blockEnd - blockStart);
  
  nscoord inlineStart = aWM.IsVertical() || aWM.IsBidiLTR()
                         ? lineLeft - mLineLeft
                         : mLineLeft + aContainerWidth - lineRight;

  return nsFlowAreaRect(aWM, inlineStart, blockStart - mBlockStart,
                        lineRight - lineLeft, blockSize, haveFloats);
}

nsresult
nsFloatManager::AddFloat(nsIFrame* aFloatFrame, const LogicalRect& aMarginRect,
                         WritingMode aWM, nscoord aContainerWidth)
{
  CHECK_BLOCK_DIR(aWM);
  NS_ASSERTION(aMarginRect.ISize(aWM) >= 0, "negative inline size!");
  NS_ASSERTION(aMarginRect.BSize(aWM) >= 0, "negative block size!");

  FloatInfo info(aFloatFrame,
                 aMarginRect.LineLeft(aWM, aContainerWidth) + mLineLeft,
                 aMarginRect.BStart(aWM) + mBlockStart,
                 aMarginRect.ISize(aWM),
                 aMarginRect.BSize(aWM));

  
  if (HasAnyFloats()) {
    FloatInfo &tail = mFloats[mFloats.Length() - 1];
    info.mLeftBEnd = tail.mLeftBEnd;
    info.mRightBEnd = tail.mRightBEnd;
  } else {
    info.mLeftBEnd = nscoord_MIN;
    info.mRightBEnd = nscoord_MIN;
  }
  uint8_t floatStyle = aFloatFrame->StyleDisplay()->mFloats;
  NS_ASSERTION(floatStyle == NS_STYLE_FLOAT_LEFT ||
               floatStyle == NS_STYLE_FLOAT_RIGHT, "unexpected float");
  nscoord& sideBEnd = floatStyle == NS_STYLE_FLOAT_LEFT ? info.mLeftBEnd
                                                        : info.mRightBEnd;
  nscoord thisBEnd = info.BEnd();
  if (thisBEnd > sideBEnd)
    sideBEnd = thisBEnd;

  if (!mFloats.AppendElement(info))
    return NS_ERROR_OUT_OF_MEMORY;

  return NS_OK;
}


LogicalRect
nsFloatManager::CalculateRegionFor(WritingMode          aWM,
                                   nsIFrame*            aFloat,
                                   const LogicalMargin& aMargin,
                                   nscoord              aContainerWidth)
{
  
  LogicalRect region(aWM, nsRect(aFloat->GetNormalPosition(),
                                 aFloat->GetSize()),
                     aContainerWidth);

  
  region.Inflate(aWM, aMargin);

  
  
  if (region.ISize(aWM) < 0) {
    
    
    const nsStyleDisplay* display = aFloat->StyleDisplay();
    if ((NS_STYLE_FLOAT_LEFT == display->mFloats) == aWM.IsBidiLTR()) {
      region.IStart(aWM) = region.IEnd(aWM);
    }
    region.ISize(aWM) = 0;
  }
  if (region.BSize(aWM) < 0) {
    region.BSize(aWM) = 0;
  }
  return region;
}

NS_DECLARE_FRAME_PROPERTY(FloatRegionProperty, DeleteValue<nsMargin>)

LogicalRect
nsFloatManager::GetRegionFor(WritingMode aWM, nsIFrame* aFloat,
                             nscoord aContainerWidth)
{
  LogicalRect region = aFloat->GetLogicalRect(aWM, aContainerWidth);
  void* storedRegion = aFloat->Properties().Get(FloatRegionProperty());
  if (storedRegion) {
    nsMargin margin = *static_cast<nsMargin*>(storedRegion);
    region.Inflate(aWM, LogicalMargin(aWM, margin));
  }
  return region;
}

void
nsFloatManager::StoreRegionFor(WritingMode aWM, nsIFrame* aFloat,
                               const LogicalRect& aRegion,
                               nscoord aContainerWidth)
{
  nsRect region = aRegion.GetPhysicalRect(aWM, aContainerWidth);
  nsRect rect = aFloat->GetRect();
  FrameProperties props = aFloat->Properties();
  if (region.IsEqualEdges(rect)) {
    props.Delete(FloatRegionProperty());
  }
  else {
    nsMargin* storedMargin = static_cast<nsMargin*>
      (props.Get(FloatRegionProperty()));
    if (!storedMargin) {
      storedMargin = new nsMargin();
      props.Set(FloatRegionProperty(), storedMargin);
    }
    *storedMargin = region - rect;
  }
}

nsresult
nsFloatManager::RemoveTrailingRegions(nsIFrame* aFrameList)
{
  if (!aFrameList) {
    return NS_OK;
  }
  
  
  
  
  nsTHashtable<nsPtrHashKey<nsIFrame> > frameSet(1);

  for (nsIFrame* f = aFrameList; f; f = f->GetNextSibling()) {
    frameSet.PutEntry(f);
  }

  uint32_t newLength = mFloats.Length();
  while (newLength > 0) {
    if (!frameSet.Contains(mFloats[newLength - 1].mFrame)) {
      break;
    }
    --newLength;
  }
  mFloats.TruncateLength(newLength);

#ifdef DEBUG
  for (uint32_t i = 0; i < mFloats.Length(); ++i) {
    NS_ASSERTION(!frameSet.Contains(mFloats[i].mFrame),
                 "Frame region deletion was requested but we couldn't delete it");
  }
#endif

  return NS_OK;
}

void
nsFloatManager::PushState(SavedState* aState)
{
  NS_PRECONDITION(aState, "Need a place to save state");

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  aState->mLineLeft = mLineLeft;
  aState->mBlockStart = mBlockStart;
  aState->mPushedLeftFloatPastBreak = mPushedLeftFloatPastBreak;
  aState->mPushedRightFloatPastBreak = mPushedRightFloatPastBreak;
  aState->mSplitLeftFloatAcrossBreak = mSplitLeftFloatAcrossBreak;
  aState->mSplitRightFloatAcrossBreak = mSplitRightFloatAcrossBreak;
  aState->mFloatInfoCount = mFloats.Length();
}

void
nsFloatManager::PopState(SavedState* aState)
{
  NS_PRECONDITION(aState, "No state to restore?");

  mLineLeft = aState->mLineLeft;
  mBlockStart = aState->mBlockStart;
  mPushedLeftFloatPastBreak = aState->mPushedLeftFloatPastBreak;
  mPushedRightFloatPastBreak = aState->mPushedRightFloatPastBreak;
  mSplitLeftFloatAcrossBreak = aState->mSplitLeftFloatAcrossBreak;
  mSplitRightFloatAcrossBreak = aState->mSplitRightFloatAcrossBreak;

  NS_ASSERTION(aState->mFloatInfoCount <= mFloats.Length(),
               "somebody misused PushState/PopState");
  mFloats.TruncateLength(aState->mFloatInfoCount);
}

nscoord
nsFloatManager::GetLowestFloatTop() const
{
  if (mPushedLeftFloatPastBreak || mPushedRightFloatPastBreak) {
    return nscoord_MAX;
  }
  if (!HasAnyFloats()) {
    return nscoord_MIN;
  }
  return mFloats[mFloats.Length() -1].BStart() - mBlockStart;
}

#ifdef DEBUG_FRAME_DUMP
void
DebugListFloatManager(const nsFloatManager *aFloatManager)
{
  aFloatManager->List(stdout);
}

nsresult
nsFloatManager::List(FILE* out) const
{
  if (!HasAnyFloats())
    return NS_OK;

  for (uint32_t i = 0; i < mFloats.Length(); ++i) {
    const FloatInfo &fi = mFloats[i];
    fprintf_stderr(out, "Float %u: frame=%p rect={%d,%d,%d,%d} ymost={l:%d, r:%d}\n",
                   i, static_cast<void*>(fi.mFrame),
                   fi.LineLeft(), fi.BStart(), fi.ISize(), fi.BSize(),
                   fi.mLeftBEnd, fi.mRightBEnd);
  }
  return NS_OK;
}
#endif

nscoord
nsFloatManager::ClearFloats(nscoord aBCoord, uint8_t aBreakType,
                            uint32_t aFlags) const
{
  if (!(aFlags & DONT_CLEAR_PUSHED_FLOATS) && ClearContinues(aBreakType)) {
    return nscoord_MAX;
  }
  if (!HasAnyFloats()) {
    return aBCoord;
  }

  nscoord blockEnd = aBCoord + mBlockStart;

  const FloatInfo &tail = mFloats[mFloats.Length() - 1];
  switch (aBreakType) {
    case NS_STYLE_CLEAR_BOTH:
      blockEnd = std::max(blockEnd, tail.mLeftBEnd);
      blockEnd = std::max(blockEnd, tail.mRightBEnd);
      break;
    case NS_STYLE_CLEAR_LEFT:
      blockEnd = std::max(blockEnd, tail.mLeftBEnd);
      break;
    case NS_STYLE_CLEAR_RIGHT:
      blockEnd = std::max(blockEnd, tail.mRightBEnd);
      break;
    default:
      
      break;
  }

  blockEnd -= mBlockStart;

  return blockEnd;
}

bool
nsFloatManager::ClearContinues(uint8_t aBreakType) const
{
  return ((mPushedLeftFloatPastBreak || mSplitLeftFloatAcrossBreak) &&
          (aBreakType == NS_STYLE_CLEAR_BOTH ||
           aBreakType == NS_STYLE_CLEAR_LEFT)) ||
         ((mPushedRightFloatPastBreak || mSplitRightFloatAcrossBreak) &&
          (aBreakType == NS_STYLE_CLEAR_BOTH ||
           aBreakType == NS_STYLE_CLEAR_RIGHT));
}




nsFloatManager::FloatInfo::FloatInfo(nsIFrame* aFrame,
                                     nscoord aLineLeft, nscoord aBStart,
                                     nscoord aISize, nscoord aBSize)
  : mFrame(aFrame)
  , mRect(aLineLeft, aBStart, aISize, aBSize)
{
  MOZ_COUNT_CTOR(nsFloatManager::FloatInfo);
}

#ifdef NS_BUILD_REFCNT_LOGGING
nsFloatManager::FloatInfo::FloatInfo(const FloatInfo& aOther)
  : mFrame(aOther.mFrame),
    mLeftBEnd(aOther.mLeftBEnd),
    mRightBEnd(aOther.mRightBEnd),
    mRect(aOther.mRect)
{
  MOZ_COUNT_CTOR(nsFloatManager::FloatInfo);
}

nsFloatManager::FloatInfo::~FloatInfo()
{
  MOZ_COUNT_DTOR(nsFloatManager::FloatInfo);
}
#endif



nsAutoFloatManager::~nsAutoFloatManager()
{
  
  if (mNew) {
#ifdef NOISY_FLOATMANAGER
    printf("restoring old float manager %p\n", mOld);
#endif

    mReflowState.mFloatManager = mOld;

#ifdef NOISY_FLOATMANAGER
    if (mOld) {
      static_cast<nsFrame *>(mReflowState.frame)->ListTag(stdout);
      printf(": space-manager %p after reflow\n", mOld);
      mOld->List(stdout);
    }
#endif

    delete mNew;
  }
}

nsresult
nsAutoFloatManager::CreateFloatManager(nsPresContext *aPresContext)
{
  
  
  
  mNew = new nsFloatManager(aPresContext->PresShell(),
                            mReflowState.GetWritingMode());
  if (! mNew)
    return NS_ERROR_OUT_OF_MEMORY;

#ifdef NOISY_FLOATMANAGER
  printf("constructed new float manager %p (replacing %p)\n",
         mNew, mReflowState.mFloatManager);
#endif

  
  mOld = mReflowState.mFloatManager;
  mReflowState.mFloatManager = mNew;
  return NS_OK;
}
