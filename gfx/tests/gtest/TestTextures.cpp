




#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "mozilla/layers/TextureClient.h"
#include "mozilla/layers/TextureHost.h"
#include "gfx2DGlue.h"
#include "gfxImageSurface.h"
#include "gfxTypes.h"

using namespace gfx;
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
