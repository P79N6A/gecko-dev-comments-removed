








#include "mcom_db.h"
#include "secitem.h"
#include "nssb64.h"
#include "blapi.h"
#include "secerr.h"

#include "lgdb.h"
















#define DBS_BLOCK_SIZE (16*1024) /* 16 k */
#define DBS_MAX_ENTRY_SIZE (DBS_BLOCK_SIZE - (2048)) /* 14 k */
#define DBS_CACHE_SIZE	DBS_BLOCK_SIZE*8
#define ROUNDDIV(x,y) (x+(y-1))/y
#define BLOB_HEAD_LEN 4
#define BLOB_LENGTH_START BLOB_HEAD_LEN
#define BLOB_LENGTH_LEN 4
#define BLOB_NAME_START BLOB_LENGTH_START+BLOB_LENGTH_LEN
#define BLOB_NAME_LEN 1+ROUNDDIV(SHA1_LENGTH,3)*4+1
#define BLOB_BUF_LEN BLOB_HEAD_LEN+BLOB_LENGTH_LEN+BLOB_NAME_LEN


typedef struct DBSStr DBS;

struct DBSStr {
    DB db;
    char *blobdir;
    int mode;
    PRBool readOnly;
    PRFileMap *dbs_mapfile;
    unsigned char *dbs_addr;
    PRUint32 dbs_len;
    char staticBlobArea[BLOB_BUF_LEN];
};
    
    




static PRBool
dbs_IsBlob(DBT *blobData)
{
    unsigned char *addr = (unsigned char *)blobData->data;
    if (blobData->size < BLOB_BUF_LEN) {
	return PR_FALSE;
    }
    return addr && ((certDBEntryType) addr[1] == certDBEntryTypeBlob);
}





static const char *
dbs_getBlobFileName(DBT *blobData)
{
    char *addr = (char *)blobData->data;

    return &addr[BLOB_NAME_START];
}




static PRUint32
dbs_getBlobSize(DBT *blobData)
{
    unsigned char *addr = (unsigned char *)blobData->data;

    return (PRUint32)(addr[BLOB_LENGTH_START+3] << 24) | 
			(addr[BLOB_LENGTH_START+2] << 16) | 
			(addr[BLOB_LENGTH_START+1] << 8) | 
			addr[BLOB_LENGTH_START];
}








static void
dbs_replaceSlash(char *cp, int len)
{
   while (len--) {
	if (*cp == '/') *cp = '-';
	cp++;
   }
}





static void
dbs_mkBlob(DBS *dbsp,const DBT *key, const DBT *data, DBT *blobData)
{
   unsigned char sha1_data[SHA1_LENGTH];
   char *b = dbsp->staticBlobArea;
   PRUint32 length = data->size;
   SECItem sha1Item;

   b[0] = CERT_DB_FILE_VERSION; 
   b[1] = (char) certDBEntryTypeBlob; 
   b[2] = 0; 
   b[3] = 0; 
   b[BLOB_LENGTH_START] = length & 0xff;
   b[BLOB_LENGTH_START+1] = (length >> 8) & 0xff;
   b[BLOB_LENGTH_START+2] = (length >> 16) & 0xff;
   b[BLOB_LENGTH_START+3] = (length >> 24) & 0xff;
   sha1Item.data = sha1_data;
   sha1Item.len = SHA1_LENGTH;
   SHA1_HashBuf(sha1_data,key->data,key->size);
   b[BLOB_NAME_START]='b'; 
   NSSBase64_EncodeItem(NULL,&b[BLOB_NAME_START+1],BLOB_NAME_LEN-1,&sha1Item);
   b[BLOB_BUF_LEN-1] = 0;
   dbs_replaceSlash(&b[BLOB_NAME_START+1],BLOB_NAME_LEN-1);
   blobData->data = b;
   blobData->size = BLOB_BUF_LEN;
   return;
}
   








 
static char *
dbs_getBlobFilePath(char *blobdir,DBT *blobData)
{
    const char *name;

    if (blobdir == NULL) {
	PR_SetError(SEC_ERROR_BAD_DATABASE,0);
	return NULL;
    }
    if (!dbs_IsBlob(blobData)) {
	PR_SetError(SEC_ERROR_BAD_DATABASE,0);
	return NULL;
    }
    name = dbs_getBlobFileName(blobData);
    if (!name || *name == 0) {
	PR_SetError(SEC_ERROR_BAD_DATABASE,0);
	return NULL;
    }
    return  PR_smprintf("%s" PATH_SEPARATOR "%s", blobdir, name);
}




static void
dbs_removeBlob(DBS *dbsp, DBT *blobData)
{
    char *file;

    file = dbs_getBlobFilePath(dbsp->blobdir, blobData);
    if (!file) {
	return;
    }
    PR_Delete(file);
    PR_smprintf_free(file);
}





static int
dbs_DirMode(int mode)
{
  int x_bits = (mode >> 2) &  0111;
  return mode | x_bits;
}





