










































#include "nsSpaceManager.h"
#include "nsPoint.h"
#include "nsRect.h"
#include "nsSize.h"
#include <stdlib.h>
#include "nsVoidArray.h"
#include "nsIFrame.h"
#include "nsString.h"
#include "nsIPresShell.h"
#include "nsMemory.h"
#include "nsHTMLReflowState.h"
#include "nsHashSets.h"
#ifdef DEBUG
#include "nsIFrameDebug.h"
#endif




PRInt32 nsSpaceManager::sCachedSpaceManagerCount = 0;
void* nsSpaceManager::sCachedSpaceManagers[NS_SPACE_MANAGER_CACHE_SIZE];

#define NSCOORD_MIN (-2147483647 - 1) /* minimum signed value */

nsSpaceManager::BandList::BandList()
  : nsSpaceManager::BandRect(NSCOORD_MIN, NSCOORD_MIN, NSCOORD_MIN, NSCOORD_MIN, (nsIFrame*)nsnull)
{
  PR_INIT_CLIST(this);
  mNumFrames = 0;
}

void
nsSpaceManager::BandList::Clear()
{
  if (!IsEmpty()) {
    BandRect* bandRect = Head();
  
    while (bandRect != this) {
      BandRect* nxt = bandRect->Next();
  
      delete bandRect;
      bandRect = nxt;
    }
  
    PR_INIT_CLIST(this);
  }
}




PR_STATIC_CALLBACK(void*)
PSArenaAllocCB(size_t aSize, void* aClosure)
{
  return static_cast<nsIPresShell*>(aClosure)->AllocateFrame(aSize);
}


PR_STATIC_CALLBACK(void)
PSArenaFreeCB(size_t aSize, void* aPtr, void* aClosure)
{
  static_cast<nsIPresShell*>(aClosure)->FreeFrame(aSize, aPtr);
}




nsSpaceManager::nsSpaceManager(nsIPresShell* aPresShell, nsIFrame* aFrame)
  : mFrame(aFrame),
    mLowestTop(NSCOORD_MIN),
    mFloatDamage(PSArenaAllocCB, PSArenaFreeCB, aPresShell),
    mHaveCachedLeftYMost(PR_TRUE),
    mHaveCachedRightYMost(PR_TRUE),
    mMaximalLeftYMost(nscoord_MIN),
    mMaximalRightYMost(nscoord_MIN),
    mCachedBandPosition(nsnull)
{
  MOZ_COUNT_CTOR(nsSpaceManager);
  mX = mY = 0;
  mFrameInfoMap = nsnull;
}

void
nsSpaceManager::ClearFrameInfo()
{
  while (mFrameInfoMap) {
    FrameInfo*  next = mFrameInfoMap->mNext;
    delete mFrameInfoMap;
    mFrameInfoMap = next;
  }
}

nsSpaceManager::~nsSpaceManager()
{
  MOZ_COUNT_DTOR(nsSpaceManager);
  mBandList.Clear();
  ClearFrameInfo();
}


void* nsSpaceManager::operator new(size_t aSize) CPP_THROW_NEW
{
  if (sCachedSpaceManagerCount > 0) {
    
    
    return sCachedSpaceManagers[--sCachedSpaceManagerCount];
  }

  
  
  return nsMemory::Alloc(aSize);
}

void
nsSpaceManager::operator delete(void* aPtr, size_t aSize)
{
  if (!aPtr)
    return;
  
  
  

  if (sCachedSpaceManagerCount < NS_SPACE_MANAGER_CACHE_SIZE &&
      sCachedSpaceManagerCount >= 0) {
    
    

    sCachedSpaceManagers[sCachedSpaceManagerCount++] = aPtr;
    return;
  }

  
  
  nsMemory::Free(aPtr);
}



void nsSpaceManager::Shutdown()
{
  
  

  PRInt32 i;

  for (i = 0; i < sCachedSpaceManagerCount; i++) {
    void* spaceManager = sCachedSpaceManagers[i];
    if (spaceManager)
      nsMemory::Free(spaceManager);
  }

  
  sCachedSpaceManagerCount = -1;
}

PRBool
nsSpaceManager::XMost(nscoord& aXMost) const
{
  nscoord xMost = 0;
  for (FrameInfo* fi = mFrameInfoMap; fi; fi = fi->mNext) {
    xMost = PR_MAX(xMost, fi->mRect.XMost());
  }
  aXMost = xMost;
  return !mBandList.IsEmpty();
}

PRBool
nsSpaceManager::YMost(nscoord& aYMost) const
{
  PRBool result;

  if (mBandList.IsEmpty()) {
    aYMost = 0;
    result = PR_FALSE;

  } else {
    BandRect* lastRect = mBandList.Tail();

    aYMost = lastRect->mBottom;
    result = PR_TRUE;
  }

  return result;
}










