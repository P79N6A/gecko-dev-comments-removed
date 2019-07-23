












































#include "jar.h"
#include "jarint.h"
#include "jarjart.h"
#include "blapi.h"	
#include "pk11func.h"	
#include "certdb.h"


#define CERTDB_USER (1<<6)









static SECStatus jar_list_cert_callback 
     (CERTCertificate *cert, SECItem *k, void *data)
  {
  char *name;
  char **ugly_list;

  int trusted;

  ugly_list = (char **) data;

  if (cert)
    {
    name = cert->nickname;

    trusted = cert->trust->objectSigningFlags & CERTDB_USER;

    

    if (name && trusted)
      {
      *ugly_list = (char*)PORT_Realloc
           (*ugly_list, PORT_Strlen (*ugly_list) + PORT_Strlen (name) + 2);

      if (*ugly_list)
        {
        if (**ugly_list)
          PORT_Strcat (*ugly_list, "\n");

        PORT_Strcat (*ugly_list, name);
        }
      }
    }

  return (SECSuccess);
  }









char *JAR_JAR_list_certs (void)
  {
  SECStatus status = SECFailure;
  CERTCertDBHandle *certdb;
  CERTCertList *certs;
  CERTCertListNode *node;

  char *ugly_list;

  certdb = JAR_open_database();

  
  ugly_list = (char*)PORT_ZAlloc (16);

  if (ugly_list)
    {
    *ugly_list = 0;

    certs = PK11_ListCerts(PK11CertListUnique, NULL);
    if (certs)
      {
	for (node = CERT_LIST_HEAD(certs); !CERT_LIST_END(node,certs);
				node = CERT_LIST_NEXT(node))
           {
	    jar_list_cert_callback(node->cert, NULL, (void *)&ugly_list);
	   }
	CERT_DestroyCertList(certs);
	status = SECSuccess;
       }
    }

  JAR_close_database (certdb);

  return (status != SECSuccess) ? NULL : ugly_list;
  }

int JAR_JAR_validate_archive (char *filename)
  {
  JAR *jar;
  int status = -1;

  jar = JAR_new();

  if (jar)
    {
    status = JAR_pass_archive (jar, jarArchGuess, filename, "");

    if (status == 0)
      status = jar->valid;

    JAR_destroy (jar);
    }

  return status;
  }

char *JAR_JAR_get_error (int status)
  {
  return JAR_get_error (status);
  }

















void *JAR_JAR_new_hash (int alg)
  {
  void *context;

  MD5Context *md5;
  SHA1Context *sha1;

  

  if (!PK11_HashOK (alg == 1 ? SEC_OID_MD5 : SEC_OID_SHA1))
    return NULL;

  context = PORT_ZAlloc (512);

  if (context)
    {
    switch (alg)
      {
      case 1:  
               md5 = (MD5Context *) context;
               MD5_Begin (md5);
               break;

      case 2:  
               sha1 = (SHA1Context *) context;
               SHA1_Begin (sha1);
               break;
      }
    }

  return context;
  }

void *JAR_JAR_hash (int alg, void *cookie, int length, void *data)
  {
  MD5Context *md5;
  SHA1Context *sha1;

  

  if (!PK11_HashOK (alg == 1 ? SEC_OID_MD5 : SEC_OID_SHA1))
    return NULL;

  if (length > 0)
    {
    switch (alg)
      {
      case 1:  
               md5 = (MD5Context *) cookie;
               MD5_Update (md5, (unsigned char*)data, length);
               break;

      case 2:  
               sha1 = (SHA1Context *) cookie;
               SHA1_Update (sha1, (unsigned char*)data, length);
               break;
      }
    }

  return cookie;
  }

void *JAR_JAR_end_hash (int alg, void *cookie)
  {
  int length;
  unsigned char *data;
  char *ascii; 

  MD5Context *md5;
  SHA1Context *sha1;

  unsigned int md5_length;
  unsigned char md5_digest [MD5_LENGTH];

  unsigned int sha1_length;
  unsigned char sha1_digest [SHA1_LENGTH];

  

  if (!PK11_HashOK (alg == 1 ? SEC_OID_MD5 : SEC_OID_SHA1)) 
    return NULL;

  switch (alg)
    {
    case 1:  

             md5 = (MD5Context *) cookie;

             MD5_End (md5, md5_digest, &md5_length, MD5_LENGTH);
             

             data = md5_digest;
             length = md5_length;

             break;

    case 2:  

             sha1 = (SHA1Context *) cookie;

             SHA1_End (sha1, sha1_digest, &sha1_length, SHA1_LENGTH);
             

             data = sha1_digest;
             length = sha1_length;

             break;

    default: return NULL;
    }

  
  

  ascii = BTOA_DataToAscii(data, length);

  return ascii ? PORT_Strdup (ascii) : NULL;
  }








int JAR_JAR_sign_archive 
      (char *nickname, char *password, char *sf, char *outsig)
  {
  int status = JAR_ERR_GENERAL;
  JAR_FILE sf_fp; 
  JAR_FILE out_fp;

  CERTCertDBHandle *certdb;
  void *keydb;

  CERTCertificate *cert;

  if (PORT_Strlen (sf) < 5)
    {
    return JAR_ERR_GENERAL;
    }

  

  certdb = JAR_open_database();
  if (certdb == NULL)
    return JAR_ERR_GENERAL;

  keydb = jar_open_key_database();
  if (keydb == NULL)
    {
    JAR_close_database(certdb);
    return JAR_ERR_GENERAL;
    }

  sf_fp = JAR_FOPEN (sf, "rb");
  out_fp = JAR_FOPEN (outsig, "wb");

  cert = CERT_FindCertByNickname (certdb, nickname);

  if (cert && sf_fp && out_fp)
    {
    status = jar_create_pk7 (certdb, keydb, cert, password, sf_fp, out_fp);
    }

  
  PORT_Memset (password, 0, PORT_Strlen (password));

  JAR_FCLOSE (sf_fp);
  JAR_FCLOSE (out_fp);

  JAR_close_database (certdb);
  jar_close_key_database (keydb);

  return status;
  }
