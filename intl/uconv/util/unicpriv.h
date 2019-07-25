



#ifndef __UNIPRIV__
#define __UNIPRIV__

#include "ubase.h"
#include "umap.h"
#include "uconvutil.h"

#ifdef __cplusplus
extern "C" {
#endif

PRBool	uMapCode(const uTable *uT, 
                    uint16_t in, 
                    uint16_t* out);

PRBool 	uGenerate(uScanClassID scanClass,
                  int32_t* state, 
                  uint16_t in, 
                  unsigned char* out, 
                  uint32_t outbuflen, 
                  uint32_t* outlen);

PRBool 	uScan(uScanClassID scanClass,
              int32_t *state, 
              unsigned char *in,
              uint16_t *out, 
              uint32_t inbuflen, 
              uint32_t* inscanlen);

PRBool 	uGenerateShift(uShiftOutTable *shift,
                       int32_t* state, 
                       uint16_t in, 
                       unsigned char* out, 
                       uint32_t outbuflen, 
                       uint32_t* outlen);

PRBool 	uScanShift(uShiftInTable *shift, 
                   int32_t *state, 
                   unsigned char *in,
                   uint16_t *out, 
                   uint32_t inbuflen, 
                   uint32_t* inscanlen);

#ifdef __cplusplus
}
#endif

#endif
