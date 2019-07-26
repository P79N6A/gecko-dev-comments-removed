#include "SkXfermode.h"
#include "SkXfermode_proccoeff.h"


extern SkProcCoeffXfermode* SkPlatformXfermodeFactory(const ProcCoeff& rec,
                                                      SkXfermode::Mode mode);

extern SkXfermodeProc SkPlatformXfermodeProcFactory(SkXfermode::Mode mode);

SkProcCoeffXfermode* SkPlatformXfermodeFactory(const ProcCoeff& rec,
                                               SkXfermode::Mode mode) {
    return NULL;
}

SkXfermodeProc SkPlatformXfermodeProcFactory(SkXfermode::Mode mode) {
    return NULL;
}
