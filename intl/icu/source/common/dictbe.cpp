






#include "unicode/utypes.h"

#if !UCONFIG_NO_BREAK_ITERATION

#include "brkeng.h"
#include "dictbe.h"
#include "unicode/uniset.h"
#include "unicode/chariter.h"
#include "unicode/ubrk.h"
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
        rangeStart = (current < startPos) ? startPos : current+(isDict ? 0 : 1);
        rangeEnd = start + 1;
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











#define POSSIBLE_WORD_LIST_MAX 20

class PossibleWord {
private:
    
    int32_t   lengths[POSSIBLE_WORD_LIST_MAX];
    int32_t   count;      
    int32_t   prefix;     
    int32_t   offset;     
    int       mark;       
    int       current;    

public:
    PossibleWord();
    ~PossibleWord();
  
    
    int       candidates( UText *text, DictionaryMatcher *dict, int32_t rangeEnd );
  
    
    int32_t   acceptMarked( UText *text );
  
    
    
    UBool     backUp( UText *text );
  
    
    int32_t   longestPrefix();
  
    
    void      markCurrent();
};

inline
PossibleWord::PossibleWord() {
    offset = -1;
}

inline
PossibleWord::~PossibleWord() {
}

inline int
PossibleWord::candidates( UText *text, DictionaryMatcher *dict, int32_t rangeEnd ) {
    
    int32_t start = (int32_t)utext_getNativeIndex(text);
    if (start != offset) {
        offset = start;
        prefix = dict->matches(text, rangeEnd-start, lengths, count, sizeof(lengths)/sizeof(lengths[0]));
        
        if (count <= 0) {
            utext_setNativeIndex(text, start);
        }
    }
    if (count > 0) {
        utext_setNativeIndex(text, start+lengths[count-1]);
    }
    current = count-1;
    mark = current;
    return count;
}

inline int32_t
PossibleWord::acceptMarked( UText *text ) {
    utext_setNativeIndex(text, offset + lengths[mark]);
    return lengths[mark];
}

inline UBool
PossibleWord::backUp( UText *text ) {
    if (current > 0) {
        utext_setNativeIndex(text, offset + lengths[--current]);
        return TRUE;
    }
    return FALSE;
}

inline int32_t
PossibleWord::longestPrefix() {
    return prefix;
}

inline void
PossibleWord::markCurrent() {
    mark = current;
}


#define THAI_LOOKAHEAD 3


#define THAI_ROOT_COMBINE_THRESHOLD 3



#define THAI_PREFIX_COMBINE_THRESHOLD 3


#define THAI_PAIYANNOI 0x0E2F


#define THAI_MAIYAMOK 0x0E46


#define THAI_MIN_WORD 2


