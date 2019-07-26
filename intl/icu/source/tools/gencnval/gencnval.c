























#include "unicode/utypes.h"
#include "unicode/putil.h"
#include "unicode/ucnv.h" 
#include "ucnv_io.h"
#include "cmemory.h"
#include "cstring.h"
#include "uinvchar.h"
#include "filestrm.h"
#include "unicode/uclean.h"
#include "unewdata.h"
#include "uoptions.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>









#define STRING_STORE_SIZE 0x1FBFE   /* 130046 */
#define TAG_STORE_SIZE      0x400   /* 1024 */





#define MAX_TAG_COUNT 0x3F      /* 63 */
#define MAX_CONV_COUNT UCNV_CONVERTER_INDEX_MASK
#define MAX_ALIAS_COUNT 0xFFFF  /* 65535 */





#define MAX_TC_ALIAS_COUNT 0x1F    /* 31 */

#define MAX_LINE_SIZE 0x7FFF    /* 32767 */
#define MAX_LIST_SIZE 0xFFFF    /* 65535 */

#define DATA_NAME "cnvalias"
#define DATA_TYPE "icu" /* ICU alias table */

#define ALL_TAG_STR "ALL"
#define ALL_TAG_NUM 1
#define EMPTY_TAG_NUM 0


static const UDataInfo dataInfo={
    sizeof(UDataInfo),
    0,

    U_IS_BIG_ENDIAN,
    U_CHARSET_FAMILY,
    sizeof(UChar),
    0,

    {0x43, 0x76, 0x41, 0x6c},     
    {3, 0, 1, 0},                 
    {1, 4, 2, 0}                  
};

typedef struct {
    char *store;
    uint32_t top;
    uint32_t max;
} StringBlock;

static char stringStore[STRING_STORE_SIZE];
static StringBlock stringBlock = { stringStore, 0, STRING_STORE_SIZE };

typedef struct {
    uint16_t    aliasCount;
    uint16_t    *aliases;     
} AliasList;

typedef struct {
    uint16_t converter;     
    uint16_t totalAliasCount;    
} Converter;

static Converter converters[MAX_CONV_COUNT];
static uint16_t converterCount=0;

static char tagStore[TAG_STORE_SIZE];
static StringBlock tagBlock = { tagStore, 0, TAG_STORE_SIZE };

typedef struct {
    uint16_t    tag;        
    uint16_t    totalAliasCount; 
    AliasList   aliasList[MAX_CONV_COUNT];
} Tag;


static Tag tags[MAX_TAG_COUNT];
static uint16_t tagCount = 0;


static uint16_t knownAliases[MAX_ALIAS_COUNT];
static uint16_t knownAliasesCount = 0;



static uint16_t aliasLists[MAX_LIST_SIZE];
static uint16_t aliasListsSize = 0;


static UBool standardTagsUsed = FALSE;
static UBool verbose = FALSE;
static int lineNum = 1;

static UConverterAliasOptions tableOptions = {
    UCNV_IO_STD_NORMALIZED,
    1 
};





const char *path;



static void
parseLine(const char *line);

static void
parseFile(FileStream *in);

static int32_t
chomp(char *line);

static void
addOfficialTaggedStandards(char *line, int32_t lineLen);

static uint16_t
addAlias(const char *alias, uint16_t standard, uint16_t converter, UBool defaultName);

static uint16_t
addConverter(const char *converter);

static char *
allocString(StringBlock *block, const char *s, int32_t length);

static uint16_t
addToKnownAliases(const char *alias);

static int
compareAliases(const void *alias1, const void *alias2);

static uint16_t
getTagNumber(const char *tag, uint16_t tagLen);




static void
writeAliasTable(UNewDataMemory *out);




#define GET_ALIAS_STR(index) (stringStore + ((size_t)(index) << 1))
#define GET_TAG_STR(index) (tagStore + ((size_t)(index) << 1))


#define GET_ALIAS_NUM(str) ((uint16_t)((str - stringStore) >> 1))
#define GET_TAG_NUM(str) ((uint16_t)((str - tagStore) >> 1))

enum
{
    HELP1,
    HELP2,
    VERBOSE,
    COPYRIGHT,
    DESTDIR,
    SOURCEDIR
};

