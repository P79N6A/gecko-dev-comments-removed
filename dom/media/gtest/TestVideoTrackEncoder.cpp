



#include "gtest/gtest.h"
#include <algorithm>

#include "mozilla/ArrayUtils.h"
#include "VP8TrackEncoder.h"
#include "ImageContainer.h"
#include "MediaStreamGraph.h"
#include "WebMWriter.h" 

using ::testing::TestWithParam;
using ::testing::Values;

using namespace mozilla::layers;
using namespace mozilla;


class YUVBufferGenerator {
public:
  YUVBufferGenerator() {}

  void Init(const mozilla::gfx::IntSize &aSize)
  {
    mImageSize = aSize;

    int yPlaneLen = aSize.width * aSize.height;
    int cbcrPlaneLen = (yPlaneLen + 1) / 2;
    int frameLen = yPlaneLen + cbcrPlaneLen;

    
    mSourceBuffer.SetLength(frameLen);

    
    memset(mSourceBuffer.Elements(), 0x10, yPlaneLen);

    
    memset(mSourceBuffer.Elements() + yPlaneLen, 0x80, cbcrPlaneLen);
  }

  mozilla::gfx::IntSize GetSize() const
  {
    return mImageSize;
  }

  void Generate(nsTArray<nsRefPtr<Image> > &aImages)
  {
    aImages.AppendElement(CreateI420Image());
    aImages.AppendElement(CreateNV12Image());
    aImages.AppendElement(CreateNV21Image());
  }

private:
  Image *CreateI420Image()
  {
    PlanarYCbCrImage *image = new PlanarYCbCrImage(new BufferRecycleBin());
    PlanarYCbCrData data;

    const uint32_t yPlaneSize = mImageSize.width * mImageSize.height;
    const uint32_t halfWidth = (mImageSize.width + 1) / 2;
    const uint32_t halfHeight = (mImageSize.height + 1) / 2;
    const uint32_t uvPlaneSize = halfWidth * halfHeight;

    
    uint8_t *y = mSourceBuffer.Elements();
    data.mYChannel = y;
    data.mYSize.width = mImageSize.width;
    data.mYSize.height = mImageSize.height;
    data.mYStride = mImageSize.width;
    data.mYSkip = 0;

    
    uint8_t *cr = y + yPlaneSize + uvPlaneSize;
    data.mCrChannel = cr;
    data.mCrSkip = 0;

    
    uint8_t *cb = y + yPlaneSize;
    data.mCbChannel = cb;
    data.mCbSkip = 0;

    
    data.mCbCrStride = halfWidth;
    data.mCbCrSize.width = halfWidth;
    data.mCbCrSize.height = halfHeight;

    image->SetData(data);
    return image;
  }

  Image *CreateNV12Image()
  {
    PlanarYCbCrImage *image = new PlanarYCbCrImage(new BufferRecycleBin());
    PlanarYCbCrData data;

    const uint32_t yPlaneSize = mImageSize.width * mImageSize.height;
    const uint32_t halfWidth = (mImageSize.width + 1) / 2;
    const uint32_t halfHeight = (mImageSize.height + 1) / 2;

    
    uint8_t *y = mSourceBuffer.Elements();
    data.mYChannel = y;
    data.mYSize.width = mImageSize.width;
    data.mYSize.height = mImageSize.height;
    data.mYStride = mImageSize.width;
    data.mYSkip = 0;

    
    uint8_t *cr = y + yPlaneSize;
    data.mCrChannel = cr;
    data.mCrSkip = 1;

    
    uint8_t *cb = y + yPlaneSize + 1;
    data.mCbChannel = cb;
    data.mCbSkip = 1;

    
    data.mCbCrStride = mImageSize.width;
    data.mCbCrSize.width = halfWidth;
    data.mCbCrSize.height = halfHeight;

    image->SetData(data);
    return image;
  }

  Image *CreateNV21Image()
  {
    PlanarYCbCrImage *image = new PlanarYCbCrImage(new BufferRecycleBin());
    PlanarYCbCrData data;

    const uint32_t yPlaneSize = mImageSize.width * mImageSize.height;
    const uint32_t halfWidth = (mImageSize.width + 1) / 2;
    const uint32_t halfHeight = (mImageSize.height + 1) / 2;

    
    uint8_t *y = mSourceBuffer.Elements();
    data.mYChannel = y;
    data.mYSize.width = mImageSize.width;
    data.mYSize.height = mImageSize.height;
    data.mYStride = mImageSize.width;
    data.mYSkip = 0;

    
    uint8_t *cr = y + yPlaneSize + 1;
    data.mCrChannel = cr;
    data.mCrSkip = 1;

    
    uint8_t *cb = y + yPlaneSize;
    data.mCbChannel = cb;
    data.mCbSkip = 1;

    
    data.mCbCrStride = mImageSize.width;
    data.mCbCrSize.width = halfWidth;
    data.mCbCrSize.height = halfHeight;

    image->SetData(data);
    return image;
  }

private:
  mozilla::gfx::IntSize mImageSize;
  nsTArray<uint8_t> mSourceBuffer;
};

