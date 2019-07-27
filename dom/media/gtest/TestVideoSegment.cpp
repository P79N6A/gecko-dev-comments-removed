



#include "gtest/gtest.h"
#include "VideoSegment.h"

using namespace mozilla;

namespace mozilla {
  namespace layer {
    class Image;
  }
}

TEST(VideoSegment, TestAppendFrameForceBlack)
{
  nsRefPtr<layers::Image> testImage = nullptr;

  VideoSegment segment;
  segment.AppendFrame(testImage.forget(),
                      mozilla::TrackTicks(90000),
                      mozilla::gfx::IntSize(640, 480),
                      true);

  VideoSegment::ChunkIterator iter(segment);
  while (!iter.IsEnded()) {
    VideoChunk chunk = *iter;
    EXPECT_TRUE(chunk.mFrame.GetForceBlack());
    iter.Next();
  }
}

TEST(VideoSegment, TestAppendFrameNotForceBlack)
{
  nsRefPtr<layers::Image> testImage = nullptr;

  VideoSegment segment;
  segment.AppendFrame(testImage.forget(),
                      mozilla::TrackTicks(90000),
                      mozilla::gfx::IntSize(640, 480));

  VideoSegment::ChunkIterator iter(segment);
  while (!iter.IsEnded()) {
    VideoChunk chunk = *iter;
    EXPECT_FALSE(chunk.mFrame.GetForceBlack());
    iter.Next();
  }
}
