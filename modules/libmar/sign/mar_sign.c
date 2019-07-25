



#ifdef XP_WIN
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include "mar_private.h"
#include "mar_cmdline.h"
#include "mar.h"
#include "cryptox.h"
#ifndef XP_WIN
#include <unistd.h>
#endif

#include "nss_secutil.h"








int
NSSInitCryptoContext(const char *NSSConfigDir)
{
  SECStatus status = NSS_Initialize(NSSConfigDir, 
                                    "", "", SECMOD_DB, NSS_INIT_READONLY);
  if (SECSuccess != status) {
    fprintf(stderr, "ERROR: Could not initialize NSS\n");
    return -1;
  }

  return 0;
}








int
NSSSignBegin(const char *certName, 
             SGNContext **ctx, 
             SECKEYPrivateKey **privKey, 
             CERTCertificate **cert,
             uint32_t *signatureLength) 
{
  secuPWData pwdata = { PW_NONE, 0 };
  if (!certName || !ctx || !privKey || !cert || !signatureLength) {
    fprintf(stderr, "ERROR: Invalid parameter passed to NSSSignBegin\n");
    return -1;
  }

  
  *cert = PK11_FindCertFromNickname(certName, &pwdata);
  if (!*cert) {
    fprintf(stderr, "ERROR: Could not find cert from nickname\n");
    return -1;
  }

  
  *privKey = PK11_FindKeyByAnyCert(*cert, &pwdata);
  if (!*privKey) {
    fprintf(stderr, "ERROR: Could not find private key\n");
    return -1;
  }

  *signatureLength = PK11_SignatureLen(*privKey);

  if (*signatureLength > BLOCKSIZE) {
    fprintf(stderr, 
            "ERROR: Program must be compiled with a larger block size"
            " to support signing with signatures this large: %u.\n", 
            *signatureLength);
    return -1;
  }

  
  if (*signatureLength < XP_MIN_SIGNATURE_LEN_IN_BYTES) {
    fprintf(stderr, "ERROR: Key length must be >= %d bytes\n", 
            XP_MIN_SIGNATURE_LEN_IN_BYTES);
    return -1;
  }

  *ctx = SGN_NewContext (SEC_OID_ISO_SHA1_WITH_RSA_SIGNATURE, *privKey);
  if (!*ctx) {
    fprintf(stderr, "ERROR: Could not create signature context\n");
    return -1;
  }
  
  if (SGN_Begin(*ctx) != SECSuccess) {
    fprintf(stderr, "ERROR: Could not begin signature\n");
    return -1;
  }
  
  return 0;
}













int
WriteAndUpdateSignature(FILE *fpDest, void *buffer, 
                        uint32_t size, SGNContext *ctx,
                        const char *err) 
{
  if (!size) { 
    return 0;
  }

  if (fwrite(buffer, size, 1, fpDest) != 1) {
    fprintf(stderr, "ERROR: Could not write %s\n", err);
    return -2;
  }
  if (SGN_Update(ctx, (const unsigned char *)buffer, size) != SECSuccess) {
    fprintf(stderr, "ERROR: Could not update signature context for %s\n", err);
    return -3;
  }
  return 0;
}









void
AdjustIndexContentOffsets(char *indexBuf, uint32_t indexLength, uint32_t offsetAmount) 
{
  uint32_t *offsetToContent;
  char *indexBufLoc = indexBuf;

  
  while (indexBufLoc != (indexBuf + indexLength)) {
    
    offsetToContent = (uint32_t *)indexBufLoc; 
    *offsetToContent = ntohl(*offsetToContent);
    *offsetToContent += offsetAmount;
    *offsetToContent = htonl(*offsetToContent);
    
    indexBufLoc += 3 * sizeof(uint32_t);
    indexBufLoc += strlen(indexBufLoc) + 1;
  }
}















int
ReadWriteAndUpdateSignature(FILE *fpSrc, FILE *fpDest, void *buffer, 
                            uint32_t size, SGNContext *ctx,
                            const char *err) 
{
  if (!size) { 
    return 0;
  }

  if (fread(buffer, size, 1, fpSrc) != 1) {
    fprintf(stderr, "ERROR: Could not read %s\n", err);
    return -1;
  }

  return WriteAndUpdateSignature(fpDest, buffer, size, ctx, err);
}














