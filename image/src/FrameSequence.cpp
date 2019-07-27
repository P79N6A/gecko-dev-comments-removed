




#include "FrameSequence.h"

namespace mozilla {
namespace image {

FrameSequence::~FrameSequence()
{
  ClearFrames();
}

const FrameDataPair&
FrameSequence::GetFrame(uint32_t framenum) const
{
  if (framenum >= mFrames.Length()) {
    static FrameDataPair empty;
    return empty;
  }

  return mFrames[framenum];
}

uint32_t
FrameSequence::GetNumFrames() const
{
  return mFrames.Length();
}

void
FrameSequence::RemoveFrame(uint32_t framenum)
{
  NS_ABORT_IF_FALSE(framenum < mFrames.Length(), "Deleting invalid frame!");

  mFrames.RemoveElementAt(framenum);
}

void
FrameSequence::ClearFrames()
{
  
  
  mFrames.Clear();
}

void
FrameSequence::InsertFrame(uint32_t framenum, imgFrame* aFrame)
{
  NS_ABORT_IF_FALSE(framenum <= mFrames.Length(), "Inserting invalid frame!");
  mFrames.InsertElementAt(framenum, aFrame);
  if (GetNumFrames() > 1) {
    
    
    if (GetNumFrames() == 2) {
      mFrames[0].LockAndGetData();
    }

    
    
    mFrames[framenum].LockAndGetData();
  }
}

already_AddRefed<imgFrame>
FrameSequence::SwapFrame(uint32_t framenum, imgFrame* aFrame)
{
  NS_ABORT_IF_FALSE(framenum < mFrames.Length(), "Swapping invalid frame!");

  FrameDataPair ret;

  
  if (framenum < mFrames.Length()) {
    ret = mFrames[framenum];
  }

  if (aFrame) {
    mFrames.ReplaceElementAt(framenum, aFrame);
  } else {
    mFrames.RemoveElementAt(framenum);
  }

  return ret.GetFrame();
}

size_t
FrameSequence::SizeOfDecodedWithComputedFallbackIfHeap(gfxMemoryLocation aLocation,
                                                       MallocSizeOf aMallocSizeOf) const
{
  size_t n = 0;
  for (uint32_t i = 0; i < mFrames.Length(); ++i) {
    FrameDataPair fdp = mFrames.SafeElementAt(i, FrameDataPair());
    NS_ABORT_IF_FALSE(fdp, "Null frame in frame array!");
    n += fdp->SizeOfExcludingThisWithComputedFallbackIfHeap(aLocation, aMallocSizeOf);
  }

  return n;
}

} 
} 