static int
dbs_writeBlob(DBS *dbsp, int mode, DBT *blobData, const DBT *data)
{
    char *file = NULL;
    PRFileDesc *filed;
    PRStatus status;
    int len;
    int error = 0;

    file = dbs_getBlobFilePath(dbsp->blobdir, blobData);
    if (!file) {
	goto loser;
    }
    if (PR_Access(dbsp->blobdir, PR_ACCESS_EXISTS) != PR_SUCCESS) {
	status = PR_MkDir(dbsp->blobdir,dbs_DirMode(mode));
	if (status != PR_SUCCESS) {
	    goto loser;
	}
    }
    filed = PR_OpenFile(file,PR_CREATE_FILE|PR_TRUNCATE|PR_WRONLY, mode);
    if (filed == NULL) {
	error = PR_GetError();
	goto loser;
    }
    len = PR_Write(filed,data->data,data->size);
    error = PR_GetError();
    PR_Close(filed);
    if (len < (int)data->size) {
	goto loser;
    }
    PR_smprintf_free(file);
    return 0;

loser:
    if (file) {
	PR_Delete(file);
	PR_smprintf_free(file);
    }
    
    PR_SetError(error,0);
    return -1;
}














static void
dbs_freemap(DBS *dbsp)
{
    if (dbsp->dbs_mapfile) {
	PR_MemUnmap(dbsp->dbs_addr,dbsp->dbs_len);
	PR_CloseFileMap(dbsp->dbs_mapfile);
	dbsp->dbs_mapfile = NULL;
	dbsp->dbs_addr = NULL;
	dbsp->dbs_len = 0;
    } else if (dbsp->dbs_addr) {
	PORT_Free(dbsp->dbs_addr);
	dbsp->dbs_addr = NULL;
	dbsp->dbs_len = 0;
    }
    return;
}

static void
dbs_setmap(DBS *dbsp, PRFileMap *mapfile, unsigned char *addr, PRUint32 len)
{
    dbsp->dbs_mapfile = mapfile;
    dbsp->dbs_addr = addr;
    dbsp->dbs_len = len;
}




static unsigned char *
dbs_EmulateMap(PRFileDesc *filed, int len)
{
    unsigned char *addr;
    PRInt32 dataRead;

    addr = PORT_Alloc(len);
    if (addr == NULL) {
	return NULL;
    }

    dataRead = PR_Read(filed,addr,len);
    if (dataRead != len) {
	PORT_Free(addr);
	if (dataRead > 0) {
	    
	    PR_SetError(SEC_ERROR_BAD_DATABASE,0);
	}
	return NULL;
    }

    return addr;
}







static int
dbs_readBlob(DBS *dbsp, DBT *data)
{
    char *file = NULL;
    PRFileDesc *filed = NULL;
    PRFileMap *mapfile = NULL;
    unsigned char *addr = NULL;
    int error;
    int len = -1;

    file = dbs_getBlobFilePath(dbsp->blobdir, data);
    if (!file) {
	goto loser;
    }
    filed = PR_OpenFile(file,PR_RDONLY,0);
    PR_smprintf_free(file); file = NULL;
    if (filed == NULL) {
	goto loser;
    }

    len = dbs_getBlobSize(data);
    mapfile = PR_CreateFileMap(filed, len, PR_PROT_READONLY);
    if (mapfile == NULL) {
	 


	if (PR_GetError() != PR_NOT_IMPLEMENTED_ERROR) {
	    goto loser;
	}
	addr = dbs_EmulateMap(filed, len);
    } else {
	addr = PR_MemMap(mapfile, 0, len);
    }
    if (addr == NULL) {
	goto loser;
    }
    PR_Close(filed);
    dbs_setmap(dbsp,mapfile,addr,len);

    data->data = addr;
    data->size = len;
    return 0;

loser:
    
    error = PR_GetError();
    if (mapfile) {
	PR_CloseFileMap(mapfile);
    }
    if (filed) {
	PR_Close(filed);
    }
    PR_SetError(error,0);
    return -1;
}




static int
dbs_get(const DB *dbs, const DBT *key, DBT *data, unsigned int flags)
{
    int ret;
    DBS *dbsp = (DBS *)dbs;
    DB *db = (DB *)dbs->internal;
    

    dbs_freemap(dbsp);
    
    ret = (* db->get)(db, key, data, flags);
    if ((ret == 0) && dbs_IsBlob(data)) {
	ret = dbs_readBlob(dbsp,data);
    }

    return(ret);
}

static int
dbs_put(const DB *dbs, DBT *key, const DBT *data, unsigned int flags)
{
    DBT blob;
    int ret = 0;
    DBS *dbsp = (DBS *)dbs;
    DB *db = (DB *)dbs->internal;

    dbs_freemap(dbsp);

    
    if (!dbsp->readOnly) {
	DBT oldData;
	int ret1;

	
	ret1 = (*db->get)(db,key,&oldData,0);
        if ((ret1 == 0) && flags == R_NOOVERWRITE) {
	    
	    return (* db->put)(db, key, data, flags);
	}
	if ((ret1 == 0) && dbs_IsBlob(&oldData)) {
	    dbs_removeBlob(dbsp, &oldData);
	}

	if (data->size > DBS_MAX_ENTRY_SIZE) {
	    dbs_mkBlob(dbsp,key,data,&blob);
	    ret = dbs_writeBlob(dbsp, dbsp->mode, &blob, data);
	    data = &blob;
	}
    }

    if (ret == 0) {
	ret = (* db->put)(db, key, data, flags);
    }
    return(ret);
}

