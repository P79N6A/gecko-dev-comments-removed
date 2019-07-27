

















#include "intmath.h"


#undef av_log2
#undef av_log2_16bit
#include "common.h"

int av_log2(unsigned v)
{
    return ff_log2(v);
}

int av_log2_16bit(unsigned v)
{
    return ff_log2_16bit(v);
}

int av_ctz(int v)
{
    return ff_ctz(v);
}
