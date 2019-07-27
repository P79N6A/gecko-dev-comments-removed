
















#include <assert.h>
#include <stdio.h>
#include "reslist.h"
#include "unewdata.h"
#include "unicode/ures.h"
#include "unicode/putil.h"
#include "errmsg.h"

#include "uarrsort.h"
#include "uelement.h"
#include "uhash.h"
#include "uinvchar.h"
#include "ustr_imp.h"
#include "unicode/utf16.h"




#define BIN_ALIGNMENT 16

static UBool gIncludeCopyright = FALSE;
static UBool gUsePoolBundle = FALSE;
static int32_t gFormatVersion = 2;

static UChar gEmptyString = 0;


enum {
    STRINGS_UTF16_V1,   
    STRINGS_UTF16_V2    
};

enum {
    MAX_IMPLICIT_STRING_LENGTH = 40  
};






static const struct SResource kNoResource = { URES_NONE };

static UDataInfo dataInfo= {
    sizeof(UDataInfo),
    0,

    U_IS_BIG_ENDIAN,
    U_CHARSET_FAMILY,
    sizeof(UChar),
    0,

    {0x52, 0x65, 0x73, 0x42},     
    {1, 3, 0, 0},                 
    {1, 4, 0, 0}                  
};

static const UVersionInfo gFormatVersions[3] = {  
    { 0, 0, 0, 0 },
    { 1, 3, 0, 0 },
    { 2, 0, 0, 0 }
};

static uint8_t calcPadding(uint32_t size) {
    
    return (uint8_t) ((size % sizeof(uint32_t)) ? (sizeof(uint32_t) - (size % sizeof(uint32_t))) : 0);

}

void setIncludeCopyright(UBool val){
    gIncludeCopyright=val;
}

UBool getIncludeCopyright(void){
    return gIncludeCopyright;
}

void setFormatVersion(int32_t formatVersion) {
    gFormatVersion = formatVersion;
}

void setUsePoolBundle(UBool use) {
    gUsePoolBundle = use;
}

static void
bundle_compactStrings(struct SRBRoot *bundle, UErrorCode *status);












static void
res_preflightStrings(struct SRBRoot *bundle, struct SResource *res, UHashtable *stringSet,
                     UErrorCode *status);





static void
res_write16(struct SRBRoot *bundle, struct SResource *res,
            UErrorCode *status);















static void
res_preWrite(uint32_t *byteOffset,
             struct SRBRoot *bundle, struct SResource *res,
             UErrorCode *status);






static void
res_write(UNewDataMemory *mem, uint32_t *byteOffset,
          struct SRBRoot *bundle, struct SResource *res,
          UErrorCode *status);

static void
string_preflightStrings(struct SRBRoot *bundle, struct SResource *res, UHashtable *stringSet,
                        UErrorCode *status) {
    res->u.fString.fSame = uhash_get(stringSet, res);
    if (res->u.fString.fSame != NULL) {
        return;  
    }
    
    uhash_put(stringSet, res, res, status);

    if (bundle->fStringsForm != STRINGS_UTF16_V1) {
        const UChar *s = res->u.fString.fChars;
        int32_t len = res->u.fString.fLength;
        if (len <= MAX_IMPLICIT_STRING_LENGTH && !U16_IS_TRAIL(s[0]) && len == u_strlen(s)) {
            



            res->u.fString.fNumCharsForLength = 0;
        } else if (len <= 0x3ee) {
            res->u.fString.fNumCharsForLength = 1;
        } else if (len <= 0xfffff) {
            res->u.fString.fNumCharsForLength = 2;
        } else {
            res->u.fString.fNumCharsForLength = 3;
        }
        bundle->f16BitUnitsLength += res->u.fString.fNumCharsForLength + len + 1;  
    }
}

static void
array_preflightStrings(struct SRBRoot *bundle, struct SResource *res, UHashtable *stringSet,
                       UErrorCode *status) {
    struct SResource *current;

    if (U_FAILURE(*status)) {
        return;
    }
    for (current = res->u.fArray.fFirst; current != NULL; current = current->fNext) {
        res_preflightStrings(bundle, current, stringSet, status);
    }
}

static void
table_preflightStrings(struct SRBRoot *bundle, struct SResource *res, UHashtable *stringSet,
                       UErrorCode *status) {
    struct SResource *current;

    if (U_FAILURE(*status)) {
        return;
    }
    for (current = res->u.fTable.fFirst; current != NULL; current = current->fNext) {
        res_preflightStrings(bundle, current, stringSet, status);
    }
}

static void
res_preflightStrings(struct SRBRoot *bundle, struct SResource *res, UHashtable *stringSet,
                     UErrorCode *status) {
    if (U_FAILURE(*status) || res == NULL) {
        return;
    }
    if (res->fRes != RES_BOGUS) {
        




        return;
    }
    switch (res->fType) {
    case URES_STRING:
        string_preflightStrings(bundle, res, stringSet, status);
        break;
    case URES_ARRAY:
        array_preflightStrings(bundle, res, stringSet, status);
        break;
    case URES_TABLE:
        table_preflightStrings(bundle, res, stringSet, status);
        break;
    default:
        
        break;
    }
}

static uint16_t *
reserve16BitUnits(struct SRBRoot *bundle, int32_t length, UErrorCode *status) {
    if (U_FAILURE(*status)) {
        return NULL;
    }
    if ((bundle->f16BitUnitsLength + length) > bundle->f16BitUnitsCapacity) {
        uint16_t *newUnits;
        int32_t capacity = 2 * bundle->f16BitUnitsCapacity + length + 1024;
        capacity &= ~1;  
        newUnits = (uint16_t *)uprv_malloc(capacity * 2);
        if (newUnits == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            return NULL;
        }
        if (bundle->f16BitUnitsLength > 0) {
            uprv_memcpy(newUnits, bundle->f16BitUnits, bundle->f16BitUnitsLength * 2);
        } else {
            newUnits[0] = 0;
            bundle->f16BitUnitsLength = 1;
        }
        uprv_free(bundle->f16BitUnits);
        bundle->f16BitUnits = newUnits;
        bundle->f16BitUnitsCapacity = capacity;
    }
    return bundle->f16BitUnits + bundle->f16BitUnitsLength;
}

static int32_t
makeRes16(uint32_t resWord) {
    uint32_t type, offset;
    if (resWord == 0) {
        return 0;  
    }
    type = RES_GET_TYPE(resWord);
    offset = RES_GET_OFFSET(resWord);
    if (type == URES_STRING_V2 && offset <= 0xffff) {
        return (int32_t)offset;
    }
    return -1;
}

static int32_t
mapKey(struct SRBRoot *bundle, int32_t oldpos) {
    const KeyMapEntry *map = bundle->fKeyMap;
    int32_t i, start, limit;

    
    start = bundle->fPoolBundleKeysCount;
    limit = start + bundle->fKeysCount;
    while (start < limit - 1) {
        i = (start + limit) / 2;
        if (oldpos < map[i].oldpos) {
            limit = i;
        } else {
            start = i;
        }
    }
    assert(oldpos == map[start].oldpos);
    return map[start].newpos;
}

