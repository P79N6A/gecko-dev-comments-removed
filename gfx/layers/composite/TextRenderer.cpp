




#include "TextRenderer.h"
#include "FontData.h"
#include "png.h"
#include "mozilla/Base64.h"
#include "mozilla/layers/Compositor.h"
#include "mozilla/layers/TextureHost.h"
#include "mozilla/layers/Effects.h"

namespace mozilla {
namespace layers {

using namespace gfx;
using namespace std;

const Float sBackgroundOpacity = 0.6f;
const SurfaceFormat sTextureFormat = SurfaceFormat::B8G8R8A8;

static void PNGAPI info_callback(png_structp png_ptr, png_infop info_ptr)
{
  png_read_update_info(png_ptr, info_ptr);
}

static void PNGAPI row_callback(png_structp png_ptr, png_bytep new_row, png_uint_32 row_num, int pass)
{
  MOZ_ASSERT(sTextureFormat == SurfaceFormat::B8G8R8A8);

  DataSourceSurface::MappedSurface map = static_cast<TextRenderer*>(png_get_progressive_ptr(png_ptr))->GetSurfaceMap();

  uint32_t* dst = (uint32_t*)(map.mData + map.mStride * row_num);

  for (uint32_t x = 0; x < sTextureWidth; x++) {
    
    
    
    Float alphaValue = Float(0xFF - new_row[x]) / 255.0f;
    Float baseValue = sBackgroundOpacity * (1.0f - alphaValue);
    Color pixelColor(baseValue, baseValue, baseValue, baseValue + alphaValue);
    dst[x] = pixelColor.ToABGR();
  }
}

TextRenderer::~TextRenderer()
{
  if (mGlyphBitmaps) {
    mGlyphBitmaps->Unmap();
  }
}

void
TextRenderer::RenderText(const string& aText, const IntPoint& aOrigin,
                         const Matrix4x4& aTransform, uint32_t aTextSize,
                         uint32_t aTargetPixelWidth)
{
  EnsureInitialized();

  
  
  Float scaleFactor = Float(aTextSize) / Float(sCellHeight);

  aTargetPixelWidth /= scaleFactor;

  uint32_t numLines = 1;
  uint32_t maxWidth = 0;
  uint32_t lineWidth = 0;
  
  for (uint32_t i = 0; i < aText.length(); i++) {
    
    
    
    if (aText[i] == '\n' || (aText[i] == ' ' && lineWidth > aTargetPixelWidth)) {
      numLines++;
      lineWidth = 0;
      continue;
    }

    lineWidth += sGlyphWidths[uint32_t(aText[i])];
    maxWidth = std::max(lineWidth, maxWidth);
  }

  
  RefPtr<DataSourceSurface> textSurf =
    Factory::CreateDataSourceSurface(IntSize(maxWidth, numLines * sCellHeight), sTextureFormat);
  if (NS_WARN_IF(!textSurf)) {
    return;
  }

  DataSourceSurface::MappedSurface map;
  if (NS_WARN_IF(!textSurf->Map(DataSourceSurface::MapType::READ_WRITE, &map))) {
    return;
  }

  
  memset(map.mData, uint8_t(sBackgroundOpacity * 255.0f),
         numLines * sCellHeight * map.mStride);

  uint32_t currentXPos = 0;
  uint32_t currentYPos = 0;

  
  for (uint32_t i = 0; i < aText.length(); i++) {
    if (aText[i] == '\n' || (aText[i] == ' ' && currentXPos > aTargetPixelWidth)) {
      currentYPos += sCellHeight;
      currentXPos = 0;
      continue;
    }

    uint32_t glyphXOffset = aText[i] % (sTextureWidth / sCellWidth) * sCellWidth * BytesPerPixel(sTextureFormat);
    uint32_t truncatedLine = aText[i] / (sTextureWidth / sCellWidth);
    uint32_t glyphYOffset =  truncatedLine * sCellHeight * mMap.mStride;

    for (int y = 0; y < 16; y++) {
      memcpy(map.mData + (y + currentYPos) * map.mStride + currentXPos * BytesPerPixel(sTextureFormat),
             mMap.mData + glyphYOffset + y * mMap.mStride + glyphXOffset,
             sGlyphWidths[uint32_t(aText[i])] * BytesPerPixel(sTextureFormat));
    }

    currentXPos += sGlyphWidths[uint32_t(aText[i])];
  }

  textSurf->Unmap();

  RefPtr<DataTextureSource> src = mCompositor->CreateDataTextureSource();

  if (!src->Update(textSurf)) {
    
    return;
  }

  RefPtr<EffectRGB> effect = new EffectRGB(src, true, Filter::LINEAR);
  EffectChain chain;
  chain.mPrimaryEffect = effect;

  Matrix4x4 transform = aTransform;
  transform.PreScale(scaleFactor, scaleFactor, 1.0f);
  mCompositor->DrawQuad(Rect(aOrigin.x, aOrigin.y, maxWidth, numLines * 16),
                        Rect(-10000, -10000, 20000, 20000), chain, 1.0f, transform);
}

void
TextRenderer::EnsureInitialized()
{
  if (mGlyphBitmaps) {
    return;
  }

  mGlyphBitmaps = Factory::CreateDataSourceSurface(IntSize(sTextureWidth, sTextureHeight), sTextureFormat);
  if (NS_WARN_IF(!mGlyphBitmaps)) {
    return;
  }

  if (NS_WARN_IF(!mGlyphBitmaps->Map(DataSourceSurface::MapType::READ_WRITE, &mMap))) {
    return;
  }

  png_structp png_ptr = NULL;
  png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);

  png_set_progressive_read_fn(png_ptr, this, info_callback, row_callback, nullptr);
  png_infop info_ptr = NULL;
  info_ptr = png_create_info_struct(png_ptr);

  png_process_data(png_ptr, info_ptr, (uint8_t*)sFontPNG, sizeof(sFontPNG));

  png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
}

} 
} 