static UOption options[]={
    UOPTION_HELP_H,
    UOPTION_HELP_QUESTION_MARK,
    UOPTION_VERBOSE,
    UOPTION_COPYRIGHT,
    UOPTION_DESTDIR,
    UOPTION_SOURCEDIR
};

extern int
main(int argc, char* argv[]) {
    int i, n;
    char pathBuf[512];
    FileStream *in;
    UNewDataMemory *out;
    UErrorCode errorCode=U_ZERO_ERROR;

    U_MAIN_INIT_ARGS(argc, argv);

    
    options[DESTDIR].value=options[SOURCEDIR].value=u_getDataDirectory();
    argc=u_parseArgs(argc, argv, sizeof(options)/sizeof(options[0]), options);

    
    if(argc<0) {
        fprintf(stderr,
            "error in command line argument \"%s\"\n",
            argv[-argc]);
    }
    if(argc<0 || options[HELP1].doesOccur || options[HELP2].doesOccur) {
        fprintf(stderr,
            "usage: %s [-options] [convrtrs.txt]\n"
            "\tread convrtrs.txt and create " U_ICUDATA_NAME "_" DATA_NAME "." DATA_TYPE "\n"
            "options:\n"
            "\t-h or -? or --help  this usage text\n"
            "\t-v or --verbose     prints out extra information about the alias table\n"
            "\t-c or --copyright   include a copyright notice\n"
            "\t-d or --destdir     destination directory, followed by the path\n"
            "\t-s or --sourcedir   source directory, followed by the path\n",
            argv[0]);
        return argc<0 ? U_ILLEGAL_ARGUMENT_ERROR : U_ZERO_ERROR;
    }

    if(options[VERBOSE].doesOccur) {
        verbose = TRUE;
    }

    if(argc>=2) {
        path=argv[1];
    } else {
        path=options[SOURCEDIR].value;
        if(path!=NULL && *path!=0) {
            char *end;

            uprv_strcpy(pathBuf, path);
            end = uprv_strchr(pathBuf, 0);
            if(*(end-1)!=U_FILE_SEP_CHAR) {
                *(end++)=U_FILE_SEP_CHAR;
            }
            uprv_strcpy(end, "convrtrs.txt");
            path=pathBuf;
        } else {
            path = "convrtrs.txt";
        }
    }

    uprv_memset(stringStore, 0, sizeof(stringStore));
    uprv_memset(tagStore, 0, sizeof(tagStore));
    uprv_memset(converters, 0, sizeof(converters));
    uprv_memset(tags, 0, sizeof(tags));
    uprv_memset(aliasLists, 0, sizeof(aliasLists));
    uprv_memset(knownAliases, 0, sizeof(aliasLists));


    in=T_FileStream_open(path, "r");
    if(in==NULL) {
        fprintf(stderr, "gencnval: unable to open input file %s\n", path);
        exit(U_FILE_ACCESS_ERROR);
    }
    parseFile(in);
    T_FileStream_close(in);

    
    out=udata_create(options[DESTDIR].value, DATA_TYPE, DATA_NAME, &dataInfo,
                     options[COPYRIGHT].doesOccur ? U_COPYRIGHT_STRING : NULL, &errorCode);
    if(U_FAILURE(errorCode)) {
        fprintf(stderr, "gencnval: unable to open output file - error %s\n", u_errorName(errorCode));
        exit(errorCode);
    }

    
    writeAliasTable(out);

    
    udata_finish(out, &errorCode);
    if(U_FAILURE(errorCode)) {
        fprintf(stderr, "gencnval: error finishing output file - %s\n", u_errorName(errorCode));
        exit(errorCode);
    }

    
    for (i = 0; i < MAX_TAG_COUNT; i++) {
        for (n = 0; n < MAX_CONV_COUNT; n++) {
            if (tags[i].aliasList[n].aliases!=NULL) {
                uprv_free(tags[i].aliasList[n].aliases);
            }
        }
    }

    return 0;
}

