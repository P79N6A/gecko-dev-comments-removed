

















#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "unicode/utypes.h"
#include "unicode/ubidi.h"
#include "unicode/errorcode.h"
#include "unicode/localpointer.h"
#include "unicode/putil.h"
#include "unicode/unistr.h"
#include "intltest.h"
#include "uparse.h"

class BiDiConformanceTest : public IntlTest {
public:
    BiDiConformanceTest() :
        directionBits(0), lineNumber(0), levelsCount(0), orderingCount(0),
        errorCount(0) {}

    void runIndexedTest(int32_t index, UBool exec, const char *&name, char *par=NULL);

    void TestBidiTest();
    void TestBidiCharacterTest();
private:
    UBool parseLevels(const char *&start);
    UBool parseOrdering(const char *start);
    UBool parseInputStringFromBiDiClasses(const char *&start);

    UBool checkLevels(const UBiDiLevel actualLevels[], int32_t actualCount);
    UBool checkOrdering(UBiDi *ubidi);

    void printErrorLine();

    char line[10000];
    UBiDiLevel levels[1000];
    uint32_t directionBits;
    int32_t ordering[1000];
    int32_t lineNumber;
    int32_t levelsCount;
    int32_t orderingCount;
    int32_t errorCount;
    UnicodeString inputString;
    const char *paraLevelName;
    char levelNameString[12];
};

extern IntlTest *createBiDiConformanceTest() {
    return new BiDiConformanceTest();
}

void BiDiConformanceTest::runIndexedTest(int32_t index, UBool exec, const char *&name, char * ) {
    if(exec) {
        logln("TestSuite BiDiConformanceTest: ");
    }
    TESTCASE_AUTO_BEGIN;
    TESTCASE_AUTO(TestBidiTest);
    TESTCASE_AUTO(TestBidiCharacterTest);
    TESTCASE_AUTO_END;
}

U_DEFINE_LOCAL_OPEN_POINTER(LocalStdioFilePointer, FILE, fclose);

UBool BiDiConformanceTest::parseLevels(const char *&start) {
    directionBits=0;
    levelsCount=0;
    while(*start!=0 && *(start=u_skipWhitespace(start))!=0 && *start!=';') {
        if(*start=='x') {
            levels[levelsCount++]=UBIDI_DEFAULT_LTR;
            ++start;
        } else {
            char *end;
            uint32_t value=(uint32_t)strtoul(start, &end, 10);
            if(end<=start || (!U_IS_INV_WHITESPACE(*end) && *end!=0 && *end!=';')
                          || value>(UBIDI_MAX_EXPLICIT_LEVEL+1)) {
                errln("\nError on line %d: Levels parse error at %s", (int)lineNumber, start);
                printErrorLine();
                return FALSE;
            }
            levels[levelsCount++]=(UBiDiLevel)value;
            directionBits|=(1<<(value&1));
            start=end;
        }
    }
    return TRUE;
}

UBool BiDiConformanceTest::parseOrdering(const char *start) {
    orderingCount=0;
    while(*start!=0 && *(start=u_skipWhitespace(start))!=0 && *start!=';') {
        char *end;
        uint32_t value=(uint32_t)strtoul(start, &end, 10);
        if(end<=start || (!U_IS_INV_WHITESPACE(*end) && *end!=0 && *end!=';') || value>=1000) {
            errln("\nError on line %d: Reorder parse error at %s", (int)lineNumber, start);
            printErrorLine();
            return FALSE;
        }
        ordering[orderingCount++]=(int32_t)value;
        start=end;
    }
    return TRUE;
}

static const UChar charFromBiDiClass[U_CHAR_DIRECTION_COUNT]={
    0x6c,   
    0x52,   
    0x33,   
    0x2d,   
    0x25,   
    0x39,   
    0x2c,   
    0x2f,   
    0x5f,   
    0x20,   
    0x3d,   
    0x65,   
    0x6f,   
    0x41,   
    0x45,   
    0x4f,   
    0x2a,   
    0x60,   
    0x7c,   
    
    0x53,   
    0x69,   
    0x49,   
    0x2e    
};

U_CDECL_BEGIN

static UCharDirection U_CALLCONV
biDiConfUBiDiClassCallback(const void * , UChar32 c) {
    for(int i=0; i<U_CHAR_DIRECTION_COUNT; ++i) {
        if(c==charFromBiDiClass[i]) {
            return (UCharDirection)i;
        }
    }
    
    
    return U_BIDI_CLASS_DEFAULT;
}

