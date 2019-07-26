







#ifndef SkTypeface_android_DEFINED
#define SkTypeface_android_DEFINED

#include "SkTypeface.h"

#ifdef SK_BUILD_FOR_ANDROID

class SkPaintOptionsAndroid;












SK_API bool SkGetFallbackFamilyNameForChar(SkUnichar uni, SkString* name);
















SK_API bool SkGetFallbackFamilyNameForChar(SkUnichar uni, const char* lang, SkString* name);





SK_API void SkUseTestFontConfigFile(const char* mainconf, const char* fallbackconf,
                                    const char* fontsdir);
















SkTypeface* SkAndroidNextLogicalTypeface(SkFontID currFontID, SkFontID origFontID,
                                         const SkPaintOptionsAndroid& options);













SkTypeface* SkGetTypefaceForGlyphID(uint16_t glyphID, const SkTypeface* origTypeface,
                                    const SkPaintOptionsAndroid& options,
                                    int* lowerBounds = NULL, int* upperBounds = NULL);

#endif 
#ifdef SK_BUILD_FOR_ANDROID_FRAMEWORK

#include "SkPaintOptionsAndroid.h"
#include "../harfbuzz_ng/src/hb.h"










SK_API SkTypeface* SkCreateTypefaceForScript(hb_script_t script, SkTypeface::Style style,
        SkPaintOptionsAndroid::FontVariant fontVariant = SkPaintOptionsAndroid::kDefault_Variant);

#endif 
#endif 
