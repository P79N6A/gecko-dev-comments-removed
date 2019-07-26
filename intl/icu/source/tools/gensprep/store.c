
















#include <stdio.h>
#include <stdlib.h>
#include "unicode/utypes.h"
#include "cmemory.h"
#include "cstring.h"
#include "filestrm.h"
#include "unicode/udata.h"
#include "unicode/utf16.h"
#include "utrie.h"
#include "unewdata.h"
#include "gensprep.h"
#include "uhash.h"


#define DO_DEBUG_OUT 0



































































































#if UCONFIG_NO_IDNA


static UDataInfo dataInfo = {
    sizeof(UDataInfo),
    0,

    U_IS_BIG_ENDIAN,
    U_CHARSET_FAMILY,
    U_SIZEOF_UCHAR,
    0,

    { 0, 0, 0, 0 },                 
    { 0, 0, 0, 0 },                 
    { 0, 0, 0, 0 }                  
};

#else

static int32_t indexes[_SPREP_INDEX_TOP]={ 0 };

static uint16_t* mappingData= NULL;
static int32_t mappingDataCapacity = 0; 
static int16_t currentIndex = 0; 
static int32_t maxLength = 0;  



static UDataInfo dataInfo={
    sizeof(UDataInfo),
    0,

    U_IS_BIG_ENDIAN,
    U_CHARSET_FAMILY,
    U_SIZEOF_UCHAR,
    0,

    { 0x53, 0x50, 0x52, 0x50 },                 
    { 3, 2, UTRIE_SHIFT, UTRIE_INDEX_SHIFT },   
    { 3, 2, 0, 0 }                              
};
void
setUnicodeVersion(const char *v) {
    UVersionInfo version;
    u_versionFromString(version, v);
    uprv_memcpy(dataInfo.dataVersion, version, 4);
}

void
setUnicodeVersionNC(UVersionInfo version){
    uint32_t univer = version[0] << 24;
    univer += version[1] << 16;
    univer += version[2] << 8;
    univer += version[3];
    indexes[_SPREP_NORM_CORRECTNS_LAST_UNI_VERSION] = univer;
}
static UNewTrie *sprepTrie;

#define MAX_DATA_LENGTH 11500


#define SPREP_DELTA_RANGE_POSITIVE_LIMIT              8191 
#define SPREP_DELTA_RANGE_NEGATIVE_LIMIT              -8192


extern void
init() {

    sprepTrie = (UNewTrie *)uprv_calloc(1, sizeof(UNewTrie));

    
    if(NULL==utrie_open(sprepTrie, NULL, MAX_DATA_LENGTH, 0, 0, FALSE)) {
        fprintf(stderr, "error: failed to initialize tries\n");
        exit(U_MEMORY_ALLOCATION_ERROR);
    }
}

static UHashtable* hashTable = NULL;


typedef struct ValueStruct {
    UChar* mapping;
    int16_t length;
    UStringPrepType type;
} ValueStruct;


static void U_CALLCONV valueDeleter(void* obj){
    ValueStruct* value = (ValueStruct*) obj;
    uprv_free(value->mapping);
    uprv_free(value);
}


static int32_t U_CALLCONV hashEntry(const UHashTok parm) {
    return  parm.integer;
}


static UBool U_CALLCONV compareEntries(const UHashTok p1, const UHashTok p2) {
    return (UBool)(p1.integer != p2.integer);
}