nsresult
nsSpaceManager::GetBandAvailableSpace(const BandRect* aBand,
                                      nscoord         aY,
                                      const nsSize&   aMaxSize,
                                      nsBandData&     aBandData) const
{
  nscoord          topOfBand = aBand->mTop;
  nscoord          localY = aY - mY;
  nscoord          height = PR_MIN(aBand->mBottom - aY, aMaxSize.height);
  nsBandTrapezoid* trapezoid = aBandData.mTrapezoids;
  nscoord          rightEdge = mX + aMaxSize.width;

  
  aBandData.mCount = 0;

  
  while (aBand->mTop == topOfBand) {
    if (aBand->mRight > mX) {
      break;
    }

    
    aBand = aBand->Next();
  }

  
  
  nscoord   left = mX;

  
  while ((aBand->mTop == topOfBand) && (aBand->mLeft < rightEdge)) {
    
    
    if (aBand->mLeft > left) {
      
      
      if (aBandData.mCount >= aBandData.mSize) {
        
        aBandData.mCount += 2 * aBand->Length() + 2;  
        return NS_ERROR_FAILURE;
      }
      trapezoid->mState = nsBandTrapezoid::Available;
      trapezoid->mFrame = nsnull;

      
      
      *trapezoid = nsRect(left - mX, localY, aBand->mLeft - left, height);

      
      trapezoid++;
      aBandData.mCount++;
    }

    
    if (aBandData.mCount >= aBandData.mSize) {
      
      aBandData.mCount += 2 * aBand->Length() + 1;  
      return NS_ERROR_FAILURE;
    }
    if (1 == aBand->mNumFrames) {
      trapezoid->mState = nsBandTrapezoid::Occupied;
      trapezoid->mFrame = aBand->mFrame;
    } else {
      NS_ASSERTION(aBand->mNumFrames > 1, "unexpected frame count");
      trapezoid->mState = nsBandTrapezoid::OccupiedMultiple;
      trapezoid->mFrames = aBand->mFrames;
    }

    nscoord x = aBand->mLeft;
    
    if (x < mX) {
      
      x = mX;
    }

    
    
    *trapezoid = nsRect(x - mX, localY, aBand->mRight - x, height);

    
    trapezoid++;
    aBandData.mCount++;

    
    left = aBand->mRight;

    
    aBand = aBand->Next();
  }

  
  
  if (left < rightEdge) {
    if (aBandData.mCount >= aBandData.mSize) {
      
      aBandData.mCount++;
      return NS_ERROR_FAILURE;
    }
    trapezoid->mState = nsBandTrapezoid::Available;
    trapezoid->mFrame = nsnull;

    
    
    *trapezoid = nsRect(left - mX, localY, rightEdge - left, height);
    aBandData.mCount++;
  }

  return NS_OK;
}

nsresult
nsSpaceManager::GetBandData(nscoord       aYOffset,
                            const nsSize& aMaxSize,
                            nsBandData&   aBandData) const
{
  NS_PRECONDITION(aBandData.mSize >= 1, "bad band data");
  nsresult  result = NS_OK;

  
  nscoord   y = mY + aYOffset;

  
  
  nscoord yMost;
  nscoord maxHeight = aMaxSize.height == NS_UNCONSTRAINEDSIZE ? NS_UNCONSTRAINEDSIZE 
    : PR_MAX(0, aMaxSize.height - aYOffset);

  if (!YMost(yMost) || (y >= yMost)) {
    
    aBandData.mCount = 1;
    aBandData.mTrapezoids[0] = nsRect(0, aYOffset, aMaxSize.width, maxHeight);
    aBandData.mTrapezoids[0].mState = nsBandTrapezoid::Available;
    aBandData.mTrapezoids[0].mFrame = nsnull;
  } else {
    
    BandRect* band = GuessBandWithTopAbove(y);

    aBandData.mCount = 0;
    while (nsnull != band) {
      if (band->mTop > y) {
        
        
        aBandData.mCount = 1;
        aBandData.mTrapezoids[0] =
          nsRect(0, aYOffset, aMaxSize.width, PR_MIN(band->mTop - y, maxHeight));
        aBandData.mTrapezoids[0].mState = nsBandTrapezoid::Available;
        aBandData.mTrapezoids[0].mFrame = nsnull;
        break;
      } else if (y < band->mBottom) {
        
        
        return GetBandAvailableSpace(band, y, nsSize(aMaxSize.width, maxHeight), aBandData);
      } else {
        
        band = GetNextBand(band);
      }
    }
  }

  NS_POSTCONDITION(aBandData.mCount > 0, "unexpected band data count");
  return result;
}







nsSpaceManager::BandRect*
nsSpaceManager::GetNextBand(const BandRect* aBandRect) const
{
  nscoord topOfBand = aBandRect->mTop;

  aBandRect = aBandRect->Next();
  while (aBandRect != &mBandList) {
    
    if (aBandRect->mTop != topOfBand) {
      
      return (BandRect*)aBandRect;
    }

    aBandRect = aBandRect->Next();
  }

  
  return nsnull;
}