U_CDECL_END

static const int8_t biDiClassNameLengths[U_CHAR_DIRECTION_COUNT+1]={
    1, 1, 2, 2, 2, 2, 2, 1, 1, 2, 2, 3, 3, 2, 3, 3, 3, 3, 2, 3, 3, 3, 3, 0
};

UBool BiDiConformanceTest::parseInputStringFromBiDiClasses(const char *&start) {
    inputString.remove();
    





    while(*start!=0 && *(start=u_skipWhitespace(start))!=0 && *start!=';') {
        UCharDirection biDiClass=U_CHAR_DIRECTION_COUNT;
        
        
        if(start[0]=='L') {
            if(start[1]=='R') {
                if(start[2]=='E') {
                    biDiClass=U_LEFT_TO_RIGHT_EMBEDDING;
                } else if(start[2]=='I') {
                    biDiClass=U_LEFT_TO_RIGHT_ISOLATE;
                } else if(start[2]=='O') {
                    biDiClass=U_LEFT_TO_RIGHT_OVERRIDE;
                }
            } else {
                biDiClass=U_LEFT_TO_RIGHT;
            }
        } else if(start[0]=='R') {
            if(start[1]=='L') {
                if(start[2]=='E') {
                    biDiClass=U_RIGHT_TO_LEFT_EMBEDDING;
                } else if(start[2]=='I') {
                    biDiClass=U_RIGHT_TO_LEFT_ISOLATE;
                } else if(start[2]=='O') {
                    biDiClass=U_RIGHT_TO_LEFT_OVERRIDE;
                }
            } else {
                biDiClass=U_RIGHT_TO_LEFT;
            }
        } else if(start[0]=='E') {
            if(start[1]=='N') {
                biDiClass=U_EUROPEAN_NUMBER;
            } else if(start[1]=='S') {
                biDiClass=U_EUROPEAN_NUMBER_SEPARATOR;
            } else if(start[1]=='T') {
                biDiClass=U_EUROPEAN_NUMBER_TERMINATOR;
            }
        } else if(start[0]=='A') {
            if(start[1]=='L') {
                biDiClass=U_RIGHT_TO_LEFT_ARABIC;
            } else if(start[1]=='N') {
                biDiClass=U_ARABIC_NUMBER;
            }
        } else if(start[0]=='C' && start[1]=='S') {
            biDiClass=U_COMMON_NUMBER_SEPARATOR;
        } else if(start[0]=='B') {
            if(start[1]=='N') {
                biDiClass=U_BOUNDARY_NEUTRAL;
            } else {
                biDiClass=U_BLOCK_SEPARATOR;
            }
        } else if(start[0]=='S') {
            biDiClass=U_SEGMENT_SEPARATOR;
        } else if(start[0]=='W' && start[1]=='S') {
            biDiClass=U_WHITE_SPACE_NEUTRAL;
        } else if(start[0]=='O' && start[1]=='N') {
            biDiClass=U_OTHER_NEUTRAL;
        } else if(start[0]=='P' && start[1]=='D') {
            if(start[2]=='F') {
                biDiClass=U_POP_DIRECTIONAL_FORMAT;
            } else if(start[2]=='I') {
                biDiClass=U_POP_DIRECTIONAL_ISOLATE;
            }
        } else if(start[0]=='N' && start[1]=='S' && start[2]=='M') {
            biDiClass=U_DIR_NON_SPACING_MARK;
        } else if(start[0]=='F' && start[1]=='S' && start[2]=='I') {
            biDiClass=U_FIRST_STRONG_ISOLATE;
        }
        
        
        int8_t biDiClassNameLength=biDiClassNameLengths[biDiClass];
        char c=start[biDiClassNameLength];
        if(biDiClass<U_CHAR_DIRECTION_COUNT && (U_IS_INV_WHITESPACE(c) || c==';' || c==0)) {
            inputString.append(charFromBiDiClass[biDiClass]);
            start+=biDiClassNameLength;
            continue;
        }
        errln("\nError on line %d: BiDi class string not recognized at %s", (int)lineNumber, start);
        printErrorLine();
        return FALSE;
    }
    return TRUE;
}

