




#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "mozilla/layers/TextureClient.h"
#include "mozilla/layers/TextureHost.h"
#include "gfx2DGlue.h"
#include "gfxImageSurface.h"
#include "gfxTypes.h"
#include "ImageContainer.h"
#include "mozilla/layers/YCbCrImageDataSerializer.h"

using namespace mozilla;
using namespace mozilla::layers;
















void SetupSurface(gfxImageSurface* surface) {
  int bpp = gfxASurface::BytePerPixelFromFormat(surface->Format());
  int stride = surface->Stride();
  uint8_t val = 0;
  uint8_t* data = surface->Data();
  for (int y = 0; y < surface->Height(); ++y) {
    for (int x = 0; x < surface->Height(); ++x) {
      for (int b = 0; b < bpp; ++b) {
        data[y*stride + x*bpp + b] = val;
        if (val == 100) {
          val = 0;
        } else {
          ++val;
        }
      }
    }
  }
}


void AssertSurfacesEqual(gfxImageSurface* surface1,
                         gfxImageSurface* surface2)
{
  ASSERT_EQ(surface1->GetSize(), surface2->GetSize());
  ASSERT_EQ(surface1->Format(), surface2->Format());

  uint8_t* data1 = surface1->Data();
  uint8_t* data2 = surface2->Data();
  int stride1 = surface1->Stride();
  int stride2 = surface2->Stride();
  int bpp = gfxASurface::BytePerPixelFromFormat(surface1->Format());

  for (int y = 0; y < surface1->Height(); ++y) {
    for (int x = 0; x < surface1->Width(); ++x) {
      for (int b = 0; b < bpp; ++b) {
        ASSERT_EQ(data1[y*stride1 + x*bpp + b],
                  data2[y*stride2 + x*bpp + b]);
      }
    }
  }
}


void AssertYCbCrSurfacesEqual(PlanarYCbCrData* surface1,
                              PlanarYCbCrData* surface2)
{
  ASSERT_EQ(surface1->mYSize, surface2->mYSize);
  ASSERT_EQ(surface1->mCbCrSize, surface2->mCbCrSize);
  ASSERT_EQ(surface1->mStereoMode, surface2->mStereoMode);
  ASSERT_EQ(surface1->mPicSize, surface2->mPicSize);

  for (int y = 0; y < surface1->mYSize.height; ++y) {
    for (int x = 0; x < surface1->mYSize.width; ++x) {
      ASSERT_EQ(surface1->mYChannel[y*surface1->mYStride + x*(1+surface1->mYSkip)],
                surface2->mYChannel[y*surface2->mYStride + x*(1+surface2->mYSkip)]);
    }
  }
  for (int y = 0; y < surface1->mCbCrSize.height; ++y) {
    for (int x = 0; x < surface1->mCbCrSize.width; ++x) {
      ASSERT_EQ(surface1->mCbChannel[y*surface1->mCbCrStride + x*(1+surface1->mCbSkip)],
                surface2->mCbChannel[y*surface2->mCbCrStride + x*(1+surface2->mCbSkip)]);
      ASSERT_EQ(surface1->mCrChannel[y*surface1->mCbCrStride + x*(1+surface1->mCrSkip)],
                surface2->mCrChannel[y*surface2->mCbCrStride + x*(1+surface2->mCrSkip)]);
    }
  }
}


void TestTextureClientSurface(TextureClient* texture, gfxImageSurface* surface) {

  
  ASSERT_TRUE(texture->AsTextureClientSurface() != nullptr);
  TextureClientSurface* client = texture->AsTextureClientSurface();
  client->AllocateForSurface(ToIntSize(surface->GetSize()));
  ASSERT_TRUE(texture->IsAllocated());

  ASSERT_TRUE(texture->Lock(OPEN_READ_WRITE));
  
  client->UpdateSurface(surface);

  nsRefPtr<gfxASurface> aSurface = client->GetAsSurface();
  nsRefPtr<gfxImageSurface> clientSurface = aSurface->GetAsImageSurface();

  AssertSurfacesEqual(surface, clientSurface);
  texture->Unlock();

  
  SurfaceDescriptor descriptor;
  ASSERT_TRUE(texture->ToSurfaceDescriptor(descriptor));

  ASSERT_NE(descriptor.type(), SurfaceDescriptor::Tnull_t);

  
  RefPtr<TextureHost> host = CreateBackendIndependentTextureHost(descriptor, nullptr,
                                                                 texture->GetFlags());

  ASSERT_TRUE(host.get() != nullptr);
  ASSERT_EQ(host->GetFlags(), texture->GetFlags());

  
  ASSERT_TRUE(host->Lock());
  RefPtr<mozilla::gfx::DataSourceSurface> hostDataSurface = host->GetAsSurface();
  host->Unlock();

  nsRefPtr<gfxImageSurface> hostSurface =
    new gfxImageSurface(hostDataSurface->GetData(),
                        ThebesIntSize(hostDataSurface->GetSize()),
                        hostDataSurface->Stride(),
                        SurfaceFormatToImageFormat(hostDataSurface->GetFormat()));
  AssertSurfacesEqual(surface, hostSurface.get());
}