nsSpaceManager::BandRect*
nsSpaceManager::GetPrevBand(const BandRect* aBandRect) const
{
  NS_ASSERTION(aBandRect->Prev() == &mBandList ||
               aBandRect->Prev()->mBottom <= aBandRect->mTop,
               "aBandRect should be first rect within its band");

  BandRect* prev = aBandRect->Prev();
  nscoord topOfBand = prev->mTop;

  while (prev != &mBandList) {
    
    if (prev->mTop != topOfBand) {
      
      return (BandRect*)aBandRect;
    }

    aBandRect = prev;
    prev = aBandRect->Prev();
  }

  
  return nsnull;
}








void
nsSpaceManager::DivideBand(BandRect* aBandRect, nscoord aBottom)
{
  NS_PRECONDITION(aBottom < aBandRect->mBottom, "bad height");
  nscoord   topOfBand = aBandRect->mTop;
  BandRect* nextBand = GetNextBand(aBandRect);

  if (nsnull == nextBand) {
    nextBand = (BandRect*)&mBandList;
  }

  while (topOfBand == aBandRect->mTop) {
    
    BandRect* bottomBandRect = aBandRect->SplitVertically(aBottom);

    
    nextBand->InsertBefore(bottomBandRect);

    
    aBandRect = aBandRect->Next();
  }
}

PRBool
nsSpaceManager::CanJoinBands(BandRect* aBand, BandRect* aPrevBand)
{
  PRBool  result;
  nscoord topOfBand = aBand->mTop;
  nscoord topOfPrevBand = aPrevBand->mTop;

  
  
  
  
  
  if (aPrevBand->mBottom == aBand->mTop) {
    
    while (PR_TRUE) {
      if ((aBand->mLeft != aPrevBand->mLeft) || (aBand->mRight != aPrevBand->mRight)) {
        
        result = PR_FALSE;
        break;
      }

      if (!aBand->HasSameFrameList(aPrevBand)) {
        
        result = PR_FALSE;
        break;
      }

      
      aBand = aBand->Next();
      aPrevBand = aPrevBand->Next();

      
      PRBool  endOfBand = aBand->mTop != topOfBand;
      PRBool  endOfPrevBand = aPrevBand->mTop != topOfPrevBand;

      if (endOfBand || endOfPrevBand) {
        result = endOfBand & endOfPrevBand;
        break;  
      }
    }

  } else {
    
    result = PR_FALSE;
  }

  return result;
}







PRBool
nsSpaceManager::JoinBands(BandRect* aBand, BandRect* aPrevBand)
{
  if (CanJoinBands(aBand, aPrevBand)) {
    BandRect* startOfNextBand = aBand;
    
    
    if (mCachedBandPosition == aPrevBand) {
      SetCachedBandPosition(startOfNextBand);
    }

    while (aPrevBand != startOfNextBand) {
      
      
      aBand->mTop = aPrevBand->mTop;
      aBand = aBand->Next();

      
      BandRect* next = aPrevBand->Next();

      NS_ASSERTION(mCachedBandPosition != aPrevBand,
                   "Removing mCachedBandPosition BandRect?");
      aPrevBand->Remove();
      delete aPrevBand;
      aPrevBand = next;
    }

    return PR_TRUE;
  }

  return PR_FALSE;
}







void
nsSpaceManager::AddRectToBand(BandRect* aBand, BandRect* aBandRect)
{
  NS_PRECONDITION((aBand->mTop == aBandRect->mTop) &&
                  (aBand->mBottom == aBandRect->mBottom), "bad band");
  NS_PRECONDITION(1 == aBandRect->mNumFrames, "shared band rect");
  nscoord topOfBand = aBand->mTop;

  
  do {
    
    
    if (aBandRect->mLeft < aBand->mLeft) {
      
      
      
      
      
      
      
      
      
      
      
      
      
      if (aBandRect->mRight <= aBand->mLeft) {
        
        
        aBand->InsertBefore(aBandRect);
        if (mCachedBandPosition == aBand) {
          SetCachedBandPosition(aBandRect);
        }
        return;
      }

      
      if (aBandRect->mRight > aBand->mRight) {
        
        
        BandRect* r1 = aBandRect->SplitHorizontally(aBand->mLeft);

        
        
        aBand->InsertBefore(aBandRect);
        if (mCachedBandPosition == aBand) {
          SetCachedBandPosition(aBandRect);
        }

        
        aBandRect = r1;

      } else {
        if (aBand->mRight > aBandRect->mRight) {
          
          
          BandRect* r1 = aBand->SplitHorizontally(aBandRect->mRight);

          
          aBand->InsertAfter(r1);
        }

        
        
        aBandRect->mRight = aBand->mLeft;
        aBand->InsertBefore(aBandRect);

        if (mCachedBandPosition == aBand) {
          SetCachedBandPosition(aBandRect);
        }

        
        aBand->AddFrame(aBandRect->mFrame);
        return;
      }
    }
      
    if (aBandRect->mLeft > aBand->mLeft) {
      
      
      
      
      
      
      
      
      
      
      
      
      if (aBandRect->mLeft >= aBand->mRight) {
        
        
        aBand = aBand->Next();
        continue;
      }

      
      
      BandRect* r1 = aBand->SplitHorizontally(aBandRect->mLeft);

      
      
      aBand->InsertAfter(r1);
      aBand = r1;
    }

    
    
    NS_ASSERTION(aBandRect->mLeft == aBand->mLeft, "unexpected rect");

    
    if (aBand->mRight > aBandRect->mRight) {
      
      
      
      BandRect* r1 = aBand->SplitHorizontally(aBandRect->mRight);

      
      aBand->InsertAfter(r1);

      
      aBand->AddFrame(aBandRect->mFrame);
      return;

    } else {
      
      aBand->AddFrame(aBandRect->mFrame);

      if (aBand->mRight == aBandRect->mRight) {
        
        
        delete aBandRect;
        return;
      } else {
        
        
        aBandRect->mLeft = aBand->mRight;
        aBand = aBand->Next();
        continue;
      }
    }
  } while (aBand->mTop == topOfBand);

  
  
  aBand->InsertBefore(aBandRect);
}




























