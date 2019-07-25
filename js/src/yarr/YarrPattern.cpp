





























#include "YarrPattern.h"

#include "Yarr.h"
#include "YarrParser.h"

using namespace WTF;

namespace JSC { namespace Yarr {

#include "RegExpJitTables.h"

class CharacterClassConstructor {
public:
    CharacterClassConstructor(bool isCaseInsensitive = false)
        : m_isCaseInsensitive(isCaseInsensitive)
    {
    }
    
    void reset()
    {
        m_matches.clear();
        m_ranges.clear();
        m_matchesUnicode.clear();
        m_rangesUnicode.clear();
    }

    void append(const CharacterClass* other)
    {
        for (size_t i = 0; i < other->m_matches.size(); ++i)
            addSorted(m_matches, other->m_matches[i]);
        for (size_t i = 0; i < other->m_ranges.size(); ++i)
            addSortedRange(m_ranges, other->m_ranges[i].begin, other->m_ranges[i].end);
        for (size_t i = 0; i < other->m_matchesUnicode.size(); ++i)
            addSorted(m_matchesUnicode, other->m_matchesUnicode[i]);
        for (size_t i = 0; i < other->m_rangesUnicode.size(); ++i)
            addSortedRange(m_rangesUnicode, other->m_rangesUnicode[i].begin, other->m_rangesUnicode[i].end);
    }

    void putChar(UChar ch)
    {
        if (ch <= 0x7f) {
            if (m_isCaseInsensitive && isASCIIAlpha(ch)) {
                addSorted(m_matches, toASCIIUpper(ch));
                addSorted(m_matches, toASCIILower(ch));
            } else
                addSorted(m_matches, ch);
        } else {
            UChar upper, lower;
            if (m_isCaseInsensitive && ((upper = Unicode::toUpper(ch)) != (lower = Unicode::toLower(ch)))) {
                addSorted(m_matchesUnicode, upper);
                addSorted(m_matchesUnicode, lower);
            } else
                addSorted(m_matchesUnicode, ch);
        }
    }

    
    static inline bool isUnicodeUpper(UChar ch)
    {
        return ch != Unicode::toLower(ch);
    }

    
    static inline bool isUnicodeLower(UChar ch)
    {
        return ch != Unicode::toUpper(ch);
    }

    void putRange(UChar lo, UChar hi)
    {
        if (lo <= 0x7f) {
            char asciiLo = lo;
            char asciiHi = std::min(hi, (UChar)0x7f);
            addSortedRange(m_ranges, lo, asciiHi);
            
            if (m_isCaseInsensitive) {
                if ((asciiLo <= 'Z') && (asciiHi >= 'A'))
                    addSortedRange(m_ranges, std::max(asciiLo, 'A')+('a'-'A'), std::min(asciiHi, 'Z')+('a'-'A'));
                if ((asciiLo <= 'z') && (asciiHi >= 'a'))
                    addSortedRange(m_ranges, std::max(asciiLo, 'a')+('A'-'a'), std::min(asciiHi, 'z')+('A'-'a'));
            }
        }
        if (hi >= 0x80) {
            uint32_t unicodeCurr = std::max(lo, (UChar)0x80);
            addSortedRange(m_rangesUnicode, unicodeCurr, hi);
            
            if (m_isCaseInsensitive) {
                while (unicodeCurr <= hi) {
                    
                    
                    
                    
                    
                    ASSERT(unicodeCurr <= 0xffff);

                    if (isUnicodeUpper(unicodeCurr)) {
                        UChar lowerCaseRangeBegin = Unicode::toLower(unicodeCurr);
                        UChar lowerCaseRangeEnd = lowerCaseRangeBegin;
                        while ((++unicodeCurr <= hi) && isUnicodeUpper(unicodeCurr) && (Unicode::toLower(unicodeCurr) == (lowerCaseRangeEnd + 1)))
                            lowerCaseRangeEnd++;
                        addSortedRange(m_rangesUnicode, lowerCaseRangeBegin, lowerCaseRangeEnd);
                    } else if (isUnicodeLower(unicodeCurr)) {
                        UChar upperCaseRangeBegin = Unicode::toUpper(unicodeCurr);
                        UChar upperCaseRangeEnd = upperCaseRangeBegin;
                        while ((++unicodeCurr <= hi) && isUnicodeLower(unicodeCurr) && (Unicode::toUpper(unicodeCurr) == (upperCaseRangeEnd + 1)))
                            upperCaseRangeEnd++;
                        addSortedRange(m_rangesUnicode, upperCaseRangeBegin, upperCaseRangeEnd);
                    } else
                        ++unicodeCurr;
                }
            }
        }
    }

