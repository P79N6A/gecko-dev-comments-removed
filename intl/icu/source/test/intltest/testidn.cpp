




















#include "unicode/utypes.h"

#if !UCONFIG_NO_IDNA && !UCONFIG_NO_TRANSLITERATION

#define USPREP_TYPE_NAMES_ARRAY

#include "unicode/uchar.h"
#include "unicode/putil.h"
#include "cmemory.h"
#include "cstring.h"
#include "unicode/udata.h"
#include "unicode/utf16.h"
#include "unewdata.h"
#include "uoptions.h"
#include "uparse.h"
#include "utrie.h"
#include "umutex.h"
#include "sprpimpl.h"
#include "testidna.h"
#include "punyref.h"
#include <stdlib.h>

UBool beVerbose=FALSE, haveCopyright=TRUE;




static void
parseMappings(const char *filename, UBool reportError,TestIDNA& test, UErrorCode *pErrorCode);

static void 
compareMapping(uint32_t codepoint, uint32_t* mapping, int32_t mapLength, 
               UStringPrepType option);

static void
compareFlagsForRange(uint32_t start, uint32_t end,UStringPrepType option);

static void
testAllCodepoints(TestIDNA& test);

static TestIDNA* pTestIDNA =NULL;

static const char* fileNames[] = {
                                    "rfc3491.txt"
                                 };
static const UTrie *idnTrie              = NULL;
static const int32_t *indexes            = NULL;
static const uint16_t *mappingData       = NULL;



#define DATA_TYPE "icu"

#define SPREP_DIR "sprep"

extern int
testData(TestIDNA& test) {
    char *basename=NULL;
    UErrorCode errorCode=U_ZERO_ERROR;
    char *saveBasename =NULL;

    LocalUStringPrepProfilePointer profile(usprep_openByType(USPREP_RFC3491_NAMEPREP, &errorCode));
    if(U_FAILURE(errorCode)){
        test.errcheckln(errorCode, "Failed to load IDNA data file. " + UnicodeString(u_errorName(errorCode)));
        return errorCode;
    }
    
    char* filename = (char*) malloc(strlen(IntlTest::pathToDataDirectory())*1024);
    
    const char *srcDir=IntlTest::pathToDataDirectory();

    idnTrie     = &profile->sprepTrie;
    indexes     = profile->indexes;
    mappingData = profile->mappingData;

    
    pTestIDNA = &test;
    
    
    if(uprv_strchr(srcDir,U_FILE_SEP_CHAR) == NULL){
        filename[0] = 0x2E;
        filename[1] = U_FILE_SEP_CHAR;
        uprv_strcpy(filename+2,srcDir);
    }else{
        uprv_strcpy(filename, srcDir);
    }
    basename=filename+uprv_strlen(filename);
    if(basename>filename && *(basename-1)!=U_FILE_SEP_CHAR) {
        *basename++=U_FILE_SEP_CHAR;
    }

    
    basename=filename+uprv_strlen(filename);
    if(basename>filename && *(basename-1)!=U_FILE_SEP_CHAR) {
        *basename++=U_FILE_SEP_CHAR;
    }
    
    
    saveBasename = basename;
    (void)saveBasename;    
    uprv_strcpy(basename,SPREP_DIR);
    basename = basename + uprv_strlen(SPREP_DIR);
    *basename++=U_FILE_SEP_CHAR;
    
    
    uprv_strcpy(basename,fileNames[0]);
    parseMappings(filename,TRUE, test,&errorCode);
    if(U_FAILURE(errorCode)) {
        test.errln( "Could not open file %s for reading \n", filename);
        return errorCode;
    }

    testAllCodepoints(test);

    pTestIDNA = NULL;
    free(filename);
    return errorCode;
}
U_CDECL_BEGIN

