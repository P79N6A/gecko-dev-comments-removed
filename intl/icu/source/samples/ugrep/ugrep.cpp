




















#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "unicode/utypes.h"
#include "unicode/ustring.h"
#include "unicode/regex.h"
#include "unicode/ucnv.h"
#include "unicode/uclean.h"





const char *pattern = NULL;     
int        firstFileNum;        
UBool      displayFileName = FALSE;
UBool      displayLineNum  = FALSE;





const char *fileName;      
int         fileLen;              

UChar      *ucharBuf = 0;         
                                  

char       *charBuf = 0;          





int      lineStart;     
int      lineEnd;       
int      lineNum;





UConverter  *outConverter = 0;




void processOptions(int argc, const char **argv);
void nextLine(int start);
void printMatch();
void printUsage();
void readFile(const char *name);













int main(int argc, const char** argv) {
    UBool     matchFound = FALSE;

    
    
    
    processOptions(argc, argv);

    
    
    
    UErrorCode status = U_ZERO_ERROR;   
                                        

    UParseError    parseErr;            
                                        
                                        

    RegexPattern  *rePat = RegexPattern::compile(pattern, parseErr, status);
                                        
                                        
                                        
    if (U_FAILURE(status)) {
        fprintf(stderr, "ugrep:  error in pattern: \"%s\" at position %d\n",
            u_errorName(status), parseErr.offset);
        exit(-1);
    }

    
    
    
    UnicodeString empty;
    RegexMatcher *matcher = rePat->matcher(empty, status);
    if (U_FAILURE(status)) {
        fprintf(stderr, "ugrep:  error in creating RegexMatcher: \"%s\"\n",
            u_errorName(status));
        exit(-1);
    }

    
    
    
    for (int fileNum=firstFileNum; fileNum < argc; fileNum++) {
        readFile(argv[fileNum]);

        
        
        
        for (nextLine(0); lineStart<fileLen; nextLine(lineEnd)) {
            UnicodeString s(FALSE, ucharBuf+lineStart, lineEnd-lineStart);
            matcher->reset(s);
            if (matcher->find()) {
                matchFound = TRUE;
                printMatch();
            }
        }
    }

    
    
    
    delete matcher;
    delete rePat;
    free(ucharBuf);
    free(charBuf);
    ucnv_close(outConverter);
    
    u_cleanup();       

    return matchFound? 0: 1;
}












void processOptions(int argc, const char **argv) {
    int            optInd;
    UBool          doUsage   = FALSE;
    UBool          doVersion = FALSE;
    const char    *arg;


    for(optInd = 1; optInd < argc; ++optInd) {
        arg = argv[optInd];
        
        
        if(strcmp(arg, "-V") == 0 || strcmp(arg, "--version") == 0) {
            doVersion = TRUE;
        }
        
        else if(strcmp(arg, "--help") == 0) {
            doUsage = TRUE;
        }
        else if(strcmp(arg, "-n") == 0 || strcmp(arg, "--line-number") == 0) {
            displayLineNum = TRUE;
        }
        
        else if(strcmp(arg, "--") == 0) {
            
            ++optInd;
            break;
        }
        
        else if(strncmp(arg, "-", strlen("-")) == 0) {
            printf("ugrep: invalid option -- %s\n", arg+1);
            doUsage = TRUE;
        }
        
        else {
            break;
        }
    }

    if (doUsage) {
        printUsage();
        exit(0);
    }

    if (doVersion) {
        printf("ugrep version 0.01\n");
        if (optInd == argc) {
            exit(0);
        }
    }

    int  remainingArgs = argc-optInd;     
    if (remainingArgs < 2) {
        fprintf(stderr, "ugrep:  files or pattern are missing.\n");
        printUsage();
        exit(1);
    }

    if (remainingArgs > 2) {
        
        displayFileName = TRUE;
    }

    pattern      = argv[optInd];
    firstFileNum = optInd+1;
}






void printUsage() {
    printf("ugrep [options] pattern file...\n"
        "     -V or --version     display version information\n"
        "     --help              display this help and exit\n"
        "     --                  stop further option processing\n"
        "-n,  --line-number       Prefix each line of output with the line number within its input file.\n"
        );
    exit(0);
}