    CharacterClass* charClass()
    {
        CharacterClass* characterClass = js::OffTheBooks::new_<CharacterClass>(PassRefPtr<CharacterClassTable>(0));

        characterClass->m_matches.append(m_matches);
        characterClass->m_ranges.append(m_ranges);
        characterClass->m_matchesUnicode.append(m_matchesUnicode);
        characterClass->m_rangesUnicode.append(m_rangesUnicode);

        reset();

        return characterClass;
    }

private:
    void addSorted(Vector<UChar>& matches, UChar ch)
    {
        unsigned pos = 0;
        unsigned range = matches.size();

        
        while (range) {
            unsigned index = range >> 1;

            int val = matches[pos+index] - ch;
            if (!val)
                return;
            else if (val > 0)
                range = index;
            else {
                pos += (index+1);
                range -= (index+1);
            }
        }
        
        if (pos == matches.size())
            matches.append(ch);
        else
            matches.insert(pos, ch);
    }

    void addSortedRange(Vector<CharacterRange>& ranges, UChar lo, UChar hi)
    {
        unsigned end = ranges.size();
        
        
        
        for (unsigned i = 0; i < end; ++i) {
            
            if (hi < ranges[i].begin) {
                
                if (hi == (ranges[i].begin - 1)) {
                    ranges[i].begin = lo;
                    return;
                }
                ranges.insert(i, CharacterRange(lo, hi));
                return;
            }
            
            
            
            if (lo <= (ranges[i].end + 1)) {
                
                ranges[i].begin = std::min(ranges[i].begin, lo);
                ranges[i].end = std::max(ranges[i].end, hi);

                
                unsigned next = i+1;
                
                while (next < ranges.size()) {
                    if (ranges[next].begin <= (ranges[i].end + 1)) {
                        
                        ranges[i].end = std::max(ranges[i].end, ranges[next].end);
                        ranges.remove(next);
                    } else
                        break;
                }
                
                return;
            }
        }

        
        ranges.append(CharacterRange(lo, hi));
    }

    bool m_isCaseInsensitive;

    Vector<UChar> m_matches;
    Vector<CharacterRange> m_ranges;
    Vector<UChar> m_matchesUnicode;
    Vector<CharacterRange> m_rangesUnicode;
};

struct BeginCharHelper {
    BeginCharHelper(Vector<BeginChar>* beginChars, bool isCaseInsensitive = false)
        : m_beginChars(beginChars)
        , m_isCaseInsensitive(isCaseInsensitive)
    {}

