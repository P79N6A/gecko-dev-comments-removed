






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




nsFloatManager::nsFloatManager(nsIPresShell* aPresShell)
  : mX(0), mY(0),
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
nsFloatManager::GetFlowArea(nscoord aYOffset, BandInfoType aInfoType,
                            nscoord aHeight, nsRect aContentArea,
                            SavedState* aState) const
{
  NS_ASSERTION(aHeight >= 0, "unexpected max height");
  NS_ASSERTION(aContentArea.width >= 0, "unexpected content area width");

  nscoord top = aYOffset + mY;
  if (top < nscoord_MIN) {
    NS_WARNING("bad value");
    top = nscoord_MIN;
  }

  
  uint32_t floatCount;
  if (aState) {
    
    floatCount = aState->mFloatInfoCount;
    NS_ABORT_IF_FALSE(floatCount <= mFloats.Length(), "bad state");
  } else {
    
    floatCount = mFloats.Length();
  }

  
  
  if (floatCount == 0 ||
      (mFloats[floatCount-1].mLeftYMost <= top &&
       mFloats[floatCount-1].mRightYMost <= top)) {
    return nsFlowAreaRect(aContentArea.x, aYOffset, aContentArea.width,
                          aHeight, false);
  }

  nscoord bottom;
  if (aHeight == nscoord_MAX) {
    
    
    NS_WARN_IF_FALSE(aInfoType == BAND_FROM_POINT,
                     "bad height");
    bottom = nscoord_MAX;
  } else {
    bottom = top + aHeight;
    if (bottom < top || bottom > nscoord_MAX) {
      NS_WARNING("bad value");
      bottom = nscoord_MAX;
    }
  }
  nscoord left = mX + aContentArea.x;
  nscoord right = mX + aContentArea.XMost();
  if (right < left) {
    NS_WARNING("bad value");
    right = left;
  }

  
  
  bool haveFloats = false;
  for (uint32_t i = floatCount; i > 0; --i) {
    const FloatInfo &fi = mFloats[i-1];
    if (fi.mLeftYMost <= top && fi.mRightYMost <= top) {
      
      break;
    }
    if (fi.mRect.IsEmpty()) {
      
      
      
      continue;
    }
    nscoord floatTop = fi.mRect.y, floatBottom = fi.mRect.YMost();
    if (top < floatTop && aInfoType == BAND_FROM_POINT) {
      
      if (floatTop < bottom) {
        bottom = floatTop;
      }
    }
    
    
    
    
    
    else if (top < floatBottom &&
             (floatTop < bottom || (floatTop == bottom && top == bottom))) {
      

      
      if (floatBottom < bottom && aInfoType == BAND_FROM_POINT) {
        bottom = floatBottom;
      }

      
      if (fi.mFrame->StyleDisplay()->mFloats == NS_STYLE_FLOAT_LEFT) {
        
        nscoord rightEdge = fi.mRect.XMost();
        if (rightEdge > left) {
          left = rightEdge;
          
          
          
          
          haveFloats = true;
        }
      } else {
        
        nscoord leftEdge = fi.mRect.x;
        if (leftEdge < right) {
          right = leftEdge;
          
          haveFloats = true;
        }
      }
    }
  }

  nscoord height = (bottom == nscoord_MAX) ? nscoord_MAX : (bottom - top);
  return nsFlowAreaRect(left - mX, top - mY, right - left, height, haveFloats);
}

nsresult
nsFloatManager::AddFloat(nsIFrame* aFloatFrame, const nsRect& aMarginRect)
{
  NS_ASSERTION(aMarginRect.width >= 0, "negative width!");
  NS_ASSERTION(aMarginRect.height >= 0, "negative height!");

  FloatInfo info(aFloatFrame, aMarginRect + nsPoint(mX, mY));

  
  if (HasAnyFloats()) {
    FloatInfo &tail = mFloats[mFloats.Length() - 1];
    info.mLeftYMost = tail.mLeftYMost;
    info.mRightYMost = tail.mRightYMost;
  } else {
    info.mLeftYMost = nscoord_MIN;
    info.mRightYMost = nscoord_MIN;
  }
  uint8_t floatStyle = aFloatFrame->StyleDisplay()->mFloats;
  NS_ASSERTION(floatStyle == NS_STYLE_FLOAT_LEFT ||
               floatStyle == NS_STYLE_FLOAT_RIGHT, "unexpected float");
  nscoord& sideYMost = (floatStyle == NS_STYLE_FLOAT_LEFT) ? info.mLeftYMost
                                                           : info.mRightYMost;
  nscoord thisYMost = info.mRect.YMost();
  if (thisYMost > sideYMost)
    sideYMost = thisYMost;

  if (!mFloats.AppendElement(info))
    return NS_ERROR_OUT_OF_MEMORY;

  return NS_OK;
}

