




#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "mozilla/layers/TextureClient.h"
#include "mozilla/layers/TextureHost.h"
#include "gfx2DGlue.h"
#include "gfxImageSurface.h"
#include "gfxTypes.h"
#include "ImageContainer.h"
#include "LayerTestUtils.h"
#include "mozilla/layers/YCbCrImageDataSerializer.h"

using namespace mozilla;
using namespace mozilla::layers;















void TestTextureClientSurface(TextureClient* texture, gfxImageSurface* surface) {

  
  ASSERT_TRUE(texture->AsTextureClientSurface() != nullptr);
  TextureClientSurface* client = texture->AsTextureClientSurface();
  client->AllocateForSurface(ToIntSize(surface->GetSize()));
  ASSERT_TRUE(texture->IsAllocated());

  
  client->UpdateSurface(surface);

  nsRefPtr<gfxASurface> aSurface = client->GetAsSurface();
  nsRefPtr<gfxImageSurface> clientSurface = aSurface->GetAsImageSurface();

  ASSERT_TRUE(texture->Lock(OPEN_READ_ONLY));
  AssertSurfacesEqual(surface, clientSurface);
  texture->Unlock();

  
  texture->SetID(1);
  SurfaceDescriptor descriptor;
  ASSERT_TRUE(texture->ToSurfaceDescriptor(descriptor));

  ASSERT_NE(descriptor.type(), SurfaceDescriptor::Tnull_t);

  
  RefPtr<TextureHost> host = CreateBackendIndependentTextureHost(texture->GetID(),
                                                                 descriptor, nullptr,
                                                                 texture->GetFlags());

  ASSERT_TRUE(host.get() != nullptr);
  ASSERT_EQ(host->GetFlags(), texture->GetFlags());
  ASSERT_EQ(host->GetID(), texture->GetID());

  
  ASSERT_TRUE(host->Lock());
  nsRefPtr<gfxImageSurface> hostSurface = host->GetAsSurface();
  host->Unlock();

  AssertSurfacesEqual(surface, hostSurface.get());

  
  host->DeallocateSharedData();
}


void TestTextureClientYCbCr(TextureClient* client, PlanarYCbCrData& ycbcrData) {

  
  ASSERT_TRUE(client->AsTextureClientYCbCr() != nullptr);
  TextureClientYCbCr* texture = client->AsTextureClientYCbCr();
  texture->AllocateForYCbCr(ToIntSize(ycbcrData.mYSize),
                            ToIntSize(ycbcrData.mCbCrSize),
                            ycbcrData.mStereoMode);
  ASSERT_TRUE(client->IsAllocated());

  
  texture->UpdateYCbCr(ycbcrData);

  ASSERT_TRUE(client->Lock(OPEN_READ_ONLY));
  client->Unlock();

  
  client->SetID(1);
  SurfaceDescriptor descriptor;
  ASSERT_TRUE(client->ToSurfaceDescriptor(descriptor));

  ASSERT_NE(descriptor.type(), SurfaceDescriptor::Tnull_t);

  
  RefPtr<TextureHost> textureHost = CreateBackendIndependentTextureHost(client->GetID(),
                                                                        descriptor, nullptr,
                                                                        client->GetFlags());

  RefPtr<BufferTextureHost> host = static_cast<BufferTextureHost*>(textureHost.get());

  ASSERT_TRUE(host.get() != nullptr);
  ASSERT_EQ(host->GetFlags(), client->GetFlags());
  ASSERT_EQ(host->GetID(), client->GetID());

  
  ASSERT_EQ(host->GetFormat(), mozilla::gfx::FORMAT_YUV);

  
  ASSERT_TRUE(host->Lock());

  ASSERT_TRUE(host->GetFormat() == mozilla::gfx::FORMAT_YUV);

  YCbCrImageDataDeserializer yuvDeserializer(host->GetBuffer());
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

  
  host->DeallocateSharedData();
}

TEST(Layers, TextureSerialization) {
  
  gfxImageFormat formats[3] = {
    gfxImageFormatARGB32,
    gfxImageFormatRGB24,
    gfxImageFormatA8,
  };

  for (int f = 0; f < 3; ++f) {
    RefPtr<gfxImageSurface> surface = new gfxImageSurface(gfxIntSize(400,300), formats[f]);
    SetupSurface(surface.get());
    AssertSurfacesEqual(surface, surface);

    RefPtr<TextureClient> client
      = new MemoryTextureClient(nullptr,
                                mozilla::gfx::ImageFormatToSurfaceFormat(surface->Format()),
                                TEXTURE_FLAGS_DEFAULT);

    TestTextureClientSurface(client, surface);

    
  }
}

TEST(Layers, TextureYCbCrSerialization) {
  RefPtr<gfxImageSurface> ySurface = new gfxImageSurface(gfxIntSize(400,300), gfxImageFormatA8);
  RefPtr<gfxImageSurface> cbSurface = new gfxImageSurface(gfxIntSize(200,150), gfxImageFormatA8);
  RefPtr<gfxImageSurface> crSurface = new gfxImageSurface(gfxIntSize(200,150), gfxImageFormatA8);
  SetupSurface(ySurface.get());
  SetupSurface(cbSurface.get());
  SetupSurface(crSurface.get());

  PlanarYCbCrData clientData;
  clientData.mYChannel = ySurface->Data();
  clientData.mCbChannel = cbSurface->Data();
  clientData.mCrChannel = crSurface->Data();
  clientData.mYSize = ySurface->GetSize();
  clientData.mPicSize = ySurface->GetSize();
  clientData.mCbCrSize = cbSurface->GetSize();
  clientData.mYStride = ySurface->Stride();
  clientData.mCbCrStride = cbSurface->Stride();
  clientData.mStereoMode = STEREO_MODE_MONO;
  clientData.mYSkip = 0;
  clientData.mCbSkip = 0;
  clientData.mCrSkip = 0;
  clientData.mCrSkip = 0;
  clientData.mPicX = 0;
  clientData.mPicX = 0;

  RefPtr<TextureClient> client
    = new MemoryTextureClient(nullptr,
                              mozilla::gfx::FORMAT_YUV,
                              TEXTURE_FLAGS_DEFAULT);

  TestTextureClientYCbCr(client, clientData);

  
}