    void addBeginChar(BeginChar beginChar, Vector<TermChain>* hotTerms, QuantifierType quantityType, unsigned quantityCount)
    {
        if (quantityType == QuantifierFixedCount && quantityCount > 1) {
            
            beginChar.value |= beginChar.value << 16;
            beginChar.mask |= beginChar.mask << 16;
            addCharacter(beginChar);
        } else if (quantityType == QuantifierFixedCount && quantityCount == 1 && hotTerms->size())
            
            linkHotTerms(beginChar, hotTerms);
        else
            
            
            addCharacter(beginChar);
    }

    
    void merge(unsigned size)
    {
        for (unsigned i = 0; i < size; i++) {
            BeginChar* curr = &m_beginChars->at(i);
            BeginChar* next = &m_beginChars->at(i + 1);

            
            
            if (curr->value <= 0xFFFF && next->value > 0xFFFF)
                continue;

            unsigned diff = curr->value ^ next->value;

            curr->mask |= diff;
            curr->value |= curr->mask;

            m_beginChars->remove(i + 1);
            size--;
        }
    }

private:
    void addCharacter(BeginChar beginChar)
    {
        unsigned pos = 0;
        unsigned range = m_beginChars->size();

        
        while (range) {
            unsigned index = range >> 1;

            int val = m_beginChars->at(pos+index).value - beginChar.value;
            if (!val)
                return;
            if (val < 0)
                range = index;
            else {
                pos += (index+1);
                range -= (index+1);
            }
        }

        if (pos == m_beginChars->size())
            m_beginChars->append(beginChar);
        else
            m_beginChars->insert(pos, beginChar);
    }

    
    void linkHotTerms(BeginChar beginChar, Vector<TermChain>* hotTerms)
    {
        for (unsigned i = 0; i < hotTerms->size(); i++) {
            PatternTerm hotTerm = hotTerms->at(i).term;
            ASSERT(hotTerm.type == PatternTerm::TypePatternCharacter);

            UChar characterNext = hotTerm.patternCharacter;

            
            if (characterNext <= 0x7f) {
                unsigned mask = 0;

                if (m_isCaseInsensitive && isASCIIAlpha(characterNext)) {
                    mask = 32;
                    characterNext = toASCIILower(characterNext);
                }

                addCharacter(BeginChar(beginChar.value | (characterNext << 16), beginChar.mask | (mask << 16)));
            } else {
                UChar upper, lower;
                if (m_isCaseInsensitive && ((upper = Unicode::toUpper(characterNext)) != (lower = Unicode::toLower(characterNext)))) {
                    addCharacter(BeginChar(beginChar.value | (upper << 16), beginChar.mask));
                    addCharacter(BeginChar(beginChar.value | (lower << 16), beginChar.mask));
                } else
                    addCharacter(BeginChar(beginChar.value | (characterNext << 16), beginChar.mask));
            }
        }
    }

    Vector<BeginChar>* m_beginChars;
    bool m_isCaseInsensitive;
};

class YarrPatternConstructor {
public:
    YarrPatternConstructor(YarrPattern& pattern)
        : m_pattern(pattern)
        , m_characterClassConstructor(pattern.m_ignoreCase)
        , m_beginCharHelper(&pattern.m_beginChars, pattern.m_ignoreCase)
        , m_invertParentheticalAssertion(false)
    {
        m_pattern.m_body = js::OffTheBooks::new_<PatternDisjunction>();
        m_alternative = m_pattern.m_body->addNewAlternative();
        m_pattern.m_disjunctions.append(m_pattern.m_body);
    }

    ~YarrPatternConstructor()
    {
    }

    void reset()
    {
        m_pattern.reset();
        m_characterClassConstructor.reset();

        m_pattern.m_body = js::OffTheBooks::new_<PatternDisjunction>();
        m_alternative = m_pattern.m_body->addNewAlternative();
        m_pattern.m_disjunctions.append(m_pattern.m_body);
    }
    
    void assertionBOL()
    {
        if (!m_alternative->m_terms.size() & !m_invertParentheticalAssertion) {
            m_alternative->m_startsWithBOL = true;
            m_alternative->m_containsBOL = true;
            m_pattern.m_containsBOL = true;
        }
        m_alternative->m_terms.append(PatternTerm::BOL());
    }
    void assertionEOL()
    {
        m_alternative->m_terms.append(PatternTerm::EOL());
    }
    void assertionWordBoundary(bool invert)
    {
        m_alternative->m_terms.append(PatternTerm::WordBoundary(invert));
    }

    void atomPatternCharacter(UChar ch)
    {
        
        
        if (m_pattern.m_ignoreCase && !isASCII(ch) && (Unicode::toUpper(ch) != Unicode::toLower(ch))) {
            atomCharacterClassBegin();
            atomCharacterClassAtom(ch);
            atomCharacterClassEnd();
        } else
            m_alternative->m_terms.append(PatternTerm(ch));
    }

    void atomBuiltInCharacterClass(BuiltInCharacterClassID classID, bool invert)
    {
        switch (classID) {
        case DigitClassID:
            m_alternative->m_terms.append(PatternTerm(m_pattern.digitsCharacterClass(), invert));
            break;
        case SpaceClassID:
            m_alternative->m_terms.append(PatternTerm(m_pattern.spacesCharacterClass(), invert));
            break;
        case WordClassID:
            m_alternative->m_terms.append(PatternTerm(m_pattern.wordcharCharacterClass(), invert));
            break;
        case NewlineClassID:
            m_alternative->m_terms.append(PatternTerm(m_pattern.newlineCharacterClass(), invert));
            break;
        }
    }