void BiDiConformanceTest::TestBidiTest() {
    IcuTestErrorCode errorCode(*this, "TestBidiTest");
    const char *sourceTestDataPath=getSourceTestData(errorCode);
    if(errorCode.logIfFailureAndReset("unable to find the source/test/testdata "
                                      "folder (getSourceTestData())")) {
        return;
    }
    char bidiTestPath[400];
    strcpy(bidiTestPath, sourceTestDataPath);
    strcat(bidiTestPath, "BidiTest.txt");
    LocalStdioFilePointer bidiTestFile(fopen(bidiTestPath, "r"));
    if(bidiTestFile.isNull()) {
        errln("unable to open %s", bidiTestPath);
        return;
    }
    LocalUBiDiPointer ubidi(ubidi_open());
    ubidi_setClassCallback(ubidi.getAlias(), biDiConfUBiDiClassCallback, NULL,
                           NULL, NULL, errorCode);
    if(errorCode.logIfFailureAndReset("ubidi_setClassCallback()")) {
        return;
    }
    lineNumber=0;
    levelsCount=0;
    orderingCount=0;
    errorCount=0;
    
    paraLevelName="N/A";
    while(errorCount<10 && fgets(line, (int)sizeof(line), bidiTestFile.getAlias())!=NULL) {
        ++lineNumber;
        
        char *commentStart=strchr(line, '#');
        if(commentStart!=NULL) {
            *commentStart=0;
        }
        u_rtrim(line);
        const char *start=u_skipWhitespace(line);
        if(*start==0) {
            continue;  
        }
        if(*start=='@') {
            ++start;
            if(0==strncmp(start, "Levels:", 7)) {
                start+=7;
                if(!parseLevels(start)) {
                    return;
                }
            } else if(0==strncmp(start, "Reorder:", 8)) {
                if(!parseOrdering(start+8)) {
                    return;
                }
            }
            
        } else {
            if(!parseInputStringFromBiDiClasses(start)) {
                return;
            }
            start=u_skipWhitespace(start);
            if(*start!=';') {
                errln("missing ; separator on input line %s", line);
                return;
            }
            start=u_skipWhitespace(start+1);
            char *end;
            uint32_t bitset=(uint32_t)strtoul(start, &end, 16);
            if(end<=start || (!U_IS_INV_WHITESPACE(*end) && *end!=';' && *end!=0)) {
                errln("input bitset parse error at %s", start);
                return;
            }
            
            static const UBiDiLevel paraLevels[]={ UBIDI_DEFAULT_LTR, 0, 1, UBIDI_DEFAULT_RTL };
            static const char *const paraLevelNames[]={ "auto/LTR", "LTR", "RTL", "auto/RTL" };
            for(int i=0; i<=3; ++i) {
                if(bitset&(1<<i)) {
                    ubidi_setPara(ubidi.getAlias(), inputString.getBuffer(), inputString.length(),
                                  paraLevels[i], NULL, errorCode);
                    const UBiDiLevel *actualLevels=ubidi_getLevels(ubidi.getAlias(), errorCode);
                    if(errorCode.logIfFailureAndReset("ubidi_setPara() or ubidi_getLevels()")) {
                        errln("Input line %d: %s", (int)lineNumber, line);
                        return;
                    }
                    paraLevelName=paraLevelNames[i];
                    if(!checkLevels(actualLevels, ubidi_getProcessedLength(ubidi.getAlias()))) {
                        
                        
                        break;
                    }
                    if(!checkOrdering(ubidi.getAlias())) {
                        
                        
                        break;
                    }
                }
            }
        }
    }
}









































































