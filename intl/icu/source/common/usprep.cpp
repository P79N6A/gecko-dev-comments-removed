















#include "unicode/utypes.h"

#if !UCONFIG_NO_IDNA

#include "unicode/usprep.h"

#include "unicode/unorm.h"
#include "unicode/ustring.h"
#include "unicode/uchar.h"
#include "unicode/uversion.h"
#include "umutex.h"
#include "cmemory.h"
#include "sprpimpl.h"
#include "ustr_imp.h"
#include "uhash.h"
#include "cstring.h"
#include "udataswp.h"
#include "ucln_cmn.h"
#include "ubidi_props.h"

U_NAMESPACE_USE

U_CDECL_BEGIN




static UHashtable *SHARED_DATA_HASHTABLE = NULL;

static UMutex usprepMutex = U_MUTEX_INITIALIZER;





static UVersionInfo dataVersion={ 0, 0, 0, 0 };


static const char * const PROFILE_NAMES[] = {
    "rfc3491",      
    "rfc3530cs",    
    "rfc3530csci",  
    "rfc3491",      
    "rfc3530mixp",  
    "rfc3491",      
    "rfc3722",      
    "rfc3920node",  
    "rfc3920res",   
    "rfc4011",      
    "rfc4013",      
    "rfc4505",      
    "rfc4518",      
    "rfc4518ci",    
};

static UBool U_CALLCONV
isSPrepAcceptable(void * ,
             const char * , 
             const char * ,
             const UDataInfo *pInfo) {
    if(
        pInfo->size>=20 &&
        pInfo->isBigEndian==U_IS_BIG_ENDIAN &&
        pInfo->charsetFamily==U_CHARSET_FAMILY &&
        pInfo->dataFormat[0]==0x53 &&   
        pInfo->dataFormat[1]==0x50 &&
        pInfo->dataFormat[2]==0x52 &&
        pInfo->dataFormat[3]==0x50 &&
        pInfo->formatVersion[0]==3 &&
        pInfo->formatVersion[2]==UTRIE_SHIFT &&
        pInfo->formatVersion[3]==UTRIE_INDEX_SHIFT
    ) {
        
        uprv_memcpy(dataVersion, pInfo->dataVersion, 4);
        return TRUE;
    } else {
        return FALSE;
    }
}

static int32_t U_CALLCONV
getSPrepFoldingOffset(uint32_t data) {

    return (int32_t)data;

}


static int32_t U_CALLCONV 
hashEntry(const UHashTok parm) {
    UStringPrepKey *b = (UStringPrepKey *)parm.pointer;
    UHashTok namekey, pathkey;
    namekey.pointer = b->name;
    pathkey.pointer = b->path;
    return uhash_hashChars(namekey)+37*uhash_hashChars(pathkey);
}


static UBool U_CALLCONV 
compareEntries(const UHashTok p1, const UHashTok p2) {
    UStringPrepKey *b1 = (UStringPrepKey *)p1.pointer;
    UStringPrepKey *b2 = (UStringPrepKey *)p2.pointer;
    UHashTok name1, name2, path1, path2;
    name1.pointer = b1->name;
    name2.pointer = b2->name;
    path1.pointer = b1->path;
    path2.pointer = b2->path;
    return ((UBool)(uhash_compareChars(name1, name2) & 
        uhash_compareChars(path1, path2)));
}

static void 
usprep_unload(UStringPrepProfile* data){
    udata_close(data->sprepData);
}

static int32_t 
usprep_internal_flushCache(UBool noRefCount){
    UStringPrepProfile *profile = NULL;
    UStringPrepKey  *key  = NULL;
    int32_t pos = -1;
    int32_t deletedNum = 0;
    const UHashElement *e;

    



    umtx_lock(&usprepMutex);
    if (SHARED_DATA_HASHTABLE == NULL) {
        umtx_unlock(&usprepMutex);
        return 0;
    }

    
    while ((e = uhash_nextElement(SHARED_DATA_HASHTABLE, &pos)) != NULL)
    {
        profile = (UStringPrepProfile *) e->value.pointer;
        key  = (UStringPrepKey *) e->key.pointer;

        if ((noRefCount== FALSE && profile->refCount == 0) || 
             noRefCount== TRUE) {
            deletedNum++;
            uhash_removeElement(SHARED_DATA_HASHTABLE, e);

            
            usprep_unload(profile);

            if(key->name != NULL) {
                uprv_free(key->name);
                key->name=NULL;
            }
            if(key->path != NULL) {
                uprv_free(key->path);
                key->path=NULL;
            }
            uprv_free(profile);
            uprv_free(key);
        }
       
    }
    umtx_unlock(&usprepMutex);

    return deletedNum;
}








