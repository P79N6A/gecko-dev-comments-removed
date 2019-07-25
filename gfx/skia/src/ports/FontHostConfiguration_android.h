















#ifndef FONTHOSTCONFIGURATION_ANDROID_H_
#define FONTHOSTCONFIGURATION_ANDROID_H_

#include "SkTDArray.h"









struct FontFamily {
    SkTDArray<const char*>  fNames;
    SkTDArray<const char*>  fFileNames;
    int order;
};





void getFontFamilies(SkTDArray<FontFamily*> &fontFamilies);





void getSystemFontFamilies(SkTDArray<FontFamily*> &fontFamilies);






void getFallbackFontFamilies(SkTDArray<FontFamily*> &fallbackFonts);

struct AndroidLocale {
    char language[3];
    char region[3];
};

void getLocale(AndroidLocale &locale);

#endif 
