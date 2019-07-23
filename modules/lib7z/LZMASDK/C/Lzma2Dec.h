


#ifndef __LZMA2_DEC_H
#define __LZMA2_DEC_H

#include "LzmaDec.h"

#ifdef __cplusplus
extern "C" {
#endif



typedef struct
{
  CLzmaDec decoder;
  UInt32 packSize;
  UInt32 unpackSize;
  int state;
  Byte control;
  Bool needInitDic;
  Bool needInitState;
  Bool needInitProp;
} CLzma2Dec;

#define Lzma2Dec_Construct(p) LzmaDec_Construct(&(p)->decoder)
#define Lzma2Dec_FreeProbs(p, alloc) LzmaDec_FreeProbs(&(p)->decoder, alloc);
#define Lzma2Dec_Free(p, alloc) LzmaDec_Free(&(p)->decoder, alloc);

SRes Lzma2Dec_AllocateProbs(CLzma2Dec *p, Byte prop, ISzAlloc *alloc);
SRes Lzma2Dec_Allocate(CLzma2Dec *p, Byte prop, ISzAlloc *alloc);
void Lzma2Dec_Init(CLzma2Dec *p);

















SRes Lzma2Dec_DecodeToDic(CLzma2Dec *p, SizeT dicLimit,
    const Byte *src, SizeT *srcLen, ELzmaFinishMode finishMode, ELzmaStatus *status);

SRes Lzma2Dec_DecodeToBuf(CLzma2Dec *p, Byte *dest, SizeT *destLen,
    const Byte *src, SizeT *srcLen, ELzmaFinishMode finishMode, ELzmaStatus *status);





















SRes Lzma2Decode(Byte *dest, SizeT *destLen, const Byte *src, SizeT *srcLen,
    Byte prop, ELzmaFinishMode finishMode, ELzmaStatus *status, ISzAlloc *alloc);

#ifdef __cplusplus
}
#endif

#endif