void
nsSpaceManager::InsertBandRect(BandRect* aBandRect)
{
  
  
  nscoord yMost;
  if (!YMost(yMost) || (aBandRect->mTop >= yMost)) {
    mBandList.Append(aBandRect);
    SetCachedBandPosition(aBandRect);
    return;
  }

  
  
  
  BandRect* band = GuessBandWithTopAbove(aBandRect->mTop);

  while (nsnull != band) {
    
    if (aBandRect->mTop < band->mTop) {
      
      
      if (aBandRect->mBottom <= band->mTop) {
        
        
        band->InsertBefore(aBandRect);
        SetCachedBandPosition(aBandRect);
        break;  
      }

      
      
      BandRect* bandRect1 = new BandRect(aBandRect->mLeft, aBandRect->mTop,
                                         aBandRect->mRight, band->mTop,
                                         aBandRect->mFrame);

      
      band->InsertBefore(bandRect1);

      
      aBandRect->mTop = band->mTop;

    } else if (aBandRect->mTop > band->mTop) {
      
      
      if (aBandRect->mTop >= band->mBottom) {
        
        band = GetNextBand(band);
        continue;
      }

      
      
      DivideBand(band, aBandRect->mTop);

      
      band = GetNextBand(band);
    }

    
    NS_ASSERTION(aBandRect->mTop == band->mTop, "unexpected band");

    
    if (band->mBottom > aBandRect->mBottom) {
      
      
      DivideBand(band, aBandRect->mBottom);
    }

    if (aBandRect->mBottom == band->mBottom) {
      
      SetCachedBandPosition(band);  
      AddRectToBand(band, aBandRect);
      break;

    } else {
      
      
      BandRect* bandRect1 = new BandRect(aBandRect->mLeft, aBandRect->mTop,
                                         aBandRect->mRight, band->mBottom,
                                         aBandRect->mFrame);

      
      AddRectToBand(band, bandRect1);

      
      aBandRect->mTop = band->mBottom;

      
      band = GetNextBand(band);
      if (nsnull == band) {
        
        mBandList.Append(aBandRect);
        SetCachedBandPosition(aBandRect);
        break;
      }
    }
  }
}

nsresult
nsSpaceManager::AddRectRegion(nsIFrame* aFrame, const nsRect& aUnavailableSpace)
{
  NS_PRECONDITION(nsnull != aFrame, "null frame");

#ifdef DEBUG
  
  NS_ASSERTION(!GetFrameInfoFor(aFrame),
               "aFrame is already associated with a region");
#endif
  
  
  nsRect  rect(aUnavailableSpace.x + mX, aUnavailableSpace.y + mY,
               aUnavailableSpace.width, aUnavailableSpace.height);

  if (rect.y > mLowestTop)
    mLowestTop = rect.y;

  
  FrameInfo* frameInfo = CreateFrameInfo(aFrame, rect);
  if (nsnull == frameInfo) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  if (aUnavailableSpace.height <= 0)
    return NS_OK;

  
  BandRect* bandRect = new BandRect(rect.x, rect.y, rect.XMost(), rect.YMost(), aFrame);
  if (nsnull == bandRect) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  
  InsertBandRect(bandRect);
  return NS_OK;
}

nsresult
nsSpaceManager::RemoveTrailingRegions(nsIFrame* aFrameList) {
  nsVoidHashSet frameSet;

  frameSet.Init(1);
  for (nsIFrame* f = aFrameList; f; f = f->GetNextSibling()) {
    frameSet.Put(f);
  }

  
  
  while (mFrameInfoMap && frameSet.Contains(mFrameInfoMap->mFrame)) {
    RemoveRegion(mFrameInfoMap->mFrame);
  }

#ifdef DEBUG
  for (FrameInfo* frameInfo = mFrameInfoMap; frameInfo;
       frameInfo = frameInfo->mNext) {
    NS_ASSERTION(!frameSet.Contains(frameInfo->mFrame),
                 "Frame region deletion was requested but we couldn't delete it");
  }
#endif

  return NS_OK;
}