static void
parseFile(FileStream *in) {
    char line[MAX_LINE_SIZE];
    char lastLine[MAX_LINE_SIZE];
    int32_t lineSize = 0;
    int32_t lastLineSize = 0;
    UBool validParse = TRUE;

    lineNum = 0;

    
    getTagNumber("", 0);
    getTagNumber(ALL_TAG_STR, 3);
    allocString(&stringBlock, "", 0);

    
    while (validParse) {
        validParse = FALSE;

        
        while (T_FileStream_readLine(in, lastLine, MAX_LINE_SIZE) != NULL) {
            lastLineSize = chomp(lastLine);
            if (lineSize == 0 || (lastLineSize > 0 && isspace((int)*lastLine))) {
                uprv_strcpy(line + lineSize, lastLine);
                lineSize += lastLineSize;
            } else if (lineSize > 0) {
                validParse = TRUE;
                break;
            }
            lineNum++;
        }

        if (validParse || lineSize > 0) {
            if (isspace((int)*line)) {
                fprintf(stderr, "%s:%d: error: cannot start an alias with a space\n", path, lineNum-1);
                exit(U_PARSE_ERROR);
            } else if (line[0] == '{') {
                if (!standardTagsUsed && line[lineSize - 1] != '}') {
                    fprintf(stderr, "%s:%d: error: alias needs to start with a converter name\n", path, lineNum);
                    exit(U_PARSE_ERROR);
                }
                addOfficialTaggedStandards(line, lineSize);
                standardTagsUsed = TRUE;
            } else {
                if (standardTagsUsed) {
                    parseLine(line);
                }
                else {
                    fprintf(stderr, "%s:%d: error: alias table needs to start a list of standard tags\n", path, lineNum);
                    exit(U_PARSE_ERROR);
                }
            }
            
            if (lastLineSize > 0) {
                uprv_strcpy(line, lastLine);
                lineSize = lastLineSize;
            }
            else {
                lineSize = 0;
            }
        }
        lineNum++;
    }
}




static int32_t
chomp(char *line) {
    char *s = line;
    char *lastNonSpace = line;
    while(*s!=0) {
        
        if(*s == '\r' || *s == '\n' || *s == '#') {
            *s = 0;
            break;
        }
        if (!isspace((int)*s)) {
            lastNonSpace = s;
        }
        ++s;
    }
    if (lastNonSpace++ > line) {
        *lastNonSpace = 0;
        s = lastNonSpace;
    }
    return (int32_t)(s - line);
}

static void
parseLine(const char *line) {
    uint16_t pos=0, start, limit, length, cnv;
    char *converter, *alias;

    
    





    
    if(line[pos]==0) {
        return;
    }

    
    start=pos;
    while(line[pos]!=0 && !isspace((int)line[pos])) {
        ++pos;
    }
    limit=pos;

    
    length=(uint16_t)(limit-start);
    converter=allocString(&stringBlock, line+start, length);

    
    cnv=addConverter(converter);

    
    pos = start;

    
    for(;;) {

        
        while(line[pos]!=0 && isspace((int)line[pos])) {
            ++pos;
        }

        
        if(line[pos]==0) {
            break;
        }

        
        start=pos;
        while(line[pos]!=0 && line[pos]!='{' && !isspace((int)line[pos])) {
            ++pos;
        }
        limit=pos;

        
        length=(uint16_t)(limit-start);
        if (start == 0) {
            
            alias = converter;
            addAlias(alias, ALL_TAG_NUM, cnv, TRUE);
        }
        else {
            alias=allocString(&stringBlock, line+start, length);
            addAlias(alias, ALL_TAG_NUM, cnv, FALSE);
        }
        addToKnownAliases(alias);

        
        

        
        while (line[pos] && isspace((int)line[pos])) {
            ++pos;
        }

        
        if (line[pos] == '{') {
            ++pos;
            do {
                start = pos;
                while (line[pos] && line[pos] != '}' && !isspace((int)line[pos])) {
                    ++pos;
                }
                limit = pos;

                if (start != limit) {
                    
                    uint16_t tag = getTagNumber(line + start, (uint16_t)(limit - start));
                    addAlias(alias, tag, cnv, (UBool)(line[limit-1] == '*'));
                }

                while (line[pos] && isspace((int)line[pos])) {
                    ++pos;
                }
            } while (line[pos] && line[pos] != '}');

            if (line[pos] == '}') {
                ++pos;
            } else {
                fprintf(stderr, "%s:%d: Unterminated tag list\n", path, lineNum);
                exit(U_UNMATCHED_BRACES);
            }
        } else {
            addAlias(alias, EMPTY_TAG_NUM, cnv, (UBool)(tags[0].aliasList[cnv].aliasCount == 0));
        }
    }
}

