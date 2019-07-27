






#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "unicode/utypes.h"
#include "unicode/uchar.h"
#include "unicode/uchriter.h"
#include "unicode/brkiter.h"
#include "unicode/locid.h"
#include "unicode/unistr.h"
#include "unicode/uniset.h"
#include "unicode/ustring.h"















class SpaceBreakIterator
{
public:
    
    
    
    SpaceBreakIterator(const UChar *text, int32_t count);

    
    ~SpaceBreakIterator();

    
    int32_t next();

    
    int32_t getWordCount();

    
    int32_t getSpaceCount();

private:
    
    SpaceBreakIterator();

    
    BreakIterator *fBreakIter;

    
    const UChar *fText;

    
    int32_t fTextCount;

    
    int32_t fWordCount;

    
    int32_t fSpaceCount;
    
    
    UnicodeSet fComplexContext;

    
    UBool fDone;
};




class ThaiWordbreakTest
{
public:
    
    
    
    
    
    
    ThaiWordbreakTest(const UChar *spaces, int32_t spaceCount, const UChar *noSpaces, int32_t noSpaceCount, UBool verbose);
    ~ThaiWordbreakTest();

    
    
    int32_t getBreaksNotFound();

    
    
    int32_t getInvalidBreaks();

    
    int32_t getWordCount();

    
    
    
    
    static const UChar *readFile(char *fileName, int32_t &charCount);

    
    
    
    
    
    static const UChar *crunchSpaces(const UChar *spaces, int32_t count, int32_t &nonSpaceCount);

private:
    
    ThaiWordbreakTest();

    
    
    
    
    
    
    UBool compareWordBreaks(const UChar *spaces, int32_t spaceCount,
                            const UChar *noSpaces, int32_t noSpaceCount);

    
    
    void breakNotFound(int32_t br);

    
    
    void foundInvalidBreak(int32_t br);

    
    
    int32_t fBreaksNotFound;

    
    
    int32_t fInvalidBreaks;

    
    int32_t fWordCount;

    
    UBool fVerbose;
};




ThaiWordbreakTest::ThaiWordbreakTest(const UChar *spaces, int32_t spaceCount,
                                     const UChar *noSpaces, int32_t noSpaceCount, UBool verbose)
: fBreaksNotFound(0), fInvalidBreaks(0), fWordCount(0), fVerbose(verbose)
{
    compareWordBreaks(spaces, spaceCount, noSpaces, noSpaceCount);
}




ThaiWordbreakTest::ThaiWordbreakTest()
{
    
}




ThaiWordbreakTest::~ThaiWordbreakTest()
{
    
}





inline int32_t ThaiWordbreakTest::getBreaksNotFound()
{
    return fBreaksNotFound;
}





inline int32_t ThaiWordbreakTest::getInvalidBreaks()
{
    return fInvalidBreaks;
}




inline int32_t ThaiWordbreakTest::getWordCount()
{
    return fWordCount;
}







UBool ThaiWordbreakTest::compareWordBreaks(const UChar *spaces, int32_t spaceCount,
                                           const UChar *noSpaces, int32_t noSpaceCount)
{
    UBool result = TRUE;
    Locale thai("th");
    UCharCharacterIterator *noSpaceIter = new UCharCharacterIterator(noSpaces, noSpaceCount);
    UErrorCode status = U_ZERO_ERROR;
    
    BreakIterator *breakIter = BreakIterator::createWordInstance(thai, status);
    breakIter->adoptText(noSpaceIter);
    
    SpaceBreakIterator spaceIter(spaces, spaceCount);
    
    int32_t nextBreak = 0;
    int32_t nextSpaceBreak = 0;
    int32_t iterCount = 0;
    
    while (TRUE) {
        nextSpaceBreak = spaceIter.next();
        nextBreak = breakIter->next();
        
        if (nextSpaceBreak == BreakIterator::DONE || nextBreak == BreakIterator::DONE) {
            if (nextBreak != BreakIterator::DONE) {
                fprintf(stderr, "break iterator didn't end.\n");
            } else if (nextSpaceBreak != BreakIterator::DONE) {
                fprintf(stderr, "premature break iterator end.\n");
            }
            
            break;
        }
        
        while (nextSpaceBreak != nextBreak &&
               nextSpaceBreak != BreakIterator::DONE && nextBreak != BreakIterator::DONE) {
            if (nextSpaceBreak < nextBreak) {
                breakNotFound(nextSpaceBreak);
                result = FALSE;
                nextSpaceBreak = spaceIter.next();
            } else if (nextSpaceBreak > nextBreak) {
                foundInvalidBreak(nextBreak);
                result = FALSE;
                nextBreak = breakIter->next();
            }
        }
        
        if (fVerbose) {
            printf("%d   %d\n", nextSpaceBreak, nextBreak);
        }
    }
        
   
    fWordCount = spaceIter.getWordCount();
    
    delete breakIter;

    return result;
}