static void U_CALLCONV
strprepProfileLineFn(void * ,
              char *fields[][2], int32_t fieldCount,
              UErrorCode *pErrorCode) {
    uint32_t mapping[40];
    char *end, *map;
    uint32_t code;
    int32_t length;
   
    const char* typeName;
    uint32_t rangeStart=0,rangeEnd =0;
    const char *s;

    s = u_skipWhitespace(fields[0][0]);
    if (*s == '@') {
        
        return;
    }

    if(fieldCount != 3){
        *pErrorCode = U_INVALID_FORMAT_ERROR;
        return;
    }

    typeName = fields[2][0];
    map = fields[1][0];
   
    if(uprv_strstr(typeName, usprepTypeNames[USPREP_UNASSIGNED])!=NULL){

        u_parseCodePointRange(s, &rangeStart,&rangeEnd, pErrorCode);

        
        compareFlagsForRange(rangeStart,rangeEnd,USPREP_UNASSIGNED);

    }else if(uprv_strstr(typeName, usprepTypeNames[USPREP_PROHIBITED])!=NULL){

        u_parseCodePointRange(s, &rangeStart,&rangeEnd, pErrorCode);

        
        compareFlagsForRange(rangeStart,rangeEnd,USPREP_PROHIBITED);

    }else if(uprv_strstr(typeName, usprepTypeNames[USPREP_MAP])!=NULL){
        
        code=(uint32_t)uprv_strtoul(s, &end, 16);

        
        length=u_parseCodePoints(map, mapping, sizeof(mapping)/4, pErrorCode);
        
        
        compareMapping(code,mapping, length,USPREP_MAP);

    }else{
        *pErrorCode = U_INVALID_FORMAT_ERROR;
    }

}

U_CDECL_END

static void
parseMappings(const char *filename,UBool reportError, TestIDNA& test, UErrorCode *pErrorCode) {
    char *fields[3][2];

    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return;
    }

    u_parseDelimitedFile(filename, ';', fields, 3, strprepProfileLineFn, (void*)filename, pErrorCode);

    

    if(U_FAILURE(*pErrorCode) && (reportError || *pErrorCode!=U_FILE_ACCESS_ERROR)) {
        test.errln( "testidn error: u_parseDelimitedFile(\"%s\") failed - %s\n", filename, u_errorName(*pErrorCode));
    }
}


static inline UStringPrepType
getValues(uint32_t result, int32_t& value, UBool& isIndex){

    UStringPrepType type;

    if(result == 0){
        




        type = USPREP_TYPE_LIMIT;
        isIndex =FALSE;
        value = 0;
    }else if(result >= _SPREP_TYPE_THRESHOLD){
        type = (UStringPrepType) (result - _SPREP_TYPE_THRESHOLD);
        isIndex =FALSE;
        value = 0;
    }else{
        
        type = USPREP_MAP;
        
        if(result & 0x02){
            isIndex = TRUE;
            value = result  >> 2; 

        }else{
            isIndex = FALSE;
            value = (int16_t)result;
            value =  (value >> 2);

        }
        if((result>>2) == _SPREP_MAX_INDEX_VALUE){
            type = USPREP_DELETE;
            isIndex =FALSE;
            value = 0;
        }
    }
    return type;
}



static void
testAllCodepoints(TestIDNA& test){
    



















    uint32_t i = 0;
    int32_t unassigned      = 0;
    int32_t prohibited      = 0;
    int32_t mappedWithNorm  = 0;
    int32_t mapped          = 0;
    int32_t noValueInTrie   = 0;

    UStringPrepType type;
    int32_t value;
    UBool isIndex = FALSE;

    for(i=0;i<=0x10FFFF;i++){
        uint32_t result = 0;
        UTRIE_GET16(idnTrie,i, result);
        type = getValues(result,value, isIndex);
        if(type != USPREP_TYPE_LIMIT ){
            if(type == USPREP_UNASSIGNED){
                unassigned++;
            }
            if(type == USPREP_PROHIBITED){
                prohibited++;
            }
            if(type == USPREP_MAP){
                mapped++;
            }
        }else{
            noValueInTrie++;
            if(result > 0){
                test.errln("The return value for 0x%06X is wrong. %i\n",i,result);
            }
        }
    }

    test.logln("Number of Unassinged code points : %i \n",unassigned);
    test.logln("Number of Prohibited code points : %i \n",prohibited);
    test.logln("Number of Mapped code points : %i \n",mapped);
    test.logln("Number of Mapped with NFKC code points : %i \n",mappedWithNorm);
    test.logln("Number of code points that have no value in Trie: %i \n",noValueInTrie);

    
}

