




































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
#include "mar.h"
#include "cryptox.h"

int mar_verify_signature_fp(FILE *fp, 
                            CryptoX_ProviderHandle provider, 
                            CryptoX_PublicKey key);
int mar_verify_signature_for_fp(FILE *fp, 
                                CryptoX_ProviderHandle provider, 
                                CryptoX_PublicKey key, 
                                PRUint32 signatureCount,
                                char *extractedSignature);















int
ReadAndUpdateVerifyContext(FILE *fp, 
                           void *buffer,
                           PRUint32 size, 
                           CryptoX_SignatureHandle *ctx,
                           const char *err) 
{
  if (!fp || !buffer || !ctx || !err) {
    fprintf(stderr, "ERROR: Invalid parameter specified.\n");
    return CryptoX_Error;
  }

  if (!size) { 
    return CryptoX_Success;
  }

  if (fread(buffer, size, 1, fp) != 1) {
    fprintf(stderr, "ERROR: Could not read %s\n", err);
    return CryptoX_Error;
  }

  if (CryptoX_Failed(CryptoX_VerifyUpdate(ctx, buffer, size))) {
    fprintf(stderr, "ERROR: Could not update verify context for %s\n", err);
    return -2;
  }
  return CryptoX_Success;
}
















int
mar_verify_signature(const char *pathToMARFile, 
                     const char *certData,
                     PRUint32 sizeOfCertData,
                     const char *certName) {
  int rv;
  CryptoX_ProviderHandle provider = CryptoX_InvalidHandleValue;
  CryptoX_Certificate cert;
  CryptoX_PublicKey key;
  FILE *fp;
  
  if (!pathToMARFile || (!certData && !certName)) {
    fprintf(stderr, "ERROR: Invalid parameter specified.\n");
    return CryptoX_Error;
  }

  fp = fopen(pathToMARFile, "rb");
  if (!fp) {
    fprintf(stderr, "ERROR: Could not open MAR file.\n");
    return CryptoX_Error;
  }

  if (CryptoX_Failed(CryptoX_InitCryptoProvider(&provider))) {
    fclose(fp);
    fprintf(stderr, "ERROR: Could not init crytpo library.\n");
    return CryptoX_Error;
  }

  if (CryptoX_Failed(CryptoX_LoadPublicKey(provider, certData, sizeOfCertData,
                                           &key, certName, &cert))) {
    fclose(fp);
    fprintf(stderr, "ERROR: Could not load public key.\n");
    return CryptoX_Error;
  }

  rv = mar_verify_signature_fp(fp, provider, key);
  fclose(fp);
  if (key) {
    CryptoX_FreePublicKey(&key);
  }

  if (cert) {
    CryptoX_FreeCertificate(&cert);
  }
  return rv;
}

#ifdef XP_WIN










int
mar_verify_signatureW(MarFile *mar, 
                      const char *certData,
                      PRUint32 sizeOfCertData) {
  int rv;
  CryptoX_ProviderHandle provider = CryptoX_InvalidHandleValue;
  CryptoX_Certificate cert;
  CryptoX_PublicKey key;
  
  if (!mar || !certData) {
    fprintf(stderr, "ERROR: Invalid parameter specified.\n");
    return CryptoX_Error;
  }

  if (!mar->fp) {
    fprintf(stderr, "ERROR: MAR file is not open.\n");
    return CryptoX_Error;
  }

  if (CryptoX_Failed(CryptoX_InitCryptoProvider(&provider))) { 
    fprintf(stderr, "ERROR: Could not init crytpo library.\n");
    return CryptoX_Error;
  }

  if (CryptoX_Failed(CryptoX_LoadPublicKey(provider, certData, sizeOfCertData,
                                           &key, "", &cert))) {
    fprintf(stderr, "ERROR: Could not load public key.\n");
    return CryptoX_Error;
  }

  rv = mar_verify_signature_fp(mar->fp, provider, key);
  if (key) {
    CryptoX_FreePublicKey(&key);
  }

  if (cert) {
    CryptoX_FreeCertificate(&cert);
  }
  return rv;
}
#endif










