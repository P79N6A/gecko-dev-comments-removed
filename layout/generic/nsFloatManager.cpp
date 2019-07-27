






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
  : mWritingMode(aWM),
    mOffset(aWM),
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

  
  
  return nsMemory::Alloc(aSize);
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

  
  
  nsMemory::Free(aPtr);
}



void nsFloatManager::Shutdown()
{
  
  

  int32_t i;

  for (i = 0; i < sCachedFloatManagerCount; i++) {
    void* floatManager = sCachedFloatManagers[i];
    if (floatManager)
      nsMemory::Free(floatManager);
  }

  
  sCachedFloatManagerCount = -1;
}

nsFlowAreaRect
nsFloatManager::GetFlowArea(WritingMode aWM, nscoord aBOffset,
                            BandInfoType aInfoType, nscoord aBSize,
                            LogicalRect aContentArea, SavedState* aState,
                            nscoord aContainerWidth) const
{
  NS_ASSERTION(aBSize >= 0, "unexpected max block size");
  NS_ASSERTION(aContentArea.ISize(aWM) >= 0,
               "unexpected content area inline size");

  LogicalPoint offset = mOffset.ConvertTo(aWM, mWritingMode, 0);
  nscoord blockStart = aBOffset + offset.B(aWM);
  if (blockStart < nscoord_MIN) {
    NS_WARNING("bad value");
    blockStart = nscoord_MIN;
  }

  
  uint32_t floatCount;
  if (aState) {
    
    floatCount = aState->mFloatInfoCount;
    NS_ABORT_IF_FALSE(floatCount <= mFloats.Length(), "bad state");
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
  nscoord inlineStart = offset.I(aWM) + aContentArea.IStart(aWM);
  nscoord inlineEnd = offset.I(aWM) + aContentArea.IEnd(aWM);
  if (inlineEnd < inlineStart) {
    NS_WARNING("bad value");
    inlineEnd = inlineStart;
  }

  
  
  bool haveFloats = false;
  for (uint32_t i = floatCount; i > 0; --i) {
    const FloatInfo &fi = mFloats[i-1];
    if (fi.mLeftBEnd <= blockStart && fi.mRightBEnd <= blockStart) {
      
      break;
    }
    if (fi.mRect.IsEmpty()) {
      
      
      
      continue;
    }

    LogicalRect rect = fi.mRect.ConvertTo(aWM, fi.mWritingMode,
                                          aContainerWidth);
    nscoord floatBStart = rect.BStart(aWM);
    nscoord floatBEnd = rect.BEnd(aWM);
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

      
      if ((fi.mFrame->StyleDisplay()->mFloats == NS_STYLE_FLOAT_LEFT) ==
          aWM.IsBidiLTR()) {
        
        nscoord inlineEndEdge = rect.IEnd(aWM);
        if (inlineEndEdge > inlineStart) {
          inlineStart = inlineEndEdge;
          
          
          
          
          haveFloats = true;
        }
      } else {
        
        nscoord inlineStartEdge = rect.IStart(aWM);
        if (inlineStartEdge < inlineEnd) {
          inlineEnd = inlineStartEdge;
          
          haveFloats = true;
        }
      }
    }
  }

  nscoord blockSize = (blockEnd == nscoord_MAX) ?
                       nscoord_MAX : (blockEnd - blockStart);
  return nsFlowAreaRect(aWM,
                        inlineStart - offset.I(aWM), blockStart - offset.B(aWM),
                        inlineEnd - inlineStart, blockSize, haveFloats);
}