static uint16_t
getTagNumber(const char *tag, uint16_t tagLen) {
    char *atag;
    uint16_t t;
    UBool preferredName = ((tagLen > 0) ? (tag[tagLen - 1] == '*') : (FALSE));

    if (tagCount >= MAX_TAG_COUNT) {
        fprintf(stderr, "%s:%d: too many tags\n", path, lineNum);
        exit(U_BUFFER_OVERFLOW_ERROR);
    }

    if (preferredName) {

        tagLen--;
    }

    for (t = 0; t < tagCount; ++t) {
        const char *currTag = GET_TAG_STR(tags[t].tag);
        if (uprv_strlen(currTag) == tagLen && !uprv_strnicmp(currTag, tag, tagLen)) {
            return t;
        }
    }

    
    if (tagCount >= MAX_TAG_COUNT) {
        fprintf(stderr, "%s:%d: error: too many tags\n", path, lineNum);
        exit(U_BUFFER_OVERFLOW_ERROR);
    }

    
    atag = allocString(&tagBlock, tag, tagLen);

    if (standardTagsUsed) {
        fprintf(stderr, "%s:%d: error: Tag \"%s\" is not declared at the beginning of the alias table.\n",
            path, lineNum, atag);
        exit(1);
    }
    else if (tagLen > 0 && strcmp(tag, ALL_TAG_STR) != 0) {
        fprintf(stderr, "%s:%d: warning: Tag \"%s\" was added to the list of standards because it was not declared at beginning of the alias table.\n",
            path, lineNum, atag);
    }

    
    tags[tagCount].tag = GET_TAG_NUM(atag);
    

    return tagCount++;
}







static void
addOfficialTaggedStandards(char *line, int32_t lineLen) {
    char *atag;
    char *endTagExp;
    char *tag;
    static const char WHITESPACE[] = " \t";

    if (tagCount > UCNV_NUM_RESERVED_TAGS) {
        fprintf(stderr, "%s:%d: error: official tags already added\n", path, lineNum);
        exit(U_BUFFER_OVERFLOW_ERROR);
    }
    tag = strchr(line, '{');
    if (tag == NULL) {
        
        fprintf(stderr, "%s:%d: error: Missing start of tag group\n", path, lineNum);
        exit(U_PARSE_ERROR);
    }
    tag++;
    endTagExp = strchr(tag, '}');
    if (endTagExp == NULL) {
        fprintf(stderr, "%s:%d: error: Missing end of tag group\n", path, lineNum);
        exit(U_PARSE_ERROR);
    }
    endTagExp[0] = 0;

    tag = strtok(tag, WHITESPACE);
    while (tag != NULL) {


        
        atag = allocString(&tagBlock, tag, -1);

        
        tags[tagCount++].tag = (uint16_t)((atag - tagStore) >> 1);

        

        
        tag = strtok(NULL, WHITESPACE);
    }
}

static uint16_t
addToKnownAliases(const char *alias) {

    






















    if (knownAliasesCount >= MAX_ALIAS_COUNT) {
        fprintf(stderr, "%s:%d: warning: Too many aliases defined for all converters\n",
            path, lineNum);
        exit(U_BUFFER_OVERFLOW_ERROR);
    }
    
    return knownAliases[knownAliasesCount++] = GET_ALIAS_NUM(alias);
}