void readFile(const char *name) {

    
    
    
    fileName = name;
    fileLen  = 0;      


    
    
    
    FILE *file = fopen(name, "rb");
    if (file == 0 ) {
        fprintf(stderr, "ugrep: Could not open file \"%s\"\n", fileName);
        return;
    }
    fseek(file, 0, SEEK_END);
    int rawFileLen = ftell(file);
    fseek(file, 0, SEEK_SET);
    

    
    
    
    charBuf    = (char *)realloc(charBuf, rawFileLen+1);   
    int t = fread(charBuf, 1, rawFileLen, file);
    if (t != rawFileLen)  {
        fprintf(stderr, "Error reading file \"%s\"\n", fileName);
        fclose(file);
        return;
    }
    charBuf[rawFileLen]=0;
    fclose(file);

    
    
    
    int32_t        signatureLength;
    const char *   charDataStart = charBuf;
    UErrorCode     status        = U_ZERO_ERROR;
    const char*    encoding      = ucnv_detectUnicodeSignature(
                           charDataStart, rawFileLen, &signatureLength, &status);
    if (U_FAILURE(status)) {
        fprintf(stderr, "ugrep: ICU Error \"%s\" from ucnv_detectUnicodeSignature()\n",
            u_errorName(status));
        return;
    }
    if(encoding!=NULL ){
        charDataStart  += signatureLength;
        rawFileLen     -= signatureLength;
    }

    
    
    
    UConverter* conv;
    conv = ucnv_open(encoding, &status);
    if (U_FAILURE(status)) {
        fprintf(stderr, "ugrep: ICU Error \"%s\" from ucnv_open()\n", u_errorName(status));
        return;
    }

    
    
    
    
    uint32_t destCap = ucnv_toUChars(conv,
                       NULL,           
                       0,              
                       charDataStart,
                       rawFileLen,
                       &status);
    if (status != U_BUFFER_OVERFLOW_ERROR) {
        fprintf(stderr, "ugrep: ucnv_toUChars: ICU Error \"%s\"\n", u_errorName(status));
        return;
    };
    
    status = U_ZERO_ERROR;
    ucharBuf = (UChar *)realloc(ucharBuf, (destCap+1) * sizeof(UChar));
    ucnv_toUChars(conv,
        ucharBuf,           
        destCap+1,
        charDataStart,
        rawFileLen,
        &status);
    if (U_FAILURE(status)) {
        fprintf(stderr, "ugrep: ucnv_toUChars: ICU Error \"%s\"\n", u_errorName(status));
        return;
    };
    ucnv_close(conv);
    
    
    
    
    
    fileLen = destCap;
}
    
    
    












void nextLine(int  startPos) {
    if (startPos == 0) {
        lineNum = 0;
    } else {
        lineNum++;
    }
    lineStart = lineEnd = startPos;

    for (;;) {
        if (lineEnd >= fileLen) {
            return;
        }
        UChar c = ucharBuf[lineEnd];
        lineEnd++;
        if (c == 0x0a   ||       
            c == 0x0c   ||       
            c == 0x0d   ||       
            c == 0x85   ||       
            c == 0x2028 ||       
            c == 0x2029)         
        { 
            break;
        }
    }

    
    if (lineEnd < fileLen           &&
        ucharBuf[lineEnd-1] == 0x0d &&
        ucharBuf[lineEnd]   == 0x0a) 
    {
        lineEnd++;
    }
}









void printMatch() {
    char                buf[2000];
    UErrorCode         status       = U_ZERO_ERROR;

    
    if (outConverter == 0) {
        outConverter = ucnv_open(NULL, &status);
        if (U_FAILURE(status)) {
            fprintf(stderr, "ugrep:  Error opening default converter: \"%s\"\n",
                u_errorName(status));
            exit(-1);
        }
    };

    
    
    ucnv_fromUChars(outConverter,
                    buf,                   
                    sizeof(buf),           
                    &ucharBuf[lineStart],   
                    lineEnd-lineStart,     
                    &status);
    buf[sizeof(buf)-1] = 0;                
                                           
                                           
   
    if (displayFileName) {
        printf("%s:", fileName);
    }
    if (displayLineNum) {
        printf("%d:", lineNum);
    }
    printf("%s", buf);
}
    
