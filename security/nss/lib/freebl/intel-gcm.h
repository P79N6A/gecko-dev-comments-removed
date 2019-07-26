




















#ifndef INTEL_GCM_H
#define INTEL_GCM_H 1

#include "blapii.h"

typedef struct intel_AES_GCMContextStr intel_AES_GCMContext;

intel_AES_GCMContext *intel_AES_GCM_CreateContext(void *context, freeblCipherFunc cipher,
			const unsigned char *params, unsigned int blocksize);

void intel_AES_GCM_DestroyContext(intel_AES_GCMContext *gcm, PRBool freeit);

SECStatus intel_AES_GCM_EncryptUpdate(intel_AES_GCMContext  *gcm, unsigned char *outbuf,
			unsigned int *outlen, unsigned int maxout,
			const unsigned char *inbuf, unsigned int inlen,
			unsigned int blocksize);

SECStatus intel_AES_GCM_DecryptUpdate(intel_AES_GCMContext *gcm, unsigned char *outbuf,
			unsigned int *outlen, unsigned int maxout,
			const unsigned char *inbuf, unsigned int inlen,
			unsigned int blocksize);





       

void intel_aes_gcmINIT(unsigned char Htbl[16*16],
                       unsigned char *KS,
                       int NR);


void intel_aes_gcmTAG(unsigned char Htbl[16*16], 
                      unsigned char *Tp, 
                      unsigned long Mlen, 
                      unsigned long Alen, 
                      unsigned char* X0, 
                      unsigned char* TAG);



void intel_aes_gcmAAD(unsigned char Htbl[16*16], 
                      unsigned char *AAD, 
                      unsigned long Alen, 
                      unsigned char *Tp);




void intel_aes_gcmENC(const unsigned char* PT, 
                      unsigned char* CT, 
                      void *Gctx, 
                      unsigned long len);
                  

void intel_aes_gcmDEC(const unsigned char* CT, 
                      unsigned char* PT, 
                      void *Gctx, 
                      unsigned long len);

#endif