static uint16_t
addAlias(const char *alias, uint16_t standard, uint16_t converter, UBool defaultName) {
    uint32_t idx, idx2;
    UBool startEmptyWithoutDefault = FALSE;
    AliasList *aliasList;

    if(standard>=MAX_TAG_COUNT) {
        fprintf(stderr, "%s:%d: error: too many standard tags\n", path, lineNum);
        exit(U_BUFFER_OVERFLOW_ERROR);
    }
    if(converter>=MAX_CONV_COUNT) {
        fprintf(stderr, "%s:%d: error: too many converter names\n", path, lineNum);
        exit(U_BUFFER_OVERFLOW_ERROR);
    }
    aliasList = &tags[standard].aliasList[converter];

    if (strchr(alias, '}')) {
        fprintf(stderr, "%s:%d: error: unmatched } found\n", path, 
            lineNum);
    }

    if(aliasList->aliasCount + 1 >= MAX_TC_ALIAS_COUNT) {
        fprintf(stderr, "%s:%d: error: too many aliases for alias %s and converter %s\n", path, 
            lineNum, alias, GET_ALIAS_STR(converters[converter].converter));
        exit(U_BUFFER_OVERFLOW_ERROR);
    }

    
    if (standard == ALL_TAG_NUM && GET_ALIAS_STR(converters[converter].converter) != alias) {
        


        if (uprv_strchr(alias, UCNV_OPTION_SEP_CHAR) != 0)
        {
            fprintf(stderr, "warning(line %d): alias %s contains a \""UCNV_OPTION_SEP_STRING"\". Options are parsed at run-time and do not need to be in the alias table.\n",
                lineNum, alias);
        }
        if (uprv_strchr(alias, UCNV_VALUE_SEP_CHAR) != 0)
        {
            fprintf(stderr, "warning(line %d): alias %s contains an \""UCNV_VALUE_SEP_STRING"\". Options are parsed at run-time and do not need to be in the alias table.\n",
                lineNum, alias);
        }
    }

    if (standard != ALL_TAG_NUM) {
        
        for (idx = 0; idx < converterCount; idx++) {
            for (idx2 = 0; idx2 < tags[standard].aliasList[idx].aliasCount; idx2++) {
                uint16_t aliasNum = tags[standard].aliasList[idx].aliases[idx2];
                if (aliasNum
                    && ucnv_compareNames(alias, GET_ALIAS_STR(aliasNum)) == 0)
                {
                    if (idx == converter) {
                        




                        if (verbose || 0 == uprv_strcmp(alias, GET_ALIAS_STR(aliasNum))) {
                            fprintf(stderr, "%s:%d: warning: duplicate aliases %s and %s found for standard %s and converter %s\n", path, 
                                lineNum, alias, GET_ALIAS_STR(aliasNum),
                                GET_TAG_STR(tags[standard].tag),
                                GET_ALIAS_STR(converters[converter].converter));
                        }
                    } else {
                        fprintf(stderr, "%s:%d: warning: duplicate aliases %s and %s found for standard tag %s between converter %s and converter %s\n", path, 
                            lineNum, alias, GET_ALIAS_STR(aliasNum),
                            GET_TAG_STR(tags[standard].tag),
                            GET_ALIAS_STR(converters[converter].converter),
                            GET_ALIAS_STR(converters[idx].converter));
                    }
                    break;
                }
            }
        }

        
        














    }

    if (aliasList->aliasCount <= 0) {
        aliasList->aliasCount++;
        startEmptyWithoutDefault = TRUE;
    }
    aliasList->aliases = (uint16_t *)uprv_realloc(aliasList->aliases, (aliasList->aliasCount + 1) * sizeof(aliasList->aliases[0]));
    if (startEmptyWithoutDefault) {
        aliasList->aliases[0] = 0;
    }
    if (defaultName) {
        if (aliasList->aliases[0] != 0) {
            fprintf(stderr, "%s:%d: error: Alias %s and %s cannot both be the default alias for standard tag %s and converter %s\n", path, 
                lineNum,
                alias,
                GET_ALIAS_STR(aliasList->aliases[0]),
                GET_TAG_STR(tags[standard].tag),
                GET_ALIAS_STR(converters[converter].converter));
            exit(U_PARSE_ERROR);
        }
        aliasList->aliases[0] = GET_ALIAS_NUM(alias);
    } else {
        aliasList->aliases[aliasList->aliasCount++] = GET_ALIAS_NUM(alias);
    }


    converters[converter].totalAliasCount++; 
    tags[standard].totalAliasCount++; 

    return aliasList->aliasCount;
}

