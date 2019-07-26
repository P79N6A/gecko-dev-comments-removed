






#ifndef SkFontStream_DEFINED
#define SkFontStream_DEFINED

class SkStream;

#include "SkTypeface.h"

class SkFontStream {
public:
    







    static int CountTTCEntries(SkStream*);

    





    static int GetTableTags(SkStream*, int ttcIndex, SkFontTableTag tags[]);

    





    static size_t GetTableData(SkStream*, int ttcIndex, SkFontTableTag tag,
                               size_t offset, size_t length, void* data);

    static size_t GetTableSize(SkStream* stream, int ttcIndex, SkFontTableTag tag) {
        return GetTableData(stream, ttcIndex, tag, 0, ~0U, NULL);
    }
};

#endif
