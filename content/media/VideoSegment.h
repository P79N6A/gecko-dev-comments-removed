




#ifndef MOZILLA_VIDEOSEGMENT_H_
#define MOZILLA_VIDEOSEGMENT_H_

#include "MediaSegment.h"
#include "ImageContainer.h"

namespace mozilla {

class VideoFrame {
public:
  typedef mozilla::layers::Image Image;

  VideoFrame(already_AddRefed<Image> aImage, const gfxIntSize& aIntrinsicSize)
    : mImage(aImage), mIntrinsicSize(aIntrinsicSize) {}
  VideoFrame() : mIntrinsicSize(0, 0) {}

  bool operator==(const VideoFrame& aFrame) const
  {
    return mImage == aFrame.mImage && mIntrinsicSize == aFrame.mIntrinsicSize;
  }
  bool operator!=(const VideoFrame& aFrame) const
  {
    return !operator==(aFrame);
  }

  Image* GetImage() const { return mImage; }
  const gfxIntSize& GetIntrinsicSize() const { return mIntrinsicSize; }
  void SetNull() { mImage = nullptr; mIntrinsicSize = gfxIntSize(0, 0); }
  void TakeFrom(VideoFrame* aFrame)
  {
    mImage = aFrame->mImage.forget();
    mIntrinsicSize = aFrame->mIntrinsicSize;
  }

protected:
  
  
  nsRefPtr<Image> mImage;
  
  gfxIntSize mIntrinsicSize;
};


struct VideoChunk {
  void SliceTo(TrackTicks aStart, TrackTicks aEnd)
  {
    NS_ASSERTION(aStart >= 0 && aStart < aEnd && aEnd <= mDuration,
                 "Slice out of bounds");
    mDuration = aEnd - aStart;
  }
  TrackTicks GetDuration() const { return mDuration; }
  bool CanCombineWithFollowing(const VideoChunk& aOther) const
  {
    return aOther.mFrame == mFrame;
  }
  bool IsNull() const { return !mFrame.GetImage(); }
  void SetNull(TrackTicks aDuration)
  {
    mDuration = aDuration;
    mFrame.SetNull();
  }

  TrackTicks mDuration;
  VideoFrame mFrame;
};

class VideoSegment : public MediaSegmentBase<VideoSegment, VideoChunk> {
public:
  typedef mozilla::layers::Image Image;

  VideoSegment() : MediaSegmentBase<VideoSegment, VideoChunk>(VIDEO) {}

  void AppendFrame(already_AddRefed<Image> aImage, TrackTicks aDuration,
                   const gfxIntSize& aIntrinsicSize)
  {
    VideoChunk* chunk = AppendChunk(aDuration);
    VideoFrame frame(aImage, aIntrinsicSize);
    chunk->mFrame.TakeFrom(&frame);
  }
  const VideoFrame* GetFrameAt(TrackTicks aOffset, TrackTicks* aStart = nullptr)
  {
    VideoChunk* c = FindChunkContaining(aOffset, aStart);
    if (!c) {
      return nullptr;
    }
    return &c->mFrame;
  }
  const VideoFrame* GetLastFrame(TrackTicks* aStart = nullptr)
  {
    VideoChunk* c = GetLastChunk();
    if (!c) {
      return nullptr;
    }
    if (aStart) {
      *aStart = mDuration - c->mDuration;
    }
    return &c->mFrame;
  }

  
  void InitFrom(const VideoSegment& aOther)
  {
  }
  void CheckCompatible(const VideoSegment& aOther) const
  {
  }
  static Type StaticType() { return VIDEO; }
};

}

#endif 