static uint16_t
addConverter(const char *converter) {
    uint32_t idx;
    if(converterCount>=MAX_CONV_COUNT) {
        fprintf(stderr, "%s:%d: error: too many converters\n", path, lineNum);
        exit(U_BUFFER_OVERFLOW_ERROR);
    }

    for (idx = 0; idx < converterCount; idx++) {
        if (ucnv_compareNames(converter, GET_ALIAS_STR(converters[idx].converter)) == 0) {
            fprintf(stderr, "%s:%d: error: duplicate converter %s found!\n", path, lineNum, converter);
            exit(U_PARSE_ERROR);
            break;
        }
    }

    converters[converterCount].converter = GET_ALIAS_NUM(converter);
    converters[converterCount].totalAliasCount = 0;

    return converterCount++;
}


static void
resolveAliasToConverter(uint16_t alias, uint16_t *tagNum, uint16_t *converterNum) {
    uint16_t idx, idx2, idx3;

    for (idx = UCNV_NUM_RESERVED_TAGS; idx < tagCount; idx++) {
        for (idx2 = 0; idx2 < converterCount; idx2++) {
            for (idx3 = 0; idx3 < tags[idx].aliasList[idx2].aliasCount; idx3++) {
                uint16_t aliasNum = tags[idx].aliasList[idx2].aliases[idx3];
                if (aliasNum == alias) {
                    *tagNum = idx;
                    *converterNum = idx2;
                    return;
                }
            }
        }
    }
    
    
    idx = 0;
    for (idx2 = 0; idx2 < converterCount; idx2++) {
        for (idx3 = 0; idx3 < tags[idx].aliasList[idx2].aliasCount; idx3++) {
            uint16_t aliasNum = tags[idx].aliasList[idx2].aliases[idx3];
            if (aliasNum == alias) {
                *tagNum = idx;
                *converterNum = idx2;
                return;
            }
        }
    }
    *tagNum = UINT16_MAX;
    *converterNum = UINT16_MAX;
    fprintf(stderr, "%s: warning: alias %s not found\n",
        path,
        GET_ALIAS_STR(alias));
    return;
}


static uint32_t
resolveAliases(uint16_t *uniqueAliasArr, uint16_t *uniqueAliasToConverterArr, uint16_t aliasOffset) {
    uint32_t uniqueAliasIdx = 0;
    uint32_t idx;
    uint16_t currTagNum, oldTagNum;
    uint16_t currConvNum, oldConvNum;
    const char *lastName;

    resolveAliasToConverter(knownAliases[0], &oldTagNum, &currConvNum);
    uniqueAliasToConverterArr[uniqueAliasIdx] = currConvNum;
    oldConvNum = currConvNum;
    uniqueAliasArr[uniqueAliasIdx] = knownAliases[0] + aliasOffset;
    uniqueAliasIdx++;
    lastName = GET_ALIAS_STR(knownAliases[0]);

    for (idx = 1; idx < knownAliasesCount; idx++) {
        resolveAliasToConverter(knownAliases[idx], &currTagNum, &currConvNum);
        if (ucnv_compareNames(lastName, GET_ALIAS_STR(knownAliases[idx])) == 0) {
            
            if ((currTagNum < oldTagNum && currTagNum >= UCNV_NUM_RESERVED_TAGS)
                || oldTagNum == 0) {
                oldTagNum = currTagNum;
                uniqueAliasToConverterArr[uniqueAliasIdx - 1] = currConvNum;
                uniqueAliasArr[uniqueAliasIdx - 1] = knownAliases[idx] + aliasOffset;
                if (verbose) {
                    printf("using %s instead of %s -> %s", 
                        GET_ALIAS_STR(knownAliases[idx]),
                        lastName,
                        GET_ALIAS_STR(converters[currConvNum].converter));
                    if (oldConvNum != currConvNum) {
                        printf(" (alias conflict)");
                    }
                    puts("");
                }
            }
            else {
                
                if (verbose) {
                    printf("folding %s into %s -> %s",
                        GET_ALIAS_STR(knownAliases[idx]),
                        lastName,
                        GET_ALIAS_STR(converters[oldConvNum].converter));
                    if (oldConvNum != currConvNum) {
                        printf(" (alias conflict)");
                    }
                    puts("");
                }
            }
            if (oldConvNum != currConvNum) {
                uniqueAliasToConverterArr[uniqueAliasIdx - 1] |= UCNV_AMBIGUOUS_ALIAS_MAP_BIT;
            }
        }
        else {
            uniqueAliasToConverterArr[uniqueAliasIdx] = currConvNum;
            oldConvNum = currConvNum;
            uniqueAliasArr[uniqueAliasIdx] = knownAliases[idx] + aliasOffset;
            uniqueAliasIdx++;
            lastName = GET_ALIAS_STR(knownAliases[idx]);
            oldTagNum = currTagNum;
            
        }
        if (uprv_strchr(GET_ALIAS_STR(converters[currConvNum].converter), UCNV_OPTION_SEP_CHAR) != NULL) {
            uniqueAliasToConverterArr[uniqueAliasIdx-1] |= UCNV_CONTAINS_OPTION_BIT;
        }
    }
    return uniqueAliasIdx;
}