static int
dbs_sync(const DB *dbs, unsigned int flags)
{
    DB *db = (DB *)dbs->internal;
    DBS *dbsp = (DBS *)dbs;

    dbs_freemap(dbsp);

    return (* db->sync)(db, flags);
}

static int
dbs_del(const DB *dbs, const DBT *key, unsigned int flags)
{
    int ret;
    DBS *dbsp = (DBS *)dbs;
    DB *db = (DB *)dbs->internal;

    dbs_freemap(dbsp);

    if (!dbsp->readOnly) {
	DBT oldData;
	ret = (*db->get)(db,key,&oldData,0);
	if ((ret == 0) && dbs_IsBlob(&oldData)) {
	    dbs_removeBlob(dbsp,&oldData);
	}
    }

    return (* db->del)(db, key, flags);
}

static int
dbs_seq(const DB *dbs, DBT *key, DBT *data, unsigned int flags)
{
    int ret;
    DBS *dbsp = (DBS *)dbs;
    DB *db = (DB *)dbs->internal;
    
    dbs_freemap(dbsp);
    
    ret = (* db->seq)(db, key, data, flags);
    if ((ret == 0) && dbs_IsBlob(data)) {
	
	(void) dbs_readBlob(dbsp,data);
    }

    return(ret);
}

static int
dbs_close(DB *dbs)
{
    DBS *dbsp = (DBS *)dbs;
    DB *db = (DB *)dbs->internal;
    int ret;

    dbs_freemap(dbsp);
    ret = (* db->close)(db);
    PORT_Free(dbsp->blobdir);
    PORT_Free(dbsp);
    return ret;
}

static int
dbs_fd(const DB *dbs)
{
    DB *db = (DB *)dbs->internal;

    return (* db->fd)(db);
}







#define DIRSUFFIX ".dir"
static char *
dbs_mkBlobDirName(const char *dbname)
{
    int dbname_len = PORT_Strlen(dbname);
    int dbname_end = dbname_len;
    const char *cp;
    char *blobDir = NULL;

    



    for (cp = &dbname[dbname_len]; 
		(cp > dbname) && (*cp != '.') && (*cp != *PATH_SEPARATOR) ;
			cp--)
	 ;
    if (*cp == '.') {
	dbname_end = cp - dbname;
	if (PORT_Strcmp(cp,DIRSUFFIX) == 0) {
	    dbname_end = dbname_len;
	}
    }
    blobDir = PORT_ZAlloc(dbname_end+sizeof(DIRSUFFIX));
    if (blobDir == NULL) {
	return NULL;
    }
    PORT_Memcpy(blobDir,dbname,dbname_end);
    PORT_Memcpy(&blobDir[dbname_end],DIRSUFFIX,sizeof(DIRSUFFIX));
    return blobDir;
}

#define DBM_DEFAULT 0
static const HASHINFO dbs_hashInfo = {
	DBS_BLOCK_SIZE,		


	DBM_DEFAULT,		
	DBM_DEFAULT,		
	DBS_CACHE_SIZE,		
	DBM_DEFAULT,		
	DBM_DEFAULT,		
};





DB *
dbsopen(const char *dbname, int flags, int mode, DBTYPE type,
							 const void *userData)
{
    DB *db = NULL,*dbs = NULL;
    DBS *dbsp = NULL;

    


    dbsp = (DBS *)PORT_ZAlloc(sizeof(DBS));
    if (!dbsp) {
	return NULL;
    }
    dbs = &dbsp->db;

    dbsp->blobdir=dbs_mkBlobDirName(dbname);
    if (dbsp->blobdir == NULL) {
	goto loser;
    }
    dbsp->mode = mode;
    dbsp->readOnly = (PRBool)(flags == NO_RDONLY);
    dbsp->dbs_mapfile = NULL;
    dbsp->dbs_addr = NULL;
    dbsp->dbs_len = 0;

    
    db = dbopen(dbname, flags, mode, type, &dbs_hashInfo);
    if (db == NULL) {
	goto loser;
    }
    dbs->internal = (void *) db;
    dbs->type = type;
    dbs->close = dbs_close;
    dbs->get = dbs_get;
    dbs->del = dbs_del;
    dbs->put = dbs_put;
    dbs->seq = dbs_seq;
    dbs->sync = dbs_sync;
    dbs->fd = dbs_fd;

    return dbs;
loser:
    if (db) {
	(*db->close)(db);
    }
    if (dbsp) {
	if (dbsp->blobdir) {
	    PORT_Free(dbsp->blobdir);
	}
	PORT_Free(dbsp);
    }
    return NULL;
}