nsresult
nsSpaceManager::RemoveRegion(nsIFrame* aFrame)
{
  
  FrameInfo*  frameInfo = GetFrameInfoFor(aFrame);

  if (nsnull == frameInfo) {
    NS_WARNING("no region associated with aFrame");
    return NS_ERROR_INVALID_ARG;
  }

  if (frameInfo->mRect.height > 0) {
    NS_ASSERTION(!mBandList.IsEmpty(), "no bands");
    BandRect* band = mBandList.Head();
    BandRect* prevBand = nsnull;
    PRBool    prevFoundMatchingRect = PR_FALSE;

    
    while (nsnull != band) {
      BandRect* rect = band;
      BandRect* prevRect = nsnull;
      nscoord   topOfBand = band->mTop;
      PRBool    foundMatchingRect = PR_FALSE;
      PRBool    prevIsSharedRect = PR_FALSE;

      
      do {
        PRBool  isSharedRect = PR_FALSE;

        if (rect->IsOccupiedBy(aFrame)) {
          
          foundMatchingRect = PR_TRUE;

          if (rect->mNumFrames > 1) {
            
            rect->RemoveFrame(aFrame);

            
            
            isSharedRect = PR_TRUE;
          } else {
            
            BandRect* next = rect->Next();
            rect->Remove();
            if (rect == band) {
              
              if (topOfBand == next->mTop) {
                band = next;
              } else {
                band = nsnull;
              }
              if (mCachedBandPosition == rect) {
                SetCachedBandPosition(band);
              }                
            }
            delete rect;
            rect = next;

            
            prevRect = nsnull;
            prevIsSharedRect = PR_FALSE;
            continue;
          }
        }
           
        
        
        if (prevIsSharedRect || (isSharedRect && (nsnull != prevRect))) {
          NS_ASSERTION(nsnull != prevRect, "no previous rect");
          if ((prevRect->mRight == rect->mLeft) && (prevRect->HasSameFrameList(rect))) {
            
            rect->mLeft = prevRect->mLeft;
            prevRect->Remove();
            if (prevRect == band) {
              
              band = rect;
              if (mCachedBandPosition == prevRect) {
                SetCachedBandPosition(band);
              }
            }
            delete prevRect;
          }
        }

        
        prevRect = rect;
        prevIsSharedRect = isSharedRect;
        rect = rect->Next();
      } while (rect->mTop == topOfBand);

      if (nsnull != band) {
        
        
        if ((nsnull != prevBand) && (foundMatchingRect || prevFoundMatchingRect)) {
          
          JoinBands(band, prevBand);
        }
      }

      
      prevFoundMatchingRect = foundMatchingRect;
      prevBand = band;
      band = (rect == &mBandList) ? nsnull : rect;
      if (!mCachedBandPosition) {
        SetCachedBandPosition(band);
      }
    }
  }

  DestroyFrameInfo(frameInfo);
  return NS_OK;
}

void
nsSpaceManager::ClearRegions()
{
  ClearFrameInfo();
  mBandList.Clear();
  mLowestTop = NSCOORD_MIN;
  mHaveCachedLeftYMost = mHaveCachedRightYMost = PR_TRUE;
  mMaximalLeftYMost = mMaximalRightYMost = nscoord_MIN;
}

void
nsSpaceManager::PushState(SavedState* aState)
{
  NS_PRECONDITION(aState, "Need a place to save state");

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  aState->mX = mX;
  aState->mY = mY;
  aState->mLowestTop = mLowestTop;
  aState->mHaveCachedLeftYMost = mHaveCachedLeftYMost;
  aState->mHaveCachedRightYMost = mHaveCachedRightYMost;
  aState->mMaximalLeftYMost = mMaximalLeftYMost;
  aState->mMaximalRightYMost = mMaximalRightYMost;

  if (mFrameInfoMap) {
    aState->mLastFrame = mFrameInfoMap->mFrame;
  } else {
    aState->mLastFrame = nsnull;
  }
}

void
nsSpaceManager::PopState(SavedState* aState)
{
  NS_PRECONDITION(aState, "No state to restore?");

  
  
  
  

  
  
  
  mHaveCachedLeftYMost = mHaveCachedRightYMost = PR_FALSE;

  
  
  while (mFrameInfoMap && mFrameInfoMap->mFrame != aState->mLastFrame) {
    RemoveRegion(mFrameInfoMap->mFrame);
  }

  
  
  
  

  NS_ASSERTION(((aState->mLastFrame && mFrameInfoMap) ||
               (!aState->mLastFrame && !mFrameInfoMap)),
               "Unexpected outcome!");

  mX = aState->mX;
  mY = aState->mY;
  mLowestTop = aState->mLowestTop;
  mHaveCachedLeftYMost = aState->mHaveCachedLeftYMost;
  mHaveCachedRightYMost = aState->mHaveCachedRightYMost;
  mMaximalLeftYMost = aState->mMaximalLeftYMost;
  mMaximalRightYMost = aState->mMaximalRightYMost;
}

