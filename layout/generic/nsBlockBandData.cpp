






































#include "nsCOMPtr.h"
#include "nsBlockBandData.h"
#include "nsIFrame.h"
#include "nsHTMLReflowState.h"
#include "nsPresContext.h"
#include "nsIPresShell.h"

nsBlockBandData::nsBlockBandData()
  : mSpaceManager(nsnull),
    mSpaceManagerX(0),
    mSpaceManagerY(0),
    mSpace(0, 0)
{
  mSize = NS_BLOCK_BAND_DATA_TRAPS;
  mTrapezoids = mData;
}

nsBlockBandData::~nsBlockBandData()
{
  if (mTrapezoids != mData) {
    delete [] mTrapezoids;
  }
}

nsresult
nsBlockBandData::Init(nsSpaceManager* aSpaceManager,
                      const nsSize& aSpace)
{
  NS_PRECONDITION(aSpaceManager, "null pointer");

  mSpaceManager = aSpaceManager;
  aSpaceManager->GetTranslation(mSpaceManagerX, mSpaceManagerY);

  mSpace = aSpace;
  mLeftFloats = 0;
  mRightFloats = 0;
  return NS_OK;
}




nsresult
nsBlockBandData::GetAvailableSpace(nscoord aY, PRBool aRelaxHeightConstraint,
                                   nsRect& aResult)
{
  
  nsresult rv = GetBandData(aY, aRelaxHeightConstraint);
  if (NS_FAILED(rv)) { return rv; }

  
  
  ComputeAvailSpaceRect();
  aResult = mAvailSpace;
#ifdef REALLY_NOISY_COMPUTEAVAILSPACERECT
  printf("nsBBD %p GetAvailableSpace(%d) returning (%d, %d, %d, %d)\n",
          this, aY, aResult.x, aResult.y, aResult.width, aResult.height);
#endif
  return NS_OK;
}



#define ERROR_TOO_MANY_ITERATIONS 1000




nsresult
nsBlockBandData::GetBandData(nscoord aY, PRBool aRelaxHeightConstraint)
{
  NS_ASSERTION(mSpaceManager, "bad state, no space manager");
  PRInt32 iterations =0;
  nsSize space = mSpace;
  if (aRelaxHeightConstraint) {
    space.height = NS_UNCONSTRAINEDSIZE;
  }
  nsresult rv = mSpaceManager->GetBandData(aY, space, *this);
  while (NS_FAILED(rv)) {
    iterations++;
    if (iterations>ERROR_TOO_MANY_ITERATIONS)
    {
      NS_ASSERTION(PR_FALSE, "too many iterations in nsBlockBandData::GetBandData");
      return NS_ERROR_FAILURE;
    }
    
    NS_ASSERTION(mTrapezoids, "bad state, no mTrapezoids");
    if (mTrapezoids && (mTrapezoids != mData)) {
      delete [] mTrapezoids;
    }
    PRInt32 newSize = mSize * 2;
    if (newSize<mCount) {
      newSize = mCount;
    }
    mTrapezoids = new nsBandTrapezoid[newSize];
    NS_POSTCONDITION(mTrapezoids, "failure allocating mTrapezoids");
    if (!mTrapezoids) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    mSize = newSize;
    rv = mSpaceManager->GetBandData(aY, space, *this);
  }
  NS_POSTCONDITION(mCount<=mSize, "bad state, count > size");
  return NS_OK;
}