static void 
storeMappingData(){

    int32_t pos = -1;
    const UHashElement* element = NULL;
    ValueStruct* value  = NULL;
    int32_t codepoint = 0;
    int32_t elementCount = 0;
    int32_t writtenElementCount = 0;
    int32_t mappingLength = 1; 
    int32_t oldMappingLength = 0;
    uint16_t trieWord =0;
    int32_t limitIndex = 0;

    if (hashTable == NULL) {
        return;
    }
    elementCount = uhash_count(hashTable);

	
    mappingData = (uint16_t*) uprv_calloc(mappingDataCapacity, U_SIZEOF_UCHAR);

    while(writtenElementCount < elementCount){

        while( (element = uhash_nextElement(hashTable, &pos))!=NULL){
            
            codepoint = element->key.integer;
            value = (ValueStruct*)element->value.pointer;
            
            
            if(oldMappingLength != mappingLength){
                
                if(oldMappingLength <=_SPREP_MAX_INDEX_TOP_LENGTH){
                    indexes[_SPREP_NORM_CORRECTNS_LAST_UNI_VERSION+mappingLength] = currentIndex;
                }
                if(oldMappingLength <= _SPREP_MAX_INDEX_TOP_LENGTH &&
                   mappingLength == _SPREP_MAX_INDEX_TOP_LENGTH +1){
                   
                    limitIndex = currentIndex;
                     
                }
                oldMappingLength = mappingLength;
            }

            if(value->length == mappingLength){
                uint32_t savedTrieWord = 0;
                trieWord = currentIndex << 2;
                
                trieWord += 0x02;
            
                if(trieWord > _SPREP_TYPE_THRESHOLD){
                    fprintf(stderr,"trieWord cannot contain value greater than 0x%04X.\n",_SPREP_TYPE_THRESHOLD);
                    exit(U_ILLEGAL_CHAR_FOUND);
                }
                
                savedTrieWord= utrie_get32(sprepTrie,codepoint,NULL);
                if(savedTrieWord!=0){
                    if((savedTrieWord- _SPREP_TYPE_THRESHOLD) == USPREP_PROHIBITED){
                        
                        trieWord += 0x01;
                    }else{
                        



                        fprintf(stderr,"Type for codepoint \\U%08X already set!.\n", (int)codepoint);
                        exit(U_ILLEGAL_ARGUMENT_ERROR); 
                    } 
                } 
                
                
                if(!utrie_set32(sprepTrie,codepoint,trieWord)){
                    fprintf(stderr,"Could not set the value for code point.\n");
                    exit(U_ILLEGAL_ARGUMENT_ERROR);   
                }

                
                writtenElementCount++;

                
                if(currentIndex+value->length+1 > _SPREP_MAX_INDEX_VALUE){
                    fprintf(stderr, "Too many entries in the mapping table %i. Maximum allowed is %i\n", 
                        currentIndex+value->length, _SPREP_MAX_INDEX_VALUE);
                    exit(U_INDEX_OUTOFBOUNDS_ERROR);
                }

                
                
                if(mappingLength > _SPREP_MAX_INDEX_TOP_LENGTH ){
                     
                     mappingData[currentIndex++] = (uint16_t) mappingLength;
                }
                
                uprv_memmove(mappingData+currentIndex, value->mapping, value->length*U_SIZEOF_UCHAR);
                currentIndex += value->length;
                if (currentIndex > mappingDataCapacity) {
                    
                    fprintf(stderr, "gensprep, fatal error at %s, %d.  Aborting.\n", __FILE__, __LINE__);
                    exit(U_INTERNAL_PROGRAM_ERROR);
                }
            }
        }
        mappingLength++;
        pos = -1;
    }
    
    if(mappingLength <= _SPREP_MAX_INDEX_TOP_LENGTH){
        indexes[_SPREP_NORM_CORRECTNS_LAST_UNI_VERSION+mappingLength] = currentIndex+1;
    }else{
        indexes[_SPREP_FOUR_UCHARS_MAPPING_INDEX_START] = limitIndex;
    }
    
}