nscoord
nsSpaceManager::GetLowestRegionTop()
{
  if (mLowestTop == NSCOORD_MIN)
    return mLowestTop;
  return mLowestTop - mY;
}

#ifdef DEBUG
void
DebugListSpaceManager(nsSpaceManager *aSpaceManager)
{
  aSpaceManager->List(stdout);
}

nsresult
nsSpaceManager::List(FILE* out)
{
  nsAutoString tmp;

  fprintf(out, "SpaceManager@%p", this);
  if (mFrame) {
    nsIFrameDebug*  frameDebug;

    if (NS_SUCCEEDED(mFrame->QueryInterface(NS_GET_IID(nsIFrameDebug), (void**)&frameDebug))) {
      frameDebug->GetFrameName(tmp);
      fprintf(out, " frame=");
      fputs(NS_LossyConvertUTF16toASCII(tmp).get(), out);
      fprintf(out, "@%p", mFrame);
    }
  }
  fprintf(out, " xy=%d,%d <\n", mX, mY);
  if (mBandList.IsEmpty()) {
    fprintf(out, "  no bands\n");
  }
  else {
    BandRect* band = mBandList.Head();
    do {
      fprintf(out, "  left=%d top=%d right=%d bottom=%d numFrames=%d",
              band->mLeft, band->mTop, band->mRight, band->mBottom,
              band->mNumFrames);
      if (1 == band->mNumFrames) {
        nsIFrameDebug*  frameDebug;

        if (NS_SUCCEEDED(band->mFrame->QueryInterface(NS_GET_IID(nsIFrameDebug), (void**)&frameDebug))) {
          frameDebug->GetFrameName(tmp);
          fprintf(out, " frame=");
          fputs(NS_LossyConvertUTF16toASCII(tmp).get(), out);
          fprintf(out, "@%p", band->mFrame);
        }
      }
      else if (1 < band->mNumFrames) {
        fprintf(out, "\n    ");
        nsVoidArray* a = band->mFrames;
        PRInt32 i, n = a->Count();
        for (i = 0; i < n; i++) {
          nsIFrame* frame = (nsIFrame*) a->ElementAt(i);
          if (frame) {
            nsIFrameDebug*  frameDebug;

            if (NS_SUCCEEDED(frame->QueryInterface(NS_GET_IID(nsIFrameDebug), (void**)&frameDebug))) {
              frameDebug->GetFrameName(tmp);
              fputs(NS_LossyConvertUTF16toASCII(tmp).get(), out);
              fprintf(out, "@%p ", frame);
            }
          }
        }
      }
      fprintf(out, "\n");
      band = band->Next();
    } while (band != mBandList.Head());
  }
  fprintf(out, ">\n");
  return NS_OK;
}
#endif

nsSpaceManager::FrameInfo*
nsSpaceManager::GetFrameInfoFor(nsIFrame* aFrame)
{
  FrameInfo*  result = nsnull;

  for (result = mFrameInfoMap; result; result = result->mNext) {
    if (result->mFrame == aFrame) {
      break;
    }
  }

  return result;
}

nsSpaceManager::FrameInfo*
nsSpaceManager::CreateFrameInfo(nsIFrame* aFrame, const nsRect& aRect)
{
  FrameInfo*  frameInfo = new FrameInfo(aFrame, aRect);

  if (frameInfo) {
    
    frameInfo->mNext = mFrameInfoMap;
    mFrameInfoMap = frameInfo;

    
    
    nscoord ymost = aRect.YMost();
    PRUint8 floatType = aFrame->GetStyleDisplay()->mFloats;
    if (mHaveCachedLeftYMost && ymost > mMaximalLeftYMost &&
        floatType == NS_STYLE_FLOAT_LEFT) {
      mMaximalLeftYMost = ymost;
    }
    else if (mHaveCachedRightYMost && ymost > mMaximalRightYMost &&
             floatType == NS_STYLE_FLOAT_RIGHT) {
      mMaximalRightYMost = ymost;
    }
  }
  return frameInfo;
}

void
nsSpaceManager::DestroyFrameInfo(FrameInfo* aFrameInfo)
{
  
  if (mFrameInfoMap == aFrameInfo) {
    mFrameInfoMap = aFrameInfo->mNext;

  } else {
    FrameInfo*  prev;
    
    
    for (prev = mFrameInfoMap; prev && (prev->mNext != aFrameInfo); prev = prev->mNext) {
      ;
    }

    
    NS_ASSERTION(prev, "element not in list");
    if (prev) {
      prev->mNext = aFrameInfo->mNext;
    }
  }

  
  
  
  if (mHaveCachedLeftYMost || mHaveCachedRightYMost) {
    PRUint8 floatType = aFrameInfo->mFrame->GetStyleDisplay()->mFloats;
    if (floatType == NS_STYLE_FLOAT_LEFT) {
      mHaveCachedLeftYMost = PR_FALSE;
    }
    else {
      NS_ASSERTION(floatType == NS_STYLE_FLOAT_RIGHT, "Unexpected float type");
      mHaveCachedRightYMost = PR_FALSE;
    }
  }

  delete aFrameInfo;
}

