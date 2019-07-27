







#ifndef SkTypeface_android_DEFINED
#define SkTypeface_android_DEFINED

#include "SkTypeface.h"

#ifdef SK_BUILD_FOR_ANDROID

class SkPaintOptionsAndroid;
















SK_API bool SkGetFallbackFamilyNameForChar(SkUnichar uni, const char* lang, SkString* name);





SK_API void SkUseTestFontConfigFile(const char* mainconf, const char* fallbackconf,
                                    const char* fontsdir);
















SkTypeface* SkAndroidNextLogicalTypeface(SkFontID currFontID, SkFontID origFontID,
                                         const SkPaintOptionsAndroid& options);













SkTypeface* SkGetTypefaceForGlyphID(uint16_t glyphID, const SkTypeface* origTypeface,
                                    const SkPaintOptionsAndroid& options,
                                    int* lowerBounds = NULL, int* upperBounds = NULL);

#endif 
#endif 