extern void setOptions(int32_t options){
    indexes[_SPREP_OPTIONS] = options;
}
extern void
storeMapping(uint32_t codepoint, uint32_t* mapping,int32_t length,
             UStringPrepType type, UErrorCode* status){
    
 
    UChar* map = NULL;
    int16_t adjustedLen=0, i, j;
    uint16_t trieWord = 0;
    ValueStruct *value = NULL;
    uint32_t savedTrieWord = 0;

    
    if(hashTable==NULL){
        hashTable = uhash_open(hashEntry, compareEntries, NULL, status);
        uhash_setValueDeleter(hashTable, valueDeleter);
    }
    
    
    savedTrieWord= utrie_get32(sprepTrie,codepoint,NULL);
    if(savedTrieWord!=0){
        if((savedTrieWord- _SPREP_TYPE_THRESHOLD) == USPREP_PROHIBITED){
            
            trieWord += 0x01;
        }else{
            



            fprintf(stderr,"Type for codepoint \\U%08X already set!.\n", (int)codepoint);
            exit(U_ILLEGAL_ARGUMENT_ERROR); 
        } 
    }

     
    for(i=0; i<length; i++){
        adjustedLen += U16_LENGTH(mapping[i]);
    }

    if(adjustedLen == 0){
        trieWord = (uint16_t)(_SPREP_MAX_INDEX_VALUE << 2);
        
        if(trieWord < _SPREP_TYPE_THRESHOLD){   
            
            if(!utrie_set32(sprepTrie,codepoint,trieWord)){
                fprintf(stderr,"Could not set the value for code point.\n");
                exit(U_ILLEGAL_ARGUMENT_ERROR);   
            }
            
            return;
        }else{
            fprintf(stderr,"trieWord cannot contain value greater than threshold 0x%04X.\n",_SPREP_TYPE_THRESHOLD);
            exit(U_ILLEGAL_CHAR_FOUND);
        }
    }

    if(adjustedLen == 1){
        
        int16_t delta = (int16_t)((int32_t)codepoint - (int16_t) mapping[0]);
        if(delta >= SPREP_DELTA_RANGE_NEGATIVE_LIMIT && delta <= SPREP_DELTA_RANGE_POSITIVE_LIMIT){

            trieWord = delta << 2;


            
            if((trieWord & 0x02) != 0 ){
                fprintf(stderr,"The second bit in the trie word is not zero while storing a delta.\n");
                exit(U_INTERNAL_PROGRAM_ERROR);
            }
            
            if(trieWord < _SPREP_TYPE_THRESHOLD){   
                
                if(!utrie_set32(sprepTrie,codepoint,trieWord)){
                    fprintf(stderr,"Could not set the value for code point.\n");
                    exit(U_ILLEGAL_ARGUMENT_ERROR);   
                }
                
                return;
            }
        }
        



    }

    map = (UChar*) uprv_calloc(adjustedLen + 1, U_SIZEOF_UCHAR);
    
    for (i=0, j=0; i<length; i++) {
        U16_APPEND_UNSAFE(map, j, mapping[i]);
    }
    
    value = (ValueStruct*) uprv_malloc(sizeof(ValueStruct));
    value->mapping = map;
    value->type    = type;
    value->length  = adjustedLen;
    if(value->length > _SPREP_MAX_INDEX_TOP_LENGTH){
        mappingDataCapacity++;
    }
    if(maxLength < value->length){
        maxLength = value->length;
    }
    uhash_iput(hashTable,codepoint,value,status);
    mappingDataCapacity += adjustedLen;

    if(U_FAILURE(*status)){
        fprintf(stderr, "Failed to put entries into the hastable. Error: %s\n", u_errorName(*status));
        exit(*status);
    }
}


extern void
storeRange(uint32_t start, uint32_t end, UStringPrepType type,UErrorCode* status){
    uint16_t trieWord = 0;

    if((int)(_SPREP_TYPE_THRESHOLD + type) > 0xFFFF){
        fprintf(stderr,"trieWord cannot contain value greater than 0xFFFF.\n");
        exit(U_ILLEGAL_CHAR_FOUND);
    }
    trieWord = (_SPREP_TYPE_THRESHOLD + type); 
    if(start == end){
        uint32_t savedTrieWord = utrie_get32(sprepTrie, start, NULL);
        if(savedTrieWord>0){
            if(savedTrieWord < _SPREP_TYPE_THRESHOLD && type == USPREP_PROHIBITED){
                






                
                savedTrieWord += 0x01;

                
                trieWord = (uint16_t)savedTrieWord;

                
                if(trieWord < _SPREP_TYPE_THRESHOLD){   
                    
                    if(!utrie_set32(sprepTrie,start,trieWord)){
                        fprintf(stderr,"Could not set the value for code point.\n");
                        exit(U_ILLEGAL_ARGUMENT_ERROR);   
                    }
                    
                    return;
                }else{
                    fprintf(stderr,"trieWord cannot contain value greater than threshold 0x%04X.\n",_SPREP_TYPE_THRESHOLD);
                    exit(U_ILLEGAL_CHAR_FOUND);
                }
 
            }else if(savedTrieWord != trieWord){
                fprintf(stderr,"Value for codepoint \\U%08X already set!.\n", (int)start);
                exit(U_ILLEGAL_ARGUMENT_ERROR);
            }
            
        }
        if(!utrie_set32(sprepTrie,start,trieWord)){
            fprintf(stderr,"Could not set the value for code point \\U%08X.\n", (int)start);
            exit(U_ILLEGAL_ARGUMENT_ERROR);   
        }
    }else{
        if(!utrie_setRange32(sprepTrie, start, end+1, trieWord, FALSE)){
            fprintf(stderr,"Value for certain codepoint already set.\n");
            exit(U_ILLEGAL_CHAR_FOUND);   
        }
    }

}


