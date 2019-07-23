










































#include "jar.h"
#include "jarint.h"
#include "secpkcs7.h"
#include "pk11func.h"
#include "sechash.h"


typedef void (*ETVoidPtrFunc) (void * data);

#ifdef MOZILLA_CLIENT_OLD

extern void ET_moz_CallFunction (ETVoidPtrFunc fn, void *data);



extern void *XP_FindSomeContext(void);

#endif







#define CHUNQ 64000
#define FILECHUNQ 32768












JAR_Digest * PR_CALLBACK JAR_calculate_digest (void ZHUGEP *data, long length)
  {
  long chunq;
  JAR_Digest *dig;

  unsigned int md5_length, sha1_length;

  PK11Context *md5  = 0;
  PK11Context *sha1 = 0;

  dig = (JAR_Digest *) PORT_ZAlloc (sizeof (JAR_Digest));

  if (dig == NULL) 
    {
    
    return NULL;
    }

#if defined(XP_WIN16)
  PORT_Assert ( !IsBadHugeReadPtr(data, length) );
#endif

  md5  = PK11_CreateDigestContext (SEC_OID_MD5);
  sha1 = PK11_CreateDigestContext (SEC_OID_SHA1);

  if (length >= 0) 
    {
    PK11_DigestBegin (md5);
    PK11_DigestBegin (sha1);

    do {
       chunq = length;

#ifdef XP_WIN16
       if (length > CHUNQ) chunq = CHUNQ;

       









       if (OFFSETOF(data) + chunq >= 0x10000) 
         chunq = 0x10000 - OFFSETOF(data);
#endif

       PK11_DigestOp (md5,  (unsigned char*)data, chunq);
       PK11_DigestOp (sha1, (unsigned char*)data, chunq);

       length -= chunq;
       data = ((char ZHUGEP *) data + chunq);
       } 
    while (length > 0);

    PK11_DigestFinal (md5,  dig->md5,  &md5_length,  MD5_LENGTH);
    PK11_DigestFinal (sha1, dig->sha1, &sha1_length, SHA1_LENGTH);

    PK11_DestroyContext (md5,  PR_TRUE);
    PK11_DestroyContext (sha1, PR_TRUE);
    }

  return dig;
  }









int JAR_digest_file (char *filename, JAR_Digest *dig)
    {
    JAR_FILE fp;

    int num;
    unsigned char *buf;

    PK11Context *md5 = 0;
    PK11Context *sha1 = 0;

    unsigned int md5_length, sha1_length;

    buf = (unsigned char *) PORT_ZAlloc (FILECHUNQ);
    if (buf == NULL)
      {
      
      return JAR_ERR_MEMORY;
      }
 
    if ((fp = JAR_FOPEN (filename, "rb")) == 0)
      {
      
      PORT_Free (buf);
      return JAR_ERR_FNF;
      }

    md5 = PK11_CreateDigestContext (SEC_OID_MD5);
    sha1 = PK11_CreateDigestContext (SEC_OID_SHA1);

    if (md5 == NULL || sha1 == NULL) 
      {
      
      PORT_Free (buf);
      JAR_FCLOSE (fp);
      return JAR_ERR_GENERAL;
      }

    PK11_DigestBegin (md5);
    PK11_DigestBegin (sha1);

    while (1)
      {
      if ((num = JAR_FREAD (fp, buf, FILECHUNQ)) == 0)
        break;

      PK11_DigestOp (md5, buf, num);
      PK11_DigestOp (sha1, buf, num);
      }

    PK11_DigestFinal (md5, dig->md5, &md5_length, MD5_LENGTH);
    PK11_DigestFinal (sha1, dig->sha1, &sha1_length, SHA1_LENGTH);

    PK11_DestroyContext (md5, PR_TRUE);
    PK11_DestroyContext (sha1, PR_TRUE);

    PORT_Free (buf);
    JAR_FCLOSE (fp);

    return 0;
    }






void* jar_open_key_database (void)
  {
    return NULL;
  }

int jar_close_key_database (void *keydb)
  {
  
  return 0;
  }







static void jar_pk7_out (void *arg, const char *buf, unsigned long len)
  {
  JAR_FWRITE ((JAR_FILE) arg, buf, len);
  }

int jar_create_pk7 
   (CERTCertDBHandle *certdb, void *keydb, 
       CERTCertificate *cert, char *password, JAR_FILE infp, JAR_FILE outfp)
  {
  int nb;
  unsigned char buffer [4096], digestdata[32];
  const SECHashObject *hashObj;
  void *hashcx;
  unsigned int len;

  int status = 0;
  char *errstring;

  SECItem digest;
  SEC_PKCS7ContentInfo *cinfo;
  SECStatus rv;

  void  *mw;

  if (outfp == NULL || infp == NULL || cert == NULL)
    return JAR_ERR_GENERAL;

  
  hashObj = HASH_GetHashObject(HASH_AlgSHA1);

  hashcx = (* hashObj->create)();
  if (hashcx == NULL)
    return JAR_ERR_GENERAL;

  (* hashObj->begin)(hashcx);

  while (1)
    {
    


    nb = JAR_FREAD (infp, buffer, sizeof (buffer));
    if (nb == 0) 
      {
      
      break;
      }
    (* hashObj->update) (hashcx, buffer, nb);
    }

  (* hashObj->end) (hashcx, digestdata, &len, 32);
  (* hashObj->destroy) (hashcx, PR_TRUE);

  digest.data = digestdata;
  digest.len = len;

  


#ifdef MOZILLA_CLIENT_OLD
  mw = XP_FindSomeContext();
#else
  mw = NULL;
#endif

  PORT_SetError (0);

  cinfo = SEC_PKCS7CreateSignedData 
             (cert, certUsageObjectSigner, NULL, 
                SEC_OID_SHA1, &digest, NULL, (void *) mw);

  if (cinfo == NULL)
    return JAR_ERR_PK7;

  rv = SEC_PKCS7IncludeCertChain (cinfo, NULL);
  if (rv != SECSuccess) 
    {
    status = PORT_GetError();
    SEC_PKCS7DestroyContentInfo (cinfo);
    return status;
    }

  


  rv = SEC_PKCS7AddSigningTime (cinfo);
  if (rv != SECSuccess)
    {
    
    }

  PORT_SetError (0);

  
  rv = SEC_PKCS7Encode 
             (cinfo, jar_pk7_out, outfp, 
                 NULL,   NULL,   (void *) mw);

  if (rv != SECSuccess)
    status = PORT_GetError();

  SEC_PKCS7DestroyContentInfo (cinfo);

  if (rv != SECSuccess)
    {
    errstring = JAR_get_error (status);
    
    return status < 0 ? status : JAR_ERR_GENERAL;
    }

  return 0;
  }
