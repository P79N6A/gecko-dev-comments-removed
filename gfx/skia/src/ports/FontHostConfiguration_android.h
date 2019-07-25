















#ifndef FONTHOSTCONFIGURATION_ANDROID_H_
#define FONTHOSTCONFIGURATION_ANDROID_H_

#include "SkTDArray.h"









struct FontFamily {
    SkTDArray<const char*>  fNames;
    SkTDArray<const char*>  fFileNames;
    int order;
};





void getFontFamilies(SkTDArray<FontFamily*> &fontFamilies);

#endif 
