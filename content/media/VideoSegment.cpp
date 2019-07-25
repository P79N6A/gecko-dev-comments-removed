




#include "VideoSegment.h"
#include "ImageContainer.h"

namespace mozilla {

using namespace layers;

VideoFrame::VideoFrame(already_AddRefed<Image> aImage, const gfxIntSize& aIntrinsicSize)
  : mImage(aImage), mIntrinsicSize(aIntrinsicSize)
{}

VideoFrame::VideoFrame()
  : mIntrinsicSize(0, 0)
{}

VideoFrame::~VideoFrame()
{}

void
VideoFrame::SetNull() {
  mImage = nullptr;
  mIntrinsicSize = gfxIntSize(0, 0);
}

void
VideoFrame::TakeFrom(VideoFrame* aFrame)
{
  mImage = aFrame->mImage.forget();
  mIntrinsicSize = aFrame->mIntrinsicSize;
}

VideoChunk::VideoChunk()
{}

VideoChunk::~VideoChunk()
{}

void
VideoSegment::AppendFrame(already_AddRefed<Image> aImage, TrackTicks aDuration,
                          const gfxIntSize& aIntrinsicSize)
{
  VideoChunk* chunk = AppendChunk(aDuration);
  VideoFrame frame(aImage, aIntrinsicSize);
  chunk->mFrame.TakeFrom(&frame);
}

VideoSegment::VideoSegment()
  : MediaSegmentBase<VideoSegment, VideoChunk>(VIDEO)
{}

VideoSegment::~VideoSegment()
{}

}
