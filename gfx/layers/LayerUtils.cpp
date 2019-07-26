




#include "LayerUtils.h"
#include "PremultiplyTables.h"
#include "mozilla/Endian.h"

namespace mozilla {
namespace layers {

using namespace mozilla::gfx;

static inline const uint8_t PremultiplyValue(uint8_t a, uint8_t v) {
  return PremultiplyTable[a*256+v];
}

static inline const uint8_t UnpremultiplyValue(uint8_t a, uint8_t v) {
  return UnpremultiplyTable[a*256+v];
}

#ifdef DEBUG
static bool IsLittleEndian()
{
    
    
    uint16_t testShort;
    static const uint8_t testBytes[2] = { 0xAA, 0xBB };
    memcpy(&testShort, testBytes, sizeof(testBytes));
    return testShort == 0xBBAA;
}
#endif 

#ifdef MOZ_LITTLE_ENDIAN
#define ASSERT_ENDIAN() MOZ_ASSERT(IsLittleEndian(), "Defined as little endian, but actually big!")
#else
#define ASSERT_ENDIAN() MOZ_ASSERT(!IsLittleEndian(), "Defined as big endian, but actually little!")
#endif

void
PremultiplySurface(DataSourceSurface* srcSurface,
                   DataSourceSurface* destSurface)
{
  if (!destSurface)
    destSurface = srcSurface;

  IntSize srcSize = srcSurface->GetSize();
  MOZ_ASSERT(srcSurface->GetFormat() == destSurface->GetFormat() &&
             srcSize.width  == destSurface->GetSize().width &&
             srcSize.height  == destSurface->GetSize().height &&
             srcSurface->Stride() == destSurface->Stride(),
             "Source and destination surfaces don't have identical characteristics");

  MOZ_ASSERT(srcSurface->Stride() == srcSize.width * 4,
             "Source surface stride isn't tightly packed");

  
  if (srcSurface->GetFormat() != SurfaceFormat::B8G8R8A8) {
    if (destSurface != srcSurface) {
      memcpy(destSurface->GetData(), srcSurface->GetData(),
             srcSurface->Stride() * srcSize.height);
    }
    return;
  }

  uint8_t *src = srcSurface->GetData();
  uint8_t *dst = destSurface->GetData();

  
  ASSERT_ENDIAN();

  uint32_t dim = srcSize.width * srcSize.height;
  for (uint32_t i = 0; i < dim; ++i) {
#ifdef MOZ_LITTLE_ENDIAN
    uint8_t b = *src++;
    uint8_t g = *src++;
    uint8_t r = *src++;
    uint8_t a = *src++;

    *dst++ = PremultiplyValue(a, b);
    *dst++ = PremultiplyValue(a, g);
    *dst++ = PremultiplyValue(a, r);
    *dst++ = a;
#else
    uint8_t a = *src++;
    uint8_t r = *src++;
    uint8_t g = *src++;
    uint8_t b = *src++;

    *dst++ = a;
    *dst++ = PremultiplyValue(a, r);
    *dst++ = PremultiplyValue(a, g);
    *dst++ = PremultiplyValue(a, b);
#endif
  }
}


}
}
