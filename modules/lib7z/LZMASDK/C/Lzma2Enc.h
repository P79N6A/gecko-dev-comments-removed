


#ifndef __LZMA2_ENC_H
#define __LZMA2_ENC_H

#include "LzmaEnc.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
  CLzmaEncProps lzmaProps;
  size_t blockSize;
  int numBlockThreads;
  int numTotalThreads;
} CLzma2EncProps;

void Lzma2EncProps_Init(CLzma2EncProps *p);
void Lzma2EncProps_Normalize(CLzma2EncProps *p);













typedef void * CLzma2EncHandle;

CLzma2EncHandle Lzma2Enc_Create(ISzAlloc *alloc, ISzAlloc *allocBig);
void Lzma2Enc_Destroy(CLzma2EncHandle p);
SRes Lzma2Enc_SetProps(CLzma2EncHandle p, const CLzma2EncProps *props);
Byte Lzma2Enc_WriteProperties(CLzma2EncHandle p);
SRes Lzma2Enc_Encode(CLzma2EncHandle p,
    ISeqOutStream *outStream, ISeqInStream *inStream, ICompressProgress *progress);


















#ifdef __cplusplus
}
#endif

#endif
