








#include <stdio.h>
#include <unicode/brkiter.h>
#include <stdlib.h>

U_CFUNC int c_main(void);


void printUnicodeString(const UnicodeString &s) {
    char charBuf[1000];
    s.extract(0, s.length(), charBuf, sizeof(charBuf)-1, 0);   
    charBuf[sizeof(charBuf)-1] = 0;          
    printf("%s", charBuf);
}


void printTextRange( BreakIterator& iterator, 
                    int32_t start, int32_t end )
{
    CharacterIterator *strIter = iterator.getText().clone();
    UnicodeString  s;
    strIter->getText(s);

    printf(" %ld %ld\t", (long)start, (long)end);
    printUnicodeString(UnicodeString(s, 0, start));
    printf("|");
    printUnicodeString(UnicodeString(s, start, end-start));
    printf("|");
    printUnicodeString(UnicodeString(s, end));
    puts("");
    delete strIter;
}



void printEachForward( BreakIterator& boundary)
{
    int32_t start = boundary.first();
    for (int32_t end = boundary.next();
         end != BreakIterator::DONE;
         start = end, end = boundary.next())
    {
        printTextRange( boundary, start, end );
    }
}


void printEachBackward( BreakIterator& boundary)
{
    int32_t end = boundary.last();
    for (int32_t start = boundary.previous();
         start != BreakIterator::DONE;
         end = start, start = boundary.previous())
    {
        printTextRange( boundary, start, end );
    }
}


void printFirst(BreakIterator& boundary)
{
    int32_t start = boundary.first();
    int32_t end = boundary.next();
    printTextRange( boundary, start, end );
}


void printLast(BreakIterator& boundary)
{
    int32_t end = boundary.last();
    int32_t start = boundary.previous();
    printTextRange( boundary, start, end );
}


void printAt(BreakIterator &boundary, int32_t pos )
{
    int32_t end = boundary.following(pos);
    int32_t start = boundary.previous();
    printTextRange( boundary, start, end );
}


int main( void )
{
    puts("ICU Break Iterator Sample Program\n");
    puts("C++ Break Iteration\n");
    BreakIterator* boundary;
    UnicodeString stringToExamine("Aaa bbb ccc. Ddd eee fff.");
    printf("Examining: ");
    printUnicodeString(stringToExamine);
    puts("");

    
    UErrorCode status = U_ZERO_ERROR;
    boundary = BreakIterator::createSentenceInstance(
        Locale::getUS(), status );
    if (U_FAILURE(status)) {
        printf("failed to create sentence break iterator.  status = %s", 
            u_errorName(status));
        exit(1);
    }

    boundary->setText(stringToExamine);
    puts("\n Sentence Boundaries... ");
    puts("----- forward: -----------");
    printEachForward(*boundary);
    puts("----- backward: ----------");
    printEachBackward(*boundary);
    delete boundary;

    
    printf("\n Word Boundaries... \n");
    boundary = BreakIterator::createWordInstance(
        Locale::getUS(), status);
    boundary->setText(stringToExamine);
    puts("----- forward: -----------");
    printEachForward(*boundary);
    
    puts("----- first: -------------");
    printFirst(*boundary);
    
    puts("----- last: --------------");
    printLast(*boundary);
    
    puts("----- at pos 10: ---------");
    printAt(*boundary, 10 );

    delete boundary;

    puts("\nEnd C++ Break Iteration");

    
    return c_main();
}
