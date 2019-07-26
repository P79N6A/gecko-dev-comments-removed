




























#include "unicode/utypes.h"
#include "unicode/unistr.h"
#include "unicode/uclean.h"
#include "unicode/udata.h"
#include "unicode/putil.h"

#include "uoptions.h"
#include "unewdata.h"
#include "ucmndata.h"
#include "uspoof_impl.h"
#include "cmemory.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

U_NAMESPACE_USE

static char *progName;
static UOption options[]={
    UOPTION_HELP_H,             
    UOPTION_HELP_QUESTION_MARK, 
    UOPTION_VERBOSE,            
    { "rules", NULL, NULL, NULL, 'r', UOPT_REQUIRES_ARG, 0 },   
    { "wsrules", NULL, NULL, NULL, 'w', UOPT_REQUIRES_ARG, 0},  
    { "out",   NULL, NULL, NULL, 'o', UOPT_REQUIRES_ARG, 0 },   
    UOPTION_ICUDATADIR,         
    UOPTION_DESTDIR,            
    UOPTION_COPYRIGHT,          
};

void usageAndDie(int retCode) {
        printf("Usage: %s [-v] [-options] -r confusablesRules.txt -w wholeScriptConfusables.txt -o output-file\n", progName);
        printf("\tRead in Unicode confusable character definitions and write out the binary data\n"
            "options:\n"
            "\t-h or -? or --help  this usage text\n"
            "\t-V or --version     show a version message\n"
            "\t-c or --copyright   include a copyright notice\n"
            "\t-v or --verbose     turn on verbose output\n"
            "\t-i or --icudatadir  directory for locating any needed intermediate data files,\n"
            "\t                    followed by path, defaults to %s\n"
            "\t-d or --destdir     destination directory, followed by the path\n",
            u_getDataDirectory());
        exit (retCode);
}


#if UCONFIG_NO_REGULAR_EXPRESSIONS || UCONFIG_NO_NORMALIZATION || UCONFIG_NO_FILE_IO