    void atomCharacterClassBegin(bool invert = false)
    {
        m_invertCharacterClass = invert;
    }

    void atomCharacterClassAtom(UChar ch)
    {
        m_characterClassConstructor.putChar(ch);
    }

    void atomCharacterClassRange(UChar begin, UChar end)
    {
        m_characterClassConstructor.putRange(begin, end);
    }

    void atomCharacterClassBuiltIn(BuiltInCharacterClassID classID, bool invert)
    {
        ASSERT(classID != NewlineClassID);

        switch (classID) {
        case DigitClassID:
            m_characterClassConstructor.append(invert ? m_pattern.nondigitsCharacterClass() : m_pattern.digitsCharacterClass());
            break;
        
        case SpaceClassID:
            m_characterClassConstructor.append(invert ? m_pattern.nonspacesCharacterClass() : m_pattern.spacesCharacterClass());
            break;
        
        case WordClassID:
            m_characterClassConstructor.append(invert ? m_pattern.nonwordcharCharacterClass() : m_pattern.wordcharCharacterClass());
            break;
        
        default:
            ASSERT_NOT_REACHED();
        }
    }

    void atomCharacterClassEnd()
    {
        CharacterClass* newCharacterClass = m_characterClassConstructor.charClass();
        m_pattern.m_userCharacterClasses.append(newCharacterClass);
        m_alternative->m_terms.append(PatternTerm(newCharacterClass, m_invertCharacterClass));
    }

    void atomParenthesesSubpatternBegin(bool capture = true)
    {
        unsigned subpatternId = m_pattern.m_numSubpatterns + 1;
        if (capture)
            m_pattern.m_numSubpatterns++;

        PatternDisjunction* parenthesesDisjunction = js::OffTheBooks::new_<PatternDisjunction>(m_alternative);
        m_pattern.m_disjunctions.append(parenthesesDisjunction);
        m_alternative->m_terms.append(PatternTerm(PatternTerm::TypeParenthesesSubpattern, subpatternId, parenthesesDisjunction, capture, false));
        m_alternative = parenthesesDisjunction->addNewAlternative();
    }

    void atomParentheticalAssertionBegin(bool invert = false)
    {
        PatternDisjunction* parenthesesDisjunction = js::OffTheBooks::new_<PatternDisjunction>(m_alternative);
        m_pattern.m_disjunctions.append(parenthesesDisjunction);
        m_alternative->m_terms.append(PatternTerm(PatternTerm::TypeParentheticalAssertion, m_pattern.m_numSubpatterns + 1, parenthesesDisjunction, false, invert));
        m_alternative = parenthesesDisjunction->addNewAlternative();
        m_invertParentheticalAssertion = invert;
    }

    void atomParenthesesEnd()
    {
        ASSERT(m_alternative->m_parent);
        ASSERT(m_alternative->m_parent->m_parent);

        PatternDisjunction* parenthesesDisjunction = m_alternative->m_parent;
        m_alternative = m_alternative->m_parent->m_parent;

        PatternTerm& lastTerm = m_alternative->lastTerm();

        unsigned numParenAlternatives = parenthesesDisjunction->m_alternatives.size();
        unsigned numBOLAnchoredAlts = 0;

        for (unsigned i = 0; i < numParenAlternatives; i++) {
            
            if (parenthesesDisjunction->m_alternatives[i]->m_startsWithBOL)
                numBOLAnchoredAlts++;
        }

        if (numBOLAnchoredAlts) {
            m_alternative->m_containsBOL = true;
            
            if (numBOLAnchoredAlts == numParenAlternatives)
                m_alternative->m_startsWithBOL = true;
        }

        lastTerm.parentheses.lastSubpatternId = m_pattern.m_numSubpatterns;
        m_invertParentheticalAssertion = false;
    }