static void
createOneAliasList(uint16_t *aliasArrLists, uint32_t tag, uint32_t converter, uint16_t offset) {
    uint32_t aliasNum;
    AliasList *aliasList = &tags[tag].aliasList[converter];

    if (aliasList->aliasCount == 0) {
        aliasArrLists[tag*converterCount + converter] = 0;
    }
    else {
        aliasLists[aliasListsSize++] = aliasList->aliasCount;

        
        aliasArrLists[tag*converterCount + converter] = aliasListsSize;




        for (aliasNum = 0; aliasNum < aliasList->aliasCount; aliasNum++) {
            uint16_t value;


            if (aliasList->aliases[aliasNum]) {
                value = aliasList->aliases[aliasNum] + offset;
            } else {
                value = 0;
                if (tag != 0) { 
                    fprintf(stderr, "%s: warning: tag %s does not have a default alias for %s\n",
                            path,
                            GET_TAG_STR(tags[tag].tag),
                            GET_ALIAS_STR(converters[converter].converter));
                }
            }
            aliasLists[aliasListsSize++] = value;
            if (aliasListsSize >= MAX_LIST_SIZE) {
                fprintf(stderr, "%s: error: Too many alias lists\n", path);
                exit(U_BUFFER_OVERFLOW_ERROR);
            }

        }
    }
}

static void
createNormalizedAliasStrings(char *normalizedStrings, const char *origStringBlock, int32_t stringBlockLength) {
    int32_t currStrLen;
    uprv_memcpy(normalizedStrings, origStringBlock, stringBlockLength);
    while ((currStrLen = (int32_t)uprv_strlen(origStringBlock)) < stringBlockLength) {
        int32_t currStrSize = currStrLen + 1;
        if (currStrLen > 0) {
            int32_t normStrLen;
            ucnv_io_stripForCompare(normalizedStrings, origStringBlock);
            normStrLen = uprv_strlen(normalizedStrings);
            if (normStrLen > 0) {
                uprv_memset(normalizedStrings + normStrLen, 0, currStrSize - normStrLen);
            }
        }
        stringBlockLength -= currStrSize;
        normalizedStrings += currStrSize;
        origStringBlock += currStrSize;
    }
}

