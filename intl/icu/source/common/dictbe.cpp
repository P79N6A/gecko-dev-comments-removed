






#include "unicode/utypes.h"

#if !UCONFIG_NO_BREAK_ITERATION

#include "brkeng.h"
#include "dictbe.h"
#include "unicode/uniset.h"
#include "unicode/chariter.h"
#include "unicode/ubrk.h"
#include "uvectr32.h"
#include "uvector.h"
#include "uassert.h"
#include "unicode/normlzr.h"
#include "cmemory.h"
#include "dictionarydata.h"

U_NAMESPACE_BEGIN





DictionaryBreakEngine::DictionaryBreakEngine(uint32_t breakTypes) {
    fTypes = breakTypes;
}

DictionaryBreakEngine::~DictionaryBreakEngine() {
}

UBool
DictionaryBreakEngine::handles(UChar32 c, int32_t breakType) const {
    return (breakType >= 0 && breakType < 32 && (((uint32_t)1 << breakType) & fTypes)
            && fSet.contains(c));
}

int32_t
DictionaryBreakEngine::findBreaks( UText *text,
                                 int32_t startPos,
                                 int32_t endPos,
                                 UBool reverse,
                                 int32_t breakType,
                                 UStack &foundBreaks ) const {
    int32_t result = 0;

    
    
    

    int32_t start = (int32_t)utext_getNativeIndex(text);
    int32_t current;
    int32_t rangeStart;
    int32_t rangeEnd;
    UChar32 c = utext_current32(text);
    if (reverse) {
        UBool   isDict = fSet.contains(c);
        while((current = (int32_t)utext_getNativeIndex(text)) > startPos && isDict) {
            c = utext_previous32(text);
            isDict = fSet.contains(c);
        }
        if (current < startPos) {
            rangeStart = startPos;
        } else {
            rangeStart = current;
            if (!isDict) {
                utext_next32(text);
                rangeStart = utext_getNativeIndex(text);
            }
        }
        
        utext_setNativeIndex(text, start);
        utext_next32(text);
        rangeEnd = utext_getNativeIndex(text);
    }
    else {
        while((current = (int32_t)utext_getNativeIndex(text)) < endPos && fSet.contains(c)) {
            utext_next32(text);         
            c = utext_current32(text);
        }
        rangeStart = start;
        rangeEnd = current;
    }
    if (breakType >= 0 && breakType < 32 && (((uint32_t)1 << breakType) & fTypes)) {
        result = divideUpDictionaryRange(text, rangeStart, rangeEnd, foundBreaks);
        utext_setNativeIndex(text, current);
    }
    
    return result;
}

void
DictionaryBreakEngine::setCharacters( const UnicodeSet &set ) {
    fSet = set;
    
    fSet.compact();
}











static const int32_t POSSIBLE_WORD_LIST_MAX = 20;

class PossibleWord {
private:
    
    
    int32_t   count;      
    int32_t   prefix;     
    int32_t   offset;     
    int32_t   mark;       
    int32_t   current;    
    int32_t   cuLengths[POSSIBLE_WORD_LIST_MAX];   
    int32_t   cpLengths[POSSIBLE_WORD_LIST_MAX];   

public:
    PossibleWord() : count(0), prefix(0), offset(-1), mark(0), current(0) {};
    ~PossibleWord() {};
  
    
    int32_t   candidates( UText *text, DictionaryMatcher *dict, int32_t rangeEnd );
  
    
    int32_t   acceptMarked( UText *text );
  
    
    
    UBool     backUp( UText *text );
  
    
    
    int32_t   longestPrefix() { return prefix; };
  
    
    void      markCurrent() { mark = current; };
    
    
    int32_t   markedCPLength() { return cpLengths[mark]; };
};


int32_t PossibleWord::candidates( UText *text, DictionaryMatcher *dict, int32_t rangeEnd ) {
    
    int32_t start = (int32_t)utext_getNativeIndex(text);
    if (start != offset) {
        offset = start;
        count = dict->matches(text, rangeEnd-start, UPRV_LENGTHOF(cuLengths), cuLengths, cpLengths, NULL, &prefix);
        
        if (count <= 0) {
            utext_setNativeIndex(text, start);
        }
    }
    if (count > 0) {
        utext_setNativeIndex(text, start+cuLengths[count-1]);
    }
    current = count-1;
    mark = current;
    return count;
}

int32_t
PossibleWord::acceptMarked( UText *text ) {
    utext_setNativeIndex(text, offset + cuLengths[mark]);
    return cuLengths[mark];
}


UBool
PossibleWord::backUp( UText *text ) {
    if (current > 0) {
        utext_setNativeIndex(text, offset + cuLengths[--current]);
        return TRUE;
    }
    return FALSE;
}







static const int32_t THAI_LOOKAHEAD = 3;


static const int32_t THAI_ROOT_COMBINE_THRESHOLD = 3;



static const int32_t THAI_PREFIX_COMBINE_THRESHOLD = 3;


static const int32_t THAI_PAIYANNOI = 0x0E2F;


static const int32_t THAI_MAIYAMOK = 0x0E46;


static const int32_t THAI_MIN_WORD = 2;


static const int32_t THAI_MIN_WORD_SPAN = THAI_MIN_WORD * 2;