static void 
compareMapping(uint32_t codepoint, uint32_t* mapping,int32_t mapLength, 
               UStringPrepType type){
    uint32_t result = 0;
    UTRIE_GET16(idnTrie,codepoint, result);

    int32_t length=0;
    UBool isIndex;
    UStringPrepType retType;
    int32_t value, index=0, delta=0;
  
    retType = getValues(result,value,isIndex);


    if(type != retType && retType != USPREP_DELETE){

        pTestIDNA->errln( "Did not get the assigned type for codepoint 0x%08X. Expected: %i Got: %i\n",codepoint, USPREP_MAP, type);

    }

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
            length = mappingData[index++];
        }
    }else{
        delta = value;
        length = (retType == USPREP_DELETE)? 0 :  1;
    }

    int32_t realLength =0;
     
    for(int32_t j=0; j<mapLength; j++){
        if(mapping[j] > 0xFFFF){
            realLength +=2;
        }else{
            realLength++;
        }      
    }

    if(realLength != length){
        pTestIDNA->errln( "Did not get the expected length. Expected: %i Got: %i\n", mapLength, length);
    }
    
    if(isIndex){
        for(int8_t i =0; i< mapLength; i++){
            if(mapping[i] <= 0xFFFF){
                if(mappingData[index+i] != (uint16_t)mapping[i]){
                    pTestIDNA->errln("Did not get the expected result. Expected: 0x%04X Got: 0x%04X \n", mapping[i], mappingData[index+i]);
                }
            }else{
                UChar lead  = U16_LEAD(mapping[i]);
                UChar trail = U16_TRAIL(mapping[i]);
                if(mappingData[index+i] != lead ||
                    mappingData[index+i+1] != trail){
                    pTestIDNA->errln( "Did not get the expected result. Expected: 0x%04X 0x%04X  Got: 0x%04X 0x%04X", lead, trail, mappingData[index+i], mappingData[index+i+1]);
                }
            }
        }
    }else{
        if(retType!=USPREP_DELETE && (codepoint-delta) != (uint16_t)mapping[0]){
            pTestIDNA->errln("Did not get the expected result. Expected: 0x%04X Got: 0x%04X \n", mapping[0],(codepoint-delta));
        }
    }

}

static void
compareFlagsForRange(uint32_t start, uint32_t end,
                     UStringPrepType type){

    uint32_t result =0 ;
    UStringPrepType retType;
    UBool isIndex=FALSE;
    int32_t value=0;


















    while(start < end+1){
        UTRIE_GET16(idnTrie,start, result);
        retType = getValues(result,value,isIndex);
        if(result > _SPREP_TYPE_THRESHOLD){
            if(retType != type){
                pTestIDNA->errln( "FAIL: Did not get the expected type for 0x%06X. Expected: %s Got: %s\n",start,usprepTypeNames[type], usprepTypeNames[retType]);
            }
        }else{
            if(type == USPREP_PROHIBITED && ((result & 0x01) != 0x01)){
                pTestIDNA->errln( "FAIL: Did not get the expected type for 0x%06X. Expected: %s Got: %s\n",start,usprepTypeNames[type], usprepTypeNames[retType]);
            }
        }

        start++;
    }
    
}


#endif 