void ThaiWordbreakTest::breakNotFound(int32_t br)
{
    if (fVerbose) {
        printf("%d   ****\n", br);
    } else {
        fprintf(stderr, "break not found: %d\n", br);
    }
    
    fBreaksNotFound += 1;
}





void ThaiWordbreakTest::foundInvalidBreak(int32_t br)
{
    if (fVerbose) {
        printf("****   %d\n", br);
    } else {
        fprintf(stderr, "found invalid break: %d\n", br);
    }
    
    fInvalidBreaks += 1;
}





const UChar *ThaiWordbreakTest::readFile(char *fileName, int32_t &charCount)
{
    FILE *f;
    int32_t fileSize;
    
    UChar *buffer;
    char *bufferChars;
    
    f = fopen(fileName, "rb");
    
    if( f == NULL ) {
        fprintf(stderr,"Couldn't open %s reason: %s \n", fileName, strerror(errno));
        return 0;
    }
    
    fseek(f, 0, SEEK_END);
    fileSize = ftell(f);
    
    fseek(f, 0, SEEK_SET);
    bufferChars = new char[fileSize];
    
    if(bufferChars == 0) {
        fprintf(stderr,"Couldn't get memory for reading %s reason: %s \n", fileName, strerror(errno));
        fclose(f);
        return 0;
    }
    
    fread(bufferChars, sizeof(char), fileSize, f);
    if( ferror(f) ) {
        fprintf(stderr,"Couldn't read %s reason: %s \n", fileName, strerror(errno));
        fclose(f);
        delete[] bufferChars;
        return 0;
    }
    fclose(f);
    
    UnicodeString myText(bufferChars, fileSize, "UTF-8");

    delete[] bufferChars;
    
    charCount = myText.length();
    buffer = new UChar[charCount];
    if(buffer == 0) {
        fprintf(stderr,"Couldn't get memory for reading %s reason: %s \n", fileName, strerror(errno));
        return 0;
    }
    
    myText.extract(1, myText.length(), buffer);
    charCount--;  
    buffer[charCount] = 0;    
    
    return buffer;
}








const UChar *ThaiWordbreakTest::crunchSpaces(const UChar *spaces, int32_t count, int32_t &nonSpaceCount)
{
    int32_t i, out, spaceCount;

    spaceCount = 0;
    for (i = 0; i < count; i += 1) {
        if (spaces[i] == 0x0020 ) {
            spaceCount += 1;
        }
    }

    nonSpaceCount = count - spaceCount;
    UChar *noSpaces = new UChar[nonSpaceCount];

    if (noSpaces == 0) {
        fprintf(stderr, "Couldn't allocate memory for the space stripped text.\n");
        return 0;
    }

    for (out = 0, i = 0; i < count; i += 1) {
        if (spaces[i] != 0x0020 ) {
            noSpaces[out++] = spaces[i];
        }
    }

    return noSpaces;
}




