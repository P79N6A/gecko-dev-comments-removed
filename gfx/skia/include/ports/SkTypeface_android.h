







#ifndef SkTypeface_android_DEFINED
#define SkTypeface_android_DEFINED

#include "SkTypeface.h"

enum FallbackScripts {
    kArabic_FallbackScript,
    kEthiopic_FallbackScript,
    kHebrewRegular_FallbackScript,
    kHebrewBold_FallbackScript,
    kThai_FallbackScript,
    kArmenian_FallbackScript,
    kGeorgian_FallbackScript,
    kDevanagari_FallbackScript,
    kBengali_FallbackScript,
    kTamil_FallbackScript,
    kFallbackScriptNumber
};

#define SkTypeface_ValidScript(s) (s >= 0 && s < kFallbackScriptNumber)








SK_API SkTypeface* SkCreateTypefaceForScript(FallbackScripts script);





SK_API const char* SkGetFallbackScriptID(FallbackScripts script);






SK_API FallbackScripts SkGetFallbackScriptFromID(const char* id);

#endif