void TestTextureClientYCbCr(TextureClient* client, PlanarYCbCrData& ycbcrData) {

  
  ASSERT_TRUE(client->AsTextureClientYCbCr() != nullptr);
  TextureClientYCbCr* texture = client->AsTextureClientYCbCr();
  texture->AllocateForYCbCr(ycbcrData.mYSize,
                            ycbcrData.mCbCrSize,
                            ycbcrData.mStereoMode);
  ASSERT_TRUE(client->IsAllocated());

  ASSERT_TRUE(client->Lock(OPEN_READ_WRITE));
  
  texture->UpdateYCbCr(ycbcrData);
  client->Unlock();

  
  SurfaceDescriptor descriptor;
  ASSERT_TRUE(client->ToSurfaceDescriptor(descriptor));

  ASSERT_NE(descriptor.type(), SurfaceDescriptor::Tnull_t);

  
  RefPtr<TextureHost> textureHost = CreateBackendIndependentTextureHost(descriptor, nullptr,
                                                                        client->GetFlags());

  RefPtr<BufferTextureHost> host = static_cast<BufferTextureHost*>(textureHost.get());

  ASSERT_TRUE(host.get() != nullptr);
  ASSERT_EQ(host->GetFlags(), client->GetFlags());

  
  ASSERT_TRUE(host->Lock());

  
  ASSERT_EQ(host->GetFormat(), mozilla::gfx::SurfaceFormat::YUV);

  YCbCrImageDataDeserializer yuvDeserializer(host->GetBuffer(), host->GetBufferSize());
  ASSERT_TRUE(yuvDeserializer.IsValid());
  PlanarYCbCrData data;
  data.mYChannel = yuvDeserializer.GetYData();
  data.mCbChannel = yuvDeserializer.GetCbData();
  data.mCrChannel = yuvDeserializer.GetCrData();
  data.mYStride = yuvDeserializer.GetYStride();
  data.mCbCrStride = yuvDeserializer.GetCbCrStride();
  data.mStereoMode = yuvDeserializer.GetStereoMode();
  data.mYSize = yuvDeserializer.GetYSize();
  data.mCbCrSize = yuvDeserializer.GetCbCrSize();
  data.mYSkip = 0;
  data.mCbSkip = 0;
  data.mCrSkip = 0;
  data.mPicSize = data.mYSize;
  data.mPicX = 0;
  data.mPicY = 0;

  AssertYCbCrSurfacesEqual(&ycbcrData, &data);
  host->Unlock();
}

TEST(Layers, TextureSerialization) {
  
  gfxImageFormat formats[3] = {
    gfxImageFormat::ARGB32,
    gfxImageFormat::RGB24,
    gfxImageFormat::A8,
  };

  for (int f = 0; f < 3; ++f) {
    RefPtr<gfxImageSurface> surface = new gfxImageSurface(gfxIntSize(400,300), formats[f]);
    SetupSurface(surface.get());
    AssertSurfacesEqual(surface, surface);

    RefPtr<TextureClient> client
      = new MemoryTextureClient(nullptr,
                                mozilla::gfx::ImageFormatToSurfaceFormat(surface->Format()),
                                gfx::BackendType::CAIRO,
                                TEXTURE_DEALLOCATE_CLIENT);

    TestTextureClientSurface(client, surface);

    
  }
}

TEST(Layers, TextureYCbCrSerialization) {
  RefPtr<gfxImageSurface> ySurface = new gfxImageSurface(gfxIntSize(400,300), gfxImageFormat::A8);
  RefPtr<gfxImageSurface> cbSurface = new gfxImageSurface(gfxIntSize(200,150), gfxImageFormat::A8);
  RefPtr<gfxImageSurface> crSurface = new gfxImageSurface(gfxIntSize(200,150), gfxImageFormat::A8);
  SetupSurface(ySurface.get());
  SetupSurface(cbSurface.get());
  SetupSurface(crSurface.get());

  PlanarYCbCrData clientData;
  clientData.mYChannel = ySurface->Data();
  clientData.mCbChannel = cbSurface->Data();
  clientData.mCrChannel = crSurface->Data();
  clientData.mYSize = ySurface->GetSize().ToIntSize();
  clientData.mPicSize = ySurface->GetSize().ToIntSize();
  clientData.mCbCrSize = cbSurface->GetSize().ToIntSize();
  clientData.mYStride = ySurface->Stride();
  clientData.mCbCrStride = cbSurface->Stride();
  clientData.mStereoMode = StereoMode::MONO;
  clientData.mYSkip = 0;
  clientData.mCbSkip = 0;
  clientData.mCrSkip = 0;
  clientData.mCrSkip = 0;
  clientData.mPicX = 0;
  clientData.mPicX = 0;

  RefPtr<TextureClient> client
    = new MemoryTextureClient(nullptr,
                              mozilla::gfx::SurfaceFormat::YUV,
                              gfx::BackendType::CAIRO,
                              TEXTURE_DEALLOCATE_CLIENT);

  TestTextureClientYCbCr(client, clientData);

  
}