ThaiBreakEngine::ThaiBreakEngine(DictionaryMatcher *adoptDictionary, UErrorCode &status)
    : DictionaryBreakEngine((1<<UBRK_WORD) | (1<<UBRK_LINE)),
      fDictionary(adoptDictionary)
{
    fThaiWordSet.applyPattern(UNICODE_STRING_SIMPLE("[[:Thai:]&[:LineBreak=SA:]]"), status);
    if (U_SUCCESS(status)) {
        setCharacters(fThaiWordSet);
    }
    fMarkSet.applyPattern(UNICODE_STRING_SIMPLE("[[:Thai:]&[:LineBreak=SA:]&[:M:]]"), status);
    fMarkSet.add(0x0020);
    fEndWordSet = fThaiWordSet;
    fEndWordSet.remove(0x0E31);             
    fEndWordSet.remove(0x0E40, 0x0E44);     
    fBeginWordSet.add(0x0E01, 0x0E2E);      
    fBeginWordSet.add(0x0E40, 0x0E44);      
    fSuffixSet.add(THAI_PAIYANNOI);
    fSuffixSet.add(THAI_MAIYAMOK);

    
    fMarkSet.compact();
    fEndWordSet.compact();
    fBeginWordSet.compact();
    fSuffixSet.compact();
}

ThaiBreakEngine::~ThaiBreakEngine() {
    delete fDictionary;
}

int32_t
ThaiBreakEngine::divideUpDictionaryRange( UText *text,
                                                int32_t rangeStart,
                                                int32_t rangeEnd,
                                                UStack &foundBreaks ) const {
    utext_setNativeIndex(text, rangeStart);
    utext_moveIndex32(text, THAI_MIN_WORD_SPAN);
    if (utext_getNativeIndex(text) >= rangeEnd) {
        return 0;       
    }
    utext_setNativeIndex(text, rangeStart);


    uint32_t wordsFound = 0;
    int32_t cpWordLength = 0;    
    int32_t cuWordLength = 0;    
    int32_t current;
    UErrorCode status = U_ZERO_ERROR;
    PossibleWord words[THAI_LOOKAHEAD];
    
    utext_setNativeIndex(text, rangeStart);
    
    while (U_SUCCESS(status) && (current = (int32_t)utext_getNativeIndex(text)) < rangeEnd) {
        cpWordLength = 0;
        cuWordLength = 0;

        
        int32_t candidates = words[wordsFound%THAI_LOOKAHEAD].candidates(text, fDictionary, rangeEnd);
        
        
        if (candidates == 1) {
            cuWordLength = words[wordsFound % THAI_LOOKAHEAD].acceptMarked(text);
            cpWordLength = words[wordsFound % THAI_LOOKAHEAD].markedCPLength();
            wordsFound += 1;
        }
        
        else if (candidates > 1) {
            
            if ((int32_t)utext_getNativeIndex(text) >= rangeEnd) {
                goto foundBest;
            }
            do {
                int32_t wordsMatched = 1;
                if (words[(wordsFound + 1) % THAI_LOOKAHEAD].candidates(text, fDictionary, rangeEnd) > 0) {
                    if (wordsMatched < 2) {
                        
                        words[wordsFound%THAI_LOOKAHEAD].markCurrent();
                        wordsMatched = 2;
                    }
                    
                    
                    if ((int32_t)utext_getNativeIndex(text) >= rangeEnd) {
                        goto foundBest;
                    }
                    
                    
                    do {
                        
                        if (words[(wordsFound + 2) % THAI_LOOKAHEAD].candidates(text, fDictionary, rangeEnd)) {
                            words[wordsFound % THAI_LOOKAHEAD].markCurrent();
                            goto foundBest;
                        }
                    }
                    while (words[(wordsFound + 1) % THAI_LOOKAHEAD].backUp(text));
                }
            }
            while (words[wordsFound % THAI_LOOKAHEAD].backUp(text));
foundBest:
            
            cuWordLength = words[wordsFound % THAI_LOOKAHEAD].acceptMarked(text);
            cpWordLength = words[wordsFound % THAI_LOOKAHEAD].markedCPLength();
            wordsFound += 1;
        }
        
        
        
        
        
        
        
        UChar32 uc = 0;
        if ((int32_t)utext_getNativeIndex(text) < rangeEnd &&  cpWordLength < THAI_ROOT_COMBINE_THRESHOLD) {
            
            
            
            if (words[wordsFound % THAI_LOOKAHEAD].candidates(text, fDictionary, rangeEnd) <= 0
                  && (cuWordLength == 0
                      || words[wordsFound%THAI_LOOKAHEAD].longestPrefix() < THAI_PREFIX_COMBINE_THRESHOLD)) {
                
                int32_t remaining = rangeEnd - (current+cuWordLength);
                UChar32 pc;
                int32_t chars = 0;
                for (;;) {
                    int32_t pcIndex = utext_getNativeIndex(text);
                    pc = utext_next32(text);
                    int32_t pcSize = utext_getNativeIndex(text) - pcIndex;
                    chars += pcSize;
                    remaining -= pcSize;
                    if (remaining <= 0) {
                        break;
                    }
                    uc = utext_current32(text);
                    if (fEndWordSet.contains(pc) && fBeginWordSet.contains(uc)) {
                        
                        
                        
                        
                        
                        int32_t candidates = words[(wordsFound + 1) % THAI_LOOKAHEAD].candidates(text, fDictionary, rangeEnd);
                        utext_setNativeIndex(text, current + cuWordLength + chars);
                        if (candidates > 0) {
                            break;
                        }
                    }
                }
                
                
                if (cuWordLength <= 0) {
                    wordsFound += 1;
                }
                
                
                cuWordLength += chars;
            }
            else {
                
                utext_setNativeIndex(text, current+cuWordLength);
            }
        }
        
        
        int32_t currPos;
        while ((currPos = (int32_t)utext_getNativeIndex(text)) < rangeEnd && fMarkSet.contains(utext_current32(text))) {
            utext_next32(text);
            cuWordLength += (int32_t)utext_getNativeIndex(text) - currPos;
        }
        
        
        
        
        
        if ((int32_t)utext_getNativeIndex(text) < rangeEnd && cuWordLength > 0) {
            if (words[wordsFound%THAI_LOOKAHEAD].candidates(text, fDictionary, rangeEnd) <= 0
                && fSuffixSet.contains(uc = utext_current32(text))) {
                if (uc == THAI_PAIYANNOI) {
                    if (!fSuffixSet.contains(utext_previous32(text))) {
                        
                        utext_next32(text);
                        int32_t paiyannoiIndex = utext_getNativeIndex(text);
                        utext_next32(text);
                        cuWordLength += utext_getNativeIndex(text) - paiyannoiIndex;    
                        uc = utext_current32(text);     
                    }
                    else {
                        
                        utext_next32(text);
                    }
                }
                if (uc == THAI_MAIYAMOK) {
                    if (utext_previous32(text) != THAI_MAIYAMOK) {
                        
                        utext_next32(text);
                        int32_t maiyamokIndex = utext_getNativeIndex(text);
                        utext_next32(text);
                        cuWordLength += utext_getNativeIndex(text) - maiyamokIndex;    
                    }
                    else {
                        
                        utext_next32(text);
                    }
                }
            }
            else {
                utext_setNativeIndex(text, current+cuWordLength);
            }
        }

        
        if (cuWordLength > 0) {
            foundBreaks.push((current+cuWordLength), status);
        }
    }

    
    if (foundBreaks.peeki() >= rangeEnd) {
        (void) foundBreaks.popi();
        wordsFound -= 1;
    }

    return wordsFound;
}







