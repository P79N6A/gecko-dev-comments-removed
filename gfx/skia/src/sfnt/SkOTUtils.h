






#ifndef SkOTUtils_DEFINED
#define SkOTUtils_DEFINED

#include "SkOTTableTypes.h"
class SkData;
class SkStream;

struct SkOTUtils {
    


    static uint32_t CalcTableChecksum(SK_OT_ULONG *data, size_t length);

    












    static SkData* RenameFont(SkStream* fontData, const char* fontName, int fontNameLen);
};

#endif