#define THAI_MIN_WORD_SPAN (THAI_MIN_WORD * 2)

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
    if ((rangeEnd - rangeStart) < THAI_MIN_WORD_SPAN) {
        return 0;       
    }

    uint32_t wordsFound = 0;
    int32_t wordLength;
    int32_t current;
    UErrorCode status = U_ZERO_ERROR;
    PossibleWord words[THAI_LOOKAHEAD];
    UChar32 uc;
    
    utext_setNativeIndex(text, rangeStart);
    
    while (U_SUCCESS(status) && (current = (int32_t)utext_getNativeIndex(text)) < rangeEnd) {
        wordLength = 0;

        
        int candidates = words[wordsFound%THAI_LOOKAHEAD].candidates(text, fDictionary, rangeEnd);
        
        
        if (candidates == 1) {
            wordLength = words[wordsFound % THAI_LOOKAHEAD].acceptMarked(text);
            wordsFound += 1;
        }
        
        else if (candidates > 1) {
            
            if ((int32_t)utext_getNativeIndex(text) >= rangeEnd) {
                goto foundBest;
            }
            do {
                int wordsMatched = 1;
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
            wordLength = words[wordsFound % THAI_LOOKAHEAD].acceptMarked(text);
            wordsFound += 1;
        }
        
        
        
        
        
        
        if ((int32_t)utext_getNativeIndex(text) < rangeEnd && wordLength < THAI_ROOT_COMBINE_THRESHOLD) {
            
            
            
            if (words[wordsFound % THAI_LOOKAHEAD].candidates(text, fDictionary, rangeEnd) <= 0
                  && (wordLength == 0
                      || words[wordsFound%THAI_LOOKAHEAD].longestPrefix() < THAI_PREFIX_COMBINE_THRESHOLD)) {
                
                
                int32_t remaining = rangeEnd - (current+wordLength);
                UChar32 pc = utext_current32(text);
                int32_t chars = 0;
                for (;;) {
                    utext_next32(text);
                    uc = utext_current32(text);
                    
                    
                    chars += 1;
                    if (--remaining <= 0) {
                        break;
                    }
                    if (fEndWordSet.contains(pc) && fBeginWordSet.contains(uc)) {
                        
                        
                        
                        
                        
                        int candidates = words[(wordsFound + 1) % THAI_LOOKAHEAD].candidates(text, fDictionary, rangeEnd);
                        utext_setNativeIndex(text, current + wordLength + chars);
                        if (candidates > 0) {
                            break;
                        }
                    }
                    pc = uc;
                }
                
                
                if (wordLength <= 0) {
                    wordsFound += 1;
                }
                
                
                wordLength += chars;
            }
            else {
                
                utext_setNativeIndex(text, current+wordLength);
            }
        }
        
        
        int32_t currPos;
        while ((currPos = (int32_t)utext_getNativeIndex(text)) < rangeEnd && fMarkSet.contains(utext_current32(text))) {
            utext_next32(text);
            wordLength += (int32_t)utext_getNativeIndex(text) - currPos;
        }
        
        
        
        
        
        if ((int32_t)utext_getNativeIndex(text) < rangeEnd && wordLength > 0) {
            if (words[wordsFound%THAI_LOOKAHEAD].candidates(text, fDictionary, rangeEnd) <= 0
                && fSuffixSet.contains(uc = utext_current32(text))) {
                if (uc == THAI_PAIYANNOI) {
                    if (!fSuffixSet.contains(utext_previous32(text))) {
                        
                        utext_next32(text);
                        utext_next32(text);
                        wordLength += 1;            
                        uc = utext_current32(text);     
                    }
                    else {
                        
                        utext_next32(text);
                    }
                }
                if (uc == THAI_MAIYAMOK) {
                    if (utext_previous32(text) != THAI_MAIYAMOK) {
                        
                        utext_next32(text);
                        utext_next32(text);
                        wordLength += 1;            
                    }
                    else {
                        
                        utext_next32(text);
                    }
                }
            }
            else {
                utext_setNativeIndex(text, current+wordLength);
            }
        }

        
        if (wordLength > 0) {
            foundBreaks.push((current+wordLength), status);
        }
    }

    
    if (foundBreaks.peeki() >= rangeEnd) {
        (void) foundBreaks.popi();
        wordsFound -= 1;
    }

    return wordsFound;
}


#define KHMER_LOOKAHEAD 3


#define KHMER_ROOT_COMBINE_THRESHOLD 3



#define KHMER_PREFIX_COMBINE_THRESHOLD 3


#define KHMER_MIN_WORD 2