static uint32_t U_CALLCONV
getFoldedValue(UNewTrie *trie, UChar32 start, int32_t offset) {
    uint32_t value;
    UChar32 limit=0;
    UBool inBlockZero;

    limit=start+0x400;
    while(start<limit) {
        value=utrie_get32(trie, start, &inBlockZero);
        if(inBlockZero) {
            start+=UTRIE_DATA_BLOCK_LENGTH;
        } else if(value!=0) {
            return (uint32_t)offset;
        } else {
            ++start;
        }
    }
    return 0;

}

#endif 

extern void
generateData(const char *dataDir, const char* bundleName) {
    static uint8_t sprepTrieBlock[100000];

    UNewDataMemory *pData;
    UErrorCode errorCode=U_ZERO_ERROR;
    int32_t size, dataLength;
    char* fileName = (char*) uprv_malloc(uprv_strlen(bundleName) +100);

#if UCONFIG_NO_IDNA

    size=0;

#else

    int32_t sprepTrieSize;

    
    storeMappingData();
    
    sprepTrieSize=utrie_serialize(sprepTrie, sprepTrieBlock, sizeof(sprepTrieBlock), getFoldedValue, TRUE, &errorCode);
    if(U_FAILURE(errorCode)) {
        fprintf(stderr, "error: utrie_serialize(sprep trie) failed, %s\n", u_errorName(errorCode));
        exit(errorCode);
    }
    
    size = sprepTrieSize + mappingDataCapacity*U_SIZEOF_UCHAR + sizeof(indexes);
    if(beVerbose) {
        printf("size of sprep trie              %5u bytes\n", (int)sprepTrieSize);
        printf("size of " U_ICUDATA_NAME "_%s." DATA_TYPE " contents: %ld bytes\n", bundleName,(long)size);
        printf("size of mapping data array %5u bytes\n",(int)mappingDataCapacity * U_SIZEOF_UCHAR);
        printf("Number of code units in mappingData (currentIndex) are: %i \n", currentIndex);
        printf("Maximum length of the mapping string is : %i \n", (int)maxLength);
    }

#endif

    fileName[0]=0;
    uprv_strcat(fileName,bundleName);
    
    pData=udata_create(dataDir, DATA_TYPE, fileName, &dataInfo,
                       haveCopyright ? U_COPYRIGHT_STRING : NULL, &errorCode);
    if(U_FAILURE(errorCode)) {
        fprintf(stderr, "gensprep: unable to create the output file, error %d\n", errorCode);
        exit(errorCode);
    }

#if !UCONFIG_NO_IDNA

    indexes[_SPREP_INDEX_TRIE_SIZE]=sprepTrieSize;
    indexes[_SPREP_INDEX_MAPPING_DATA_SIZE]=mappingDataCapacity*U_SIZEOF_UCHAR;
    
    udata_writeBlock(pData, indexes, sizeof(indexes));
    udata_writeBlock(pData, sprepTrieBlock, sprepTrieSize);
    udata_writeBlock(pData, mappingData, indexes[_SPREP_INDEX_MAPPING_DATA_SIZE]);
    

#endif

    
    dataLength=udata_finish(pData, &errorCode);
    if(U_FAILURE(errorCode)) {
        fprintf(stderr, "gensprep: error %d writing the output file\n", errorCode);
        exit(errorCode);
    }

    if(dataLength!=size) {
        fprintf(stderr, "gensprep error: data length %ld != calculated size %ld\n",
            (long)dataLength, (long)size);
        exit(U_INTERNAL_PROGRAM_ERROR);
    }

#if !UCONFIG_NO_IDNA
    
    if (hashTable != NULL) {
        uhash_close(hashTable);
    }
#endif

    uprv_free(fileName);
}

#if !UCONFIG_NO_IDNA

extern void
cleanUpData(void) {
    uprv_free(mappingData);
    utrie_close(sprepTrie);
    uprv_free(sprepTrie);
}

#endif 