static UBool U_CALLCONV usprep_cleanup(void){
    if (SHARED_DATA_HASHTABLE != NULL) {
        usprep_internal_flushCache(TRUE);
        if (SHARED_DATA_HASHTABLE != NULL && uhash_count(SHARED_DATA_HASHTABLE) == 0) {
            uhash_close(SHARED_DATA_HASHTABLE);
            SHARED_DATA_HASHTABLE = NULL;
        }
    }

    return (SHARED_DATA_HASHTABLE == NULL);
}
U_CDECL_END



static void 
initCache(UErrorCode *status) {
    UBool makeCache;
    UMTX_CHECK(&usprepMutex, (SHARED_DATA_HASHTABLE ==  NULL), makeCache);
    if(makeCache) {
        UHashtable *newCache = uhash_open(hashEntry, compareEntries, NULL, status);
        if (U_SUCCESS(*status)) {
            umtx_lock(&usprepMutex);
            if(SHARED_DATA_HASHTABLE == NULL) {
                SHARED_DATA_HASHTABLE = newCache;
                ucln_common_registerCleanup(UCLN_COMMON_USPREP, usprep_cleanup);
                newCache = NULL;
            }
            umtx_unlock(&usprepMutex);
        }
        if(newCache != NULL) {
            uhash_close(newCache);
        }
    }
}

static UBool U_CALLCONV
loadData(UStringPrepProfile* profile, 
         const char* path, 
         const char* name, 
         const char* type, 
         UErrorCode* errorCode) {
        
    UTrie _sprepTrie={ 0,0,0,0,0,0,0 };
    UDataMemory *dataMemory;
    const int32_t *p=NULL;
    const uint8_t *pb;
    UVersionInfo normUnicodeVersion;
    int32_t normUniVer, sprepUniVer, normCorrVer;

    if(errorCode==NULL || U_FAILURE(*errorCode)) {
        return 0;
    }

    
    
    dataMemory=udata_openChoice(path, type, name, isSPrepAcceptable, NULL, errorCode);
    if(U_FAILURE(*errorCode)) {
        return FALSE;
    }

    p=(const int32_t *)udata_getMemory(dataMemory);
    pb=(const uint8_t *)(p+_SPREP_INDEX_TOP);
    utrie_unserialize(&_sprepTrie, pb, p[_SPREP_INDEX_TRIE_SIZE], errorCode);
    _sprepTrie.getFoldingOffset=getSPrepFoldingOffset;


    if(U_FAILURE(*errorCode)) {
        udata_close(dataMemory);
        return FALSE;
    }

    
    umtx_lock(&usprepMutex);
    if(profile->sprepData==NULL) {
        profile->sprepData=dataMemory;
        dataMemory=NULL;
        uprv_memcpy(&profile->indexes, p, sizeof(profile->indexes));
        uprv_memcpy(&profile->sprepTrie, &_sprepTrie, sizeof(UTrie));
    } else {
        p=(const int32_t *)udata_getMemory(profile->sprepData);
    }
    umtx_unlock(&usprepMutex);
    
    profile->mappingData=(uint16_t *)((uint8_t *)(p+_SPREP_INDEX_TOP)+profile->indexes[_SPREP_INDEX_TRIE_SIZE]);
    
    u_getUnicodeVersion(normUnicodeVersion);
    normUniVer = (normUnicodeVersion[0] << 24) + (normUnicodeVersion[1] << 16) + 
                 (normUnicodeVersion[2] << 8 ) + (normUnicodeVersion[3]);
    sprepUniVer = (dataVersion[0] << 24) + (dataVersion[1] << 16) + 
                  (dataVersion[2] << 8 ) + (dataVersion[3]);
    normCorrVer = profile->indexes[_SPREP_NORM_CORRECTNS_LAST_UNI_VERSION];
    
    if(U_FAILURE(*errorCode)){
        udata_close(dataMemory);
        return FALSE;
    }
    if( normUniVer < sprepUniVer && 
        normUniVer < normCorrVer && 
        ((profile->indexes[_SPREP_OPTIONS] & _SPREP_NORMALIZATION_ON) > 0) 
      ){
        *errorCode = U_INVALID_FORMAT_ERROR;
        udata_close(dataMemory);
        return FALSE;
    }
    profile->isDataLoaded = TRUE;

    
    if(dataMemory!=NULL) {
        udata_close(dataMemory); 
    }


    return profile->isDataLoaded;
}