static uint16_t
makeKey16(struct SRBRoot *bundle, int32_t key) {
    if (key >= 0) {
        return (uint16_t)key;
    } else {
        return (uint16_t)(key + bundle->fLocalKeyLimit);  
    }
}






static void
string_write16(struct SRBRoot *bundle, struct SResource *res, UErrorCode *status) {
    struct SResource *same;
    if ((same = res->u.fString.fSame) != NULL) {
        
        assert(same->fRes != RES_BOGUS && same->fWritten);
        res->fRes = same->fRes;
        res->fWritten = same->fWritten;
    }
}

static void
array_write16(struct SRBRoot *bundle, struct SResource *res,
              UErrorCode *status) {
    struct SResource *current;
    int32_t res16 = 0;

    if (U_FAILURE(*status)) {
        return;
    }
    if (res->u.fArray.fCount == 0 && gFormatVersion > 1) {
        res->fRes = URES_MAKE_EMPTY_RESOURCE(URES_ARRAY);
        res->fWritten = TRUE;
        return;
    }
    for (current = res->u.fArray.fFirst; current != NULL; current = current->fNext) {
        res_write16(bundle, current, status);
        res16 |= makeRes16(current->fRes);
    }
    if (U_SUCCESS(*status) && res->u.fArray.fCount <= 0xffff && res16 >= 0 && gFormatVersion > 1) {
        uint16_t *p16 = reserve16BitUnits(bundle, 1 + res->u.fArray.fCount, status);
        if (U_SUCCESS(*status)) {
            res->fRes = URES_MAKE_RESOURCE(URES_ARRAY16, bundle->f16BitUnitsLength);
            *p16++ = (uint16_t)res->u.fArray.fCount;
            for (current = res->u.fArray.fFirst; current != NULL; current = current->fNext) {
                *p16++ = (uint16_t)makeRes16(current->fRes);
            }
            bundle->f16BitUnitsLength += 1 + res->u.fArray.fCount;
            res->fWritten = TRUE;
        }
    }
}

static void
table_write16(struct SRBRoot *bundle, struct SResource *res,
              UErrorCode *status) {
    struct SResource *current;
    int32_t maxKey = 0, maxPoolKey = 0x80000000;
    int32_t res16 = 0;
    UBool hasLocalKeys = FALSE, hasPoolKeys = FALSE;

    if (U_FAILURE(*status)) {
        return;
    }
    if (res->u.fTable.fCount == 0 && gFormatVersion > 1) {
        res->fRes = URES_MAKE_EMPTY_RESOURCE(URES_TABLE);
        res->fWritten = TRUE;
        return;
    }
    
    for (current = res->u.fTable.fFirst; current != NULL; current = current->fNext) {
        int32_t key;
        res_write16(bundle, current, status);
        if (bundle->fKeyMap == NULL) {
            key = current->fKey;
        } else {
            key = current->fKey = mapKey(bundle, current->fKey);
        }
        if (key >= 0) {
            hasLocalKeys = TRUE;
            if (key > maxKey) {
                maxKey = key;
            }
        } else {
            hasPoolKeys = TRUE;
            if (key > maxPoolKey) {
                maxPoolKey = key;
            }
        }
        res16 |= makeRes16(current->fRes);
    }
    if (U_FAILURE(*status)) {
        return;
    }
    if(res->u.fTable.fCount > (uint32_t)bundle->fMaxTableLength) {
        bundle->fMaxTableLength = res->u.fTable.fCount;
    }
    maxPoolKey &= 0x7fffffff;
    if (res->u.fTable.fCount <= 0xffff &&
        (!hasLocalKeys || maxKey < bundle->fLocalKeyLimit) &&
        (!hasPoolKeys || maxPoolKey < (0x10000 - bundle->fLocalKeyLimit))
    ) {
        if (res16 >= 0 && gFormatVersion > 1) {
            uint16_t *p16 = reserve16BitUnits(bundle, 1 + res->u.fTable.fCount * 2, status);
            if (U_SUCCESS(*status)) {
                
                res->fRes = URES_MAKE_RESOURCE(URES_TABLE16, bundle->f16BitUnitsLength);
                *p16++ = (uint16_t)res->u.fTable.fCount;
                for (current = res->u.fTable.fFirst; current != NULL; current = current->fNext) {
                    *p16++ = makeKey16(bundle, current->fKey);
                }
                for (current = res->u.fTable.fFirst; current != NULL; current = current->fNext) {
                    *p16++ = (uint16_t)makeRes16(current->fRes);
                }
                bundle->f16BitUnitsLength += 1 + res->u.fTable.fCount * 2;
                res->fWritten = TRUE;
            }
        } else {
            
            res->u.fTable.fType = URES_TABLE;
        }
    } else {
        
        res->u.fTable.fType = URES_TABLE32;
    }
}

static void
res_write16(struct SRBRoot *bundle, struct SResource *res,
            UErrorCode *status) {
    if (U_FAILURE(*status) || res == NULL) {
        return;
    }
    if (res->fRes != RES_BOGUS) {
        





        return;
    }
    switch (res->fType) {
    case URES_STRING:
        string_write16(bundle, res, status);
        break;
    case URES_ARRAY:
        array_write16(bundle, res, status);
        break;
    case URES_TABLE:
        table_write16(bundle, res, status);
        break;
    default:
        
        break;
    }
}






static void
string_preWrite(uint32_t *byteOffset,
                struct SRBRoot *bundle, struct SResource *res,
                UErrorCode *status) {
    
    res->fRes = URES_MAKE_RESOURCE(URES_STRING, *byteOffset >> 2);
    *byteOffset += 4 + (res->u.fString.fLength + 1) * U_SIZEOF_UCHAR;
}

static void
bin_preWrite(uint32_t *byteOffset,
             struct SRBRoot *bundle, struct SResource *res,
             UErrorCode *status) {
    uint32_t pad       = 0;
    uint32_t dataStart = *byteOffset + sizeof(res->u.fBinaryValue.fLength);

    if (dataStart % BIN_ALIGNMENT) {
        pad = (BIN_ALIGNMENT - dataStart % BIN_ALIGNMENT);
        *byteOffset += pad;  
    }
    res->fRes = URES_MAKE_RESOURCE(URES_BINARY, *byteOffset >> 2);
    *byteOffset += 4 + res->u.fBinaryValue.fLength;
}

static void
array_preWrite(uint32_t *byteOffset,
               struct SRBRoot *bundle, struct SResource *res,
               UErrorCode *status) {
    struct SResource *current;

    if (U_FAILURE(*status)) {
        return;
    }
    for (current = res->u.fArray.fFirst; current != NULL; current = current->fNext) {
        res_preWrite(byteOffset, bundle, current, status);
    }
    res->fRes = URES_MAKE_RESOURCE(URES_ARRAY, *byteOffset >> 2);
    *byteOffset += (1 + res->u.fArray.fCount) * 4;
}