#define KHMER_MIN_WORD_SPAN (KHMER_MIN_WORD * 2)

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
    int32_t wordLength;
    int32_t current;
    UErrorCode status = U_ZERO_ERROR;
    PossibleWord words[KHMER_LOOKAHEAD];
    UChar32 uc;

    utext_setNativeIndex(text, rangeStart);

    while (U_SUCCESS(status) && (current = (int32_t)utext_getNativeIndex(text)) < rangeEnd) {
        wordLength = 0;

        
        int candidates = words[wordsFound%KHMER_LOOKAHEAD].candidates(text, fDictionary, rangeEnd);

        
        if (candidates == 1) {
            wordLength = words[wordsFound%KHMER_LOOKAHEAD].acceptMarked(text);
            wordsFound += 1;
        }

        
        else if (candidates > 1) {
            
            if ((int32_t)utext_getNativeIndex(text) >= rangeEnd) {
                goto foundBest;
            }
            do {
                int wordsMatched = 1;
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
            wordLength = words[wordsFound % KHMER_LOOKAHEAD].acceptMarked(text);
            wordsFound += 1;
        }

        
        
        
        
        
        if ((int32_t)utext_getNativeIndex(text) < rangeEnd && wordLength < KHMER_ROOT_COMBINE_THRESHOLD) {
            
            
            
            if (words[wordsFound % KHMER_LOOKAHEAD].candidates(text, fDictionary, rangeEnd) <= 0
                  && (wordLength == 0
                      || words[wordsFound % KHMER_LOOKAHEAD].longestPrefix() < KHMER_PREFIX_COMBINE_THRESHOLD)) {
                
                
                int32_t remaining = rangeEnd - (current+wordLength);
                UChar32 pc = utext_current32(text);
                int32_t chars = 0;
                for (;;) {
                    utext_next32(text);
                    uc = utext_current32(text);
                    
                    
                    chars += 1;
                    if (--remaining <= 0) {
                        break;
                    }
                    if (fEndWordSet.contains(pc) && fBeginWordSet.contains(uc)) {
                        
                        int candidates = words[(wordsFound + 1) % KHMER_LOOKAHEAD].candidates(text, fDictionary, rangeEnd);
                        utext_setNativeIndex(text, current+wordLength+chars);
                        if (candidates > 0) {
                            break;
                        }
                    }
                    pc = uc;
                }

                
                if (wordLength <= 0) {
                    wordsFound += 1;
                }

                
                wordLength += chars;
            }
            else {
                
                utext_setNativeIndex(text, current+wordLength);
            }
        }

        
        int32_t currPos;
        while ((currPos = (int32_t)utext_getNativeIndex(text)) < rangeEnd && fMarkSet.contains(utext_current32(text))) {
            utext_next32(text);
            wordLength += (int32_t)utext_getNativeIndex(text) - currPos;
        }

        
        
        
        


































        
        if (wordLength > 0) {
            foundBreaks.push((current+wordLength), status);
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

    if (U_SUCCESS(status)) {
        
        if (type == kKorean) {
            setCharacters(fHangulWordSet);
        } else { 
            UnicodeSet cjSet;
            cjSet.addAll(fHanWordSet);
            cjSet.addAll(fKatakanaWordSet);
            cjSet.addAll(fHiraganaWordSet);
            cjSet.add(UNICODE_STRING_SIMPLE("\\uff70\\u30fc"));
            setCharacters(cjSet);
        }
    }
}

CjkBreakEngine::~CjkBreakEngine(){
    delete fDictionary;
}



static const int kMaxKatakanaLength = 8;
static const int kMaxKatakanaGroupLength = 20;
static const uint32_t maxSnlp = 255;

static inline uint32_t getKatakanaCost(int wordLength){
    
    static const uint32_t katakanaCost[kMaxKatakanaLength + 1]
                                       = {8192, 984, 408, 240, 204, 252, 300, 372, 480};
    return (wordLength > kMaxKatakanaLength) ? 8192 : katakanaCost[wordLength];
}

static inline bool isKatakana(uint16_t value) {
    return (value >= 0x30A1u && value <= 0x30FEu && value != 0x30FBu) ||
            (value >= 0xFF66u && value <= 0xFF9fu);
}



template<class T, size_t N>
class AutoBuffer {
public:
    AutoBuffer(size_t size) : buffer(stackBuffer), capacity(N) {
        if (size > N) {
            buffer = reinterpret_cast<T*>(uprv_malloc(sizeof(T)*size));
            capacity = size;
        }
    }
    ~AutoBuffer() {
        if (buffer != stackBuffer)
            uprv_free(buffer);
    }

    T* elems() {
        return buffer;
    }

    const T& operator[] (size_t i) const {
        return buffer[i];
    }

    T& operator[] (size_t i) {
        return buffer[i];
    }

    
    void resize(size_t size) {
        if (size <= capacity)
            return;
        if (buffer != stackBuffer)
            uprv_free(buffer);
        buffer = reinterpret_cast<T*>(uprv_malloc(sizeof(T)*size));
        capacity = size;
    }

private:
    T stackBuffer[N];
    T* buffer;
    AutoBuffer();
    size_t capacity;
};









int32_t 
CjkBreakEngine::divideUpDictionaryRange( UText *text,
        int32_t rangeStart,
        int32_t rangeEnd,
        UStack &foundBreaks ) const {
    if (rangeStart >= rangeEnd) {
        return 0;
    }

    const size_t defaultInputLength = 80;
    size_t inputLength = rangeEnd - rangeStart;
    
    AutoBuffer<UChar, defaultInputLength> charString(inputLength);

    
    
    
    UErrorCode status = U_ZERO_ERROR;
    utext_extract(text, rangeStart, rangeEnd, charString.elems(), inputLength, &status);
    if (U_FAILURE(status)) {
        return 0;
    }

    UnicodeString inputString(charString.elems(), inputLength);
    
    UNormalizationMode norm_mode = UNORM_NFKC;
    UBool isNormalized =
        Normalizer::quickCheck(inputString, norm_mode, status) == UNORM_YES ||
        Normalizer::isNormalized(inputString, norm_mode, status);

    
    AutoBuffer<int32_t, defaultInputLength> charPositions(inputLength + 1);
    int numChars = 0;
    UText normalizedText = UTEXT_INITIALIZER;
    
    UnicodeString normalizedString;
    if (isNormalized) {
        int32_t index = 0;
        charPositions[0] = 0;
        while(index < inputString.length()) {
            index = inputString.moveIndex32(index, 1);
            charPositions[++numChars] = index;
        }
        utext_openUnicodeString(&normalizedText, &inputString, &status);
    }
    else {
        Normalizer::normalize(inputString, norm_mode, 0, normalizedString, status);
        if (U_FAILURE(status)) {
            return 0;
        }
        charPositions.resize(normalizedString.length() + 1);
        Normalizer normalizer(charString.elems(), inputLength, norm_mode);
        int32_t index = 0;
        charPositions[0] = 0;
        while(index < normalizer.endIndex()){
             normalizer.next();
            charPositions[++numChars] = index = normalizer.getIndex();
        }
        utext_openUnicodeString(&normalizedText, &normalizedString, &status);
    }

    if (U_FAILURE(status)) {
        return 0;
    }

    
    

    
    
    
    AutoBuffer<uint32_t, defaultInputLength> bestSnlp(numChars + 1);
    bestSnlp[0] = 0;
    for(int i = 1; i <= numChars; i++) {
        bestSnlp[i] = kuint32max;
    }

    
    
    
    AutoBuffer<int, defaultInputLength> prev(numChars + 1);
    for(int i = 0; i <= numChars; i++){
        prev[i] = -1;
    }

    const size_t maxWordSize = 20;
    
    AutoBuffer<int32_t, maxWordSize> values(numChars);
    AutoBuffer<int32_t, maxWordSize> lengths(numChars);

    
    bool is_prev_katakana = false;
    for (int32_t i = 0; i < numChars; ++i) {
        
        utext_setNativeIndex(&normalizedText, i);
        if (bestSnlp[i] == kuint32max)
            continue;

        int32_t count;
        
        int32_t maxSearchLength = (i + maxWordSize < (size_t) numChars)? maxWordSize : (numChars - i);

        fDictionary->matches(&normalizedText, maxSearchLength, lengths.elems(), count, maxSearchLength, values.elems());

        
        
        
        
        
        if((count == 0 || lengths[0] != 1) &&
                !fHangulWordSet.contains(utext_current32(&normalizedText))) {
            values[count] = maxSnlp;
            lengths[count++] = 1;
        }

        for (int j = 0; j < count; j++) {
            uint32_t newSnlp = bestSnlp[i] + values[j];
            if (newSnlp < bestSnlp[lengths[j] + i]) {
                bestSnlp[lengths[j] + i] = newSnlp;
                prev[lengths[j] + i] = i;
            }
        }

        
        
        
        
        
        
        utext_setNativeIndex(&normalizedText, i);
        bool is_katakana = isKatakana(utext_current32(&normalizedText));
        if (!is_prev_katakana && is_katakana) {
            int j = i + 1;
            utext_next32(&normalizedText);
            
            while (j < numChars && (j - i) < kMaxKatakanaGroupLength &&
                    isKatakana(utext_current32(&normalizedText))) {
                utext_next32(&normalizedText);
                ++j;
            }
            if ((j - i) < kMaxKatakanaGroupLength) {
                uint32_t newSnlp = bestSnlp[i] + getKatakanaCost(j - i);
                if (newSnlp < bestSnlp[j]) {
                    bestSnlp[j] = newSnlp;
                    prev[j] = i;
                }
            }
        }
        is_prev_katakana = is_katakana;
    }

    
    
    
    
    
    AutoBuffer<int, maxWordSize> t_boundary(numChars + 1);

    int numBreaks = 0;
    
    if (bestSnlp[numChars] == kuint32max) {
        t_boundary[numBreaks++] = numChars;
    } else {
        for (int i = numChars; i > 0; i = prev[i]) {
            t_boundary[numBreaks++] = i;
        }
        U_ASSERT(prev[t_boundary[numBreaks - 1]] == 0);
    }

    
    
    
    if (foundBreaks.size() == 0 || foundBreaks.peeki() < rangeStart) {
        t_boundary[numBreaks++] = 0;
    }

    
    
    
    for (int i = numBreaks-1; i >= 0; i--) {
        foundBreaks.push(charPositions[t_boundary[i]] + rangeStart, status);
    }

    utext_close(&normalizedText);
    return numBreaks;
}
#endif

U_NAMESPACE_END

#endif 

