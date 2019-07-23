


#ifndef __BCJ2_H
#define __BCJ2_H

#include "Types.h"

#ifdef __cplusplus
extern "C" {
#endif















int Bcj2_Decode(
    const Byte *buf0, SizeT size0,
    const Byte *buf1, SizeT size1,
    const Byte *buf2, SizeT size2,
    const Byte *buf3, SizeT size3,
    Byte *outBuf, SizeT outSize);

#ifdef __cplusplus
}
#endif

#endif