static void
writeAliasTable(UNewDataMemory *out) {
    uint32_t i, j;
    uint32_t uniqueAliasesSize;
    uint16_t aliasOffset = (uint16_t)(tagBlock.top/sizeof(uint16_t));
    uint16_t *aliasArrLists = (uint16_t *)uprv_malloc(tagCount * converterCount * sizeof(uint16_t));
    uint16_t *uniqueAliases = (uint16_t *)uprv_malloc(knownAliasesCount * sizeof(uint16_t));
    uint16_t *uniqueAliasesToConverter = (uint16_t *)uprv_malloc(knownAliasesCount * sizeof(uint16_t));

    qsort(knownAliases, knownAliasesCount, sizeof(knownAliases[0]), compareAliases);
    uniqueAliasesSize = resolveAliases(uniqueAliases, uniqueAliasesToConverter, aliasOffset);

    
    aliasListsSize = 0;

    
    for (i = 0; i < tagCount; ++i) {
        for (j = 0; j < converterCount; ++j) {
            createOneAliasList(aliasArrLists, i, j, aliasOffset);
        }
    }

    
    if (tableOptions.stringNormalizationType == UCNV_IO_UNNORMALIZED) {
        udata_write32(out, 8);
    }
    else {
        udata_write32(out, 9);
    }

    
    
    udata_write32(out, converterCount);
    udata_write32(out, tagCount);
    udata_write32(out, uniqueAliasesSize);  
    udata_write32(out, uniqueAliasesSize);  
    udata_write32(out, tagCount * converterCount);
    udata_write32(out, aliasListsSize + 1);
    udata_write32(out, sizeof(tableOptions) / sizeof(uint16_t));
    udata_write32(out, (tagBlock.top + stringBlock.top) / sizeof(uint16_t));
    if (tableOptions.stringNormalizationType != UCNV_IO_UNNORMALIZED) {
        udata_write32(out, (tagBlock.top + stringBlock.top) / sizeof(uint16_t));
    }

    
    
    for(i=0; i<converterCount; ++i) {
        udata_write16(out, (uint16_t)(converters[i].converter + aliasOffset));
    }

    
    
    for(i=UCNV_NUM_RESERVED_TAGS; i<tagCount; ++i) {
        udata_write16(out, tags[i].tag);
    }
    
    udata_write16(out, tags[EMPTY_TAG_NUM].tag);
    udata_write16(out, tags[ALL_TAG_NUM].tag);

    
    udata_writeBlock(out, uniqueAliases, uniqueAliasesSize * sizeof(uint16_t));

    
    udata_writeBlock(out, uniqueAliasesToConverter, uniqueAliasesSize * sizeof(uint16_t));

    
    udata_writeBlock(out, (const void *)(aliasArrLists + (2*converterCount)), (((tagCount - 2) * converterCount) * sizeof(uint16_t)));
    
    udata_writeBlock(out, (const void *)aliasArrLists, (2 * converterCount * sizeof(uint16_t)));

    
    udata_write16(out, 0xDEAD);

    
    udata_writeBlock(out, (const void *)aliasLists, aliasListsSize * sizeof(uint16_t));

    
    udata_writeBlock(out, (const void *)&tableOptions, sizeof(tableOptions));

    
    udata_writeString(out, tagBlock.store, tagBlock.top);

    
    udata_writeString(out, stringBlock.store, stringBlock.top);

    
    if (tableOptions.stringNormalizationType != UCNV_IO_UNNORMALIZED) {
        char *normalizedStrings = (char *)uprv_malloc(tagBlock.top + stringBlock.top);
        createNormalizedAliasStrings(normalizedStrings, tagBlock.store, tagBlock.top);
        createNormalizedAliasStrings(normalizedStrings + tagBlock.top, stringBlock.store, stringBlock.top);

        
        udata_writeString(out, normalizedStrings, tagBlock.top + stringBlock.top);
        uprv_free(normalizedStrings);
    }

    uprv_free(uniqueAliasesToConverter);
    uprv_free(uniqueAliases);
    uprv_free(aliasArrLists);
}

static char *
allocString(StringBlock *block, const char *s, int32_t length) {
    uint32_t top;
    char *p;

    if(length<0) {
        length=(int32_t)uprv_strlen(s);
    }

    




    top=block->top + (uint32_t)((length + 1 + 1) & ~1);

    if(top >= block->max) {
        fprintf(stderr, "%s:%d: error: out of memory\n", path, lineNum);
        exit(U_MEMORY_ALLOCATION_ERROR);
    }

    
    p = block->store + block->top;
    uprv_memcpy(p, s, length);
    p[length] = 0; 
    if((length & 1) == 0) {
        p[length + 1] = 0; 
    }

    
    if(!uprv_isInvariantString(p, length)) {
        fprintf(stderr, "%s:%d: error: the name %s contains not just invariant characters\n", path, lineNum, p);
        exit(U_INVALID_TABLE_FORMAT);
    }

    block->top = top;
    return p;
}

static int
compareAliases(const void *alias1, const void *alias2) {
    
    int result = ucnv_compareNames(GET_ALIAS_STR(*(uint16_t*)alias1), GET_ALIAS_STR(*(uint16_t*)alias2));
    if (!result) {
        
        return (int)uprv_strlen(GET_ALIAS_STR(*(uint16_t*)alias1)) - (int)uprv_strlen(GET_ALIAS_STR(*(uint16_t*)alias2));
    }
    return result;
}










