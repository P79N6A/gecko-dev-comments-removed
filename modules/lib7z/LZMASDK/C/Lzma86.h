


#ifndef __LZMA86_H
#define __LZMA86_H

#include "Types.h"

EXTERN_C_BEGIN

#define LZMA86_SIZE_OFFSET (1 + 5)
#define LZMA86_HEADER_SIZE (LZMA86_SIZE_OFFSET + 8)



















































enum ESzFilterMode
{
  SZ_FILTER_NO,
  SZ_FILTER_YES,
  SZ_FILTER_AUTO
};

SRes Lzma86_Encode(Byte *dest, size_t *destLen, const Byte *src, size_t srcLen,
    int level, UInt32 dictSize, int filterMode);














SRes Lzma86_GetUnpackSize(const Byte *src, SizeT srcLen, UInt64 *unpackSize);



















SRes Lzma86_Decode(Byte *dest, SizeT *destLen, const Byte *src, SizeT *srcLen);

EXTERN_C_END

#endif