static void
table_preWrite(uint32_t *byteOffset,
               struct SRBRoot *bundle, struct SResource *res,
               UErrorCode *status) {
    struct SResource *current;

    if (U_FAILURE(*status)) {
        return;
    }
    for (current = res->u.fTable.fFirst; current != NULL; current = current->fNext) {
        res_preWrite(byteOffset, bundle, current, status);
    }
    if (res->u.fTable.fType == URES_TABLE) {
        
        res->fRes = URES_MAKE_RESOURCE(URES_TABLE, *byteOffset >> 2);
        *byteOffset += 2 + res->u.fTable.fCount * 6;
    } else {
        
        res->fRes = URES_MAKE_RESOURCE(URES_TABLE32, *byteOffset >> 2);
        *byteOffset += 4 + res->u.fTable.fCount * 8;
    }
}

static void
res_preWrite(uint32_t *byteOffset,
             struct SRBRoot *bundle, struct SResource *res,
             UErrorCode *status) {
    if (U_FAILURE(*status) || res == NULL) {
        return;
    }
    if (res->fRes != RES_BOGUS) {
        





        return;
    }
    switch (res->fType) {
    case URES_STRING:
        string_preWrite(byteOffset, bundle, res, status);
        break;
    case URES_ALIAS:
        res->fRes = URES_MAKE_RESOURCE(URES_ALIAS, *byteOffset >> 2);
        *byteOffset += 4 + (res->u.fString.fLength + 1) * U_SIZEOF_UCHAR;
        break;
    case URES_INT_VECTOR:
        if (res->u.fIntVector.fCount == 0 && gFormatVersion > 1) {
            res->fRes = URES_MAKE_EMPTY_RESOURCE(URES_INT_VECTOR);
            res->fWritten = TRUE;
        } else {
            res->fRes = URES_MAKE_RESOURCE(URES_INT_VECTOR, *byteOffset >> 2);
            *byteOffset += (1 + res->u.fIntVector.fCount) * 4;
        }
        break;
    case URES_BINARY:
        bin_preWrite(byteOffset, bundle, res, status);
        break;
    case URES_INT:
        break;
    case URES_ARRAY:
        array_preWrite(byteOffset, bundle, res, status);
        break;
    case URES_TABLE:
        table_preWrite(byteOffset, bundle, res, status);
        break;
    default:
        *status = U_INTERNAL_PROGRAM_ERROR;
        break;
    }
    *byteOffset += calcPadding(*byteOffset);
}





static void string_write(UNewDataMemory *mem, uint32_t *byteOffset,
                         struct SRBRoot *bundle, struct SResource *res,
                         UErrorCode *status) {
    
    int32_t length = res->u.fString.fLength;
    udata_write32(mem, length);
    udata_writeUString(mem, res->u.fString.fChars, length + 1);
    *byteOffset += 4 + (length + 1) * U_SIZEOF_UCHAR;
    res->fWritten = TRUE;
}

static void alias_write(UNewDataMemory *mem, uint32_t *byteOffset,
                        struct SRBRoot *bundle, struct SResource *res,
                        UErrorCode *status) {
    int32_t length = res->u.fString.fLength;
    udata_write32(mem, length);
    udata_writeUString(mem, res->u.fString.fChars, length + 1);
    *byteOffset += 4 + (length + 1) * U_SIZEOF_UCHAR;
}

static void array_write(UNewDataMemory *mem, uint32_t *byteOffset,
                        struct SRBRoot *bundle, struct SResource *res,
                        UErrorCode *status) {
    uint32_t  i;

    struct SResource *current = NULL;

    if (U_FAILURE(*status)) {
        return;
    }
    for (i = 0, current = res->u.fArray.fFirst; current != NULL; ++i, current = current->fNext) {
        res_write(mem, byteOffset, bundle, current, status);
    }
    assert(i == res->u.fArray.fCount);

    udata_write32(mem, res->u.fArray.fCount);
    for (current = res->u.fArray.fFirst; current != NULL; current = current->fNext) {
        udata_write32(mem, current->fRes);
    }
    *byteOffset += (1 + res->u.fArray.fCount) * 4;
}

static void intvector_write(UNewDataMemory *mem, uint32_t *byteOffset,
                            struct SRBRoot *bundle, struct SResource *res,
                            UErrorCode *status) {
    uint32_t i = 0;
    udata_write32(mem, res->u.fIntVector.fCount);
    for(i = 0; i<res->u.fIntVector.fCount; i++) {
      udata_write32(mem, res->u.fIntVector.fArray[i]);
    }
    *byteOffset += (1 + res->u.fIntVector.fCount) * 4;
}

static void bin_write(UNewDataMemory *mem, uint32_t *byteOffset,
                      struct SRBRoot *bundle, struct SResource *res,
                      UErrorCode *status) {
    uint32_t pad       = 0;
    uint32_t dataStart = *byteOffset + sizeof(res->u.fBinaryValue.fLength);

    if (dataStart % BIN_ALIGNMENT) {
        pad = (BIN_ALIGNMENT - dataStart % BIN_ALIGNMENT);
        udata_writePadding(mem, pad);  
        *byteOffset += pad;
    }

    udata_write32(mem, res->u.fBinaryValue.fLength);
    if (res->u.fBinaryValue.fLength > 0) {
        udata_writeBlock(mem, res->u.fBinaryValue.fData, res->u.fBinaryValue.fLength);
    }
    *byteOffset += 4 + res->u.fBinaryValue.fLength;
}

static void table_write(UNewDataMemory *mem, uint32_t *byteOffset,
                        struct SRBRoot *bundle, struct SResource *res,
                        UErrorCode *status) {
    struct SResource *current;
    uint32_t i;

    if (U_FAILURE(*status)) {
        return;
    }
    for (i = 0, current = res->u.fTable.fFirst; current != NULL; ++i, current = current->fNext) {
        assert(i < res->u.fTable.fCount);
        res_write(mem, byteOffset, bundle, current, status);
    }
    assert(i == res->u.fTable.fCount);

    if(res->u.fTable.fType == URES_TABLE) {
        udata_write16(mem, (uint16_t)res->u.fTable.fCount);
        for (current = res->u.fTable.fFirst; current != NULL; current = current->fNext) {
            udata_write16(mem, makeKey16(bundle, current->fKey));
        }
        *byteOffset += (1 + res->u.fTable.fCount)* 2;
        if ((res->u.fTable.fCount & 1) == 0) {
            
            udata_writePadding(mem, 2);
            *byteOffset += 2;
        }
    } else  {
        udata_write32(mem, res->u.fTable.fCount);
        for (current = res->u.fTable.fFirst; current != NULL; current = current->fNext) {
            udata_write32(mem, (uint32_t)current->fKey);
        }
        *byteOffset += (1 + res->u.fTable.fCount)* 4;
    }
    for (current = res->u.fTable.fFirst; current != NULL; current = current->fNext) {
        udata_write32(mem, current->fRes);
    }
    *byteOffset += res->u.fTable.fCount * 4;
}