static const int32_t LAO_LOOKAHEAD = 3;


static const int32_t LAO_ROOT_COMBINE_THRESHOLD = 3;



static const int32_t LAO_PREFIX_COMBINE_THRESHOLD = 3;


static const int32_t LAO_MIN_WORD = 2;


static const int32_t LAO_MIN_WORD_SPAN = LAO_MIN_WORD * 2;

LaoBreakEngine::LaoBreakEngine(DictionaryMatcher *adoptDictionary, UErrorCode &status)
    : DictionaryBreakEngine((1<<UBRK_WORD) | (1<<UBRK_LINE)),
      fDictionary(adoptDictionary)
{
    fLaoWordSet.applyPattern(UNICODE_STRING_SIMPLE("[[:Laoo:]&[:LineBreak=SA:]]"), status);
    if (U_SUCCESS(status)) {
        setCharacters(fLaoWordSet);
    }
    fMarkSet.applyPattern(UNICODE_STRING_SIMPLE("[[:Laoo:]&[:LineBreak=SA:]&[:M:]]"), status);
    fMarkSet.add(0x0020);
    fEndWordSet = fLaoWordSet;
    fEndWordSet.remove(0x0EC0, 0x0EC4);     
    fBeginWordSet.add(0x0E81, 0x0EAE);      
    fBeginWordSet.add(0x0EDC, 0x0EDD);      
    fBeginWordSet.add(0x0EC0, 0x0EC4);      

    
    fMarkSet.compact();
    fEndWordSet.compact();
    fBeginWordSet.compact();
}

LaoBreakEngine::~LaoBreakEngine() {
    delete fDictionary;
}