static UStringPrepProfile* 
usprep_getProfile(const char* path, 
                  const char* name,
                  UErrorCode *status){

    UStringPrepProfile* profile = NULL;

    initCache(status);

    if(U_FAILURE(*status)){
        return NULL;
    }

    UStringPrepKey stackKey;
    




    stackKey.name = (char*) name;
    stackKey.path = (char*) path;

    
    umtx_lock(&usprepMutex);
    profile = (UStringPrepProfile*) (uhash_get(SHARED_DATA_HASHTABLE,&stackKey));
    if(profile != NULL) {
        profile->refCount++;
    }
    umtx_unlock(&usprepMutex);
    
    if(profile == NULL) {
        
        LocalMemory<UStringPrepProfile> newProfile;
        if(newProfile.allocateInsteadAndReset() == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            return NULL;
        }

        
        if(!loadData(newProfile.getAlias(), path, name, _SPREP_DATA_TYPE, status) || U_FAILURE(*status) ){
            return NULL;
        }

        
        newProfile->doNFKC = (UBool)((newProfile->indexes[_SPREP_OPTIONS] & _SPREP_NORMALIZATION_ON) > 0);
        newProfile->checkBiDi = (UBool)((newProfile->indexes[_SPREP_OPTIONS] & _SPREP_CHECK_BIDI_ON) > 0);

        if(newProfile->checkBiDi) {
            newProfile->bdp = ubidi_getSingleton();
        }

        LocalMemory<UStringPrepKey> key;
        LocalMemory<char> keyName;
        LocalMemory<char> keyPath;
        if( key.allocateInsteadAndReset() == NULL ||
            keyName.allocateInsteadAndCopy(uprv_strlen(name)+1) == NULL ||
            (path != NULL &&
             keyPath.allocateInsteadAndCopy(uprv_strlen(path)+1) == NULL)
         ) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            usprep_unload(newProfile.getAlias());
            return NULL;
        }

        umtx_lock(&usprepMutex);
        
        profile = (UStringPrepProfile*) (uhash_get(SHARED_DATA_HASHTABLE,&stackKey));
        if(profile != NULL) {
            profile->refCount++;
            usprep_unload(newProfile.getAlias());
        }
        else {
            
            key->name = keyName.orphan();
            uprv_strcpy(key->name, name);
            if(path != NULL){
                key->path = keyPath.orphan();
                uprv_strcpy(key->path, path);
            }        
            profile = newProfile.orphan();
    
            
            profile->refCount = 1;
            uhash_put(SHARED_DATA_HASHTABLE, key.orphan(), profile, status);
        }
        umtx_unlock(&usprepMutex);
    }

    return profile;
}

U_CAPI UStringPrepProfile* U_EXPORT2
usprep_open(const char* path, 
            const char* name,
            UErrorCode* status){

    if(status == NULL || U_FAILURE(*status)){
        return NULL;
    }
       
    
    return usprep_getProfile(path,name,status);
}

U_CAPI UStringPrepProfile* U_EXPORT2
usprep_openByType(UStringPrepProfileType type,
				  UErrorCode* status) {
    if(status == NULL || U_FAILURE(*status)){
        return NULL;
    }
    int32_t index = (int32_t)type;
    if (index < 0 || index >= (int32_t)(sizeof(PROFILE_NAMES)/sizeof(PROFILE_NAMES[0]))) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }
    return usprep_open(NULL, PROFILE_NAMES[index], status);
}

U_CAPI void U_EXPORT2
usprep_close(UStringPrepProfile* profile){
    if(profile==NULL){
        return;
    }

    umtx_lock(&usprepMutex);
    
    if(profile->refCount > 0){
        profile->refCount--;
    }
    umtx_unlock(&usprepMutex);
    
}