static UDataInfo dummyDataInfo = {
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




DataHeader dh ={
    {sizeof(DataHeader),           
        0xda,
        0x27},

    {                               
        sizeof(UDataInfo),          
        0,                          
        U_IS_BIG_ENDIAN,
        U_CHARSET_FAMILY,
        U_SIZEOF_UCHAR,
        0,                          

    { 0x43, 0x66, 0x75, 0x20 },     
    { 0xff, 0, 0, 0 },              
                                    
                                    
        { 5, 1, 0, 0 }              
    }};

#endif


static const char *readFile(const char *fileName, int32_t *len);






int  main(int argc, char **argv) {
    UErrorCode  status = U_ZERO_ERROR;
    const char *confFileName;
    const char *confWSFileName;
    const char *outFileName;
    const char *outDir = NULL;
    const char *copyright = NULL;

    
    
    
    
    U_MAIN_INIT_ARGS(argc, argv);
    progName = argv[0];
    argc=u_parseArgs(argc, argv, sizeof(options)/sizeof(options[0]), options);
    if(argc<0) {
        
        fprintf(stderr, "error in command line argument \"%s\"\n", argv[-argc]);
        usageAndDie(U_ILLEGAL_ARGUMENT_ERROR);
    }

    if(options[0].doesOccur || options[1].doesOccur) {
        
        usageAndDie(0);
    }

    if (!(options[3].doesOccur && options[4].doesOccur && options[5].doesOccur)) {
        fprintf(stderr, "confusables file, whole script confusables file and output file must all be specified.\n");
        usageAndDie(U_ILLEGAL_ARGUMENT_ERROR);
    }
    confFileName   = options[3].value;
    confWSFileName = options[4].value;
    outFileName    = options[5].value;

    if (options[6].doesOccur) {
        u_setDataDirectory(options[6].value);
    }

    status = U_ZERO_ERROR;

    
    if(options[7].doesOccur) {
        outDir = options[7].value;
    }
    if (options[8].doesOccur) {
        copyright = U_COPYRIGHT_STRING;
    }

#if UCONFIG_NO_REGULAR_EXPRESSIONS || UCONFIG_NO_NORMALIZATION || UCONFIG_NO_FILE_IO
    
    
    

    UNewDataMemory *pData;
    char msg[1024];

    
    sprintf(msg, "gencfu writes dummy %s because of UCONFIG_NO_REGULAR_EXPRESSIONS and/or UCONFIG_NO_NORMALIZATION and/or UCONFIG_NO_FILE_IO, see uconfig.h", outFileName);
    fprintf(stderr, "%s\n", msg);

    
    pData = udata_create(outDir, NULL, outFileName, &dummyDataInfo, NULL, &status);
    udata_writeBlock(pData, msg, strlen(msg));
    udata_finish(pData, &status);
    return (int)status;

#else
    
    u_init(&status);
    if (U_FAILURE(status)) {
        fprintf(stderr, "%s: can not initialize ICU.  status = %s\n",
            argv[0], u_errorName(status));
        exit(1);
    }
    status = U_ZERO_ERROR;

    

    int32_t      confusablesLen = 0;
    const char  *confusables = readFile(confFileName, &confusablesLen);
    if (confusables == NULL) {
        printf("gencfu: error reading file  \"%s\"\n", confFileName);
        exit(-1);
    }

    int32_t     wsConfusablesLen = 0;
    const char *wsConfsables =  readFile(confWSFileName, &wsConfusablesLen);
    if (wsConfsables == NULL) {
        printf("gencfu: error reading file  \"%s\"\n", confFileName);
        exit(-1);
    }

    
    
    
    
    UParseError parseError;
    parseError.line = 0;
    parseError.offset = 0;
    int32_t errType;
    USpoofChecker *sc = uspoof_openFromSource(confusables, confusablesLen,
                                              wsConfsables, wsConfusablesLen,
                                              &errType, &parseError, &status);
    if (U_FAILURE(status)) {
        const char *errFile = 
            (errType == USPOOF_WHOLE_SCRIPT_CONFUSABLE)? confWSFileName : confFileName;
        fprintf(stderr, "gencfu: uspoof_openFromSource error \"%s\"  at file %s, line %d, column %d\n",
                u_errorName(status), errFile, (int)parseError.line, (int)parseError.offset);
        exit(status);
    };


    
    
    
    uint32_t        outDataSize;
    uint8_t        *outData;
    outDataSize = uspoof_serialize(sc, NULL, 0, &status);
    if (status != U_BUFFER_OVERFLOW_ERROR) {
        fprintf(stderr, "gencfu: uspoof_serialize() returned %s\n", u_errorName(status));
        exit(status);
    }
    status = U_ZERO_ERROR;
    outData = new uint8_t[outDataSize];
    uspoof_serialize(sc, outData, outDataSize, &status);

    
    
    uprv_memcpy(dh.info.formatVersion, 
                reinterpret_cast<SpoofDataHeader *>(outData)->fFormatVersion,
                sizeof(dh.info.formatVersion));

    
    
    
    size_t bytesWritten;
    UNewDataMemory *pData;
    pData = udata_create(outDir, NULL, outFileName, &(dh.info), copyright, &status);
    if(U_FAILURE(status)) {
        fprintf(stderr, "gencfu: Could not open output file \"%s\", \"%s\"\n", 
                         outFileName, u_errorName(status));
        exit(status);
    }


    
    udata_writeBlock(pData, outData, outDataSize);
    
    bytesWritten = udata_finish(pData, &status);
    if(U_FAILURE(status)) {
        fprintf(stderr, "gencfu: Error %d writing the output file\n", status);
        exit(status);
    }
    
    if (bytesWritten != outDataSize) {
        fprintf(stderr, "gencfu: Error writing to output file \"%s\"\n", outFileName);
        exit(-1);
    }

    uspoof_close(sc);
    delete [] outData;
    delete [] confusables;
    delete [] wsConfsables;
    u_cleanup();
    printf("gencfu: tool completed successfully.\n");
    return 0;
#endif   
}


 
 
 
 static const char *readFile(const char *fileName, int32_t *len) {
    char       *result;
    long        fileSize;
    FILE        *file;

    file = fopen(fileName, "rb");
    if( file == 0 ) {
        return NULL;
    }
    fseek(file, 0, SEEK_END);
    fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);
    result = new char[fileSize+10];
    if (result==NULL) {
        fclose(file);
        return NULL;
    }

    long t = fread(result, 1, fileSize, file);
    if (t != fileSize)  {
        delete [] result;
        fclose(file);
        return NULL;
    }
    result[fileSize]=0;
    *len = static_cast<int32_t>(fileSize);
    fclose(file);
    return result;
 }