int32_t
LaoBreakEngine::divideUpDictionaryRange( UText *text,
                                                int32_t rangeStart,
                                                int32_t rangeEnd,
                                                UStack &foundBreaks ) const {
    if ((rangeEnd - rangeStart) < LAO_MIN_WORD_SPAN) {
        return 0;       
    }

    uint32_t wordsFound = 0;
    int32_t cpWordLength = 0;
    int32_t cuWordLength = 0;
    int32_t current;
    UErrorCode status = U_ZERO_ERROR;
    PossibleWord words[LAO_LOOKAHEAD];
    
    utext_setNativeIndex(text, rangeStart);
    
    while (U_SUCCESS(status) && (current = (int32_t)utext_getNativeIndex(text)) < rangeEnd) {
        cuWordLength = 0;
        cpWordLength = 0;

        
        int32_t candidates = words[wordsFound%LAO_LOOKAHEAD].candidates(text, fDictionary, rangeEnd);
        
        
        if (candidates == 1) {
            cuWordLength = words[wordsFound % LAO_LOOKAHEAD].acceptMarked(text);
            cpWordLength = words[wordsFound % LAO_LOOKAHEAD].markedCPLength();
            wordsFound += 1;
        }
        
        else if (candidates > 1) {
            
            if (utext_getNativeIndex(text) >= rangeEnd) {
                goto foundBest;
            }
            do {
                int32_t wordsMatched = 1;
                if (words[(wordsFound + 1) % LAO_LOOKAHEAD].candidates(text, fDictionary, rangeEnd) > 0) {
                    if (wordsMatched < 2) {
                        
                        words[wordsFound%LAO_LOOKAHEAD].markCurrent();
                        wordsMatched = 2;
                    }
                    
                    
                    if ((int32_t)utext_getNativeIndex(text) >= rangeEnd) {
                        goto foundBest;
                    }
                    
                    
                    do {
                        
                        if (words[(wordsFound + 2) % LAO_LOOKAHEAD].candidates(text, fDictionary, rangeEnd)) {
                            words[wordsFound % LAO_LOOKAHEAD].markCurrent();
                            goto foundBest;
                        }
                    }
                    while (words[(wordsFound + 1) % LAO_LOOKAHEAD].backUp(text));
                }
            }
            while (words[wordsFound % LAO_LOOKAHEAD].backUp(text));
foundBest:
            cuWordLength = words[wordsFound % LAO_LOOKAHEAD].acceptMarked(text);
            cpWordLength = words[wordsFound % LAO_LOOKAHEAD].markedCPLength();
            wordsFound += 1;
        }
        
        
        
        
        
        
        if ((int32_t)utext_getNativeIndex(text) < rangeEnd && cpWordLength < LAO_ROOT_COMBINE_THRESHOLD) {
            
            
            
            if (words[wordsFound % LAO_LOOKAHEAD].candidates(text, fDictionary, rangeEnd) <= 0
                  && (cuWordLength == 0
                      || words[wordsFound%LAO_LOOKAHEAD].longestPrefix() < LAO_PREFIX_COMBINE_THRESHOLD)) {
                
                int32_t remaining = rangeEnd - (current + cuWordLength);
                UChar32 pc;
                UChar32 uc;
                int32_t chars = 0;
                for (;;) {
                    int32_t pcIndex = utext_getNativeIndex(text);
                    pc = utext_next32(text);
                    int32_t pcSize = utext_getNativeIndex(text) - pcIndex;
                    chars += pcSize;
                    remaining -= pcSize;
                    if (remaining <= 0) {
                        break;
                    }
                    uc = utext_current32(text);
                    if (fEndWordSet.contains(pc) && fBeginWordSet.contains(uc)) {
                        
                        
                        int32_t candidates = words[(wordsFound + 1) % LAO_LOOKAHEAD].candidates(text, fDictionary, rangeEnd);
                        utext_setNativeIndex(text, current + cuWordLength + chars);
                        if (candidates > 0) {
                            break;
                        }
                    }
                }
                
                
                if (cuWordLength <= 0) {
                    wordsFound += 1;
                }
                
                
                cuWordLength += chars;
            }
            else {
                
                utext_setNativeIndex(text, current + cuWordLength);
            }
        }
        
        
        int32_t currPos;
        while ((currPos = (int32_t)utext_getNativeIndex(text)) < rangeEnd && fMarkSet.contains(utext_current32(text))) {
            utext_next32(text);
            cuWordLength += (int32_t)utext_getNativeIndex(text) - currPos;
        }
        
        
        
        
        
        

        
        if (cuWordLength > 0) {
            foundBreaks.push((current+cuWordLength), status);
        }
    }

    
    if (foundBreaks.peeki() >= rangeEnd) {
        (void) foundBreaks.popi();
        wordsFound -= 1;
    }

    return wordsFound;
}







static const int32_t BURMESE_LOOKAHEAD = 3;


static const int32_t BURMESE_ROOT_COMBINE_THRESHOLD = 3;



static const int32_t BURMESE_PREFIX_COMBINE_THRESHOLD = 3;


static const int32_t BURMESE_MIN_WORD = 2;


static const int32_t BURMESE_MIN_WORD_SPAN = BURMESE_MIN_WORD * 2;

BurmeseBreakEngine::BurmeseBreakEngine(DictionaryMatcher *adoptDictionary, UErrorCode &status)
    : DictionaryBreakEngine((1<<UBRK_WORD) | (1<<UBRK_LINE)),
      fDictionary(adoptDictionary)
{
    fBurmeseWordSet.applyPattern(UNICODE_STRING_SIMPLE("[[:Mymr:]&[:LineBreak=SA:]]"), status);
    if (U_SUCCESS(status)) {
        setCharacters(fBurmeseWordSet);
    }
    fMarkSet.applyPattern(UNICODE_STRING_SIMPLE("[[:Mymr:]&[:LineBreak=SA:]&[:M:]]"), status);
    fMarkSet.add(0x0020);
    fEndWordSet = fBurmeseWordSet;
    fBeginWordSet.add(0x1000, 0x102A);      

    
    fMarkSet.compact();
    fEndWordSet.compact();
    fBeginWordSet.compact();
}

BurmeseBreakEngine::~BurmeseBreakEngine() {
    delete fDictionary;
}