U_CFUNC void 
uprv_syntaxError(const UChar* rules, 
                 int32_t pos,
                 int32_t rulesLen,
                 UParseError* parseError){
    if(parseError == NULL){
        return;
    }
    parseError->offset = pos;
    parseError->line = 0 ; 
    
    
    int32_t start = (pos < U_PARSE_CONTEXT_LEN)? 0 : (pos - (U_PARSE_CONTEXT_LEN-1));
    int32_t limit = pos;
    
    u_memcpy(parseError->preContext,rules+start,limit-start);
    
    parseError->preContext[limit-start] = 0;
    
    
    start = pos;
    limit = start + (U_PARSE_CONTEXT_LEN-1);
    if (limit > rulesLen) {
        limit = rulesLen;
    }
    if (start < rulesLen) {
        u_memcpy(parseError->postContext,rules+start,limit-start);
    }
    
    parseError->postContext[limit-start]= 0;
}


static inline UStringPrepType
getValues(uint16_t trieWord, int16_t& value, UBool& isIndex){

    UStringPrepType type;
    if(trieWord == 0){
        




        type = USPREP_TYPE_LIMIT;
        isIndex =FALSE;
        value = 0;
    }else if(trieWord >= _SPREP_TYPE_THRESHOLD){
        type = (UStringPrepType) (trieWord - _SPREP_TYPE_THRESHOLD);
        isIndex =FALSE;
        value = 0;
    }else{
        
        type = USPREP_MAP;
        
        if(trieWord & 0x02){
            isIndex = TRUE;
            value = trieWord  >> 2; 
        }else{
            isIndex = FALSE;
            value = (int16_t)trieWord;
            value =  (value >> 2);
        }
 
        if((trieWord>>2) == _SPREP_MAX_INDEX_VALUE){
            type = USPREP_DELETE;
            isIndex =FALSE;
            value = 0;
        }
    }
    return type;
}



static int32_t 
usprep_map(  const UStringPrepProfile* profile, 
             const UChar* src, int32_t srcLength, 
             UChar* dest, int32_t destCapacity,
             int32_t options,
             UParseError* parseError,
             UErrorCode* status ){
    
    uint16_t result;
    int32_t destIndex=0;
    int32_t srcIndex;
    UBool allowUnassigned = (UBool) ((options & USPREP_ALLOW_UNASSIGNED)>0);
    UStringPrepType type;
    int16_t value;
    UBool isIndex;
    const int32_t* indexes = profile->indexes;

    
    

    for(srcIndex=0;srcIndex<srcLength;){
        UChar32 ch;

        U16_NEXT(src,srcIndex,srcLength,ch);
        
        result=0;

        UTRIE_GET16(&profile->sprepTrie,ch,result);
        
        type = getValues(result, value, isIndex);

        
        if(type == USPREP_UNASSIGNED && allowUnassigned == FALSE){

            uprv_syntaxError(src,srcIndex-U16_LENGTH(ch), srcLength,parseError);
            *status = U_STRINGPREP_UNASSIGNED_ERROR;
            return 0;
            
        }else if(type == USPREP_MAP){
            
            int32_t index, length;

            if(isIndex){
                index = value;
                if(index >= indexes[_SPREP_ONE_UCHAR_MAPPING_INDEX_START] &&
                         index < indexes[_SPREP_TWO_UCHARS_MAPPING_INDEX_START]){
                    length = 1;
                }else if(index >= indexes[_SPREP_TWO_UCHARS_MAPPING_INDEX_START] &&
                         index < indexes[_SPREP_THREE_UCHARS_MAPPING_INDEX_START]){
                    length = 2;
                }else if(index >= indexes[_SPREP_THREE_UCHARS_MAPPING_INDEX_START] &&
                         index < indexes[_SPREP_FOUR_UCHARS_MAPPING_INDEX_START]){
                    length = 3;
                }else{
                    length = profile->mappingData[index++];
         
                }

                
                for(int32_t i=0; i< length; i++){
                    if(destIndex < destCapacity  ){
                        dest[destIndex] = profile->mappingData[index+i];
                    }
                    destIndex++; 
                }  
                continue;
            }else{
                
                ch -= value;
            }

        }else if(type==USPREP_DELETE){
             
            continue;
        }
        
        if(ch <= 0xFFFF){
            if(destIndex < destCapacity ){
                dest[destIndex] = (UChar)ch;
            }
            destIndex++;
        }else{
            if(destIndex+1 < destCapacity ){
                dest[destIndex]   = U16_LEAD(ch);
                dest[destIndex+1] = U16_TRAIL(ch);
            }
            destIndex +=2;
        }
       
    }
        
    return u_terminateUChars(dest, destCapacity, destIndex, status);
}