int
ReadAndWrite(FILE *fpSrc, FILE *fpDest, void *buffer, 
             uint32_t size, const char *err) 
{
  if (!size) { 
    return 0;
  }

  if (fread(buffer, size, 1, fpSrc) != 1) {
    fprintf(stderr, "ERROR: Could not read %s\n", err);
    return -1;
  }

  if (fwrite(buffer, size, 1, fpDest) != 1) {
    fprintf(stderr, "ERROR: Could not write %s\n", err);
    return -2;
  }

  return 0;
}










int
strip_signature_block(const char *src, const char * dest)
{
  uint32_t offsetToIndex, dstOffsetToIndex, indexLength, 
    numSignatures = 0, leftOver;
  int32_t stripAmount = 0;
  int64_t oldPos, sizeOfEntireMAR = 0, realSizeOfSrcMAR, numBytesToCopy,
    numChunks, i;
  FILE *fpSrc = NULL, *fpDest = NULL;
  int rv = -1, hasSignatureBlock;
  char buf[BLOCKSIZE];
  char *indexBuf = NULL, *indexBufLoc;

  if (!src || !dest) {
    fprintf(stderr, "ERROR: Invalid parameter passed in.\n");
    return -1;
  }

  fpSrc = fopen(src, "rb");
  if (!fpSrc) {
    fprintf(stderr, "ERROR: could not open source file: %s\n", dest);
    goto failure;
  }

  fpDest = fopen(dest, "wb");
  if (!fpDest) {
    fprintf(stderr, "ERROR: could not create target file: %s\n", dest);
    goto failure;
  }

  
  if (get_mar_file_info(src, &hasSignatureBlock, NULL, NULL, NULL, NULL)) {
    fprintf(stderr, "ERROR: could not determine if MAR is old or new.\n");
    goto failure;
  }

  
  if (ReadAndWrite(fpSrc, fpDest, buf, MAR_ID_SIZE, "MAR ID")) {
    goto failure;
  }

  
  if (fread(&offsetToIndex, sizeof(offsetToIndex), 1, fpSrc) != 1) {
    fprintf(stderr, "ERROR: Could not read offset\n");
    goto failure;
  }
  offsetToIndex = ntohl(offsetToIndex);

  
  oldPos = ftello(fpSrc);
  if (fseeko(fpSrc, 0, SEEK_END)) {
    fprintf(stderr, "ERROR: Could not seek to end of file.\n");
    goto failure;
  }
  realSizeOfSrcMAR = ftello(fpSrc);
  if (fseeko(fpSrc, oldPos, SEEK_SET)) {
    fprintf(stderr, "ERROR: Could not seek back to current location.\n");
    goto failure;
  }

  if (hasSignatureBlock) {
    
    if (fread(&sizeOfEntireMAR, 
              sizeof(sizeOfEntireMAR), 1, fpSrc) != 1) {
      fprintf(stderr, "ERROR: Could read mar size\n");
      goto failure;
    }
    sizeOfEntireMAR = NETWORK_TO_HOST64(sizeOfEntireMAR);
    if (sizeOfEntireMAR != realSizeOfSrcMAR) {
      fprintf(stderr, "ERROR: Source MAR is not of the right size\n");
      goto failure;
    }
  
    
    if (fread(&numSignatures, sizeof(numSignatures), 1, fpSrc) != 1) {
      fprintf(stderr, "ERROR: Could read num signatures\n");
      goto failure;
    }
    numSignatures = ntohl(numSignatures);

    for (i = 0; i < numSignatures; i++) {
      uint32_t signatureLen;

      
      if (fseeko(fpSrc, sizeof(uint32_t), SEEK_CUR)) {
        fprintf(stderr, "ERROR: Could not skip past signature algorithm ID\n");
      }

      
      if (fread(&signatureLen, sizeof(uint32_t), 1, fpSrc) != 1) {
        fprintf(stderr, "ERROR: Could not read signatures length.\n");
        return CryptoX_Error;
      }
      signatureLen = ntohl(signatureLen);

      
      if (fseeko(fpSrc, signatureLen, SEEK_CUR)) {
        fprintf(stderr, "ERROR: Could not skip past signature algorithm ID\n");
      }

      stripAmount += sizeof(uint32_t) + sizeof(uint32_t) + signatureLen; 
    }

  } else {
    sizeOfEntireMAR = realSizeOfSrcMAR;
    numSignatures = 0;
  }

  if (((int64_t)offsetToIndex) > sizeOfEntireMAR) {
    fprintf(stderr, "ERROR: Offset to index is larger than the file size.\n");
    goto failure;
  }

  dstOffsetToIndex = offsetToIndex;
  if (!hasSignatureBlock) {
    dstOffsetToIndex += sizeof(sizeOfEntireMAR) + sizeof(numSignatures);
  }
  dstOffsetToIndex -= stripAmount;

  
  dstOffsetToIndex = htonl(dstOffsetToIndex);
  if (fwrite(&dstOffsetToIndex, sizeof(dstOffsetToIndex), 1, fpDest) != 1) {
    fprintf(stderr, "ERROR: Could not write offset to index\n");
    goto failure;
  }
  dstOffsetToIndex = ntohl(dstOffsetToIndex);

  
  if (!hasSignatureBlock) {
    sizeOfEntireMAR += sizeof(sizeOfEntireMAR) + sizeof(numSignatures);
  }
  sizeOfEntireMAR -= stripAmount;

  
  sizeOfEntireMAR = HOST_TO_NETWORK64(sizeOfEntireMAR);
  if (fwrite(&sizeOfEntireMAR, sizeof(sizeOfEntireMAR), 1, fpDest) != 1) {
    fprintf(stderr, "ERROR: Could not write size of MAR\n");
    goto failure;
  }
  sizeOfEntireMAR = NETWORK_TO_HOST64(sizeOfEntireMAR);

  
  numSignatures = 0;
  if (fwrite(&numSignatures, sizeof(numSignatures), 1, fpDest) != 1) {
    fprintf(stderr, "ERROR: Could not write out num signatures\n");
    goto failure;
  }

  


  if (ftello(fpSrc) > ((int64_t)offsetToIndex)) {
    fprintf(stderr, "ERROR: Index offset is too small.\n");
    goto failure;
  }
  numBytesToCopy = ((int64_t)offsetToIndex) - ftello(fpSrc);
  numChunks = numBytesToCopy / BLOCKSIZE;
  leftOver = numBytesToCopy % BLOCKSIZE;

  
  for (i = 0; i < numChunks; ++i) {
    if (ReadAndWrite(fpSrc, fpDest, buf, BLOCKSIZE, "content block")) {
      goto failure;
    }
  }

  
  if (ReadAndWrite(fpSrc, fpDest, buf, 
                   leftOver, "left over content block")) {
    goto failure;
  }

  
  if (ReadAndWrite(fpSrc, fpDest, &indexLength, 
                   sizeof(indexLength), "index length")) {
    goto failure;
  }
  indexLength = ntohl(indexLength);

  
  indexBuf = malloc(indexLength);
  indexBufLoc = indexBuf;
  if (fread(indexBuf, indexLength, 1, fpSrc) != 1) {
    fprintf(stderr, "ERROR: Could not read index\n");
    goto failure;
  }

  
  if (hasSignatureBlock) {
    AdjustIndexContentOffsets(indexBuf, indexLength, -stripAmount);
  } else {
    AdjustIndexContentOffsets(indexBuf, indexLength, 
                              sizeof(sizeOfEntireMAR) + 
                              sizeof(numSignatures) - 
                              stripAmount);
  }

  if (fwrite(indexBuf, indexLength, 1, fpDest) != 1) {
    fprintf(stderr, "ERROR: Could not write index\n");
    goto failure;
  }

  rv = 0;
failure: 
  if (fpSrc) {
    fclose(fpSrc);
  }

  if (fpDest) {
    fclose(fpDest);
  }

  if (rv) {
    remove(dest);
  }

  if (indexBuf) { 
    free(indexBuf);
  }

  if (rv) {
    remove(dest);
  }
  return rv;
}