    void atomBackReference(unsigned subpatternId)
    {
        ASSERT(subpatternId);
        m_pattern.m_containsBackreferences = true;
        m_pattern.m_maxBackReference = std::max(m_pattern.m_maxBackReference, subpatternId);

        if (subpatternId > m_pattern.m_numSubpatterns) {
            m_alternative->m_terms.append(PatternTerm::ForwardReference());
            return;
        }

        PatternAlternative* currentAlternative = m_alternative;
        ASSERT(currentAlternative);

        
        while ((currentAlternative = currentAlternative->m_parent->m_parent)) {
            PatternTerm& term = currentAlternative->lastTerm();
            ASSERT((term.type == PatternTerm::TypeParenthesesSubpattern) || (term.type == PatternTerm::TypeParentheticalAssertion));

            if ((term.type == PatternTerm::TypeParenthesesSubpattern) && term.capture() && (subpatternId == term.parentheses.subpatternId)) {
                m_alternative->m_terms.append(PatternTerm::ForwardReference());
                return;
            }
        }

        m_alternative->m_terms.append(PatternTerm(subpatternId));
    }

    
    
    PatternDisjunction* copyDisjunction(PatternDisjunction* disjunction, bool filterStartsWithBOL = false)
    {
        PatternDisjunction* newDisjunction = 0;
        for (unsigned alt = 0; alt < disjunction->m_alternatives.size(); ++alt) {
            PatternAlternative* alternative = disjunction->m_alternatives[alt];
            if (!filterStartsWithBOL || !alternative->m_startsWithBOL) {
                if (!newDisjunction) {
                    newDisjunction = js::OffTheBooks::new_<PatternDisjunction>();
                    newDisjunction->m_parent = disjunction->m_parent;
                }
                PatternAlternative* newAlternative = newDisjunction->addNewAlternative();
                for (unsigned i = 0; i < alternative->m_terms.size(); ++i)
                    newAlternative->m_terms.append(copyTerm(alternative->m_terms[i], filterStartsWithBOL));
            }
        }
        
        if (newDisjunction)
            m_pattern.m_disjunctions.append(newDisjunction);
        return newDisjunction;
    }
    
    PatternTerm copyTerm(PatternTerm& term, bool filterStartsWithBOL = false)
    {
        if ((term.type != PatternTerm::TypeParenthesesSubpattern) && (term.type != PatternTerm::TypeParentheticalAssertion))
            return PatternTerm(term);
        
        PatternTerm termCopy = term;
        termCopy.parentheses.disjunction = copyDisjunction(termCopy.parentheses.disjunction, filterStartsWithBOL);
        return termCopy;
    }
    
    void quantifyAtom(unsigned min, unsigned max, bool greedy)
    {
        ASSERT(min <= max);
        ASSERT(m_alternative->m_terms.size());

        if (!max) {
            m_alternative->removeLastTerm();
            return;
        }

        PatternTerm& term = m_alternative->lastTerm();
        ASSERT(term.type > PatternTerm::TypeAssertionWordBoundary);
        ASSERT((term.quantityCount == 1) && (term.quantityType == QuantifierFixedCount));

        
        
        
        
        
        if (term.type == PatternTerm::TypeParentheticalAssertion) {
            if (!min)
                m_alternative->removeLastTerm();
            return;
        }

        if (min == 0)
            term.quantify(max, greedy   ? QuantifierGreedy : QuantifierNonGreedy);
        else if (min == max)
            term.quantify(min, QuantifierFixedCount);
        else {
            term.quantify(min, QuantifierFixedCount);
            m_alternative->m_terms.append(copyTerm(term));
            
            m_alternative->lastTerm().quantify((max == quantifyInfinite) ? max : max - min, greedy ? QuantifierGreedy : QuantifierNonGreedy);
            if (m_alternative->lastTerm().type == PatternTerm::TypeParenthesesSubpattern)
                m_alternative->lastTerm().parentheses.isCopy = true;
        }
    }

    void disjunction()
    {
        m_alternative = m_alternative->m_parent->addNewAlternative();
    }