nscoord
nsSpaceManager::ClearFloats(nscoord aY, PRUint8 aBreakType)
{
  nscoord bottom = aY + mY;

  if ((!mHaveCachedLeftYMost && aBreakType != NS_STYLE_CLEAR_RIGHT) ||
      (!mHaveCachedRightYMost && aBreakType != NS_STYLE_CLEAR_LEFT)) {
    
    
    nscoord maximalLeftYMost = mHaveCachedLeftYMost ? mMaximalLeftYMost : nscoord_MIN;
    nscoord maximalRightYMost = mHaveCachedRightYMost ? mMaximalRightYMost : nscoord_MIN;

    
    for (FrameInfo *frame = mFrameInfoMap; frame; frame = frame->mNext) {
      nscoord ymost = frame->mRect.YMost();
      if (ymost > maximalLeftYMost) {
        if (frame->mFrame->GetStyleDisplay()->mFloats == NS_STYLE_FLOAT_LEFT) {
          NS_ASSERTION(!mHaveCachedLeftYMost, "Shouldn't happen");
          maximalLeftYMost = ymost;
          
          continue;
        }
      }

      if (ymost > maximalRightYMost) {
        if (frame->mFrame->GetStyleDisplay()->mFloats == NS_STYLE_FLOAT_RIGHT) {
          NS_ASSERTION(!mHaveCachedRightYMost, "Shouldn't happen");
          maximalRightYMost = ymost;
        }
      }
    }

    mMaximalLeftYMost = maximalLeftYMost;
    mMaximalRightYMost = maximalRightYMost;
    mHaveCachedRightYMost = mHaveCachedLeftYMost = PR_TRUE;
  }
  
  switch (aBreakType) {
    case NS_STYLE_CLEAR_LEFT_AND_RIGHT:
      NS_ASSERTION(mHaveCachedLeftYMost && mHaveCachedRightYMost,
                   "Need cached values!");
      bottom = PR_MAX(bottom, mMaximalLeftYMost);
      bottom = PR_MAX(bottom, mMaximalRightYMost);
      break;
    case NS_STYLE_CLEAR_LEFT:
      NS_ASSERTION(mHaveCachedLeftYMost, "Need cached value!");
      bottom = PR_MAX(bottom, mMaximalLeftYMost);
      break;
    case NS_STYLE_CLEAR_RIGHT:
      NS_ASSERTION(mHaveCachedRightYMost, "Need cached value!");
      bottom = PR_MAX(bottom, mMaximalRightYMost);
      break;
    default:
      
      break;
  }

  bottom -= mY;

  return bottom;
}

nsSpaceManager::BandRect*
nsSpaceManager::GuessBandWithTopAbove(nscoord aYOffset) const
{
  NS_ASSERTION(!mBandList.IsEmpty(), "no bands");
  BandRect* band = nsnull;
  if (mCachedBandPosition) {
    band = mCachedBandPosition;
    
    
    while (band && band->mTop > aYOffset) {
      band = GetPrevBand(band);
    }
  }

  if (band) {
    return band;
  }
  
  return mBandList.Head();
}




nsSpaceManager::FrameInfo::FrameInfo(nsIFrame* aFrame, const nsRect& aRect)
  : mFrame(aFrame), mRect(aRect), mNext(0)
{
  MOZ_COUNT_CTOR(nsSpaceManager::FrameInfo);
}

#ifdef NS_BUILD_REFCNT_LOGGING
nsSpaceManager::FrameInfo::~FrameInfo()
{
  MOZ_COUNT_DTOR(nsSpaceManager::FrameInfo);
}
#endif




nsSpaceManager::BandRect::BandRect(nscoord    aLeft,
                                   nscoord    aTop,
                                   nscoord    aRight,
                                   nscoord    aBottom,
                                   nsIFrame*  aFrame)
{
  MOZ_COUNT_CTOR(BandRect);
  mLeft = aLeft;
  mTop = aTop;
  mRight = aRight;
  mBottom = aBottom;
  mFrame = aFrame;
  mNumFrames = 1;
}

nsSpaceManager::BandRect::BandRect(nscoord      aLeft,
                                   nscoord      aTop,
                                   nscoord      aRight,
                                   nscoord      aBottom,
                                   nsVoidArray* aFrames)
{
  MOZ_COUNT_CTOR(BandRect);
  mLeft = aLeft;
  mTop = aTop;
  mRight = aRight;
  mBottom = aBottom;
  mFrames = new nsVoidArray;
  mFrames->operator=(*aFrames);
  mNumFrames = mFrames->Count();
}