int32_t
BurmeseBreakEngine::divideUpDictionaryRange( UText *text,
                                                int32_t rangeStart,
                                                int32_t rangeEnd,
                                                UStack &foundBreaks ) const {
    if ((rangeEnd - rangeStart) < BURMESE_MIN_WORD_SPAN) {
        return 0;       
    }

    uint32_t wordsFound = 0;
    int32_t cpWordLength = 0;
    int32_t cuWordLength = 0;
    int32_t current;
    UErrorCode status = U_ZERO_ERROR;
    PossibleWord words[BURMESE_LOOKAHEAD];
    
    utext_setNativeIndex(text, rangeStart);
    
    while (U_SUCCESS(status) && (current = (int32_t)utext_getNativeIndex(text)) < rangeEnd) {
        cuWordLength = 0;
        cpWordLength = 0;

        
        int32_t candidates = words[wordsFound%BURMESE_LOOKAHEAD].candidates(text, fDictionary, rangeEnd);
        
        
        if (candidates == 1) {
            cuWordLength = words[wordsFound % BURMESE_LOOKAHEAD].acceptMarked(text);
            cpWordLength = words[wordsFound % BURMESE_LOOKAHEAD].markedCPLength();
            wordsFound += 1;
        }
        
        else if (candidates > 1) {
            
            if (utext_getNativeIndex(text) >= rangeEnd) {
                goto foundBest;
            }
            do {
                int32_t wordsMatched = 1;
                if (words[(wordsFound + 1) % BURMESE_LOOKAHEAD].candidates(text, fDictionary, rangeEnd) > 0) {
                    if (wordsMatched < 2) {
                        
                        words[wordsFound%BURMESE_LOOKAHEAD].markCurrent();
                        wordsMatched = 2;
                    }
                    
                    
                    if ((int32_t)utext_getNativeIndex(text) >= rangeEnd) {
                        goto foundBest;
                    }
                    
                    
                    do {
                        
                        if (words[(wordsFound + 2) % BURMESE_LOOKAHEAD].candidates(text, fDictionary, rangeEnd)) {
                            words[wordsFound % BURMESE_LOOKAHEAD].markCurrent();
                            goto foundBest;
                        }
                    }
                    while (words[(wordsFound + 1) % BURMESE_LOOKAHEAD].backUp(text));
                }
            }
            while (words[wordsFound % BURMESE_LOOKAHEAD].backUp(text));
foundBest:
            cuWordLength = words[wordsFound % BURMESE_LOOKAHEAD].acceptMarked(text);
            cpWordLength = words[wordsFound % BURMESE_LOOKAHEAD].markedCPLength();
            wordsFound += 1;
        }
        
        
        
        
        
        
        if ((int32_t)utext_getNativeIndex(text) < rangeEnd && cpWordLength < BURMESE_ROOT_COMBINE_THRESHOLD) {
            
            
            
            if (words[wordsFound % BURMESE_LOOKAHEAD].candidates(text, fDictionary, rangeEnd) <= 0
                  && (cuWordLength == 0
                      || words[wordsFound%BURMESE_LOOKAHEAD].longestPrefix() < BURMESE_PREFIX_COMBINE_THRESHOLD)) {
                
                int32_t remaining = rangeEnd - (current + cuWordLength);
                UChar32 pc;
                UChar32 uc;
                int32_t chars = 0;
                for (;;) {
                    int32_t pcIndex = utext_getNativeIndex(text);
                    pc = utext_next32(text);
                    int32_t pcSize = utext_getNativeIndex(text) - pcIndex;
                    chars += pcSize;
                    remaining -= pcSize;
                    if (remaining <= 0) {
                        break;
                    }
                    uc = utext_current32(text);
                    if (fEndWordSet.contains(pc) && fBeginWordSet.contains(uc)) {
                        
                        
                        int32_t candidates = words[(wordsFound + 1) % BURMESE_LOOKAHEAD].candidates(text, fDictionary, rangeEnd);
                        utext_setNativeIndex(text, current + cuWordLength + chars);
                        if (candidates > 0) {
                            break;
                        }
                    }
                }
                
                
                if (cuWordLength <= 0) {
                    wordsFound += 1;
                }
                
                
                cuWordLength += chars;
            }
            else {
                
                utext_setNativeIndex(text, current + cuWordLength);
            }
        }
        
        
        int32_t currPos;
        while ((currPos = (int32_t)utext_getNativeIndex(text)) < rangeEnd && fMarkSet.contains(utext_current32(text))) {
            utext_next32(text);
            cuWordLength += (int32_t)utext_getNativeIndex(text) - currPos;
        }
        
        
        
        
        
        

        
        if (cuWordLength > 0) {
            foundBreaks.push((current+cuWordLength), status);
        }
    }

    
    if (foundBreaks.peeki() >= rangeEnd) {
        (void) foundBreaks.popi();
        wordsFound -= 1;
    }

    return wordsFound;
}







static const int32_t KHMER_LOOKAHEAD = 3;


static const int32_t KHMER_ROOT_COMBINE_THRESHOLD = 3;



static const int32_t KHMER_PREFIX_COMBINE_THRESHOLD = 3;


static const int32_t KHMER_MIN_WORD = 2;


static const int32_t KHMER_MIN_WORD_SPAN = KHMER_MIN_WORD * 2;

KhmerBreakEngine::KhmerBreakEngine(DictionaryMatcher *adoptDictionary, UErrorCode &status)
    : DictionaryBreakEngine((1 << UBRK_WORD) | (1 << UBRK_LINE)),
      fDictionary(adoptDictionary)
{
    fKhmerWordSet.applyPattern(UNICODE_STRING_SIMPLE("[[:Khmr:]&[:LineBreak=SA:]]"), status);
    if (U_SUCCESS(status)) {
        setCharacters(fKhmerWordSet);
    }
    fMarkSet.applyPattern(UNICODE_STRING_SIMPLE("[[:Khmr:]&[:LineBreak=SA:]&[:M:]]"), status);
    fMarkSet.add(0x0020);
    fEndWordSet = fKhmerWordSet;
    fBeginWordSet.add(0x1780, 0x17B3);
    
    
    
    fEndWordSet.remove(0x17D2);             
    







    
    fMarkSet.compact();
    fEndWordSet.compact();
    fBeginWordSet.compact();

}

