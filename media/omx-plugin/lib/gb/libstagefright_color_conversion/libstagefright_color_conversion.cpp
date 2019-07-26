





#include "mozilla/Types.h"
#include "stagefright/ColorConverter.h"

namespace android {

MOZ_EXPORT ColorConverter::ColorConverter(OMX_COLOR_FORMATTYPE srcFormat,
                                          OMX_COLOR_FORMATTYPE dstFormat)
{
}

MOZ_EXPORT bool ColorConverter::isValid() const { return false; }

MOZ_EXPORT void ColorConverter::convert(unsigned int, unsigned int,
                                        const void*, unsigned int,
                                        void*, unsigned int)
{
}

MOZ_EXPORT ColorConverter::~ColorConverter() { }

} 