int generateFile(const UChar *chars, int32_t length) {
    Locale root("");
    UCharCharacterIterator *noSpaceIter = new UCharCharacterIterator(chars, length);
    UErrorCode status = U_ZERO_ERROR;
    
    UnicodeSet complexContext(UNICODE_STRING_SIMPLE("[:LineBreak=SA:]"), status);
    BreakIterator *breakIter = BreakIterator::createWordInstance(root, status);
    breakIter->adoptText(noSpaceIter);
    char outbuf[1024];
    int32_t strlength;
    UChar bom = 0xFEFF;
    
    printf("%s", u_strToUTF8(outbuf, sizeof(outbuf), &strlength, &bom, 1, &status));
    int32_t prevbreak = 0;
    while (U_SUCCESS(status)) {
        int32_t nextbreak = breakIter->next();
        if (nextbreak == BreakIterator::DONE) {
            break;
        }
        printf("%s", u_strToUTF8(outbuf, sizeof(outbuf), &strlength, &chars[prevbreak],
                                    nextbreak-prevbreak, &status));
        if (nextbreak > 0 && complexContext.contains(chars[nextbreak-1])
            && complexContext.contains(chars[nextbreak])) {
            printf(" ");
        }
        prevbreak = nextbreak;
    }
    
    if (U_FAILURE(status)) {
        fprintf(stderr, "generate failed: %s\n", u_errorName(status));
        return status;
    }
    else {
        return 0;
    }
}





int main(int argc, char **argv)
{
    char *fileName = "space.txt";
    int arg = 1;
    UBool verbose = FALSE;
    UBool generate = FALSE;

    if (argc >= 2 && strcmp(argv[1], "-generate") == 0) {
        generate = TRUE;
        arg += 1;
    }

    if (argc >= 2 && strcmp(argv[1], "-verbose") == 0) {
        verbose = TRUE;
        arg += 1;
    }

    if (arg == argc - 1) {
        fileName = argv[arg++];
    }

    if (arg != argc) {
        fprintf(stderr, "Usage: %s [-verbose] [<file>]\n", argv[0]);
        return 1;
    }

    int32_t spaceCount, nonSpaceCount;
    const UChar *spaces, *noSpaces;

    spaces = ThaiWordbreakTest::readFile(fileName, spaceCount);

    if (spaces == 0) {
        return 1;
    }
    
    if (generate) {
        return generateFile(spaces, spaceCount);
    }

    noSpaces = ThaiWordbreakTest::crunchSpaces(spaces, spaceCount, nonSpaceCount);

    if (noSpaces == 0) {
        return 1;
    }

    ThaiWordbreakTest test(spaces, spaceCount, noSpaces, nonSpaceCount, verbose);

    printf("word count: %d\n", test.getWordCount());
    printf("breaks not found: %d\n", test.getBreaksNotFound());
    printf("invalid breaks found: %d\n", test.getInvalidBreaks());

    return 0;
}





SpaceBreakIterator::SpaceBreakIterator(const UChar *text, int32_t count)
  : fBreakIter(0), fText(text), fTextCount(count), fWordCount(0), fSpaceCount(0), fDone(FALSE)
{
    UCharCharacterIterator *iter = new UCharCharacterIterator(text, count);
    UErrorCode status = U_ZERO_ERROR;
    fComplexContext.applyPattern(UNICODE_STRING_SIMPLE("[:LineBreak=SA:]"), status);
    Locale root("");

    fBreakIter = BreakIterator::createWordInstance(root, status);
    fBreakIter->adoptText(iter);
}

SpaceBreakIterator::SpaceBreakIterator()
{
    
}




SpaceBreakIterator::~SpaceBreakIterator()
{
    delete fBreakIter;
}




int32_t SpaceBreakIterator::next()
{
    if (fDone) {
        return BreakIterator::DONE;
    }
    
    int32_t nextBreak;
    do {
        nextBreak = fBreakIter->next();
        
        if (nextBreak == BreakIterator::DONE) {
            fDone = TRUE;
            return BreakIterator::DONE;
        }
    }
    while(nextBreak > 0 && fComplexContext.contains(fText[nextBreak-1])
            && fComplexContext.contains(fText[nextBreak]));
    
   int32_t result = nextBreak - fSpaceCount;
    
    if (nextBreak < fTextCount) {
        if (fText[nextBreak] == 0x0020 ) {
            fSpaceCount += fBreakIter->next() - nextBreak;
        }
    }
    
    fWordCount += 1;

    return result;
}




int32_t SpaceBreakIterator::getSpaceCount()
{
    return fSpaceCount;
}




int32_t SpaceBreakIterator::getWordCount()
{
    return fWordCount;
}