void res_write(UNewDataMemory *mem, uint32_t *byteOffset,
               struct SRBRoot *bundle, struct SResource *res,
               UErrorCode *status) {
    uint8_t paddingSize;

    if (U_FAILURE(*status) || res == NULL) {
        return;
    }
    if (res->fWritten) {
        assert(res->fRes != RES_BOGUS);
        return;
    }
    switch (res->fType) {
    case URES_STRING:
        string_write    (mem, byteOffset, bundle, res, status);
        break;
    case URES_ALIAS:
        alias_write     (mem, byteOffset, bundle, res, status);
        break;
    case URES_INT_VECTOR:
        intvector_write (mem, byteOffset, bundle, res, status);
        break;
    case URES_BINARY:
        bin_write       (mem, byteOffset, bundle, res, status);
        break;
    case URES_INT:
        break;  
    case URES_ARRAY:
        array_write     (mem, byteOffset, bundle, res, status);
        break;
    case URES_TABLE:
        table_write     (mem, byteOffset, bundle, res, status);
        break;
    default:
        *status = U_INTERNAL_PROGRAM_ERROR;
        break;
    }
    paddingSize = calcPadding(*byteOffset);
    if (paddingSize > 0) {
        udata_writePadding(mem, paddingSize);
        *byteOffset += paddingSize;
    }
    res->fWritten = TRUE;
}

void bundle_write(struct SRBRoot *bundle,
                  const char *outputDir, const char *outputPkg,
                  char *writtenFilename, int writtenFilenameLen,
                  UErrorCode *status) {
    UNewDataMemory *mem        = NULL;
    uint32_t        byteOffset = 0;
    uint32_t        top, size;
    char            dataName[1024];
    int32_t         indexes[URES_INDEX_TOP];

    bundle_compactKeys(bundle, status);
    



    while (bundle->fKeysTop & 3) {
        bundle->fKeys[bundle->fKeysTop++] = (char)0xaa;
    }
    







    if (bundle->fKeysBottom < bundle->fKeysTop) {
        if (bundle->fKeysTop <= 0x10000) {
            bundle->fLocalKeyLimit = bundle->fKeysTop;
        } else {
            bundle->fLocalKeyLimit = 0x10000;
        }
    } else {
        bundle->fLocalKeyLimit = 0;
    }

    bundle_compactStrings(bundle, status);
    res_write16(bundle, bundle->fRoot, status);
    if (bundle->f16BitUnitsLength & 1) {
        bundle->f16BitUnits[bundle->f16BitUnitsLength++] = 0xaaaa;  
    }
    
    uprv_free(bundle->fKeyMap);
    bundle->fKeyMap = NULL;

    byteOffset = bundle->fKeysTop + bundle->f16BitUnitsLength * 2;
    res_preWrite(&byteOffset, bundle, bundle->fRoot, status);

    
    top = byteOffset;

    if (U_FAILURE(*status)) {
        return;
    }

    if (writtenFilename && writtenFilenameLen) {
        *writtenFilename = 0;
    }

    if (writtenFilename) {
       int32_t off = 0, len = 0;
       if (outputDir) {
           len = (int32_t)uprv_strlen(outputDir);
           if (len > writtenFilenameLen) {
               len = writtenFilenameLen;
           }
           uprv_strncpy(writtenFilename, outputDir, len);
       }
       if (writtenFilenameLen -= len) {
           off += len;
           writtenFilename[off] = U_FILE_SEP_CHAR;
           if (--writtenFilenameLen) {
               ++off;
               if(outputPkg != NULL)
               {
                   uprv_strcpy(writtenFilename+off, outputPkg);
                   off += (int32_t)uprv_strlen(outputPkg);
                   writtenFilename[off] = '_';
                   ++off;
               }

               len = (int32_t)uprv_strlen(bundle->fLocale);
               if (len > writtenFilenameLen) {
                   len = writtenFilenameLen;
               }
               uprv_strncpy(writtenFilename + off, bundle->fLocale, len);
               if (writtenFilenameLen -= len) {
                   off += len;
                   len = 5;
                   if (len > writtenFilenameLen) {
                       len = writtenFilenameLen;
                   }
                   uprv_strncpy(writtenFilename +  off, ".res", len);
               }
           }
       }
    }

    if(outputPkg)
    {
        uprv_strcpy(dataName, outputPkg);
        uprv_strcat(dataName, "_");
        uprv_strcat(dataName, bundle->fLocale);
    }
    else
    {
        uprv_strcpy(dataName, bundle->fLocale);
    }

    uprv_memcpy(dataInfo.formatVersion, gFormatVersions + gFormatVersion, sizeof(UVersionInfo));

    mem = udata_create(outputDir, "res", dataName, &dataInfo, (gIncludeCopyright==TRUE)? U_COPYRIGHT_STRING:NULL, status);
    if(U_FAILURE(*status)){
        return;
    }

    
    udata_write32(mem, bundle->fRoot->fRes);

    




    uprv_memset(indexes, 0, sizeof(indexes));
    indexes[URES_INDEX_LENGTH]=             bundle->fIndexLength;
    indexes[URES_INDEX_KEYS_TOP]=           bundle->fKeysTop>>2;
    indexes[URES_INDEX_RESOURCES_TOP]=      (int32_t)(top>>2);
    indexes[URES_INDEX_BUNDLE_TOP]=         indexes[URES_INDEX_RESOURCES_TOP];
    indexes[URES_INDEX_MAX_TABLE_LENGTH]=   bundle->fMaxTableLength;

    




    if (bundle->noFallback) {
        indexes[URES_INDEX_ATTRIBUTES]=URES_ATT_NO_FALLBACK;
    }
    



    if (URES_INDEX_16BIT_TOP < bundle->fIndexLength) {
        indexes[URES_INDEX_16BIT_TOP] = (bundle->fKeysTop>>2) + (bundle->f16BitUnitsLength>>1);
    }
    if (URES_INDEX_POOL_CHECKSUM < bundle->fIndexLength) {
        if (bundle->fIsPoolBundle) {
            indexes[URES_INDEX_ATTRIBUTES] |= URES_ATT_IS_POOL_BUNDLE | URES_ATT_NO_FALLBACK;
            indexes[URES_INDEX_POOL_CHECKSUM] =
                (int32_t)computeCRC((char *)(bundle->fKeys + bundle->fKeysBottom),
                                    (uint32_t)(bundle->fKeysTop - bundle->fKeysBottom),
                                    0);
        } else if (gUsePoolBundle) {
            indexes[URES_INDEX_ATTRIBUTES] |= URES_ATT_USES_POOL_BUNDLE;
            indexes[URES_INDEX_POOL_CHECKSUM] = bundle->fPoolChecksum;
        }
    }

    
    udata_writeBlock(mem, indexes, bundle->fIndexLength*4);

    
    udata_writeBlock(mem, bundle->fKeys+bundle->fKeysBottom,
                          bundle->fKeysTop-bundle->fKeysBottom);

    
    udata_writeBlock(mem, bundle->f16BitUnits, bundle->f16BitUnitsLength*2);

    
    byteOffset = bundle->fKeysTop + bundle->f16BitUnitsLength * 2;
    res_write(mem, &byteOffset, bundle, bundle->fRoot, status);
    assert(byteOffset == top);

    size = udata_finish(mem, status);
    if(top != size) {
        fprintf(stderr, "genrb error: wrote %u bytes but counted %u\n",
                (int)size, (int)top);
        *status = U_INTERNAL_PROGRAM_ERROR;
    }
}