int
mar_repackage_and_sign(const char *NSSConfigDir, 
                       const char *certName, 
                       const char *src, 
                       const char *dest) 
{
  uint32_t offsetToIndex, dstOffsetToIndex, indexLength, 
    numSignatures = 0, signatureLength, leftOver,
    signatureAlgorithmID, signatureSectionLength;
  int64_t oldPos, sizeOfEntireMAR = 0, realSizeOfSrcMAR, 
    signaturePlaceholderOffset, numBytesToCopy, 
    numChunks, i;
  FILE *fpSrc = NULL, *fpDest = NULL;
  int rv = -1, hasSignatureBlock;
  SGNContext *ctx = NULL;
  SECItem secItem;
  char buf[BLOCKSIZE];
  SECKEYPrivateKey *privKey = NULL;
  CERTCertificate *cert = NULL; 
  char *indexBuf = NULL, *indexBufLoc;

  if (!NSSConfigDir || !certName || !src || !dest) {
    fprintf(stderr, "ERROR: Invalid parameter passed in.\n");
    return -1;
  }

  if (NSSInitCryptoContext(NSSConfigDir)) {
    fprintf(stderr, "ERROR: Could not init config dir: %s\n", NSSConfigDir);
    goto failure;
  }

  PK11_SetPasswordFunc(SECU_GetModulePassword);

  if (NSSSignBegin(certName, &ctx, &privKey, &cert, &signatureLength)) {
    fprintf(stderr, "ERROR: NSSSignBegin failed\n");
    goto failure;
  }
  
  fpSrc = fopen(src, "rb");
  if (!fpSrc) {
    fprintf(stderr, "ERROR: could not open source file: %s\n", dest);
    goto failure;
  }

  fpDest = fopen(dest, "wb");
  if (!fpDest) {
    fprintf(stderr, "ERROR: could not create target file: %s\n", dest);
    goto failure;
  }

  
  if (get_mar_file_info(src, &hasSignatureBlock, NULL, NULL, NULL, NULL)) {
    fprintf(stderr, "ERROR: could not determine if MAR is old or new.\n");
    goto failure;
  }

  
  if (ReadWriteAndUpdateSignature(fpSrc, fpDest, 
                                  buf, MAR_ID_SIZE, 
                                  ctx, "MAR ID")) {
    goto failure;
  }

  
  if (fread(&offsetToIndex, sizeof(offsetToIndex), 1, fpSrc) != 1) {
    fprintf(stderr, "ERROR: Could not read offset\n");
    goto failure;
  }
  offsetToIndex = ntohl(offsetToIndex);

  
  oldPos = ftello(fpSrc);
  if (fseeko(fpSrc, 0, SEEK_END)) {
    fprintf(stderr, "ERROR: Could not seek to end of file.\n");
    goto failure;
  }
  realSizeOfSrcMAR = ftello(fpSrc);
  if (fseeko(fpSrc, oldPos, SEEK_SET)) {
    fprintf(stderr, "ERROR: Could not seek back to current location.\n");
    goto failure;
  }

  if (hasSignatureBlock) {
    
    if (fread(&sizeOfEntireMAR, 
              sizeof(sizeOfEntireMAR), 1, fpSrc) != 1) {
      fprintf(stderr, "ERROR: Could read mar size\n");
      goto failure;
    }
    sizeOfEntireMAR = NETWORK_TO_HOST64(sizeOfEntireMAR);
    if (sizeOfEntireMAR != realSizeOfSrcMAR) {
      fprintf(stderr, "ERROR: Source MAR is not of the right size\n");
      goto failure;
    }
  
    
    if (fread(&numSignatures, sizeof(numSignatures), 1, fpSrc) != 1) {
      fprintf(stderr, "ERROR: Could read num signatures\n");
      goto failure;
    }
    numSignatures = ntohl(numSignatures);

    
    if (numSignatures) {
      fprintf(stderr, "ERROR: MAR is already signed\n");
      goto failure;
    }
  } else {
    sizeOfEntireMAR = realSizeOfSrcMAR;
  }

  if (((int64_t)offsetToIndex) > sizeOfEntireMAR) {
    fprintf(stderr, "ERROR: Offset to index is larger than the file size.\n");
    goto failure;
  }

  
  signatureSectionLength = sizeof(signatureAlgorithmID) + 
                           sizeof(signatureLength) +
                           signatureLength;
  dstOffsetToIndex = offsetToIndex;
  if (!hasSignatureBlock) {
    dstOffsetToIndex += sizeof(sizeOfEntireMAR) + sizeof(numSignatures);
  }
  dstOffsetToIndex += signatureSectionLength;

  
  dstOffsetToIndex = htonl(dstOffsetToIndex);
  if (WriteAndUpdateSignature(fpDest, &dstOffsetToIndex, 
                              sizeof(dstOffsetToIndex), ctx, "index offset")) {
    goto failure;
  }
  dstOffsetToIndex = ntohl(dstOffsetToIndex);

  
  sizeOfEntireMAR += signatureSectionLength;
  if (!hasSignatureBlock) {
    sizeOfEntireMAR += sizeof(sizeOfEntireMAR) + sizeof(numSignatures);
  }

  
  sizeOfEntireMAR = HOST_TO_NETWORK64(sizeOfEntireMAR);
  if (WriteAndUpdateSignature(fpDest, &sizeOfEntireMAR, 
                              sizeof(sizeOfEntireMAR), ctx, "size of MAR")) {
    goto failure;
  }
  sizeOfEntireMAR = NETWORK_TO_HOST64(sizeOfEntireMAR);

  
  numSignatures = 1;
  numSignatures = htonl(numSignatures);
  if (WriteAndUpdateSignature(fpDest, &numSignatures, 
                              sizeof(numSignatures), ctx, "num signatures")) {
    goto failure;
  }
  numSignatures = ntohl(numSignatures);

  
  signatureAlgorithmID = htonl(1);
  if (WriteAndUpdateSignature(fpDest, &signatureAlgorithmID, 
                              sizeof(signatureAlgorithmID), 
                              ctx, "num signatures")) {
    goto failure;
  }
  signatureAlgorithmID = ntohl(signatureAlgorithmID);

  
  signatureLength = htonl(signatureLength);
  if (WriteAndUpdateSignature(fpDest, &signatureLength, 
                              sizeof(signatureLength), 
                              ctx, "signature length")) {
    goto failure;
  }
  signatureLength = ntohl(signatureLength);

  


  memset(buf, 0, sizeof(buf));
  signaturePlaceholderOffset = ftello(fpDest);
  if (fwrite(buf, signatureLength, 1, fpDest) != 1) {
    fprintf(stderr, "ERROR: Could not write signature length\n");
    goto failure;
  }

  


  if (ftello(fpSrc) > ((int64_t)offsetToIndex)) {
    fprintf(stderr, "ERROR: Index offset is too small.\n");
    goto failure;
  }
  numBytesToCopy = ((int64_t)offsetToIndex) - ftello(fpSrc);
  numChunks = numBytesToCopy / BLOCKSIZE;
  leftOver = numBytesToCopy % BLOCKSIZE;

  
  for (i = 0; i < numChunks; ++i) {
    if (ReadWriteAndUpdateSignature(fpSrc, fpDest, buf, 
                                    BLOCKSIZE, ctx, "content block")) {
      goto failure;
    }
  }

  
  if (ReadWriteAndUpdateSignature(fpSrc, fpDest, buf, 
                                  leftOver, ctx, "left over content block")) {
    goto failure;
  }

  
  if (ReadWriteAndUpdateSignature(fpSrc, fpDest, &indexLength, 
                                  sizeof(indexLength), ctx, "index length")) {
    goto failure;
  }
  indexLength = ntohl(indexLength);

  
  indexBuf = malloc(indexLength);
  indexBufLoc = indexBuf;
  if (fread(indexBuf, indexLength, 1, fpSrc) != 1) {
    fprintf(stderr, "ERROR: Could not read index\n");
    goto failure;
  }

  
  if (hasSignatureBlock) {
    AdjustIndexContentOffsets(indexBuf, indexLength, signatureSectionLength);
  } else {
    AdjustIndexContentOffsets(indexBuf, indexLength, 
                              sizeof(sizeOfEntireMAR) + 
                              sizeof(numSignatures) + 
                              signatureSectionLength);
  }

  if (WriteAndUpdateSignature(fpDest, indexBuf, 
                              indexLength, ctx, "index")) {
    goto failure;
  }

  

  if (ftello(fpDest) > MAX_SIZE_OF_MAR_FILE) {
    goto failure;
  }

  
  if (SGN_End(ctx, &secItem) != SECSuccess) {
    fprintf(stderr, "ERROR: Could not end signature context\n");
    goto failure;
  }
  if (signatureLength != secItem.len) {
    fprintf(stderr, "ERROR: Signature is not the expected length\n");
    goto failure;
  }

  
  if (fseeko(fpDest, signaturePlaceholderOffset, SEEK_SET)) {
    fprintf(stderr, "ERROR: Could not seek to signature offset\n");
    goto failure;
  }

  

  if (fwrite(secItem.data, secItem.len, 1, fpDest) != 1) {
    fprintf(stderr, "ERROR: Could not write signature\n");
    goto failure;
  }

  rv = 0;
failure: 
  if (fpSrc) {
    fclose(fpSrc);
  }

  if (fpDest) {
    fclose(fpDest);
  }

  if (rv) {
    remove(dest);
  }

  if (indexBuf) { 
    free(indexBuf);
  }

  if (ctx) {
    SGN_DestroyContext(ctx, PR_TRUE);
  }

  if (cert) {
    CERT_DestroyCertificate(cert);
  }

  if (privKey) {
    SECKEY_DestroyPrivateKey(privKey);
  }

  if (rv) {
    remove(dest);
  }
  return rv;
}