int
mar_verify_signature_fp(FILE *fp,
                        CryptoX_ProviderHandle provider, 
                        CryptoX_PublicKey key) {
  char buf[5] = {0};
  PRUint32 signatureAlgorithmID, signatureCount, signatureLen, numVerified = 0;
  int rv = -1;
  PRInt64 curPos;
  char *extractedSignature;
  PRUint32 i;

  if (!fp) {
    fprintf(stderr, "ERROR: Invalid file pointer passed.\n");
    return CryptoX_Error;
  }
  
  

  if (fseeko(fp, 0, SEEK_END)) {
    fprintf(stderr, "ERROR: Could not seek to the end of the MAR file.\n");
    return CryptoX_Error;
  }
  if (ftello(fp) > MAX_SIZE_OF_MAR_FILE) {
    fprintf(stderr, "ERROR: MAR file is too large to be verified.\n");
    return CryptoX_Error;
  }

  
  if (fseeko(fp, SIGNATURE_BLOCK_OFFSET, SEEK_SET)) {
    fprintf(stderr, "ERROR: Could not seek to the signature block.\n");
    return CryptoX_Error;
  }

  
  if (fread(&signatureCount, sizeof(signatureCount), 1, fp) != 1) {
    fprintf(stderr, "ERROR: Could not read number of signatures.\n");
    return CryptoX_Error;
  }
  signatureCount = ntohl(signatureCount);

  

  if (signatureCount > MAX_SIGNATURES) {
    fprintf(stderr, "ERROR: At most %d signatures can be specified.\n",
            MAX_SIGNATURES);
    return CryptoX_Error;
  }

  for (i = 0; i < signatureCount && numVerified == 0; i++) {
    
    if (fread(&signatureAlgorithmID, sizeof(PRUint32), 1, fp) != 1) {
      fprintf(stderr, "ERROR: Could not read signatures algorithm ID.\n");
      return CryptoX_Error;
    }
    signatureAlgorithmID = ntohl(signatureAlgorithmID);
  
    if (fread(&signatureLen, sizeof(PRUint32), 1, fp) != 1) {
      fprintf(stderr, "ERROR: Could not read signatures length.\n");
      return CryptoX_Error;
    }
    signatureLen = ntohl(signatureLen);

    

    if (signatureLen > MAX_SIGNATURE_LENGTH) {
      fprintf(stderr, "ERROR: Signature length is too large to verify.\n");
      return CryptoX_Error;
    }

    extractedSignature = malloc(signatureLen);
    if (!extractedSignature) {
      fprintf(stderr, "ERROR: Could allocate buffer for signature.\n");
      return CryptoX_Error;
    }
    if (fread(extractedSignature, signatureLen, 1, fp) != 1) {
      fprintf(stderr, "ERROR: Could not read extracted signature.\n");
      free(extractedSignature);
      return CryptoX_Error;
    }

    
    if (1 == signatureAlgorithmID) {
      curPos = ftello(fp);
      rv = mar_verify_signature_for_fp(fp, 
                                       provider, 
                                       key,
                                       signatureCount,
                                       extractedSignature);
      if (CryptoX_Succeeded(rv)) {
        numVerified++;
      }
      free(extractedSignature);
      if (fseeko(fp, curPos, SEEK_SET)) {
        fprintf(stderr, "ERROR: Could not seek back to last signature.\n");
        return CryptoX_Error;
      }
    } else {
      free(extractedSignature);
    }
  }

  

  if (numVerified > 0) {
    return CryptoX_Success;
  } else {
    fprintf(stderr, "ERROR: No signatures were verified.\n");
    return CryptoX_Error;
  }
}











int
mar_verify_signature_for_fp(FILE *fp, 
                            CryptoX_ProviderHandle provider, 
                            CryptoX_PublicKey key, 
                            PRUint32 signatureCount,
                            char *extractedSignature) {
  CryptoX_SignatureHandle signatureHandle;
  char buf[BLOCKSIZE];
  PRUint32 signatureLen;
  PRUint32 i;

  if (!extractedSignature) {
    fprintf(stderr, "ERROR: Invalid parameter specified.\n");
    return CryptoX_Error;
  }

  



  if (!signatureCount) {
    fprintf(stderr, "ERROR: There must be at least one signature.\n");
    return CryptoX_Error;
  }

  CryptoX_VerifyBegin(provider, &signatureHandle, &key);

  
  if (fseeko(fp, 0, SEEK_SET)) {
    fprintf(stderr, "ERROR: Could not seek to start of the file\n");
    return CryptoX_Error;
  }

  



  if (CryptoX_Failed(ReadAndUpdateVerifyContext(fp, buf, 
                                                SIGNATURE_BLOCK_OFFSET +
                                                sizeof(PRUint32),
                                                &signatureHandle,
                                                "signature block"))) {
    return CryptoX_Error;
  }

  for (i = 0; i < signatureCount; i++) {
    
    if (CryptoX_Failed(ReadAndUpdateVerifyContext(fp,
                                                  &buf, 
                                                  sizeof(PRUint32),
                                                  &signatureHandle, 
                                                  "signature algorithm ID"))) {
        return CryptoX_Error;
    }

    if (CryptoX_Failed(ReadAndUpdateVerifyContext(fp, 
                                                  &signatureLen, 
                                                  sizeof(PRUint32), 
                                                  &signatureHandle, 
                                                  "signature length"))) {
      return CryptoX_Error;
    }
    signatureLen = ntohl(signatureLen);

    
    if (fseeko(fp, signatureLen, SEEK_CUR)) {
      fprintf(stderr, "ERROR: Could not seek past signature.\n");
      return CryptoX_Error;
    }
  }

  while (!feof(fp)) {
    int numRead = fread(buf, 1, BLOCKSIZE , fp);
    if (ferror(fp)) {
      fprintf(stderr, "ERROR: Error reading data block.\n");
      return CryptoX_Error;
    }

    if (CryptoX_Failed(CryptoX_VerifyUpdate(&signatureHandle, 
                                            buf, numRead))) {
      fprintf(stderr, "ERROR: Error updating verify context with"
                      " data block.\n");
      return CryptoX_Error;
    }
  }

  if (CryptoX_Failed(CryptoX_VerifySignature(&signatureHandle, 
                                             &key,
                                             extractedSignature, 
                                             signatureLen))) {
    fprintf(stderr, "ERROR: Error verifying signature.\n");
    return CryptoX_Error;
  }

  return CryptoX_Success;
}