struct SResource* res_open(struct SRBRoot *bundle, const char *tag,
                           const struct UString* comment, UErrorCode* status);

struct SResource* res_open(struct SRBRoot *bundle, const char *tag,
                           const struct UString* comment, UErrorCode* status){
    struct SResource *res;
    int32_t key = bundle_addtag(bundle, tag, status);
    if (U_FAILURE(*status)) {
        return NULL;
    }

    res = (struct SResource *) uprv_malloc(sizeof(struct SResource));
    if (res == NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }
    uprv_memset(res, 0, sizeof(struct SResource));
    res->fKey = key;
    res->fRes = RES_BOGUS;

    ustr_init(&res->fComment);
    if(comment != NULL){
        ustr_cpy(&res->fComment, comment, status);
        if (U_FAILURE(*status)) {
            res_close(res);
            return NULL;
        }
    }
    return res;
}

struct SResource* res_none() {
    return (struct SResource*)&kNoResource;
}

struct SResource* table_open(struct SRBRoot *bundle, const char *tag, const struct UString* comment, UErrorCode *status) {
    struct SResource *res = res_open(bundle, tag, comment, status);
    if (U_FAILURE(*status)) {
        return NULL;
    }
    res->fType = URES_TABLE;
    res->u.fTable.fRoot = bundle;
    return res;
}

struct SResource* array_open(struct SRBRoot *bundle, const char *tag, const struct UString* comment, UErrorCode *status) {
    struct SResource *res = res_open(bundle, tag, comment, status);
    if (U_FAILURE(*status)) {
        return NULL;
    }
    res->fType = URES_ARRAY;
    return res;
}

static int32_t U_CALLCONV
string_hash(const UElement key) {
    const struct SResource *res = (struct SResource *)key.pointer;
    return ustr_hashUCharsN(res->u.fString.fChars, res->u.fString.fLength);
}

static UBool U_CALLCONV
string_comp(const UElement key1, const UElement key2) {
    const struct SResource *res1 = (struct SResource *)key1.pointer;
    const struct SResource *res2 = (struct SResource *)key2.pointer;
    return 0 == u_strCompare(res1->u.fString.fChars, res1->u.fString.fLength,
                             res2->u.fString.fChars, res2->u.fString.fLength,
                             FALSE);
}

static struct SResource *
stringbase_open(struct SRBRoot *bundle, const char *tag, int8_t type,
                const UChar *value, int32_t len, const struct UString* comment,
                UErrorCode *status) {
    struct SResource *res = res_open(bundle, tag, comment, status);
    if (U_FAILURE(*status)) {
        return NULL;
    }
    res->fType = type;

    if (len == 0 && gFormatVersion > 1) {
        res->u.fString.fChars = &gEmptyString;
        res->fRes = URES_MAKE_EMPTY_RESOURCE(type);
        res->fWritten = TRUE;
        return res;
    }

    res->u.fString.fLength = len;
    res->u.fString.fChars = (UChar *) uprv_malloc(sizeof(UChar) * (len + 1));
    if (res->u.fString.fChars == NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        uprv_free(res);
        return NULL;
    }
    uprv_memcpy(res->u.fString.fChars, value, sizeof(UChar) * len);
    res->u.fString.fChars[len] = 0;
    return res;
}

struct SResource *string_open(struct SRBRoot *bundle, const char *tag, const UChar *value, int32_t len, const struct UString* comment, UErrorCode *status) {
    return stringbase_open(bundle, tag, URES_STRING, value, len, comment, status);
}

struct SResource *alias_open(struct SRBRoot *bundle, const char *tag, UChar *value, int32_t len, const struct UString* comment, UErrorCode *status) {
    return stringbase_open(bundle, tag, URES_ALIAS, value, len, comment, status);
}


struct SResource* intvector_open(struct SRBRoot *bundle, const char *tag, const struct UString* comment, UErrorCode *status) {
    struct SResource *res = res_open(bundle, tag, comment, status);
    if (U_FAILURE(*status)) {
        return NULL;
    }
    res->fType = URES_INT_VECTOR;

    res->u.fIntVector.fCount = 0;
    res->u.fIntVector.fArray = (uint32_t *) uprv_malloc(sizeof(uint32_t) * RESLIST_MAX_INT_VECTOR);
    if (res->u.fIntVector.fArray == NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        uprv_free(res);
        return NULL;
    }
    return res;
}

struct SResource *int_open(struct SRBRoot *bundle, const char *tag, int32_t value, const struct UString* comment, UErrorCode *status) {
    struct SResource *res = res_open(bundle, tag, comment, status);
    if (U_FAILURE(*status)) {
        return NULL;
    }
    res->fType = URES_INT;
    res->u.fIntValue.fValue = value;
    res->fRes = URES_MAKE_RESOURCE(URES_INT, value & 0x0FFFFFFF);
    res->fWritten = TRUE;
    return res;
}

struct SResource *bin_open(struct SRBRoot *bundle, const char *tag, uint32_t length, uint8_t *data, const char* fileName, const struct UString* comment, UErrorCode *status) {
    struct SResource *res = res_open(bundle, tag, comment, status);
    if (U_FAILURE(*status)) {
        return NULL;
    }
    res->fType = URES_BINARY;

    res->u.fBinaryValue.fLength = length;
    res->u.fBinaryValue.fFileName = NULL;
    if(fileName!=NULL && uprv_strcmp(fileName, "") !=0){
        res->u.fBinaryValue.fFileName = (char*) uprv_malloc(sizeof(char) * (uprv_strlen(fileName)+1));
        uprv_strcpy(res->u.fBinaryValue.fFileName,fileName);
    }
    if (length > 0) {
        res->u.fBinaryValue.fData   = (uint8_t *) uprv_malloc(sizeof(uint8_t) * length);

        if (res->u.fBinaryValue.fData == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            uprv_free(res);
            return NULL;
        }

        uprv_memcpy(res->u.fBinaryValue.fData, data, length);
    }
    else {
        res->u.fBinaryValue.fData = NULL;
        if (gFormatVersion > 1) {
            res->fRes = URES_MAKE_EMPTY_RESOURCE(URES_BINARY);
            res->fWritten = TRUE;
        }
    }

    return res;
}

struct SRBRoot *bundle_open(const struct UString* comment, UBool isPoolBundle, UErrorCode *status) {
    struct SRBRoot *bundle;

    if (U_FAILURE(*status)) {
        return NULL;
    }

    bundle = (struct SRBRoot *) uprv_malloc(sizeof(struct SRBRoot));
    if (bundle == NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        return 0;
    }
    uprv_memset(bundle, 0, sizeof(struct SRBRoot));