nsresult
nsFloatManager::AddFloat(nsIFrame* aFloatFrame, const LogicalRect& aMarginRect,
                         WritingMode aWM, nscoord aContainerWidth)
{
  NS_ASSERTION(aMarginRect.ISize(aWM) >= 0, "negative inline size!");
  NS_ASSERTION(aMarginRect.BSize(aWM) >= 0, "negative block size!");

  FloatInfo info(aFloatFrame, aWM, aMarginRect + mOffset);

  
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
  nscoord& sideBEnd =
    ((floatStyle == NS_STYLE_FLOAT_LEFT) == aWM.IsBidiLTR()) ? info.mLeftBEnd
                                                             : info.mRightBEnd;
  nscoord thisBEnd = info.mRect.BEnd(aWM);
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

NS_DECLARE_FRAME_PROPERTY(FloatRegionProperty, nsIFrame::DestroyMargin)

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

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  aState->mWritingMode = mWritingMode;
  aState->mOffset = mOffset;
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

  mWritingMode = aState->mWritingMode;
  mOffset = aState->mOffset;
  mPushedLeftFloatPastBreak = aState->mPushedLeftFloatPastBreak;
  mPushedRightFloatPastBreak = aState->mPushedRightFloatPastBreak;
  mSplitLeftFloatAcrossBreak = aState->mSplitLeftFloatAcrossBreak;
  mSplitRightFloatAcrossBreak = aState->mSplitRightFloatAcrossBreak;

  NS_ASSERTION(aState->mFloatInfoCount <= mFloats.Length(),
               "somebody misused PushState/PopState");
  mFloats.TruncateLength(aState->mFloatInfoCount);
}

nscoord
nsFloatManager::GetLowestFloatTop(WritingMode aWM,
                                  nscoord aContainerWidth) const
{
  if (mPushedLeftFloatPastBreak || mPushedRightFloatPastBreak) {
    return nscoord_MAX;
  }
  if (!HasAnyFloats()) {
    return nscoord_MIN;
  }
  FloatInfo fi = mFloats[mFloats.Length() - 1];
  LogicalRect rect = fi.mRect.ConvertTo(aWM, fi.mWritingMode, aContainerWidth);
  LogicalPoint offset = mOffset.ConvertTo(aWM, mWritingMode, 0);

  return rect.BStart(aWM) - offset.B(aWM);
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
                   fi.mRect.IStart(fi.mWritingMode),
                   fi.mRect.BStart(fi.mWritingMode),
                   fi.mRect.ISize(fi.mWritingMode),
                   fi.mRect.BSize(fi.mWritingMode),
                   fi.mLeftBEnd, fi.mRightBEnd);
  }
  return NS_OK;
}
#endif

nscoord
nsFloatManager::ClearFloats(WritingMode aWM, nscoord aBCoord,
                            uint8_t aBreakType, nscoord aContainerWidth,
                            uint32_t aFlags) const
{
  if (!(aFlags & DONT_CLEAR_PUSHED_FLOATS) && ClearContinues(aBreakType)) {
    return nscoord_MAX;
  }
  if (!HasAnyFloats()) {
    return aBCoord;
  }

  LogicalPoint offset = mOffset.ConvertTo(aWM, mWritingMode, 0);
  nscoord blockEnd = aBCoord + offset.B(aWM);

  const FloatInfo &tail = mFloats[mFloats.Length() - 1];
  switch (aBreakType) {
    case NS_STYLE_CLEAR_BOTH:
      blockEnd = std::max(blockEnd, tail.mLeftBEnd);
      blockEnd = std::max(blockEnd, tail.mRightBEnd);
      break;
    case NS_STYLE_CLEAR_LEFT:
      blockEnd = std::max(blockEnd, aWM.IsBidiLTR() ? tail.mLeftBEnd
                                                    : tail.mRightBEnd);
      break;
    case NS_STYLE_CLEAR_RIGHT:
      blockEnd = std::max(blockEnd, aWM.IsBidiLTR() ? tail.mRightBEnd
                                                    : tail.mLeftBEnd);
      break;
    default:
      
      break;
  }

  blockEnd -= offset.B(aWM);

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




nsFloatManager::FloatInfo::FloatInfo(nsIFrame* aFrame, WritingMode aWM,
                                     const LogicalRect& aRect)
  : mFrame(aFrame), mRect(aRect), mWritingMode(aWM)
{
  MOZ_COUNT_CTOR(nsFloatManager::FloatInfo);
}

#ifdef NS_BUILD_REFCNT_LOGGING
nsFloatManager::FloatInfo::FloatInfo(const FloatInfo& aOther)
  : mFrame(aOther.mFrame),
    mRect(aOther.mRect),
    mWritingMode(aOther.mWritingMode),
    mLeftBEnd(aOther.mLeftBEnd),
    mRightBEnd(aOther.mRightBEnd)
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