nsSpaceManager::BandRect::~BandRect()
{
  MOZ_COUNT_DTOR(BandRect);
  if (mNumFrames > 1) {
    delete mFrames;
  }
}

nsSpaceManager::BandRect*
nsSpaceManager::BandRect::SplitVertically(nscoord aBottom)
{
  NS_PRECONDITION((aBottom > mTop) && (aBottom < mBottom), "bad argument");

  
  BandRect* bottomBandRect;
                                            
  if (mNumFrames > 1) {
    bottomBandRect = new BandRect(mLeft, aBottom, mRight, mBottom, mFrames);
  } else {
    bottomBandRect = new BandRect(mLeft, aBottom, mRight, mBottom, mFrame);
  }
                                           
  
  mBottom = aBottom;
  return bottomBandRect;
}

nsSpaceManager::BandRect*
nsSpaceManager::BandRect::SplitHorizontally(nscoord aRight)
{
  NS_PRECONDITION((aRight > mLeft) && (aRight < mRight), "bad argument");
  
  
  BandRect* rightBandRect;
                                            
  if (mNumFrames > 1) {
    rightBandRect = new BandRect(aRight, mTop, mRight, mBottom, mFrames);
  } else {
    rightBandRect = new BandRect(aRight, mTop, mRight, mBottom, mFrame);
  }
                                           
  
  mRight = aRight;
  return rightBandRect;
}

PRBool
nsSpaceManager::BandRect::IsOccupiedBy(const nsIFrame* aFrame) const
{
  PRBool  result;

  if (1 == mNumFrames) {
    result = (mFrame == aFrame);
  } else {
    PRInt32 count = mFrames->Count();

    result = PR_FALSE;
    for (PRInt32 i = 0; i < count; i++) {
      nsIFrame* f = (nsIFrame*)mFrames->ElementAt(i);

      if (f == aFrame) {
        result = PR_TRUE;
        break;
      }
    }
  }

  return result;
}

void
nsSpaceManager::BandRect::AddFrame(const nsIFrame* aFrame)
{
  if (1 == mNumFrames) {
    nsIFrame* f = mFrame;
    mFrames = new nsVoidArray;
    mFrames->AppendElement(f);
  }

  mNumFrames++;
  mFrames->AppendElement((void*)aFrame);
  NS_POSTCONDITION(mFrames->Count() == mNumFrames, "bad frame count");
}

void
nsSpaceManager::BandRect::RemoveFrame(const nsIFrame* aFrame)
{
  NS_PRECONDITION(mNumFrames > 1, "only one frame");
  mFrames->RemoveElement((void*)aFrame);
  mNumFrames--;

  if (1 == mNumFrames) {
    nsIFrame* f = (nsIFrame*)mFrames->ElementAt(0);

    delete mFrames;
    mFrame = f;
  }
}

PRBool
nsSpaceManager::BandRect::HasSameFrameList(const BandRect* aBandRect) const
{
  PRBool  result;

  
  if (mNumFrames != aBandRect->mNumFrames) {
    result = PR_FALSE;
  } else if (1 == mNumFrames) {
    result = (mFrame == aBandRect->mFrame);
  } else {
    result = PR_TRUE;

    
    
    PRInt32 count = mFrames->Count();
    for (PRInt32 i = 0; i < count; i++) {
      nsIFrame* f = (nsIFrame*)mFrames->ElementAt(i);

      if (-1 == aBandRect->mFrames->IndexOf(f)) {
        result = PR_FALSE;
        break;
      }
    }
  }

  return result;
}





PRInt32
nsSpaceManager::BandRect::Length() const
{
  PRInt32   len = 1;
  BandRect* bandRect = Next();

  
  
  
  while (bandRect->mTop == mTop) {
    len++;
    bandRect = bandRect->Next();
  }

  return len;
}




nsAutoSpaceManager::~nsAutoSpaceManager()
{
  
  if (mNew) {
#ifdef NOISY_SPACEMANAGER
    printf("restoring old space manager %p\n", mOld);
#endif

    mReflowState.mSpaceManager = mOld;

#ifdef NOISY_SPACEMANAGER
    if (mOld) {
      static_cast<nsFrame *>(mReflowState.frame)->ListTag(stdout);
      printf(": space-manager %p after reflow\n", mOld);
      mOld->List(stdout);
    }
#endif

#ifdef DEBUG
    if (mOwns)
#endif
      delete mNew;
  }
}

nsresult
nsAutoSpaceManager::CreateSpaceManagerFor(nsPresContext *aPresContext, nsIFrame *aFrame)
{
  
  
  
  mNew = new nsSpaceManager(aPresContext->PresShell(), aFrame);
  if (! mNew)
    return NS_ERROR_OUT_OF_MEMORY;

#ifdef NOISY_SPACEMANAGER
  printf("constructed new space manager %p (replacing %p)\n",
         mNew, mReflowState.mSpaceManager);
#endif

  
  mOld = mReflowState.mSpaceManager;
  mReflowState.mSpaceManager = mNew;
  return NS_OK;
}