void BiDiConformanceTest::TestBidiCharacterTest() {
    IcuTestErrorCode errorCode(*this, "TestBidiCharacterTest");
    const char *sourceTestDataPath=getSourceTestData(errorCode);
    if(errorCode.logIfFailureAndReset("unable to find the source/test/testdata "
                                      "folder (getSourceTestData())")) {
        return;
    }
    char bidiTestPath[400];
    strcpy(bidiTestPath, sourceTestDataPath);
    strcat(bidiTestPath, "BidiCharacterTest.txt");
    LocalStdioFilePointer bidiTestFile(fopen(bidiTestPath, "r"));
    if(bidiTestFile.isNull()) {
        errln("unable to open %s", bidiTestPath);
        return;
    }
    LocalUBiDiPointer ubidi(ubidi_open());
    lineNumber=0;
    levelsCount=0;
    orderingCount=0;
    errorCount=0;
    while(errorCount<20 && fgets(line, (int)sizeof(line), bidiTestFile.getAlias())!=NULL) {
        ++lineNumber;
        paraLevelName="N/A";
        inputString="N/A";
        
        char *commentStart=strchr(line, '#');
        if(commentStart!=NULL) {
            *commentStart=0;
        }
        u_rtrim(line);
        const char *start=u_skipWhitespace(line);
        if(*start==0) {
            continue;  
        }
        
        UChar *buffer=inputString.getBuffer(200);
        int32_t length=u_parseString(start, buffer, inputString.getCapacity(), NULL, errorCode);
        if(errorCode.logIfFailureAndReset("Invalid string in field 0")) {
            errln("Input line %d: %s", (int)lineNumber, line);
            inputString.remove();
            continue;
        }
        inputString.releaseBuffer(length);
        start=strchr(start, ';');
        if(start==NULL) {
            errorCount++;
            errln("\nError on line %d: Missing ; separator on line: %s", (int)lineNumber, line);
            continue;
        }
        start=u_skipWhitespace(start+1);
        char *end;
        int32_t paraDirection=(int32_t)strtol(start, &end, 10);
        UBiDiLevel paraLevel=UBIDI_MAX_EXPLICIT_LEVEL+2;
        if(paraDirection==0) {
            paraLevel=0;
            paraLevelName="LTR";
        }
        else if(paraDirection==1) {
            paraLevel=1;
            paraLevelName="RTL";
        }
        else if(paraDirection==2) {
            paraLevel=UBIDI_DEFAULT_LTR;
            paraLevelName="Auto/LTR";
        }
        else if(paraDirection==3) {
            paraLevel=UBIDI_DEFAULT_RTL;
            paraLevelName="Auto/RTL";
        }
        else if(paraDirection<0 && -paraDirection<=(UBIDI_MAX_EXPLICIT_LEVEL+1)) {
            paraLevel=(UBiDiLevel)(-paraDirection);
            sprintf(levelNameString, "%d", (int)paraLevel);
            paraLevelName=levelNameString;
        }
        if(end<=start || (!U_IS_INV_WHITESPACE(*end) && *end!=';' && *end!=0) ||
                         paraLevel==(UBIDI_MAX_EXPLICIT_LEVEL+2)) {
            errln("\nError on line %d: Input paragraph direction incorrect at %s", (int)lineNumber, start);
            printErrorLine();
            continue;
        }
        start=u_skipWhitespace(end);
        if(*start!=';') {
            errorCount++;
            errln("\nError on line %d: Missing ; separator on line: %s", (int)lineNumber, line);
            continue;
        }
        start++;
        uint32_t resolvedParaLevel=(uint32_t)strtoul(start, &end, 10);
        if(end<=start || (!U_IS_INV_WHITESPACE(*end) && *end!=';' && *end!=0) ||
           resolvedParaLevel>1) {
            errln("\nError on line %d: Resolved paragraph level incorrect at %s", (int)lineNumber, start);
            printErrorLine();
            continue;
        }
        start=u_skipWhitespace(end);
        if(*start!=';') {
            errorCount++;
            errln("\nError on line %d: Missing ; separator on line: %s", (int)lineNumber, line);
            return;
        }
        start++;
        if(!parseLevels(start)) {
            continue;
        }
        start=u_skipWhitespace(start);
        if(*start==';') {
            if(!parseOrdering(start+1)) {
                continue;
            }
        }
        else
            orderingCount=-1;

        ubidi_setPara(ubidi.getAlias(), inputString.getBuffer(), inputString.length(),
                      paraLevel, NULL, errorCode);
        const UBiDiLevel *actualLevels=ubidi_getLevels(ubidi.getAlias(), errorCode);
        if(errorCode.logIfFailureAndReset("ubidi_setPara() or ubidi_getLevels()")) {
            errln("Input line %d: %s", (int)lineNumber, line);
            continue;
        }
        UBiDiLevel actualLevel;
        if((actualLevel=ubidi_getParaLevel(ubidi.getAlias()))!=resolvedParaLevel) {
            printErrorLine();
            errln("\nError on line %d: Wrong resolved paragraph level; expected %d actual %d",
                   (int)lineNumber, resolvedParaLevel, actualLevel);
            continue;
        }
        if(!checkLevels(actualLevels, ubidi_getProcessedLength(ubidi.getAlias()))) {
            continue;
        }
        if(orderingCount>=0 && !checkOrdering(ubidi.getAlias())) {
            continue;
        }
    }
}