KhmerBreakEngine::~KhmerBreakEngine() {
    delete fDictionary;
}

int32_t
KhmerBreakEngine::divideUpDictionaryRange( UText *text,
                                                int32_t rangeStart,
                                                int32_t rangeEnd,
                                                UStack &foundBreaks ) const {
    if ((rangeEnd - rangeStart) < KHMER_MIN_WORD_SPAN) {
        return 0;       
    }

    uint32_t wordsFound = 0;
    int32_t cpWordLength = 0;
    int32_t cuWordLength = 0;
    int32_t current;
    UErrorCode status = U_ZERO_ERROR;
    PossibleWord words[KHMER_LOOKAHEAD];

    utext_setNativeIndex(text, rangeStart);

    while (U_SUCCESS(status) && (current = (int32_t)utext_getNativeIndex(text)) < rangeEnd) {
        cuWordLength = 0;
        cpWordLength = 0;

        
        int32_t candidates = words[wordsFound%KHMER_LOOKAHEAD].candidates(text, fDictionary, rangeEnd);

        
        if (candidates == 1) {
            cuWordLength = words[wordsFound % KHMER_LOOKAHEAD].acceptMarked(text);
            cpWordLength = words[wordsFound % KHMER_LOOKAHEAD].markedCPLength();
            wordsFound += 1;
        }

        
        else if (candidates > 1) {
            
            if ((int32_t)utext_getNativeIndex(text) >= rangeEnd) {
                goto foundBest;
            }
            do {
                int32_t wordsMatched = 1;
                if (words[(wordsFound + 1) % KHMER_LOOKAHEAD].candidates(text, fDictionary, rangeEnd) > 0) {
                    if (wordsMatched < 2) {
                        
                        words[wordsFound % KHMER_LOOKAHEAD].markCurrent();
                        wordsMatched = 2;
                    }

                    
                    if ((int32_t)utext_getNativeIndex(text) >= rangeEnd) {
                        goto foundBest;
                    }

                    
                    do {
                        
                        if (words[(wordsFound + 2) % KHMER_LOOKAHEAD].candidates(text, fDictionary, rangeEnd)) {
                            words[wordsFound % KHMER_LOOKAHEAD].markCurrent();
                            goto foundBest;
                        }
                    }
                    while (words[(wordsFound + 1) % KHMER_LOOKAHEAD].backUp(text));
                }
            }
            while (words[wordsFound % KHMER_LOOKAHEAD].backUp(text));
foundBest:
            cuWordLength = words[wordsFound % KHMER_LOOKAHEAD].acceptMarked(text);
            cpWordLength = words[wordsFound % KHMER_LOOKAHEAD].markedCPLength();
            wordsFound += 1;
        }

        
        
        
        
        
        if ((int32_t)utext_getNativeIndex(text) < rangeEnd && cpWordLength < KHMER_ROOT_COMBINE_THRESHOLD) {
            
            
            
            if (words[wordsFound % KHMER_LOOKAHEAD].candidates(text, fDictionary, rangeEnd) <= 0
                  && (cuWordLength == 0
                      || words[wordsFound % KHMER_LOOKAHEAD].longestPrefix() < KHMER_PREFIX_COMBINE_THRESHOLD)) {
                
                int32_t remaining = rangeEnd - (current+cuWordLength);
                UChar32 pc;
                UChar32 uc;
                int32_t chars = 0;
                for (;;) {
                    int32_t pcIndex = utext_getNativeIndex(text);
                    pc = utext_next32(text);
                    int32_t pcSize = utext_getNativeIndex(text) - pcIndex;
                    chars += pcSize;
                    remaining -= pcSize;
                    if (remaining <= 0) {
                        break;
                    }
                    uc = utext_current32(text);
                    if (fEndWordSet.contains(pc) && fBeginWordSet.contains(uc)) {
                        
                        int32_t candidates = words[(wordsFound + 1) % KHMER_LOOKAHEAD].candidates(text, fDictionary, rangeEnd);
                        utext_setNativeIndex(text, current+cuWordLength+chars);
                        if (candidates > 0) {
                            break;
                        }
                    }
                }

                
                if (cuWordLength <= 0) {
                    wordsFound += 1;
                }

                
                cuWordLength += chars;
            }
            else {
                
                utext_setNativeIndex(text, current+cuWordLength);
            }
        }

        
        int32_t currPos;
        while ((currPos = (int32_t)utext_getNativeIndex(text)) < rangeEnd && fMarkSet.contains(utext_current32(text))) {
            utext_next32(text);
            cuWordLength += (int32_t)utext_getNativeIndex(text) - currPos;
        }

        
        
        
        


































        
        if (cuWordLength > 0) {
            foundBreaks.push((current+cuWordLength), status);
        }
    }
    
    
    if (foundBreaks.peeki() >= rangeEnd) {
        (void) foundBreaks.popi();
        wordsFound -= 1;
    }

    return wordsFound;
}

#if !UCONFIG_NO_NORMALIZATION