    bundle->fKeys = (char *) uprv_malloc(sizeof(char) * KEY_SPACE_SIZE);
    bundle->fRoot = table_open(bundle, NULL, comment, status);
    if (bundle->fKeys == NULL || bundle->fRoot == NULL || U_FAILURE(*status)) {
        if (U_SUCCESS(*status)) {
            *status = U_MEMORY_ALLOCATION_ERROR;
        }
        bundle_close(bundle, status);
        return NULL;
    }

    bundle->fLocale   = NULL;
    bundle->fKeysCapacity = KEY_SPACE_SIZE;
    
    bundle->fIsPoolBundle = isPoolBundle;
    if (gUsePoolBundle || isPoolBundle) {
        bundle->fIndexLength = URES_INDEX_POOL_CHECKSUM + 1;
    } else if (gFormatVersion >= 2) {
        bundle->fIndexLength = URES_INDEX_16BIT_TOP + 1;
    } else  {
        bundle->fIndexLength = URES_INDEX_ATTRIBUTES + 1;
    }
    bundle->fKeysBottom = (1  + bundle->fIndexLength) * 4;
    uprv_memset(bundle->fKeys, 0, bundle->fKeysBottom);
    bundle->fKeysTop = bundle->fKeysBottom;

    if (gFormatVersion == 1) {
        bundle->fStringsForm = STRINGS_UTF16_V1;
    } else {
        bundle->fStringsForm = STRINGS_UTF16_V2;
    }

    return bundle;
}


static void table_close(struct SResource *table) {
    struct SResource *current = NULL;
    struct SResource *prev    = NULL;

    current = table->u.fTable.fFirst;

    while (current != NULL) {
        prev    = current;
        current = current->fNext;

        res_close(prev);
    }

    table->u.fTable.fFirst = NULL;
}

static void array_close(struct SResource *array) {
    struct SResource *current = NULL;
    struct SResource *prev    = NULL;
    
    if(array==NULL){
        return;
    }
    current = array->u.fArray.fFirst;
    
    while (current != NULL) {
        prev    = current;
        current = current->fNext;

        res_close(prev);
    }
    array->u.fArray.fFirst = NULL;
}

static void string_close(struct SResource *string) {
    if (string->u.fString.fChars != NULL &&
            string->u.fString.fChars != &gEmptyString) {
        uprv_free(string->u.fString.fChars);
        string->u.fString.fChars =NULL;
    }
}

static void alias_close(struct SResource *alias) {
    if (alias->u.fString.fChars != NULL) {
        uprv_free(alias->u.fString.fChars);
        alias->u.fString.fChars =NULL;
    }
}

static void intvector_close(struct SResource *intvector) {
    if (intvector->u.fIntVector.fArray != NULL) {
        uprv_free(intvector->u.fIntVector.fArray);
        intvector->u.fIntVector.fArray =NULL;
    }
}

static void int_close(struct SResource *intres) {
    
}

static void bin_close(struct SResource *binres) {
    if (binres->u.fBinaryValue.fData != NULL) {
        uprv_free(binres->u.fBinaryValue.fData);
        binres->u.fBinaryValue.fData = NULL;
    }
    if (binres->u.fBinaryValue.fFileName != NULL) {
        uprv_free(binres->u.fBinaryValue.fFileName);
        binres->u.fBinaryValue.fFileName = NULL;
    }
}

void res_close(struct SResource *res) {
    if (res != NULL) {
        switch(res->fType) {
        case URES_STRING:
            string_close(res);
            break;
        case URES_ALIAS:
            alias_close(res);
            break;
        case URES_INT_VECTOR:
            intvector_close(res);
            break;
        case URES_BINARY:
            bin_close(res);
            break;
        case URES_INT:
            int_close(res);
            break;
        case URES_ARRAY:
            array_close(res);
            break;
        case URES_TABLE:
            table_close(res);
            break;
        default:
            
            break;
        }

        ustr_deinit(&res->fComment);
        uprv_free(res);
    }
}

void bundle_close(struct SRBRoot *bundle, UErrorCode *status) {
    res_close(bundle->fRoot);
    uprv_free(bundle->fLocale);
    uprv_free(bundle->fKeys);
    uprv_free(bundle->fKeyMap);
    uprv_free(bundle->f16BitUnits);
    uprv_free(bundle);
}


void table_add(struct SResource *table, struct SResource *res, int linenumber, UErrorCode *status) {
    struct SResource *current = NULL;
    struct SResource *prev    = NULL;
    struct SResTable *list;
    const char *resKeyString;

    if (U_FAILURE(*status)) {
        return;
    }
    if (res == &kNoResource) {
        return;
    }

    
    res->line = linenumber;

    
    list = &(table->u.fTable);
    ++(list->fCount);

    
    if (list->fFirst == NULL) {
        list->fFirst = res;
        res->fNext   = NULL;
        return;
    }

    resKeyString = list->fRoot->fKeys + res->fKey;

    current = list->fFirst;

    while (current != NULL) {
        const char *currentKeyString = list->fRoot->fKeys + current->fKey;
        int diff;
        



        if (gFormatVersion == 1 || U_CHARSET_FAMILY == U_ASCII_FAMILY) {
            diff = uprv_strcmp(currentKeyString, resKeyString);
        } else {
            diff = uprv_compareInvCharsAsAscii(currentKeyString, resKeyString);
        }
        if (diff < 0) {
            prev    = current;
            current = current->fNext;
        } else if (diff > 0) {
            
            if (prev == NULL) {
                
                list->fFirst = res;
            } else {
                
                prev->fNext = res;
            }

            res->fNext = current;
            return;
        } else {
            
            error(linenumber, "duplicate key '%s' in table, first appeared at line %d", currentKeyString, current->line);
            *status = U_UNSUPPORTED_ERROR;
            return;
        }
    }

    
    prev->fNext = res;
    res->fNext  = NULL;
}

void array_add(struct SResource *array, struct SResource *res, UErrorCode *status) {
    if (U_FAILURE(*status)) {
        return;
    }

    if (array->u.fArray.fFirst == NULL) {
        array->u.fArray.fFirst = res;
        array->u.fArray.fLast  = res;
    } else {
        array->u.fArray.fLast->fNext = res;
        array->u.fArray.fLast        = res;
    }

    (array->u.fArray.fCount)++;
}

void intvector_add(struct SResource *intvector, int32_t value, UErrorCode *status) {
    if (U_FAILURE(*status)) {
        return;
    }

    *(intvector->u.fIntVector.fArray + intvector->u.fIntVector.fCount) = value;
    intvector->u.fIntVector.fCount++;
}



void bundle_setlocale(struct SRBRoot *bundle, UChar *locale, UErrorCode *status) {

    if(U_FAILURE(*status)) {
        return;
    }

    if (bundle->fLocale!=NULL) {
        uprv_free(bundle->fLocale);
    }

    bundle->fLocale= (char*) uprv_malloc(sizeof(char) * (u_strlen(locale)+1));

    if(bundle->fLocale == NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        return;
    }

    
    u_UCharsToChars(locale, bundle->fLocale, u_strlen(locale)+1);

}

