



































#ifndef __JAR_h_
#define __JAR_h_








#include "cert.h"
#include "hasht.h"


#include "prio.h"

#define ZHUGEP

#include <stdio.h>



typedef enum {
    jarTypeMF = 2,
    jarTypeSF = 3,
    jarTypeMeta = 6,
    jarTypePhy = 7,
    jarTypeSign = 10,
    jarTypeSect = 11,
    jarTypeOwner = 13
} jarType;


typedef struct JAR_Item_ {
    char *pathname;	   
    jarType type;	   
    size_t size;	   
    void *data; 	   
} JAR_Item;


typedef enum {
    jarHashNone = 0,
    jarHashBad = 1,
    jarHashPresent = 2
} jarHash;

typedef struct JAR_Digest_ {
    jarHash md5_status;
    unsigned char md5 [MD5_LENGTH];
    jarHash sha1_status;
    unsigned char sha1 [SHA1_LENGTH];
} JAR_Digest;


typedef enum {
    jarArchGuess = 0,
    jarArchNone = 1,
    jarArchZip = 2,
    jarArchTar = 3
} jarArch;

#include "jar-ds.h"

struct JAR_;

typedef int jar_settable_callback_fn(int status, struct JAR_ *jar, 
                                     const char *metafile, char *pathname, 
				     char *errortext);


typedef struct JAR_ {
    jarArch format;	  

    char *url;		  
    char *filename;	  
    FILE *fp;		  
    

    
    ZZList *manifest;	  
    ZZList *hashes;	  
    ZZList *phy;	  
    ZZList *metainfo;	  

    JAR_Digest *globalmeta;  

    
    int pkcs7;		  
    int valid;		  

    ZZList *signers;	  

    
    void *mw;		  

    
    jar_settable_callback_fn *signal;
} JAR;









typedef struct JAR_Context_ {
    JAR *jar;		  
    char *pattern;	  
    jarType finding;	  
    ZZLink *next;	  
    ZZLink *nextsign;	  
} JAR_Context;

typedef struct JAR_Signer_ {
    int pkcs7;		  
    int valid;		  
    char *owner;	  
    JAR_Digest *digest;   
    ZZList *sf; 	  
    ZZList *certs;	  
} JAR_Signer;



typedef struct JAR_Metainfo_ {
    char *header;
    char *info;
} JAR_Metainfo;


typedef struct JAR_Physical_ {
    unsigned char compression;
    unsigned long offset;
    unsigned long length;
    unsigned long uncompressed_length;
#if defined(XP_UNIX) || defined(XP_BEOS)
    uint16 mode;
#endif
} JAR_Physical;

typedef struct JAR_Cert_ {
    size_t length;
    void *key;
    CERTCertificate *cert;
} JAR_Cert;



typedef enum {
    jarCertCompany = 1,
    jarCertCA = 2,
    jarCertSerial = 3,
    jarCertExpires = 4,
    jarCertNickname = 5,
    jarCertFinger = 6,
    jarCertJavaHack = 100
} jarCert;


#define JAR_CB_SIGNAL	1







#ifndef SEC_ERR_BASE
#define SEC_ERR_BASE	    (-0x2000)
#endif

#define JAR_BASE	SEC_ERR_BASE + 300



#define JAR_ERR_GENERAL 	(JAR_BASE + 1)
#define JAR_ERR_FNF		(JAR_BASE + 2)
#define JAR_ERR_CORRUPT     	(JAR_BASE + 3)
#define JAR_ERR_MEMORY	    	(JAR_BASE + 4)
#define JAR_ERR_DISK	    	(JAR_BASE + 5)
#define JAR_ERR_ORDER		(JAR_BASE + 6)
#define JAR_ERR_SIG		(JAR_BASE + 7)
#define JAR_ERR_METADATA	(JAR_BASE + 8)
#define JAR_ERR_ENTRY	    	(JAR_BASE + 9)
#define JAR_ERR_HASH	    	(JAR_BASE + 10)
#define JAR_ERR_PK7		(JAR_BASE + 11)
#define JAR_ERR_PNF		(JAR_BASE + 12)



extern JAR *JAR_new (void);

extern void PR_CALLBACK JAR_destroy (JAR *jar);

extern char *JAR_get_error (int status);

extern int JAR_set_callback(int type, JAR *jar, jar_settable_callback_fn *fn);

extern void 
JAR_init_callbacks(char *(*string_cb)(int), 
                   void *(*find_cx)(void), 
		   void *(*init_cx)(void) );













int JAR_set_context (JAR *jar, void  *mw);





















extern JAR_Context *JAR_find (JAR *jar, char *pattern, jarType type);

extern int JAR_find_next (JAR_Context *ctx, JAR_Item **it);

extern void JAR_find_end (JAR_Context *ctx);


























extern int 
JAR_parse_manifest(JAR *jar, char *raw_manifest, long length, const char *path,
                   const char *url);







extern JAR_Digest * PR_CALLBACK 
JAR_calculate_digest(void *data, long length);

extern int PR_CALLBACK 
JAR_verify_digest(JAR *jar, const char *name, JAR_Digest *dig);

extern int 
JAR_digest_file(char *filename, JAR_Digest *dig);










extern int PR_CALLBACK 
JAR_cert_attribute(JAR *jar, jarCert attrib, long keylen, void *key,
                   void **result, unsigned long *length);


















extern int 
JAR_get_metainfo(JAR *jar, char *name, char *header, void **info, 
                 unsigned long *length);

extern char *JAR_get_filename (JAR *jar);

extern char *JAR_get_url (JAR *jar);




extern int PR_CALLBACK 
JAR_stash_cert(JAR *jar, long keylen, void *key);



CERTCertificate *
JAR_fetch_cert(long length, void *key);














extern int 
JAR_pass_archive(JAR *jar, jarArch format, char *filename, const char *url);




extern int 
JAR_pass_archive_unverified(JAR *jar, jarArch format, char *filename, 
                            const char *url);









extern int 
JAR_verified_extract(JAR *jar, char *path, char *outpath);






extern int 
JAR_extract(JAR *jar, char *path, char *outpath);

#endif 
