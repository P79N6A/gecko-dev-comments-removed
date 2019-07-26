






#ifndef SKFONTCONFIGPARSER_ANDROID_H_
#define SKFONTCONFIGPARSER_ANDROID_H_

#include "SkTypes.h"

#include "SkPaintOptionsAndroid.h"
#include "SkString.h"
#include "SkTDArray.h"

struct FontFileInfo {
    FontFileInfo() : fFileName(NULL) {}

    const char*           fFileName;
    SkPaintOptionsAndroid fPaintOptions;
};









struct FontFamily {
    FontFamily() : fIsFallbackFont(false), order(-1) {}

    SkTDArray<const char*>   fNames;
    SkTDArray<FontFileInfo*> fFontFiles;
    bool fIsFallbackFont;
    int order; 
};

namespace SkFontConfigParser {





void GetFontFamilies(SkTDArray<FontFamily*> &fontFamilies);





void GetTestFontFamilies(SkTDArray<FontFamily*> &fontFamilies,
                         const char* testMainConfigFile,
                         const char* testFallbackConfigFile);

SkString GetLocale();

} 

#endif 
