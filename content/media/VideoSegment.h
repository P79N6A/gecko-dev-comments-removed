




#ifndef MOZILLA_VIDEOSEGMENT_H_
#define MOZILLA_VIDEOSEGMENT_H_

#include "MediaSegment.h"
#include "nsCOMPtr.h"
#include "gfxPoint.h"
#include "nsAutoPtr.h"
#include "ImageContainer.h"

namespace mozilla {

namespace layers {
class Image;
}

class VideoFrame {
public:
  typedef mozilla::layers::Image Image;

  VideoFrame(already_AddRefed<Image> aImage, const gfxIntSize& aIntrinsicSize);
  VideoFrame();
  ~VideoFrame();

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
  void SetNull();
  void TakeFrom(VideoFrame* aFrame);

protected:
  
  
  nsRefPtr<Image> mImage;
  
  gfxIntSize mIntrinsicSize;
};


struct VideoChunk {
  VideoChunk();
  ~VideoChunk();
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

  VideoSegment();
  ~VideoSegment();

  void AppendFrame(already_AddRefed<Image> aImage, TrackTicks aDuration,
                   const gfxIntSize& aIntrinsicSize);
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

  
  static Type StaticType() { return VIDEO; }
};

}

#endif 