    unsigned setupAlternativeOffsets(PatternAlternative* alternative, unsigned currentCallFrameSize, unsigned initialInputPosition)
    {
        alternative->m_hasFixedSize = true;
        unsigned currentInputPosition = initialInputPosition;

        for (unsigned i = 0; i < alternative->m_terms.size(); ++i) {
            PatternTerm& term = alternative->m_terms[i];

            switch (term.type) {
            case PatternTerm::TypeAssertionBOL:
            case PatternTerm::TypeAssertionEOL:
            case PatternTerm::TypeAssertionWordBoundary:
                term.inputPosition = currentInputPosition;
                break;

            case PatternTerm::TypeBackReference:
                term.inputPosition = currentInputPosition;
                term.frameLocation = currentCallFrameSize;
                currentCallFrameSize += YarrStackSpaceForBackTrackInfoBackReference;
                alternative->m_hasFixedSize = false;
                break;

            case PatternTerm::TypeForwardReference:
                break;

            case PatternTerm::TypePatternCharacter:
                term.inputPosition = currentInputPosition;
                if (term.quantityType != QuantifierFixedCount) {
                    term.frameLocation = currentCallFrameSize;
                    currentCallFrameSize += YarrStackSpaceForBackTrackInfoPatternCharacter;
                    alternative->m_hasFixedSize = false;
                } else
                    currentInputPosition += term.quantityCount;
                break;

            case PatternTerm::TypeCharacterClass:
                term.inputPosition = currentInputPosition;
                if (term.quantityType != QuantifierFixedCount) {
                    term.frameLocation = currentCallFrameSize;
                    currentCallFrameSize += YarrStackSpaceForBackTrackInfoCharacterClass;
                    alternative->m_hasFixedSize = false;
                } else
                    currentInputPosition += term.quantityCount;
                break;

            case PatternTerm::TypeParenthesesSubpattern:
                
                term.frameLocation = currentCallFrameSize;
                if (term.quantityCount == 1 && !term.parentheses.isCopy) {
                    if (term.quantityType != QuantifierFixedCount)
                        currentCallFrameSize += YarrStackSpaceForBackTrackInfoParenthesesOnce;
                    currentCallFrameSize = setupDisjunctionOffsets(term.parentheses.disjunction, currentCallFrameSize, currentInputPosition);
                    
                    if (term.quantityType == QuantifierFixedCount)
                        currentInputPosition += term.parentheses.disjunction->m_minimumSize;
                    term.inputPosition = currentInputPosition;
                } else if (term.parentheses.isTerminal) {
                    currentCallFrameSize += YarrStackSpaceForBackTrackInfoParenthesesTerminal;
                    currentCallFrameSize = setupDisjunctionOffsets(term.parentheses.disjunction, currentCallFrameSize, currentInputPosition);
                    term.inputPosition = currentInputPosition;
                } else {
                    term.inputPosition = currentInputPosition;
                    setupDisjunctionOffsets(term.parentheses.disjunction, 0, currentInputPosition);
                    currentCallFrameSize += YarrStackSpaceForBackTrackInfoParentheses;
                }
                
                alternative->m_hasFixedSize = false;
                break;

            case PatternTerm::TypeParentheticalAssertion:
                term.inputPosition = currentInputPosition;
                term.frameLocation = currentCallFrameSize;
                currentCallFrameSize = setupDisjunctionOffsets(term.parentheses.disjunction, currentCallFrameSize + YarrStackSpaceForBackTrackInfoParentheticalAssertion, currentInputPosition);
                break;
            }
        }

        alternative->m_minimumSize = currentInputPosition - initialInputPosition;
        return currentCallFrameSize;
    }

    unsigned setupDisjunctionOffsets(PatternDisjunction* disjunction, unsigned initialCallFrameSize, unsigned initialInputPosition)
    {
        if ((disjunction != m_pattern.m_body) && (disjunction->m_alternatives.size() > 1))
            initialCallFrameSize += YarrStackSpaceForBackTrackInfoAlternative;

        unsigned minimumInputSize = UINT_MAX;
        unsigned maximumCallFrameSize = 0;
        bool hasFixedSize = true;

        for (unsigned alt = 0; alt < disjunction->m_alternatives.size(); ++alt) {
            PatternAlternative* alternative = disjunction->m_alternatives[alt];
            unsigned currentAlternativeCallFrameSize = setupAlternativeOffsets(alternative, initialCallFrameSize, initialInputPosition);
            minimumInputSize = std::min(minimumInputSize, alternative->m_minimumSize);
            maximumCallFrameSize = std::max(maximumCallFrameSize, currentAlternativeCallFrameSize);
            hasFixedSize &= alternative->m_hasFixedSize;
        }
        
        ASSERT(minimumInputSize != UINT_MAX);
        ASSERT(maximumCallFrameSize >= initialCallFrameSize);

        disjunction->m_hasFixedSize = hasFixedSize;
        disjunction->m_minimumSize = minimumInputSize;
        disjunction->m_callFrameSize = maximumCallFrameSize;
        return maximumCallFrameSize;
    }