static const char *
getKeyString(const struct SRBRoot *bundle, int32_t key) {
    if (key < 0) {
        return bundle->fPoolBundleKeys + (key & 0x7fffffff);
    } else {
        return bundle->fKeys + key;
    }
}

const char *
res_getKeyString(const struct SRBRoot *bundle, const struct SResource *res, char temp[8]) {
    if (res->fKey == -1) {
        return NULL;
    }
    return getKeyString(bundle, res->fKey);
}

const char *
bundle_getKeyBytes(struct SRBRoot *bundle, int32_t *pLength) {
    *pLength = bundle->fKeysTop - bundle->fKeysBottom;
    return bundle->fKeys + bundle->fKeysBottom;
}

int32_t
bundle_addKeyBytes(struct SRBRoot *bundle, const char *keyBytes, int32_t length, UErrorCode *status) {
    int32_t keypos;

    if (U_FAILURE(*status)) {
        return -1;
    }
    if (length < 0 || (keyBytes == NULL && length != 0)) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return -1;
    }
    if (length == 0) {
        return bundle->fKeysTop;
    }

    keypos = bundle->fKeysTop;
    bundle->fKeysTop += length;
    if (bundle->fKeysTop >= bundle->fKeysCapacity) {
        
        bundle->fKeysCapacity += KEY_SPACE_SIZE;
        bundle->fKeys = uprv_realloc(bundle->fKeys, bundle->fKeysCapacity);
        if(bundle->fKeys == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            return -1;
        }
    }

    uprv_memcpy(bundle->fKeys + keypos, keyBytes, length);

    return keypos;
}

int32_t
bundle_addtag(struct SRBRoot *bundle, const char *tag, UErrorCode *status) {
    int32_t keypos;

    if (U_FAILURE(*status)) {
        return -1;
    }

    if (tag == NULL) {
        
        return -1;
    }

    keypos = bundle_addKeyBytes(bundle, tag, (int32_t)(uprv_strlen(tag) + 1), status);
    if (U_SUCCESS(*status)) {
        ++bundle->fKeysCount;
    }
    return keypos;
}

static int32_t
compareInt32(int32_t lPos, int32_t rPos) {
    



    if (lPos < rPos) {
        return -1;
    } else if (lPos > rPos) {
        return 1;
    } else {
        return 0;
    }
}

static int32_t U_CALLCONV
compareKeySuffixes(const void *context, const void *l, const void *r) {
    const struct SRBRoot *bundle=(const struct SRBRoot *)context;
    int32_t lPos = ((const KeyMapEntry *)l)->oldpos;
    int32_t rPos = ((const KeyMapEntry *)r)->oldpos;
    const char *lStart = getKeyString(bundle, lPos);
    const char *lLimit = lStart;
    const char *rStart = getKeyString(bundle, rPos);
    const char *rLimit = rStart;
    int32_t diff;
    while (*lLimit != 0) { ++lLimit; }
    while (*rLimit != 0) { ++rLimit; }
    
    while (lStart < lLimit && rStart < rLimit) {
        diff = (int32_t)(uint8_t)*--lLimit - (int32_t)(uint8_t)*--rLimit;
        if (diff != 0) {
            return diff;
        }
    }
    
    diff = (int32_t)(rLimit - rStart) - (int32_t)(lLimit - lStart);
    if (diff != 0) {
        return diff;
    }
    
    return compareInt32(lPos, rPos);
}

static int32_t U_CALLCONV
compareKeyNewpos(const void *context, const void *l, const void *r) {
    return compareInt32(((const KeyMapEntry *)l)->newpos, ((const KeyMapEntry *)r)->newpos);
}

static int32_t U_CALLCONV
compareKeyOldpos(const void *context, const void *l, const void *r) {
    return compareInt32(((const KeyMapEntry *)l)->oldpos, ((const KeyMapEntry *)r)->oldpos);
}

