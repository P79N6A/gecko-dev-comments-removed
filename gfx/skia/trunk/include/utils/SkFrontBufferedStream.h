






#include "SkTypes.h"

class SkStream;
class SkStreamRewindable;










class SkFrontBufferedStream {
public:
    









    static SkStreamRewindable* Create(SkStream* stream, size_t minBufferSize);
};