static const uint32_t kuint32max = 0xFFFFFFFF;
CjkBreakEngine::CjkBreakEngine(DictionaryMatcher *adoptDictionary, LanguageType type, UErrorCode &status)
: DictionaryBreakEngine(1 << UBRK_WORD), fDictionary(adoptDictionary) {
    
    fHangulWordSet.applyPattern(UNICODE_STRING_SIMPLE("[\\uac00-\\ud7a3]"), status);
    fHanWordSet.applyPattern(UNICODE_STRING_SIMPLE("[:Han:]"), status);
    fKatakanaWordSet.applyPattern(UNICODE_STRING_SIMPLE("[[:Katakana:]\\uff9e\\uff9f]"), status);
    fHiraganaWordSet.applyPattern(UNICODE_STRING_SIMPLE("[:Hiragana:]"), status);
    nfkcNorm2 = Normalizer2::getNFKCInstance(status);

    if (U_SUCCESS(status)) {
        
        if (type == kKorean) {
            setCharacters(fHangulWordSet);
        } else { 
            UnicodeSet cjSet;
            cjSet.addAll(fHanWordSet);
            cjSet.addAll(fKatakanaWordSet);
            cjSet.addAll(fHiraganaWordSet);
            cjSet.add(0xFF70); 
            cjSet.add(0x30FC); 
            setCharacters(cjSet);
        }
    }
}

CjkBreakEngine::~CjkBreakEngine(){
    delete fDictionary;
}



static const int32_t kMaxKatakanaLength = 8;
static const int32_t kMaxKatakanaGroupLength = 20;
static const uint32_t maxSnlp = 255;

static inline uint32_t getKatakanaCost(int32_t wordLength){
    
    static const uint32_t katakanaCost[kMaxKatakanaLength + 1]
                                       = {8192, 984, 408, 240, 204, 252, 300, 372, 480};
    return (wordLength > kMaxKatakanaLength) ? 8192 : katakanaCost[wordLength];
}

static inline bool isKatakana(uint16_t value) {
    return (value >= 0x30A1u && value <= 0x30FEu && value != 0x30FBu) ||
            (value >= 0xFF66u && value <= 0xFF9fu);
}





static inline int32_t utext_i32_flag(int32_t bitIndex) {
    return (int32_t)1 << bitIndex;
}

       