void
bundle_compactKeys(struct SRBRoot *bundle, UErrorCode *status) {
    KeyMapEntry *map;
    char *keys;
    int32_t i;
    int32_t keysCount = bundle->fPoolBundleKeysCount + bundle->fKeysCount;
    if (U_FAILURE(*status) || bundle->fKeysCount == 0 || bundle->fKeyMap != NULL) {
        return;
    }
    map = (KeyMapEntry *)uprv_malloc(keysCount * sizeof(KeyMapEntry));
    if (map == NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
    keys = (char *)bundle->fPoolBundleKeys;
    for (i = 0; i < bundle->fPoolBundleKeysCount; ++i) {
        map[i].oldpos =
            (int32_t)(keys - bundle->fPoolBundleKeys) | 0x80000000;  
        map[i].newpos = 0;
        while (*keys != 0) { ++keys; }  
        ++keys;  
    }
    keys = bundle->fKeys + bundle->fKeysBottom;
    for (; i < keysCount; ++i) {
        map[i].oldpos = (int32_t)(keys - bundle->fKeys);
        map[i].newpos = 0;
        while (*keys != 0) { ++keys; }  
        ++keys;  
    }
    
    uprv_sortArray(map, keysCount, (int32_t)sizeof(KeyMapEntry),
                   compareKeySuffixes, bundle, FALSE, status);
    



    if (U_SUCCESS(*status)) {
        keys = bundle->fKeys;
        for (i = 0; i < keysCount;) {
            




            const char *key;
            const char *keyLimit;
            int32_t j = i + 1;
            map[i].newpos = map[i].oldpos;
            if (j < keysCount && map[j].oldpos < 0) {
                
                i = j;
                continue;
            }
            key = getKeyString(bundle, map[i].oldpos);
            for (keyLimit = key; *keyLimit != 0; ++keyLimit) {}
            for (; j < keysCount && map[j].oldpos >= 0; ++j) {
                const char *k;
                char *suffix;
                const char *suffixLimit;
                int32_t offset;
                suffix = keys + map[j].oldpos;
                for (suffixLimit = suffix; *suffixLimit != 0; ++suffixLimit) {}
                offset = (int32_t)(keyLimit - key) - (suffixLimit - suffix);
                if (offset < 0) {
                    break;  
                }
                
                for (k = keyLimit; suffix < suffixLimit && *--k == *--suffixLimit;) {}
                if (suffix == suffixLimit && *k == *suffixLimit) {
                    map[j].newpos = map[i].oldpos + offset;  
                    
                    while (*suffix != 0) { *suffix++ = 1; }
                    *suffix = 1;
                } else {
                    break;  
                }
            }
            i = j;
        }
        



        uprv_sortArray(map, keysCount, (int32_t)sizeof(KeyMapEntry),
                       compareKeyNewpos, NULL, FALSE, status);
        if (U_SUCCESS(*status)) {
            int32_t oldpos, newpos, limit;
            oldpos = newpos = bundle->fKeysBottom;
            limit = bundle->fKeysTop;
            
            for (i = 0; i < keysCount && map[i].newpos < 0; ++i) {}
            if (i < keysCount) {
                while (oldpos < limit) {
                    if (keys[oldpos] == 1) {
                        ++oldpos;  
                    } else {
                        
                        while (i < keysCount && map[i].newpos == oldpos) {
                            map[i++].newpos = newpos;
                        }
                        
                        keys[newpos++] = keys[oldpos++];
                    }
                }
                assert(i == keysCount);
            }
            bundle->fKeysTop = newpos;
            
            uprv_sortArray(map, keysCount, (int32_t)sizeof(KeyMapEntry),
                           compareKeyOldpos, NULL, FALSE, status);
            if (U_SUCCESS(*status)) {
                
                bundle->fKeyMap = map;
                map = NULL;
            }
        }
    }
    uprv_free(map);
}

static int32_t U_CALLCONV
compareStringSuffixes(const void *context, const void *l, const void *r) {
    struct SResource *left = *((struct SResource **)l);
    struct SResource *right = *((struct SResource **)r);
    const UChar *lStart = left->u.fString.fChars;
    const UChar *lLimit = lStart + left->u.fString.fLength;
    const UChar *rStart = right->u.fString.fChars;
    const UChar *rLimit = rStart + right->u.fString.fLength;
    int32_t diff;
    
    while (lStart < lLimit && rStart < rLimit) {
        diff = (int32_t)*--lLimit - (int32_t)*--rLimit;
        if (diff != 0) {
            return diff;
        }
    }
    
    return right->u.fString.fLength - left->u.fString.fLength;
}

static int32_t U_CALLCONV
compareStringLengths(const void *context, const void *l, const void *r) {
    struct SResource *left = *((struct SResource **)l);
    struct SResource *right = *((struct SResource **)r);
    int32_t diff;
    
    diff = (int)(left->u.fString.fSame != NULL) - (int)(right->u.fString.fSame != NULL);
    if (diff != 0) {
        return diff;
    }
    
    return left->u.fString.fLength - right->u.fString.fLength;
}

static int32_t
string_writeUTF16v2(struct SRBRoot *bundle, struct SResource *res, int32_t utf16Length) {
    int32_t length = res->u.fString.fLength;
    res->fRes = URES_MAKE_RESOURCE(URES_STRING_V2, utf16Length);
    res->fWritten = TRUE;
    switch(res->u.fString.fNumCharsForLength) {
    case 0:
        break;
    case 1:
        bundle->f16BitUnits[utf16Length++] = (uint16_t)(0xdc00 + length);
        break;
    case 2:
        bundle->f16BitUnits[utf16Length] = (uint16_t)(0xdfef + (length >> 16));
        bundle->f16BitUnits[utf16Length + 1] = (uint16_t)length;
        utf16Length += 2;
        break;
    case 3:
        bundle->f16BitUnits[utf16Length] = 0xdfff;
        bundle->f16BitUnits[utf16Length + 1] = (uint16_t)(length >> 16);
        bundle->f16BitUnits[utf16Length + 2] = (uint16_t)length;
        utf16Length += 3;
        break;
    default:
        break;  
    }
    u_memcpy(bundle->f16BitUnits + utf16Length, res->u.fString.fChars, length + 1);
    return utf16Length + length + 1;
}

static void
bundle_compactStrings(struct SRBRoot *bundle, UErrorCode *status) {
    UHashtable *stringSet;
    if (gFormatVersion > 1) {
        stringSet = uhash_open(string_hash, string_comp, string_comp, status);
        res_preflightStrings(bundle, bundle->fRoot, stringSet, status);
    } else {
        stringSet = NULL;
    }
    if (U_FAILURE(*status)) {
        uhash_close(stringSet);
        return;
    }
    switch(bundle->fStringsForm) {
    case STRINGS_UTF16_V2:
        if (bundle->f16BitUnitsLength > 0) {
            struct SResource **array;
            int32_t count = uhash_count(stringSet);
            int32_t i, pos;
            




            int32_t utf16Length = (bundle->f16BitUnitsLength + 20000) & ~1;
            bundle->f16BitUnits = (UChar *)uprv_malloc(utf16Length * U_SIZEOF_UCHAR);
            array = (struct SResource **)uprv_malloc(count * sizeof(struct SResource **));
            if (bundle->f16BitUnits == NULL || array == NULL) {
                uprv_free(bundle->f16BitUnits);
                bundle->f16BitUnits = NULL;
                uprv_free(array);
                uhash_close(stringSet);
                *status = U_MEMORY_ALLOCATION_ERROR;
                return;
            }
            bundle->f16BitUnitsCapacity = utf16Length;
            
            bundle->f16BitUnits[0] = 0;
            utf16Length = 1;
            ++bundle->f16BitUnitsLength;
            for (pos = UHASH_FIRST, i = 0; i < count; ++i) {
                array[i] = (struct SResource *)uhash_nextElement(stringSet, &pos)->key.pointer;
            }
            
            uprv_sortArray(array, count, (int32_t)sizeof(struct SResource **),
                           compareStringSuffixes, NULL, FALSE, status);
            




            if (U_SUCCESS(*status)) {
                for (i = 0; i < count;) {
                    




                    struct SResource *res = array[i];
                    const UChar *strLimit = res->u.fString.fChars + res->u.fString.fLength;
                    int32_t j;
                    for (j = i + 1; j < count; ++j) {
                        struct SResource *suffixRes = array[j];
                        const UChar *s;
                        const UChar *suffix = suffixRes->u.fString.fChars;
                        const UChar *suffixLimit = suffix + suffixRes->u.fString.fLength;
                        int32_t offset = res->u.fString.fLength - suffixRes->u.fString.fLength;
                        if (offset < 0) {
                            break;  
                        }
                        
                        for (s = strLimit; suffix < suffixLimit && *--s == *--suffixLimit;) {}
                        if (suffix == suffixLimit && *s == *suffixLimit) {
                            if (suffixRes->u.fString.fNumCharsForLength == 0) {
                                
                                suffixRes->u.fString.fSame = res;
                                suffixRes->u.fString.fSuffixOffset = offset;
                            } else {
                                
                            }
                        } else {
                            break;  
                        }
                    }
                    i = j;
                }
            }
            




            uprv_sortArray(array, count, (int32_t)sizeof(struct SResource **),
                           compareStringLengths, NULL, FALSE, status);
            if (U_SUCCESS(*status)) {
                
                for (i = 0; i < count && array[i]->u.fString.fSame == NULL; ++i) {
                    utf16Length = string_writeUTF16v2(bundle, array[i], utf16Length);
                }
                
                for (; i < count; ++i) {
                    struct SResource *res = array[i];
                    struct SResource *same = res->u.fString.fSame;
                    res->fRes = same->fRes + same->u.fString.fNumCharsForLength + res->u.fString.fSuffixOffset;
                    res->u.fString.fSame = NULL;
                    res->fWritten = TRUE;
                }
            }
            assert(utf16Length <= bundle->f16BitUnitsLength);
            bundle->f16BitUnitsLength = utf16Length;
            uprv_free(array);
        }
        break;
    default:
        break;
    }
    uhash_close(stringSet);
}
