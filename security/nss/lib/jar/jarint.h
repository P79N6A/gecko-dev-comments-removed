





































#include "nspr.h"
#include "key.h"
#include "base64.h"

extern CERTCertDBHandle *JAR_open_database (void);

extern int JAR_close_database (CERTCertDBHandle *certdb);

extern int jar_close_key_database (void *keydb);

extern void *jar_open_key_database (void);

extern JAR_Signer *JAR_new_signer (void);

extern void JAR_destroy_signer (JAR_Signer *signer);

extern JAR_Signer *jar_get_signer (JAR *jar, char *basename);

extern int 
jar_append(ZZList *list, int type, char *pathname, void *data, size_t size);


PRFileDesc*
JAR_FOPEN_to_PR_Open(const char *name, const char *mode);

#define ADDITEM(list,type,pathname,data,size) \
{ \
    int err = jar_append (list, type, pathname, data, size); \
    if (err < 0) \
    	return err; \
}






#define JAR_FILE PRFileDesc *
#define JAR_FOPEN(fn,mode) JAR_FOPEN_to_PR_Open(fn,mode)
#define JAR_FCLOSE PR_Close
#define JAR_FSEEK PR_Seek
#define JAR_FREAD PR_Read
#define JAR_FWRITE PR_Write

int 
jar_create_pk7(CERTCertDBHandle *certdb, void *keydb,
               CERTCertificate *cert, char *password, JAR_FILE infp,
               JAR_FILE outfp);