nsRect
nsFloatManager::CalculateRegionFor(nsIFrame*       aFloat,
                                   const nsMargin& aMargin)
{
  
  nsRect region(aFloat->GetNormalPosition(), aFloat->GetSize());

  
  region.Inflate(aMargin);

  
  
  if (region.width < 0) {
    
    
    const nsStyleDisplay* display = aFloat->StyleDisplay();
    if (NS_STYLE_FLOAT_LEFT == display->mFloats) {
      region.x = region.XMost();
    }
    region.width = 0;
  }
  if (region.height < 0) {
    region.height = 0;
  }
  return region;
}

NS_DECLARE_FRAME_PROPERTY(FloatRegionProperty, nsIFrame::DestroyMargin)

nsRect
nsFloatManager::GetRegionFor(nsIFrame* aFloat)
{
  nsRect region = aFloat->GetRect();
  void* storedRegion = aFloat->Properties().Get(FloatRegionProperty());
  if (storedRegion) {
    nsMargin margin = *static_cast<nsMargin*>(storedRegion);
    region.Inflate(margin);
  }
  return region;
}

void
nsFloatManager::StoreRegionFor(nsIFrame* aFloat,
                               nsRect&   aRegion)
{
  nsRect rect = aFloat->GetRect();
  FrameProperties props = aFloat->Properties();
  if (aRegion.IsEqualEdges(rect)) {
    props.Delete(FloatRegionProperty());
  }
  else {
    nsMargin* storedMargin = static_cast<nsMargin*>
      (props.Get(FloatRegionProperty()));
    if (!storedMargin) {
      storedMargin = new nsMargin();
      props.Set(FloatRegionProperty(), storedMargin);
    }
    *storedMargin = aRegion - rect;
  }
}

nsresult
nsFloatManager::RemoveTrailingRegions(nsIFrame* aFrameList)
{
  if (!aFrameList) {
    return NS_OK;
  }
  
  
  
  
  nsTHashtable<nsPtrHashKey<nsIFrame> > frameSet;

  frameSet.Init(1);
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

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  aState->mX = mX;
  aState->mY = mY;
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

  mX = aState->mX;
  mY = aState->mY;
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
  return mFloats[mFloats.Length() - 1].mRect.y - mY;
}

#ifdef DEBUG
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
    printf("Float %u: frame=%p rect={%d,%d,%d,%d} ymost={l:%d, r:%d}\n",
           i, static_cast<void*>(fi.mFrame),
           fi.mRect.x, fi.mRect.y, fi.mRect.width, fi.mRect.height,
           fi.mLeftYMost, fi.mRightYMost);
  }
  return NS_OK;
}
#endif

nscoord
nsFloatManager::ClearFloats(nscoord aY, uint8_t aBreakType,
                            uint32_t aFlags) const
{
  if (!(aFlags & DONT_CLEAR_PUSHED_FLOATS) && ClearContinues(aBreakType)) {
    return nscoord_MAX;
  }
  if (!HasAnyFloats()) {
    return aY;
  }

  nscoord bottom = aY + mY;

  const FloatInfo &tail = mFloats[mFloats.Length() - 1];
  switch (aBreakType) {
    case NS_STYLE_CLEAR_LEFT_AND_RIGHT:
      bottom = std::max(bottom, tail.mLeftYMost);
      bottom = std::max(bottom, tail.mRightYMost);
      break;
    case NS_STYLE_CLEAR_LEFT:
      bottom = std::max(bottom, tail.mLeftYMost);
      break;
    case NS_STYLE_CLEAR_RIGHT:
      bottom = std::max(bottom, tail.mRightYMost);
      break;
    default:
      
      break;
  }

  bottom -= mY;

  return bottom;
}

bool
nsFloatManager::ClearContinues(uint8_t aBreakType) const
{
  return ((mPushedLeftFloatPastBreak || mSplitLeftFloatAcrossBreak) &&
          (aBreakType == NS_STYLE_CLEAR_LEFT_AND_RIGHT ||
           aBreakType == NS_STYLE_CLEAR_LEFT)) ||
         ((mPushedRightFloatPastBreak || mSplitRightFloatAcrossBreak) &&
          (aBreakType == NS_STYLE_CLEAR_LEFT_AND_RIGHT ||
           aBreakType == NS_STYLE_CLEAR_RIGHT));
}




nsFloatManager::FloatInfo::FloatInfo(nsIFrame* aFrame, const nsRect& aRect)
  : mFrame(aFrame), mRect(aRect)
{
  MOZ_COUNT_CTOR(nsFloatManager::FloatInfo);
}

#ifdef NS_BUILD_REFCNT_LOGGING
nsFloatManager::FloatInfo::FloatInfo(const FloatInfo& aOther)
  : mFrame(aOther.mFrame),
    mRect(aOther.mRect),
    mLeftYMost(aOther.mLeftYMost),
    mRightYMost(aOther.mRightYMost)
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
  
  
  
  mNew = new nsFloatManager(aPresContext->PresShell());
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
