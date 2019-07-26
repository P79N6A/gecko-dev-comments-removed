






#ifndef URESIMP_H
#define URESIMP_H

#include "unicode/ures.h"

#include "uresdata.h"

#define kRootLocaleName         "root"
#define kPoolBundleName         "pool"






#define kDefaultMinorVersion    "0"
#define kVersionSeparator       "."
#define kVersionTag             "Version"

#define MAGIC1 19700503
#define MAGIC2 19641227

#define URES_MAX_ALIAS_LEVEL 256
#define URES_MAX_BUFFER_SIZE 256

#define EMPTY_SET 0x2205












struct UResourceDataEntry;
typedef struct UResourceDataEntry UResourceDataEntry;







struct UResourceDataEntry {
    char *fName; 
    char *fPath; 
    UResourceDataEntry *fParent; 
    UResourceDataEntry *fAlias;
    UResourceDataEntry *fPool;
    ResourceData fData; 
    char fNameBuffer[3]; 
    uint32_t fCountExisting; 
    UErrorCode fBogus;
     
};

#define RES_BUFSIZE 64
#define RES_PATH_SEPARATOR   '/'
#define RES_PATH_SEPARATOR_S   "/"

struct UResourceBundle {
    const char *fKey; 
    UResourceDataEntry *fData; 
    char *fVersion;
    UResourceDataEntry *fTopLevelData; 
    char *fResPath; 
    ResourceData fResData;
    char fResBuf[RES_BUFSIZE];
    int32_t fResPathLen;
    Resource fRes;
    UBool fHasFallback;
    UBool fIsTopLevel;
    uint32_t fMagic1;   
    uint32_t fMagic2;   
    int32_t fIndex;
    int32_t fSize;

     
};

U_CAPI void U_EXPORT2 ures_initStackObject(UResourceBundle* resB);


U_CFUNC const char* ures_getName(const UResourceBundle* resB);
#ifdef URES_DEBUG
U_CFUNC const char* ures_getPath(const UResourceBundle* resB);




U_CAPI UBool U_EXPORT2 ures_dumpCacheContents(void);
#endif





U_CFUNC UResourceBundle *ures_copyResb(UResourceBundle *r, const UResourceBundle *original, UErrorCode *status);
















U_CAPI UResourceBundle* U_EXPORT2
ures_findResource(const char* pathToResource, 
                  UResourceBundle *fillIn, UErrorCode *status); 
















U_CAPI UResourceBundle* U_EXPORT2
ures_findSubResource(const UResourceBundle *resB, 
                     char* pathToResource, 
                     UResourceBundle *fillIn, UErrorCode *status);

















U_CAPI int32_t U_EXPORT2
ures_getFunctionalEquivalent(char *result, int32_t resultCapacity, 
                             const char *path, const char *resName, const char *keyword, const char *locid,
                             UBool *isAvailable, UBool omitDefault, UErrorCode *status);








U_CAPI UEnumeration* U_EXPORT2
ures_getKeywordValues(const char *path, const char *keyword, UErrorCode *status);


















U_CAPI UResourceBundle* U_EXPORT2 
ures_getByKeyWithFallback(const UResourceBundle *resB, 
                          const char* inKey, 
                          UResourceBundle *fillIn, 
                          UErrorCode *status);
















U_CAPI const UChar* U_EXPORT2 
ures_getStringByKeyWithFallback(const UResourceBundle *resB, 
                          const char* inKey,  
                          int32_t* len,
                          UErrorCode *status);








U_CAPI void U_EXPORT2
ures_getVersionByKey(const UResourceBundle *resB,
                     const char *key,
                     UVersionInfo ver,
                     UErrorCode *status);











U_CAPI const char* U_EXPORT2 
ures_getVersionNumberInternal(const UResourceBundle *resourceBundle);














U_CAPI const char* U_EXPORT2 
ures_getLocaleInternal(const UResourceBundle* resourceBundle, 
               UErrorCode* status);

#endif 