    void setupOffsets()
    {
        setupDisjunctionOffsets(m_pattern.m_body, 0, 0);
    }

    
    
    
    
    
    
    
    void checkForTerminalParentheses()
    {
        
        
        if (m_pattern.m_numSubpatterns)
            return;

        Vector<PatternAlternative*>& alternatives = m_pattern.m_body->m_alternatives;
        for (size_t i = 0; i < alternatives.size(); ++i) {
            Vector<PatternTerm>& terms = alternatives[i]->m_terms;
            if (terms.size()) {
                PatternTerm& term = terms.last();
                if (term.type == PatternTerm::TypeParenthesesSubpattern
                    && term.quantityType == QuantifierGreedy
                    && term.quantityCount == quantifyInfinite
                    && !term.capture())
                    term.parentheses.isTerminal = true;
            }
        }
    }

    void optimizeBOL()
    {
        
        
        
        
        
        PatternDisjunction* disjunction = m_pattern.m_body;
        
        if (!m_pattern.m_containsBOL || m_pattern.m_multiline)
            return;
        
        PatternDisjunction* loopDisjunction = copyDisjunction(disjunction, true);

        
        for (unsigned alt = 0; alt < disjunction->m_alternatives.size(); ++alt)
            disjunction->m_alternatives[alt]->setOnceThrough();

        if (loopDisjunction) {
            
            for (unsigned alt = 0; alt < loopDisjunction->m_alternatives.size(); ++alt)
                disjunction->m_alternatives.append(loopDisjunction->m_alternatives[alt]);
                
            loopDisjunction->m_alternatives.clear();
        }
    }

    
    
    
    bool setupDisjunctionBeginTerms(PatternDisjunction* disjunction, Vector<TermChain>* beginTerms, unsigned depth)
    {
        for (unsigned alt = 0; alt < disjunction->m_alternatives.size(); ++alt) {
            PatternAlternative* alternative = disjunction->m_alternatives[alt];

            if (!setupAlternativeBeginTerms(alternative, beginTerms, 0, depth))
                return false;
        }

        return true;
    }

    bool setupAlternativeBeginTerms(PatternAlternative* alternative, Vector<TermChain>* beginTerms, unsigned termIndex, unsigned depth)
    {
        bool checkNext = true;
        unsigned numTerms = alternative->m_terms.size();

        while (checkNext && termIndex < numTerms) {
            PatternTerm term = alternative->m_terms[termIndex];
            checkNext = false;

            switch (term.type) {
            case PatternTerm::TypeAssertionBOL:
            case PatternTerm::TypeAssertionEOL:
            case PatternTerm::TypeAssertionWordBoundary:
                return false;

            case PatternTerm::TypeBackReference:
            case PatternTerm::TypeForwardReference:
                return false;

            case PatternTerm::TypePatternCharacter:
                if (termIndex != numTerms - 1) {
                    beginTerms->append(TermChain(term));
                    termIndex++;
                    checkNext = true;
                } else if (term.quantityType == QuantifierFixedCount) {
                    beginTerms->append(TermChain(term));
                    if (depth < 2 && termIndex < numTerms - 1 && term.quantityCount == 1)
                        if (!setupAlternativeBeginTerms(alternative, &beginTerms->last().hotTerms, termIndex + 1, depth + 1))
                            return false;
                }

                break;

            case PatternTerm::TypeCharacterClass:
                return false;

            case PatternTerm::TypeParentheticalAssertion:
                if (term.invert())
                    return false;

            case PatternTerm::TypeParenthesesSubpattern:
                if (term.quantityType != QuantifierFixedCount) {
                    if (termIndex == numTerms - 1)
                        break;

                    termIndex++;
                    checkNext = true;
                }

                if (!setupDisjunctionBeginTerms(term.parentheses.disjunction, beginTerms, depth))
                    return false;

                break;
            }
        }

        return true;
    }

