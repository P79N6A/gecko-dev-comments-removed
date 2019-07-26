



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
#include "base64.h"








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
WriteAndUpdateSignatures(FILE *fpDest, void *buffer,
                         uint32_t size, SGNContext **ctxs,
                         uint32_t ctxCount,
                         const char *err)
{
  uint32_t k;
  if (!size) { 
    return 0;
  }

  if (fwrite(buffer, size, 1, fpDest) != 1) {
    fprintf(stderr, "ERROR: Could not write %s\n", err);
    return -2;
  }

  for (k = 0; k < ctxCount; ++k) {
    if (SGN_Update(ctxs[k], buffer, size) != SECSuccess) {
      fprintf(stderr, "ERROR: Could not update signature context for %s\n", err);
      return -3;
    }
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
ReadWriteAndUpdateSignatures(FILE *fpSrc, FILE *fpDest, void *buffer,
                             uint32_t size, SGNContext **ctxs,
                             uint32_t ctxCount,
                             const char *err)
{
  if (!size) { 
    return 0;
  }

  if (fread(buffer, size, 1, fpSrc) != 1) {
    fprintf(stderr, "ERROR: Could not read %s\n", err);
    return -1;
  }

  return WriteAndUpdateSignatures(fpDest, buffer, size, ctxs, ctxCount, err);
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
extract_signature(const char *src, uint32_t sigIndex, const char * dest)
{
  FILE *fpSrc = NULL, *fpDest = NULL;
  uint32_t i;
  uint32_t signatureCount;
  uint32_t signatureLen;
  uint8_t *extractedSignature = NULL;
  char *base64Encoded = NULL;
  int rv = -1;
  if (!src || !dest) {
    fprintf(stderr, "ERROR: Invalid parameter passed in.\n");
    goto failure;
  }

  fpSrc = fopen(src, "rb");
  if (!fpSrc) {
    fprintf(stderr, "ERROR: could not open source file: %s\n", src);
    goto failure;
  }

  fpDest = fopen(dest, "wb");
  if (!fpDest) {
    fprintf(stderr, "ERROR: could not create target file: %s\n", dest);
    goto failure;
  }

  
  if (fseeko(fpSrc, SIGNATURE_BLOCK_OFFSET, SEEK_SET)) {
    fprintf(stderr, "ERROR: could not seek to signature block\n");
    goto failure;
  }

  
  if (fread(&signatureCount, sizeof(signatureCount), 1, fpSrc) != 1) {
    fprintf(stderr, "ERROR: could not read signature count\n");
    goto failure;
  }
  signatureCount = ntohl(signatureCount);
  if (sigIndex >= signatureCount) {
    fprintf(stderr, "ERROR: Signature index was out of range\n");
    goto failure;
  }

  
  for (i = 0; i <= sigIndex; i++) {
    
    if (fseeko(fpSrc, sizeof(uint32_t), SEEK_CUR)) {
      fprintf(stderr, "ERROR: Could not seek past sig algorithm ID.\n");
      goto failure;
    }

    
    if (fread(&signatureLen, sizeof(signatureLen), 1, fpSrc) != 1) {
      fprintf(stderr, "ERROR: could not read signature length\n");
      goto failure;
    }
    signatureLen = ntohl(signatureLen);

    
    extractedSignature = malloc(signatureLen);
    if (fread(extractedSignature, signatureLen, 1, fpSrc) != 1) {
      fprintf(stderr, "ERROR: could not read signature\n");
      goto failure;
    }
  }

  base64Encoded = BTOA_DataToAscii(extractedSignature, signatureLen);
  if (!base64Encoded) {
    fprintf(stderr, "ERROR: could not obtain base64 encoded data\n");
    goto failure;
  }

  if (fwrite(base64Encoded, strlen(base64Encoded), 1, fpDest) != 1) {
    fprintf(stderr, "ERROR: Could not write base64 encoded string\n");
    goto failure;
  }

  rv = 0;
failure:
  if (base64Encoded) {
    PORT_Free(base64Encoded);
  }

  if (extractedSignature) {
    free(extractedSignature);
  }

  if (fpSrc) {
    fclose(fpSrc);
  }

  if (fpDest) {
    fclose(fpDest);
  }

  if (rv) {
    remove(dest);
  }

  return rv;
}
















int
mar_repackage_and_sign(const char *NSSConfigDir, 
                       const char * const *certNames,
                       uint32_t certCount,
                       const char *src, 
                       const char *dest) 
{
  uint32_t offsetToIndex, dstOffsetToIndex, indexLength, 
    numSignatures = 0, leftOver,
    signatureAlgorithmID, signatureSectionLength = 0;
  uint32_t signatureLengths[MAX_SIGNATURES];
  int64_t oldPos, sizeOfEntireMAR = 0, realSizeOfSrcMAR, 
    signaturePlaceholderOffset, numBytesToCopy, 
    numChunks, i;
  FILE *fpSrc = NULL, *fpDest = NULL;
  int rv = -1, hasSignatureBlock;
  SGNContext *ctxs[MAX_SIGNATURES];
  SECItem secItems[MAX_SIGNATURES];
  char buf[BLOCKSIZE];
  SECKEYPrivateKey *privKeys[MAX_SIGNATURES];
  CERTCertificate *certs[MAX_SIGNATURES];
  char *indexBuf = NULL, *indexBufLoc;
  uint32_t k;

  memset(signatureLengths, 0, sizeof(signatureLengths));
  memset(ctxs, 0, sizeof(ctxs));
  memset(secItems, 0, sizeof(secItems));
  memset(privKeys, 0, sizeof(privKeys));
  memset(certs, 0, sizeof(certs));

  if (!NSSConfigDir || !certNames || certCount == 0 || !src || !dest) {
    fprintf(stderr, "ERROR: Invalid parameter passed in.\n");
    return -1;
  }

  if (NSSInitCryptoContext(NSSConfigDir)) {
    fprintf(stderr, "ERROR: Could not init config dir: %s\n", NSSConfigDir);
    goto failure;
  }

  PK11_SetPasswordFunc(SECU_GetModulePassword);

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

  for (k = 0; k < certCount; k++) {
    if (NSSSignBegin(certNames[k], &ctxs[k], &privKeys[k],
                     &certs[k], &signatureLengths[k])) {
      fprintf(stderr, "ERROR: NSSSignBegin failed\n");
      goto failure;
    }
  }

  
  if (ReadWriteAndUpdateSignatures(fpSrc, fpDest,
                                   buf, MAR_ID_SIZE,
                                   ctxs, certCount, "MAR ID")) {
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

  
  for (k = 0; k < certCount; k++) {
    signatureSectionLength += sizeof(signatureAlgorithmID) +
                              sizeof(signatureLengths[k]) +
                              signatureLengths[k];
  }
  dstOffsetToIndex = offsetToIndex;
  if (!hasSignatureBlock) {
    dstOffsetToIndex += sizeof(sizeOfEntireMAR) + sizeof(numSignatures);
  }
  dstOffsetToIndex += signatureSectionLength;

  
  dstOffsetToIndex = htonl(dstOffsetToIndex);
  if (WriteAndUpdateSignatures(fpDest, &dstOffsetToIndex,
                               sizeof(dstOffsetToIndex), ctxs, certCount,
                               "index offset")) {
    goto failure;
  }
  dstOffsetToIndex = ntohl(dstOffsetToIndex);

  
  sizeOfEntireMAR += signatureSectionLength;
  if (!hasSignatureBlock) {
    sizeOfEntireMAR += sizeof(sizeOfEntireMAR) + sizeof(numSignatures);
  }

  
  sizeOfEntireMAR = HOST_TO_NETWORK64(sizeOfEntireMAR);
  if (WriteAndUpdateSignatures(fpDest, &sizeOfEntireMAR,
                               sizeof(sizeOfEntireMAR), ctxs, certCount,
                               "size of MAR")) {
    goto failure;
  }
  sizeOfEntireMAR = NETWORK_TO_HOST64(sizeOfEntireMAR);

  
  numSignatures = certCount;
  numSignatures = htonl(numSignatures);
  if (WriteAndUpdateSignatures(fpDest, &numSignatures,
                               sizeof(numSignatures), ctxs, certCount,
                               "num signatures")) {
    goto failure;
  }
  numSignatures = ntohl(numSignatures);

  signaturePlaceholderOffset = ftello(fpDest);

  for (k = 0; k < certCount; k++) {
    
    signatureAlgorithmID = htonl(1);
    if (WriteAndUpdateSignatures(fpDest, &signatureAlgorithmID,
                                 sizeof(signatureAlgorithmID),
                                 ctxs, certCount, "num signatures")) {
      goto failure;
    }
    signatureAlgorithmID = ntohl(signatureAlgorithmID);

    
    signatureLengths[k] = htonl(signatureLengths[k]);
    if (WriteAndUpdateSignatures(fpDest, &signatureLengths[k],
                                 sizeof(signatureLengths[k]),
                                 ctxs, certCount, "signature length")) {
      goto failure;
    }
    signatureLengths[k] = ntohl(signatureLengths[k]);

    


    memset(buf, 0, sizeof(buf));
    if (fwrite(buf, signatureLengths[k], 1, fpDest) != 1) {
      fprintf(stderr, "ERROR: Could not write signature length\n");
      goto failure;
    }
  }

  


  if (ftello(fpSrc) > ((int64_t)offsetToIndex)) {
    fprintf(stderr, "ERROR: Index offset is too small.\n");
    goto failure;
  }
  numBytesToCopy = ((int64_t)offsetToIndex) - ftello(fpSrc);
  numChunks = numBytesToCopy / BLOCKSIZE;
  leftOver = numBytesToCopy % BLOCKSIZE;

  
  for (i = 0; i < numChunks; ++i) {
    if (ReadWriteAndUpdateSignatures(fpSrc, fpDest, buf,
                                     BLOCKSIZE, ctxs, certCount,
                                     "content block")) {
      goto failure;
    }
  }

  
  if (ReadWriteAndUpdateSignatures(fpSrc, fpDest, buf,
                                   leftOver, ctxs, certCount,
                                   "left over content block")) {
    goto failure;
  }

  
  if (ReadWriteAndUpdateSignatures(fpSrc, fpDest, &indexLength,
                                   sizeof(indexLength), ctxs, certCount,
                                   "index length")) {
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

  if (WriteAndUpdateSignatures(fpDest, indexBuf,
                               indexLength, ctxs, certCount, "index")) {
    goto failure;
  }

  

  if (ftello(fpDest) > MAX_SIZE_OF_MAR_FILE) {
    goto failure;
  }

  for (k = 0; k < certCount; k++) {
    
    if (SGN_End(ctxs[k], &secItems[k]) != SECSuccess) {
      fprintf(stderr, "ERROR: Could not end signature context\n");
      goto failure;
    }
    if (signatureLengths[k] != secItems[k].len) {
      fprintf(stderr, "ERROR: Signature is not the expected length\n");
      goto failure;
    }
  }

  
  if (fseeko(fpDest, signaturePlaceholderOffset, SEEK_SET)) {
    fprintf(stderr, "ERROR: Could not seek to signature offset\n");
    goto failure;
  }

  for (k = 0; k < certCount; k++) {
    
    if (fseeko(fpDest, sizeof(signatureAlgorithmID) +
               sizeof(signatureLengths[k]), SEEK_CUR)) {
      fprintf(stderr, "ERROR: Could not seek to signature offset\n");
      goto failure;
    }

    

    if (fwrite(secItems[k].data, secItems[k].len, 1, fpDest) != 1) {
      fprintf(stderr, "ERROR: Could not write signature\n");
      goto failure;
    }
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

  
  for (k = 0; k < certCount; k++) {
    if (ctxs[k]) {
      SGN_DestroyContext(ctxs[k], PR_TRUE);
    }

    if (certs[k]) {
      CERT_DestroyCertificate(certs[k]);
    }

    if (privKeys[k]) {
      SECKEY_DestroyPrivateKey(privKeys[k]);
    }

    SECITEM_FreeItem(&secItems[k], PR_FALSE);
  }

  if (rv) {
    remove(dest);
  }

  return rv;
}