struct InitParam {
  bool mShouldSucceed;  
  int  mWidth;          
  int  mHeight;         
  mozilla::TrackRate mTrackRate; 
};

class TestVP8TrackEncoder: public VP8TrackEncoder
{
public:
  ::testing::AssertionResult TestInit(const InitParam &aParam)
  {
    nsresult result = Init(aParam.mWidth, aParam.mHeight, aParam.mWidth, aParam.mHeight, aParam.mTrackRate);

    if (((NS_FAILED(result) && aParam.mShouldSucceed)) || (NS_SUCCEEDED(result) && !aParam.mShouldSucceed))
    {
      return ::testing::AssertionFailure()
                << " width = " << aParam.mWidth
                << " height = " << aParam.mHeight
                << " TrackRate = " << aParam.mTrackRate << ".";
    }
    else
    {
      return ::testing::AssertionSuccess();
    }
  }
};


TEST(VP8VideoTrackEncoder, Initialization)
{
  InitParam params[] = {
    
    { false, 640, 480, 0 },      
    { false, 640, 480, -1 },     
    { false, 0, 0, 90000 },      
    { false, 0, 1, 90000 },      
    { false, 1, 0, 90000},       

    
    { true, 640, 480, 90000},    
    { true, 800, 480, 90000},    
    { true, 960, 540, 90000},    
    { true, 1280, 720, 90000}    
  };

  for (size_t i = 0; i < ArrayLength(params); i++)
  {
    TestVP8TrackEncoder encoder;
    EXPECT_TRUE(encoder.TestInit(params[i]));
  }
}


TEST(VP8VideoTrackEncoder, FetchMetaData)
{
  InitParam params[] = {
    
    { true, 640, 480, 90000},    
    { true, 800, 480, 90000},    
    { true, 960, 540, 90000},    
    { true, 1280, 720, 90000}    
  };

  for (size_t i = 0; i < ArrayLength(params); i++)
  {
    TestVP8TrackEncoder encoder;
    EXPECT_TRUE(encoder.TestInit(params[i]));

    nsRefPtr<TrackMetadataBase> meta = encoder.GetMetadata();
    nsRefPtr<VP8Metadata> vp8Meta(static_cast<VP8Metadata*>(meta.get()));

    
    EXPECT_TRUE(vp8Meta->mWidth == params[i].mWidth);
    EXPECT_TRUE(vp8Meta->mHeight == params[i].mHeight);
  }
}




#if !defined(_MSC_VER) || _MSC_VER < 1800
TEST(VP8VideoTrackEncoder, FrameEncode)
{
  
  TestVP8TrackEncoder encoder;
  InitParam param = {true, 640, 480, 90000};
  encoder.TestInit(param);

  
  nsTArray<nsRefPtr<Image>> images;
  YUVBufferGenerator generator;
  generator.Init(mozilla::gfx::IntSize(640, 480));
  generator.Generate(images);

  
  
  VideoSegment segment;
  for (nsTArray<nsRefPtr<Image>>::size_type i = 0; i < images.Length(); i++)
  {
    nsRefPtr<Image> image = images[i];
    segment.AppendFrame(image.forget(), mozilla::TrackTicks(90000), generator.GetSize());
  }

  
  encoder.NotifyQueuedTrackChanges(nullptr, 0, 0, 0, 0, segment);

  
  EncodedFrameContainer container;
  EXPECT_TRUE(NS_SUCCEEDED(encoder.GetEncodedTrack(container)));
}
#endif 


TEST(VP8VideoTrackEncoder, EncodeComplete)
{
  
  TestVP8TrackEncoder encoder;
  InitParam param = {true, 640, 480, 90000};
  encoder.TestInit(param);

  
  VideoSegment segment;
  encoder.NotifyQueuedTrackChanges(nullptr, 0, 0, 0, MediaStreamListener::TRACK_EVENT_ENDED, segment);

  
  
  
  EncodedFrameContainer container;
  EXPECT_TRUE(NS_SUCCEEDED(encoder.GetEncodedTrack(container)));
}