    void setupBeginChars()
    {
        Vector<TermChain> beginTerms;
        bool containsFixedCharacter = false;

        if ((!m_pattern.m_body->m_hasFixedSize || m_pattern.m_body->m_alternatives.size() > 1)
                && setupDisjunctionBeginTerms(m_pattern.m_body, &beginTerms, 0)) {
            unsigned size = beginTerms.size();

            
            if (!size)
                return;

            m_pattern.m_containsBeginChars = true;

            for (unsigned i = 0; i < size; i++) {
                PatternTerm term = beginTerms[i].term;

                
                ASSERT(term.type == PatternTerm::TypePatternCharacter);

                if (term.quantityType == QuantifierFixedCount)
                    containsFixedCharacter = true;

                UChar character = term.patternCharacter;
                unsigned mask = 0;

                if (character <= 0x7f) {
                    if (m_pattern.m_ignoreCase && isASCIIAlpha(character)) {
                        mask = 32;
                        character = toASCIILower(character);
                    }

                    m_beginCharHelper.addBeginChar(BeginChar(character, mask), &beginTerms[i].hotTerms, term.quantityType, term.quantityCount);
                } else {
                    UChar upper, lower;
                    if (m_pattern.m_ignoreCase && ((upper = Unicode::toUpper(character)) != (lower = Unicode::toLower(character)))) {
                        m_beginCharHelper.addBeginChar(BeginChar(upper, mask), &beginTerms[i].hotTerms, term.quantityType, term.quantityCount);
                        m_beginCharHelper.addBeginChar(BeginChar(lower, mask), &beginTerms[i].hotTerms, term.quantityType, term.quantityCount);
                    } else
                        m_beginCharHelper.addBeginChar(BeginChar(character, mask), &beginTerms[i].hotTerms, term.quantityType, term.quantityCount);
                }
            }

            
            if (!containsFixedCharacter) {
                m_pattern.m_containsBeginChars = false;
                return;
            }

            size = m_pattern.m_beginChars.size();

            if (size > 2)
                m_beginCharHelper.merge(size - 1);
            else if (size <= 1)
                m_pattern.m_containsBeginChars = false;
        }
    }

private:
    YarrPattern& m_pattern;
    PatternAlternative* m_alternative;
    CharacterClassConstructor m_characterClassConstructor;
    BeginCharHelper m_beginCharHelper;
    bool m_invertCharacterClass;
    bool m_invertParentheticalAssertion;
};

ErrorCode YarrPattern::compile(const UString& patternString)
{
    YarrPatternConstructor constructor(*this);

    if (ErrorCode error = parse(constructor, patternString))
        return error;
    
    
    
    
    
    if (containsIllegalBackReference()) {
        unsigned numSubpatterns = m_numSubpatterns;

        constructor.reset();
#if !ASSERT_DISABLED
        ErrorCode error =
#endif
            parse(constructor, patternString, numSubpatterns);

        ASSERT(!error);
        ASSERT(numSubpatterns == m_numSubpatterns);
    }

    constructor.checkForTerminalParentheses();
    constructor.optimizeBOL();
        
    constructor.setupOffsets();
    constructor.setupBeginChars();

    return NoError;
}

YarrPattern::YarrPattern(const UString& pattern, bool ignoreCase, bool multiline, ErrorCode* error)
    : m_ignoreCase(ignoreCase)
    , m_multiline(multiline)
    , m_containsBackreferences(false)
    , m_containsBeginChars(false)
    , m_containsBOL(false)
    , m_numSubpatterns(0)
    , m_maxBackReference(0)
    , newlineCached(0)
    , digitsCached(0)
    , spacesCached(0)
    , wordcharCached(0)
    , nondigitsCached(0)
    , nonspacesCached(0)
    , nonwordcharCached(0)
{
    *error = compile(pattern);
}

} }
