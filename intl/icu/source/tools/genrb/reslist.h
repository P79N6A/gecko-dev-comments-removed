
















#ifndef RESLIST_H
#define RESLIST_H

#define KEY_SPACE_SIZE 65536
#define RESLIST_MAX_INT_VECTOR 2048

#include "unicode/utypes.h"
#include "unicode/ures.h"
#include "unicode/ustring.h"
#include "uresdata.h"
#include "cmemory.h"
#include "cstring.h"
#include "unewdata.h"
#include "ustr.h"
#include "uhash.h"

U_CDECL_BEGIN

typedef struct KeyMapEntry {
    int32_t oldpos, newpos;
} KeyMapEntry;


struct SRBRoot {
  struct SResource *fRoot;
  char *fLocale;
  int32_t fIndexLength;
  int32_t fMaxTableLength;
  UBool noFallback; 
  int8_t fStringsForm; 
  UBool fIsPoolBundle;

  char *fKeys;
  KeyMapEntry *fKeyMap;
  int32_t fKeysBottom, fKeysTop;
  int32_t fKeysCapacity;
  int32_t fKeysCount;
  int32_t fLocalKeyLimit; 

  UHashtable *fStringSet;
  uint16_t *f16BitUnits;
  int32_t f16BitUnitsCapacity;
  int32_t f16BitUnitsLength;

  const char *fPoolBundleKeys;
  int32_t fPoolBundleKeysLength;
  int32_t fPoolBundleKeysCount;
  int32_t fPoolChecksum;
};

struct SRBRoot *bundle_open(const struct UString* comment, UBool isPoolBundle, UErrorCode *status);
void bundle_write(struct SRBRoot *bundle, const char *outputDir, const char *outputPkg, char *writtenFilename, int writtenFilenameLen, UErrorCode *status);


void bundle_write_java(struct SRBRoot *bundle, const char *outputDir, const char* outputEnc, char *writtenFilename, 
                       int writtenFilenameLen, const char* packageName, const char* bundleName, UErrorCode *status);







void bundle_write_xml(struct SRBRoot *bundle, const char *outputDir,const char* outputEnc, const char* rbname,
                  char *writtenFilename, int writtenFilenameLen, const char* language, const char* package, UErrorCode *status);

void bundle_close(struct SRBRoot *bundle, UErrorCode *status);
void bundle_setlocale(struct SRBRoot *bundle, UChar *locale, UErrorCode *status);
int32_t bundle_addtag(struct SRBRoot *bundle, const char *tag, UErrorCode *status);

const char *
bundle_getKeyBytes(struct SRBRoot *bundle, int32_t *pLength);

int32_t
bundle_addKeyBytes(struct SRBRoot *bundle, const char *keyBytes, int32_t length, UErrorCode *status);

void
bundle_compactKeys(struct SRBRoot *bundle, UErrorCode *status);








struct SResource* res_none(void);

struct SResTable {
    uint32_t fCount;
    int8_t fType;  
    struct SResource *fFirst;
    struct SRBRoot *fRoot;
};

struct SResource* table_open(struct SRBRoot *bundle, const char *tag, const struct UString* comment, UErrorCode *status);
void table_add(struct SResource *table, struct SResource *res, int linenumber, UErrorCode *status);

struct SResArray {
    uint32_t fCount;
    struct SResource *fFirst;
    struct SResource *fLast;
};

struct SResource* array_open(struct SRBRoot *bundle, const char *tag, const struct UString* comment, UErrorCode *status);
void array_add(struct SResource *array, struct SResource *res, UErrorCode *status);

struct SResString {
    struct SResource *fSame;  
    UChar *fChars;
    int32_t fLength;
    int32_t fSuffixOffset;  
    int8_t fNumCharsForLength;
};

struct SResource *string_open(struct SRBRoot *bundle, const char *tag, const UChar *value, int32_t len, const struct UString* comment, UErrorCode *status);






void bundle_closeString(struct SRBRoot *bundle, struct SResource *string);

struct SResource *alias_open(struct SRBRoot *bundle, const char *tag, UChar *value, int32_t len, const struct UString* comment, UErrorCode *status);

struct SResIntVector {
    uint32_t fCount;
    uint32_t *fArray;
};

struct SResource* intvector_open(struct SRBRoot *bundle, const char *tag,  const struct UString* comment, UErrorCode *status);
void intvector_add(struct SResource *intvector, int32_t value, UErrorCode *status);

struct SResInt {
    uint32_t fValue;
};

struct SResource *int_open(struct SRBRoot *bundle, const char *tag, int32_t value, const struct UString* comment, UErrorCode *status);

struct SResBinary {
    uint32_t fLength;
    uint8_t *fData;
    char* fFileName; 
};

struct SResource *bin_open(struct SRBRoot *bundle, const char *tag, uint32_t length, uint8_t *data, const char* fileName, const struct UString* comment, UErrorCode *status);



struct SResource {
    int8_t   fType;     
    UBool    fWritten;  
    uint32_t fRes;      
    int32_t  fKey;      
    int      line;      
    struct SResource *fNext; 
    struct UString fComment;
    union {
        struct SResTable fTable;
        struct SResArray fArray;
        struct SResString fString;
        struct SResIntVector fIntVector;
        struct SResInt fIntValue;
        struct SResBinary fBinaryValue;
    } u;
};

const char *
res_getKeyString(const struct SRBRoot *bundle, const struct SResource *res, char temp[8]);

void res_close(struct SResource *res);

void setIncludeCopyright(UBool val);
UBool getIncludeCopyright(void);

void setFormatVersion(int32_t formatVersion);

void setUsePoolBundle(UBool use);


uint32_t computeCRC(char *ptr, uint32_t len, uint32_t lastcrc);

U_CDECL_END
#endif 