static int32_t 
usprep_normalize(   const UChar* src, int32_t srcLength, 
                    UChar* dest, int32_t destCapacity,
                    UErrorCode* status ){
    return unorm_normalize(
        src, srcLength,
        UNORM_NFKC, UNORM_UNICODE_3_2,
        dest, destCapacity,
        status);
}


 






































#define MAX_STACK_BUFFER_SIZE 300


U_CAPI int32_t U_EXPORT2
usprep_prepare(   const UStringPrepProfile* profile,
                  const UChar* src, int32_t srcLength, 
                  UChar* dest, int32_t destCapacity,
                  int32_t options,
                  UParseError* parseError,
                  UErrorCode* status ){

    
    if(status == NULL || U_FAILURE(*status)){
        return 0;
    }
    
    
    if(profile==NULL || src==NULL || srcLength<-1 || (dest==NULL && destCapacity!=0)) {
        *status=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    UChar b1Stack[MAX_STACK_BUFFER_SIZE], b2Stack[MAX_STACK_BUFFER_SIZE];
    UChar *b1 = b1Stack, *b2 = b2Stack;
    int32_t b1Len, b2Len=0,
            b1Capacity = MAX_STACK_BUFFER_SIZE , 
            b2Capacity = MAX_STACK_BUFFER_SIZE;
    uint16_t result;
    int32_t b2Index = 0;
    UCharDirection direction=U_CHAR_DIRECTION_COUNT, firstCharDir=U_CHAR_DIRECTION_COUNT;
    UBool leftToRight=FALSE, rightToLeft=FALSE;
    int32_t rtlPos =-1, ltrPos =-1;

    
    if(srcLength == -1){
        srcLength = u_strlen(src);
    }
    
    b1Len = usprep_map(profile, src, srcLength, b1, b1Capacity, options, parseError, status);

    if(*status == U_BUFFER_OVERFLOW_ERROR){
        
        
        b1 = (UChar*) uprv_malloc(b1Len * U_SIZEOF_UCHAR);
        if(b1==NULL){
            *status = U_MEMORY_ALLOCATION_ERROR;
            goto CLEANUP;
        }

        *status = U_ZERO_ERROR; 
        
        b1Len = usprep_map(profile, src, srcLength, b1, b1Len, options, parseError, status);
        
    }

    
    if(profile->doNFKC == TRUE){
        b2Len = usprep_normalize(b1,b1Len, b2,b2Capacity,status);
        
        if(*status == U_BUFFER_OVERFLOW_ERROR){
            
            
            b2 = (UChar*) uprv_malloc(b2Len * U_SIZEOF_UCHAR);
            if(b2==NULL){
                *status = U_MEMORY_ALLOCATION_ERROR;
                goto CLEANUP;
            }

            *status = U_ZERO_ERROR; 
        
            b2Len = usprep_normalize(b1,b1Len, b2,b2Len,status);
        
        }

    }else{
        b2 = b1;
        b2Len = b1Len;
    }
    

    if(U_FAILURE(*status)){
        goto CLEANUP;
    }

    UChar32 ch;
    UStringPrepType type;
    int16_t value;
    UBool isIndex;
    
    
    for(b2Index=0; b2Index<b2Len;){
        
        ch = 0;

        U16_NEXT(b2, b2Index, b2Len, ch);

        UTRIE_GET16(&profile->sprepTrie,ch,result);
        
        type = getValues(result, value, isIndex);

        if( type == USPREP_PROHIBITED || 
            ((result < _SPREP_TYPE_THRESHOLD) && (result & 0x01) )
           ){
            *status = U_STRINGPREP_PROHIBITED_ERROR;
            uprv_syntaxError(b1, b2Index-U16_LENGTH(ch), b2Len, parseError);
            goto CLEANUP;
        }

        if(profile->checkBiDi) {
            direction = ubidi_getClass(profile->bdp, ch);
            if(firstCharDir == U_CHAR_DIRECTION_COUNT){
                firstCharDir = direction;
            }
            if(direction == U_LEFT_TO_RIGHT){
                leftToRight = TRUE;
                ltrPos = b2Index-1;
            }
            if(direction == U_RIGHT_TO_LEFT || direction == U_RIGHT_TO_LEFT_ARABIC){
                rightToLeft = TRUE;
                rtlPos = b2Index-1;
            }
        }
    }           
    if(profile->checkBiDi == TRUE){
        
        if( leftToRight == TRUE && rightToLeft == TRUE){
            *status = U_STRINGPREP_CHECK_BIDI_ERROR;
            uprv_syntaxError(b2,(rtlPos>ltrPos) ? rtlPos : ltrPos, b2Len, parseError);
            goto CLEANUP;
        }

        
        if( rightToLeft == TRUE && 
            !((firstCharDir == U_RIGHT_TO_LEFT || firstCharDir == U_RIGHT_TO_LEFT_ARABIC) &&
              (direction == U_RIGHT_TO_LEFT || direction == U_RIGHT_TO_LEFT_ARABIC))
           ){
            *status = U_STRINGPREP_CHECK_BIDI_ERROR;
            uprv_syntaxError(b2, rtlPos, b2Len, parseError);
            return FALSE;
        }
    }
    if(b2Len>0 && b2Len <= destCapacity){
        uprv_memmove(dest,b2, b2Len*U_SIZEOF_UCHAR);
    }

CLEANUP:
    if(b1!=b1Stack){
        uprv_free(b1);
        b1=NULL;
    }

    if(b2!=b1Stack && b2!=b2Stack && b2!=b1 ){
        uprv_free(b2);
        b2=NULL;
    }
    return u_terminateUChars(dest, destCapacity, b2Len, status);
}




U_CAPI int32_t U_EXPORT2
usprep_swap(const UDataSwapper *ds,
            const void *inData, int32_t length, void *outData,
            UErrorCode *pErrorCode) {
    const UDataInfo *pInfo;
    int32_t headerSize;

    const uint8_t *inBytes;
    uint8_t *outBytes;

    const int32_t *inIndexes;
    int32_t indexes[16];

    int32_t i, offset, count, size;

    
    headerSize=udata_swapDataHeader(ds, inData, length, outData, pErrorCode);
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return 0;
    }

    
    pInfo=(const UDataInfo *)((const char *)inData+4);
    if(!(
        pInfo->dataFormat[0]==0x53 &&   
        pInfo->dataFormat[1]==0x50 &&
        pInfo->dataFormat[2]==0x52 &&
        pInfo->dataFormat[3]==0x50 &&
        pInfo->formatVersion[0]==3
    )) {
        udata_printError(ds, "usprep_swap(): data format %02x.%02x.%02x.%02x (format version %02x) is not recognized as StringPrep .spp data\n",
                         pInfo->dataFormat[0], pInfo->dataFormat[1],
                         pInfo->dataFormat[2], pInfo->dataFormat[3],
                         pInfo->formatVersion[0]);
        *pErrorCode=U_UNSUPPORTED_ERROR;
        return 0;
    }

    inBytes=(const uint8_t *)inData+headerSize;
    outBytes=(uint8_t *)outData+headerSize;

    inIndexes=(const int32_t *)inBytes;

    if(length>=0) {
        length-=headerSize;
        if(length<16*4) {
            udata_printError(ds, "usprep_swap(): too few bytes (%d after header) for StringPrep .spp data\n",
                             length);
            *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
            return 0;
        }
    }

    
    for(i=0; i<16; ++i) {
        indexes[i]=udata_readInt32(ds, inIndexes[i]);
    }

    
    size=
        16*4+ 
        indexes[_SPREP_INDEX_TRIE_SIZE]+
        indexes[_SPREP_INDEX_MAPPING_DATA_SIZE];

    if(length>=0) {
        if(length<size) {
            udata_printError(ds, "usprep_swap(): too few bytes (%d after header) for all of StringPrep .spp data\n",
                             length);
            *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
            return 0;
        }

        
        if(inBytes!=outBytes) {
            uprv_memcpy(outBytes, inBytes, size);
        }

        offset=0;

        
        count=16*4;
        ds->swapArray32(ds, inBytes, count, outBytes, pErrorCode);
        offset+=count;

        
        count=indexes[_SPREP_INDEX_TRIE_SIZE];
        utrie_swap(ds, inBytes+offset, count, outBytes+offset, pErrorCode);
        offset+=count;

        
        count=indexes[_SPREP_INDEX_MAPPING_DATA_SIZE];
        ds->swapArray16(ds, inBytes+offset, count, outBytes+offset, pErrorCode);
        offset+=count;
    }

    return headerSize+size;
}

#endif 
