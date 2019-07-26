







#ifndef SkTypeface_android_DEFINED
#define SkTypeface_android_DEFINED

#include "SkTypeface.h"

enum FallbackScripts {
    kArabic_FallbackScript,
    kArmenian_FallbackScript,
    kBengali_FallbackScript,
    kDevanagari_FallbackScript,
    kEthiopic_FallbackScript,
    kGeorgian_FallbackScript,
    kHebrewRegular_FallbackScript,
    kHebrewBold_FallbackScript,
    kKannada_FallbackScript,
    kMalayalam_FallbackScript,
    kTamilRegular_FallbackScript,
    kTamilBold_FallbackScript,
    kThai_FallbackScript,
    kTelugu_FallbackScript,
    kFallbackScriptNumber
};




#define kTamil_FallbackScript kTamilRegular_FallbackScript

#define SkTypeface_ValidScript(s) (s >= 0 && s < kFallbackScriptNumber)








SK_API SkTypeface* SkCreateTypefaceForScript(FallbackScripts script);





SK_API const char* SkGetFallbackScriptID(FallbackScripts script);






SK_API FallbackScripts SkGetFallbackScriptFromID(const char* id);





SK_API void SkUseTestFontConfigFile(const char* mainconf, const char* fallbackconf,
                                    const char* fontsdir);

#endif
