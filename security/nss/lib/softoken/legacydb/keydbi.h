







#ifndef _KEYDBI_H_
#define _KEYDBI_H_

#include "nspr.h"
#include "seccomon.h"
#include "mcom_db.h"




struct NSSLOWKEYDBHandleStr {
    DB *db;
    DB *updatedb;		
    SECItem *global_salt;	
    int version;		
    char *appname;		
    char *dbname;		
    PRBool readOnly;		
    PRLock *lock;
    PRInt32 ref;		
};







typedef SECStatus (* NSSLOWKEYTraverseKeysFunc)(DBT *key, DBT *data, void *pdata);


SEC_BEGIN_PROTOS







extern SECStatus nsslowkey_TraverseKeys(NSSLOWKEYDBHandle *handle, 
				NSSLOWKEYTraverseKeysFunc f,
				void *udata);

SEC_END_PROTOS

#endif 