static UChar printLevel(UBiDiLevel level) {
    if(level<UBIDI_DEFAULT_LTR) {
        return 0x30+level;
    } else {
        return 0x78;  
    }
}

static uint32_t getDirectionBits(const UBiDiLevel actualLevels[], int32_t actualCount) {
    uint32_t actualDirectionBits=0;
    for(int32_t i=0; i<actualCount; ++i) {
        actualDirectionBits|=(1<<(actualLevels[i]&1));
    }
    return actualDirectionBits;
}

UBool BiDiConformanceTest::checkLevels(const UBiDiLevel actualLevels[], int32_t actualCount) {
    UBool isOk=TRUE;
    if(levelsCount!=actualCount) {
        errln("\nError on line %d: Wrong number of level values; expected %d actual %d",
              (int)lineNumber, (int)levelsCount, (int)actualCount);
        isOk=FALSE;
    } else {
        for(int32_t i=0; i<actualCount; ++i) {
            if(levels[i]!=actualLevels[i] && levels[i]<UBIDI_DEFAULT_LTR) {
                if(directionBits!=3 && directionBits==getDirectionBits(actualLevels, actualCount)) {
                    
                    
                    
                    
                    break;
                } else {
                    errln("\nError on line %d: Wrong level value at index %d; expected %d actual %d",
                          (int)lineNumber, (int)i, levels[i], actualLevels[i]);
                    isOk=FALSE;
                    break;
                }
            }
        }
    }
    if(!isOk) {
        printErrorLine();
        UnicodeString els("Expected levels:   ");
        int32_t i;
        for(i=0; i<levelsCount; ++i) {
            els.append((UChar)0x20).append(printLevel(levels[i]));
        }
        UnicodeString als("Actual   levels:   ");
        for(i=0; i<actualCount; ++i) {
            als.append((UChar)0x20).append(printLevel(actualLevels[i]));
        }
        errln(els);
        errln(als);
    }
    return isOk;
}






UBool BiDiConformanceTest::checkOrdering(UBiDi *ubidi) {
    UBool isOk=TRUE;
    IcuTestErrorCode errorCode(*this, "checkOrdering()");
    int32_t resultLength=ubidi_getResultLength(ubidi);  
    int32_t i, visualIndex;
    
    
    for(i=visualIndex=0; i<resultLength; ++i) {
        int32_t logicalIndex=ubidi_getLogicalIndex(ubidi, i, errorCode);
        if(errorCode.logIfFailureAndReset("ubidi_getLogicalIndex()")) {
            errln("Input line %d: %s", (int)lineNumber, line);
            return FALSE;
        }
        if(levels[logicalIndex]>=UBIDI_DEFAULT_LTR) {
            continue;  
        }
        if(visualIndex<orderingCount && logicalIndex!=ordering[visualIndex]) {
            errln("\nError on line %d: Wrong ordering value at visual index %d; expected %d actual %d",
                  (int)lineNumber, (int)visualIndex, ordering[visualIndex], logicalIndex);
            isOk=FALSE;
            break;
        }
        ++visualIndex;
    }
    
    
    if(isOk && orderingCount!=visualIndex) {
        errln("\nError on line %d: Wrong number of ordering values; expected %d actual %d",
              (int)lineNumber, (int)orderingCount, (int)visualIndex);
        isOk=FALSE;
    }
    if(!isOk) {
        printErrorLine();
        UnicodeString eord("Expected ordering: ");
        for(i=0; i<orderingCount; ++i) {
            eord.append((UChar)0x20).append((UChar)(0x30+ordering[i]));
        }
        UnicodeString aord("Actual   ordering: ");
        for(i=0; i<resultLength; ++i) {
            int32_t logicalIndex=ubidi_getLogicalIndex(ubidi, i, errorCode);
            if(levels[logicalIndex]<UBIDI_DEFAULT_LTR) {
                aord.append((UChar)0x20).append((UChar)(0x30+logicalIndex));
            }
        }
        errln(eord);
        errln(aord);
    }
    return isOk;
}

void BiDiConformanceTest::printErrorLine() {
    ++errorCount;
    errln("Input line %5d:   %s", (int)lineNumber, line);
    errln(UnicodeString("Input string:       ")+inputString);
    errln("Para level:         %s", paraLevelName);
}
