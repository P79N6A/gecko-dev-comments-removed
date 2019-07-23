










































#ifndef _REG_H_
#define _REG_H_

#include "vr_stubs.h"

#ifndef STANDALONE_REGISTRY
#include "prlock.h"
#endif





#define MAGIC_NUMBER    0x76644441L
#define MAJOR_VERSION   1          /* major version for incompatible changes */
#define MINOR_VERSION   2          /* minor ver for new (compatible) features */
#define PATHDEL         '/'
#define HDRRESERVE      128        /* number of bytes reserved for hdr */
#define INTSIZE         4
#define DOUBLESIZE      8

#define PACKBUFFERSIZE  2048



#define REGTYPE_KEY                   (1)
#define REGTYPE_DELETED               (0x0080)


#define ROOTKEY                       (0x20)
#define ROOTKEY_VERSIONS              (0x21)


#define ROOTKEY_STR             "/"
#define ROOTKEY_VERSIONS_STR    "Version Registry"
#define ROOTKEY_USERS_STR       "Users"
#define ROOTKEY_COMMON_STR      "Common"
#define ROOTKEY_PRIVATE_STR     "Private Arenas"

#define OLD_VERSIONS_STR        "ROOTKEY_VERSIONS"
#define OLD_USERS_STR           "ROOTKEY_USERS"
#define OLD_COMMON_STR          "ROOTKEY_COMMON"



#define ASW_MAGIC_PROFILE_NAME "User1"


#define COPYDESC(dest,src)  memcpy((dest),(src),sizeof(REGDESC))

#define VALID_FILEHANDLE(fh)    ((fh) != NULL)

#define INVALID_NAME_CHAR(p)    ( ((unsigned char)(p) < 0x20) )

#define TYPE_IS_ENTRY(type)       ( (type) & REGTYPE_ENTRY )
#define TYPE_IS_KEY(type)         ( !((type) & REGTYPE_ENTRY) )

#define VERIFY_HREG(h)\
    ( ((h) == NULL) ? REGERR_PARAM : \
    ( (((REGHANDLE*)(h))->magic == MAGIC_NUMBER) ? REGERR_OK : REGERR_BADMAGIC ) )







#undef REGOFF
typedef int32 REGOFF;   

typedef struct _desc
{
    REGOFF  location;   
    REGOFF  name;       
    uint16  namelen;    
    uint16  type;       
    REGOFF  left;       
    REGOFF  down;       
    REGOFF  value;      
    uint32  valuelen;   
    uint32  valuebuf;   
    REGOFF  parent;     
} REGDESC;


#define DESC_LOCATION   0
#define DESC_NAME       4
#define DESC_NAMELEN    8
#define DESC_TYPE       10
#define DESC_LEFT       12
#define DESC_DOWN       16
#define DESC_VALUE      20
#define DESC_VALUELEN   24
#define DESC_VALUEBUF   16    /* stored in place of "down" for entries */
#define DESC_PARENT     28

#define DESC_SIZE       32    /* size of desc on disk */

typedef struct _hdr
{
    uint32  magic;      
    uint16  verMajor;   
    uint16  verMinor;   
    REGOFF  avail;      
    REGOFF  root;       
} REGHDR;


#define HDR_MAGIC       0
#define HDR_VERMAJOR    4
#define HDR_VERMINOR    6
#define HDR_AVAIL       8
#define HDR_ROOT        12

typedef XP_File FILEHANDLE; 

typedef struct _stdnodes {
    REGOFF          versions;
    REGOFF          users;
    REGOFF          common;
    REGOFF          current_user;
    REGOFF          privarea;
} STDNODES;

typedef struct _regfile
{
    FILEHANDLE      fh;
    REGHDR          hdr;
    int             refCount;
    int             hdrDirty;
    int             inInit;
    int             readOnly;
    char *          filename;
    STDNODES        rkeys;
    struct _regfile *next;
    struct _regfile *prev;
#ifndef STANDALONE_REGISTRY
    PRLock          *lock;
    PRUint64        uniqkey;
#endif
} REGFILE;

typedef struct _reghandle
{
    uint32          magic;     
    REGFILE         *pReg;     
} REGHANDLE;


#endif  