void
nsBlockBandData::ComputeAvailSpaceRect()
{
#ifdef REALLY_NOISY_COMPUTEAVAILSPACERECT
  printf("nsBlockBandData::ComputeAvailSpaceRect %p with count %d\n", this, mCount);
#endif
  if (0 == mCount) {
    mAvailSpace.x = 0;
    mAvailSpace.y = 0;
    mAvailSpace.width = 0;
    mAvailSpace.height = 0;
    mLeftFloats = 0;
    mRightFloats = 0;
    return;
  }

  nsBandTrapezoid* trapezoid = mTrapezoids;
  
  nsBandTrapezoid* rightTrapezoid = nsnull;

  PRInt32 leftFloats = 0;
  PRInt32 rightFloats = 0;
  if (mCount > 1) {
    
    PRInt32 i;

    
    
    
    NS_PRECONDITION(mCount<=mSize, "bad state, count > size");
    for (i = 0; i < mCount; i++) {
      trapezoid = &mTrapezoids[i];
      if (trapezoid->mState != nsBandTrapezoid::Available) {
#ifdef REALLY_NOISY_COMPUTEAVAILSPACERECT
        printf("band %p checking !Avail trap %p with frame %p\n", this, trapezoid, trapezoid->mFrame);
#endif
        if (nsBandTrapezoid::OccupiedMultiple == trapezoid->mState) {
          PRInt32 j, numFrames = trapezoid->mFrames->Count();
          NS_ASSERTION(numFrames > 0, "bad trapezoid frame list");
          for (j = 0; j < numFrames; j++) {
            nsIFrame* f = (nsIFrame*) trapezoid->mFrames->ElementAt(j);
            const nsStyleDisplay* display = f->GetStyleDisplay();
            if (NS_STYLE_FLOAT_LEFT == display->mFloats) {
              leftFloats++;
            }
            else if (NS_STYLE_FLOAT_RIGHT == display->mFloats) {
              rightFloats++;
              if ((nsnull == rightTrapezoid) && (i > 0)) {
                rightTrapezoid = &mTrapezoids[i - 1];
              }
            }
          }
        } else {
          const nsStyleDisplay* display = trapezoid->mFrame->GetStyleDisplay();
          if (NS_STYLE_FLOAT_LEFT == display->mFloats) {
            leftFloats++;
          }
          else if (NS_STYLE_FLOAT_RIGHT == display->mFloats) {
            rightFloats++;
            if ((nsnull == rightTrapezoid) && (i > 0)) {
              rightTrapezoid = &mTrapezoids[i - 1];
            }
          }
        }
      }
    }
  }
  else if (mTrapezoids[0].mState != nsBandTrapezoid::Available) {
    
    leftFloats = 1;
  }
#ifdef REALLY_NOISY_COMPUTEAVAILSPACERECT
  printf("band %p has floats %d, %d\n", this, leftFloats, rightFloats);
#endif
  mLeftFloats = leftFloats;
  mRightFloats = rightFloats;

  
  
  
  if (nsnull != rightTrapezoid) {
    trapezoid = rightTrapezoid;
  }
  trapezoid->GetRect(mAvailSpace);

  
  
  if (nsBandTrapezoid::Available != trapezoid->mState) {
    if (nsBandTrapezoid::OccupiedMultiple == trapezoid->mState) {
      
      
      
      
      
      PRInt32 j, numFrames = trapezoid->mFrames->Count();
      NS_ASSERTION(numFrames > 0, "bad trapezoid frame list");
      for (j = 0; j < numFrames; j++) {
        nsIFrame* f = (nsIFrame*) trapezoid->mFrames->ElementAt(j);
        const nsStyleDisplay* display = f->GetStyleDisplay();
        if (NS_STYLE_FLOAT_LEFT == display->mFloats) {
          mAvailSpace.x = mAvailSpace.XMost();
          break;
        }
      }
    }
    else {
      const nsStyleDisplay* display = trapezoid->mFrame->GetStyleDisplay();
      if (NS_STYLE_FLOAT_LEFT == display->mFloats) {
        mAvailSpace.x = mAvailSpace.XMost();
      }
    }
    mAvailSpace.width = 0;
  }

  
  if (NS_UNCONSTRAINEDSIZE == mSpace.width) {
    mAvailSpace.width = NS_UNCONSTRAINEDSIZE;
  }
#ifdef REALLY_NOISY_COMPUTEAVAILSPACERECT
  printf("  ComputeAvailSpaceRect settting state mAvailSpace (%d,%d,%d,%d)\n", 
         mAvailSpace.x, mAvailSpace.y, mAvailSpace.width, mAvailSpace.height);
#endif

}

#ifdef DEBUG
void nsBlockBandData::List()
{
  printf("nsBlockBandData %p sm=%p, sm coord = (%d,%d), mSpace = (%d,%d)\n",
          this, mSpaceManager, mSpaceManagerX, mSpaceManagerY,
          mSpace.width, mSpace.height);
  printf("  availSpace=(%d, %d, %d, %d), floats l=%d r=%d\n",
          mAvailSpace.x, mAvailSpace.y, mAvailSpace.width, mAvailSpace.height,
          mLeftFloats, mRightFloats);
}
#endif