int32_t 
CjkBreakEngine::divideUpDictionaryRange( UText *inText,
        int32_t rangeStart,
        int32_t rangeEnd,
        UStack &foundBreaks ) const {
    if (rangeStart >= rangeEnd) {
        return 0;
    }

    
    UnicodeString *inString;

    
    
    UVector32     *inputMap    = NULL;

    UErrorCode     status      = U_ZERO_ERROR;


    
    if ((inText->providerProperties & utext_i32_flag(UTEXT_PROVIDER_STABLE_CHUNKS)) &&
         inText->chunkNativeStart <= rangeStart &&
         inText->chunkNativeLimit >= rangeEnd   &&
         inText->nativeIndexingLimit >= rangeEnd - inText->chunkNativeStart) {
         
        
        
        inString = new UnicodeString(FALSE, 
                              inText->chunkContents + rangeStart - inText->chunkNativeStart,
                              rangeEnd - rangeStart);
    } else {
        
        
        utext_setNativeIndex(inText, rangeStart);
        int32_t limit = rangeEnd;
        U_ASSERT(limit <= utext_nativeLength(inText));
        if (limit > utext_nativeLength(inText)) {
            limit = utext_nativeLength(inText);
        }
        inString = new UnicodeString;
        inputMap = new UVector32(status);
        while (utext_getNativeIndex(inText) < limit) {
            int32_t nativePosition = utext_getNativeIndex(inText);
            UChar32 c = utext_next32(inText);
            U_ASSERT(c != U_SENTINEL);
            inString->append(c);
            while (inputMap->size() < inString->length()) {
                inputMap->addElement(nativePosition, status);
            }
        }
        inputMap->addElement(limit, status);
    }


    if (!nfkcNorm2->isNormalized(*inString, status)) {
        UnicodeString *normalizedInput = new UnicodeString();   
        
        UVector32 *normalizedMap = new UVector32(status);
        if (U_FAILURE(status)) {
            return 0;
        }
        
        UnicodeString fragment;
        UnicodeString normalizedFragment;
        for (int32_t srcI = 0; srcI < inString->length();) {                 
            fragment.remove();
            int32_t fragmentStartI = srcI;
            UChar32 c = inString->char32At(srcI);
            for (;;) {
                fragment.append(c);
                srcI = inString->moveIndex32(srcI, 1);
                if (srcI == inString->length()) {
                    break;
                }
                c = inString->char32At(srcI);
                if (nfkcNorm2->hasBoundaryBefore(c)) {
                    break;
                }
            }
            nfkcNorm2->normalize(fragment, normalizedFragment, status);
            normalizedInput->append(normalizedFragment);

            
            
            int32_t fragmentOriginalStart = inputMap? inputMap->elementAti(fragmentStartI) : fragmentStartI+rangeStart;
            while (normalizedMap->size() < normalizedInput->length()) {
                normalizedMap->addElement(fragmentOriginalStart, status);
                if (U_FAILURE(status)) {
                    break;
                }
            }
        }
        U_ASSERT(normalizedMap->size() == normalizedInput->length());
        int32_t nativeEnd = inputMap? inputMap->elementAti(inString->length()) : inString->length()+rangeStart;
        normalizedMap->addElement(nativeEnd, status);

        delete inputMap;
        inputMap = normalizedMap;
        delete inString;
        inString = normalizedInput;
    }

    int32_t numCodePts = inString->countChar32();
    if (numCodePts != inString->length()) {
        
        
        
        
        
        UBool hadExistingMap = (inputMap != NULL);
        if (!hadExistingMap) {
            inputMap = new UVector32(status);
        }
        int32_t cpIdx = 0;
        for (int32_t cuIdx = 0; ; cuIdx = inString->moveIndex32(cuIdx, 1)) {
            U_ASSERT(cuIdx >= cpIdx);
            if (hadExistingMap) {
                inputMap->setElementAt(inputMap->elementAti(cuIdx), cpIdx);
            } else {
                inputMap->addElement(cuIdx+rangeStart, status);
            }
            cpIdx++;
            if (cuIdx == inString->length()) {
               break;
            }
        }
    }
                
    
    
    UVector32 bestSnlp(numCodePts + 1, status);
    bestSnlp.addElement(0, status);
    for(int32_t i = 1; i <= numCodePts; i++) {
        bestSnlp.addElement(kuint32max, status);
    }


    
    
    UVector32 prev(numCodePts + 1, status);
    for(int32_t i = 0; i <= numCodePts; i++){
        prev.addElement(-1, status);
    }

    const int32_t maxWordSize = 20;
    UVector32 values(numCodePts, status);
    values.setSize(numCodePts);
    UVector32 lengths(numCodePts, status);
    lengths.setSize(numCodePts);

    UText fu = UTEXT_INITIALIZER;
    utext_openUnicodeString(&fu, inString, &status);

    

    
    
    
    int32_t ix = 0;
    for (int32_t i = 0;  i < numCodePts;  ++i, ix = inString->moveIndex32(ix, 1)) {
        if ((uint32_t)bestSnlp.elementAti(i) == kuint32max) {
            continue;
        }

        int32_t count;
        utext_setNativeIndex(&fu, ix);
        count = fDictionary->matches(&fu, maxWordSize, numCodePts,
                             NULL, lengths.getBuffer(), values.getBuffer(), NULL);
                             
                             

        
        
        
        
        
        if ((count == 0 || lengths.elementAti(0) != 1) &&
                !fHangulWordSet.contains(inString->char32At(ix))) {
            values.setElementAt(maxSnlp, count);   
            lengths.setElementAt(1, count++);
        }

        for (int32_t j = 0; j < count; j++) {
            uint32_t newSnlp = (uint32_t)bestSnlp.elementAti(i) + (uint32_t)values.elementAti(j);
            int32_t ln_j_i = lengths.elementAti(j) + i;
            if (newSnlp < (uint32_t)bestSnlp.elementAti(ln_j_i)) {
                bestSnlp.setElementAt(newSnlp, ln_j_i);
                prev.setElementAt(i, ln_j_i);
            }
        }

        
        
        
        
        

        bool is_prev_katakana = false;
        bool is_katakana = isKatakana(inString->char32At(ix));
        int32_t katakanaRunLength = 1;
        if (!is_prev_katakana && is_katakana) {
            int32_t j = inString->moveIndex32(ix, 1);
            
            while (j < inString->length() && katakanaRunLength < kMaxKatakanaGroupLength &&
                    isKatakana(inString->char32At(j))) {
                j = inString->moveIndex32(j, 1);
                katakanaRunLength++;
            }
            if (katakanaRunLength < kMaxKatakanaGroupLength) {
                uint32_t newSnlp = bestSnlp.elementAti(i) + getKatakanaCost(katakanaRunLength);
                if (newSnlp < (uint32_t)bestSnlp.elementAti(j)) {
                    bestSnlp.setElementAt(newSnlp, j);
                    prev.setElementAt(i, i+katakanaRunLength);  
                }
            }
        }
        is_prev_katakana = is_katakana;
    }
    utext_close(&fu);

    
    
    
    
    UVector32 t_boundary(numCodePts+1, status);

    int32_t numBreaks = 0;
    
    if ((uint32_t)bestSnlp.elementAti(numCodePts) == kuint32max) {
        t_boundary.addElement(numCodePts, status);
        numBreaks++;
    } else {
        for (int32_t i = numCodePts; i > 0; i = prev.elementAti(i)) {
            t_boundary.addElement(i, status);
            numBreaks++;
        }
        U_ASSERT(prev.elementAti(t_boundary.elementAti(numBreaks - 1)) == 0);
    }

    
    
    if (foundBreaks.size() == 0 || foundBreaks.peeki() < rangeStart) {
        t_boundary.addElement(0, status);
        numBreaks++;
    }

    
    
    
    for (int32_t i = numBreaks-1; i >= 0; i--) {
        int32_t cpPos = t_boundary.elementAti(i);
        int32_t utextPos =  inputMap ? inputMap->elementAti(cpPos) : cpPos + rangeStart;
        
        U_ASSERT(foundBreaks.size() == 0 ||foundBreaks.peeki() < utextPos);
        foundBreaks.push(utextPos, status);
    }

    delete inString;
    delete inputMap;
    return numBreaks;
}
#endif

U_NAMESPACE_END

#endif 

