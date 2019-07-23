







































#include "nsFloatManager.h"
#include "nsIPresShell.h"
#include "nsMemory.h"
#include "nsHTMLReflowState.h"
#include "nsHashSets.h"
#include "nsBlockDebugFlags.h"

PRInt32 nsFloatManager::sCachedFloatManagerCount = 0;
void* nsFloatManager::sCachedFloatManagers[NS_FLOAT_MANAGER_CACHE_SIZE];




static void*
PSArenaAllocCB(size_t aSize, void* aClosure)
{
  return static_cast<nsIPresShell*>(aClosure)->AllocateFrame(aSize);
}


static void
PSArenaFreeCB(size_t aSize, void* aPtr, void* aClosure)
{
  static_cast<nsIPresShell*>(aClosure)->FreeFrame(aSize, aPtr);
}




nsFloatManager::nsFloatManager(nsIPresShell* aPresShell)
  : mX(0), mY(0),
    mFloatDamage(PSArenaAllocCB, PSArenaFreeCB, aPresShell)
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
  
  

  PRInt32 i;

  for (i = 0; i < sCachedFloatManagerCount; i++) {
    void* floatManager = sCachedFloatManagers[i];
    if (floatManager)
      nsMemory::Free(floatManager);
  }

  
  sCachedFloatManagerCount = -1;
}

nsRect
nsFloatManager::GetBand(nscoord aYOffset,
                        nscoord aMaxHeight,
                        nscoord aContentAreaWidth,
                        PRBool* aHasFloats) const
{
  NS_ASSERTION(aMaxHeight >= 0, "unexpected max height");
  NS_ASSERTION(aContentAreaWidth >= 0, "unexpected content area width");

  nscoord top = aYOffset + mY;
  if (top < nscoord_MIN) {
    NS_NOTREACHED("bad value");
    top = nscoord_MIN;
  }

  
  
  PRUint32 floatCount = mFloats.Length();
  if (floatCount == 0 ||
      (mFloats[floatCount-1].mLeftYMost <= top &&
       mFloats[floatCount-1].mRightYMost <= top)) {
    *aHasFloats = PR_FALSE;
    return nsRect(0, aYOffset, aContentAreaWidth, aMaxHeight);
  }

  nscoord bottom;
  if (aMaxHeight == nscoord_MAX) {
    bottom = nscoord_MAX;
  } else {
    bottom = top + aMaxHeight;
    if (bottom < top || bottom > nscoord_MAX) {
      NS_NOTREACHED("bad value");
      bottom = nscoord_MAX;
    }
  }
  nscoord left = mX;
  nscoord right = aContentAreaWidth + mX;
  if (right < left) {
    NS_NOTREACHED("bad value");
    right = left;
  }

  
  
  PRBool haveFloats = PR_FALSE;
  for (PRUint32 i = mFloats.Length(); i > 0; --i) {
    const FloatInfo &fi = mFloats[i-1];
    if (fi.mLeftYMost <= top && fi.mRightYMost <= top) {
      
      break;
    }
    if (fi.mRect.IsEmpty()) {
      
      
      
      continue;
    }
    nscoord floatTop = fi.mRect.y, floatBottom = fi.mRect.YMost();
    if (floatTop > top) {
      
      if (floatTop < bottom) {
        bottom = floatTop;
      }
    } else if (floatBottom > top) {
      
      haveFloats = PR_TRUE;

      
      if (floatBottom < bottom) {
        bottom = floatBottom;
      }

      
      if (fi.mFrame->GetStyleDisplay()->mFloats == NS_STYLE_FLOAT_LEFT) {
        
        nscoord rightEdge = fi.mRect.XMost();
        if (rightEdge > left) {
          left = rightEdge;
        }
      } else {
        
        nscoord leftEdge = fi.mRect.x;
        if (leftEdge < right) {
          right = leftEdge;
        }
      }
    }
  }

  *aHasFloats = haveFloats;
  nscoord height = (bottom == nscoord_MAX) ? nscoord_MAX : (bottom - top);
  return nsRect(left - mX, top - mY, right - left, height);
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
  PRUint8 floatStyle = aFloatFrame->GetStyleDisplay()->mFloats;
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

nsresult
nsFloatManager::RemoveTrailingRegions(nsIFrame* aFrameList)
{
  if (!aFrameList) {
    return NS_OK;
  }
  
  
  
  
  nsVoidHashSet frameSet;

  frameSet.Init(1);
  for (nsIFrame* f = aFrameList; f; f = f->GetNextSibling()) {
    frameSet.Put(f);
  }

  PRUint32 newLength = mFloats.Length();
  while (newLength > 0) {
    if (!frameSet.Contains(mFloats[newLength - 1].mFrame)) {
      break;
    }
    --newLength;
  }
  mFloats.RemoveElementsAt(newLength, mFloats.Length() - newLength);

#ifdef DEBUG
  for (PRUint32 i = 0; i < mFloats.Length(); ++i) {
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
  aState->mFloatInfoCount = mFloats.Length();
}

void
nsFloatManager::PopState(SavedState* aState)
{
  NS_PRECONDITION(aState, "No state to restore?");

  mX = aState->mX;
  mY = aState->mY;

  NS_ASSERTION(aState->mFloatInfoCount <= mFloats.Length(),
               "somebody misused PushState/PopState");
  mFloats.RemoveElementsAt(aState->mFloatInfoCount,
                           mFloats.Length() - aState->mFloatInfoCount);
}

nscoord
nsFloatManager::GetLowestFloatTop() const
{
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

  for (PRUint32 i = 0; i < mFloats.Length(); ++i) {
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
nsFloatManager::ClearFloats(nscoord aY, PRUint8 aBreakType) const
{
  if (!HasAnyFloats()) {
    return aY;
  }

  nscoord bottom = aY + mY;

  const FloatInfo &tail = mFloats[mFloats.Length() - 1];
  switch (aBreakType) {
    case NS_STYLE_CLEAR_LEFT_AND_RIGHT:
      bottom = PR_MAX(bottom, tail.mLeftYMost);
      bottom = PR_MAX(bottom, tail.mRightYMost);
      break;
    case NS_STYLE_CLEAR_LEFT:
      bottom = PR_MAX(bottom, tail.mLeftYMost);
      break;
    case NS_STYLE_CLEAR_RIGHT:
      bottom = PR_MAX(bottom, tail.mRightYMost);
      break;
    default:
      
      break;
  }

  bottom -= mY;

  return bottom;
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
