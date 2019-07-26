




#include "FrameSequence.h"

using namespace mozilla;
using namespace mozilla::image;

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

imgFrame*
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

  return ret.Forget();
}

size_t
FrameSequence::SizeOfDecodedWithComputedFallbackIfHeap(gfxMemoryLocation aLocation,
                                                       MallocSizeOf aMallocSizeOf) const
{
  size_t n = 0;
  for (uint32_t i = 0; i < mFrames.Length(); ++i) {
    imgFrame* frame = mFrames.SafeElementAt(i, FrameDataPair());
    NS_ABORT_IF_FALSE(frame, "Null frame in frame array!");
    n += frame->SizeOfExcludingThisWithComputedFallbackIfHeap(aLocation, aMallocSizeOf);
  }

  return n;
}

} 
} 
