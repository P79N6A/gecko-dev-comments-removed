


























#include "YarrPattern.h"

#include "Yarr.h"
#include "YarrCanonicalizeUCS2.h"
#include "YarrParser.h"

using namespace WTF;

namespace JSC { namespace Yarr {

#include "RegExpJitTables.h"

#if WTF_CPU_SPARC
# define BASE_FRAME_SIZE 24
#else
# define BASE_FRAME_SIZE 0
#endif

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
            return;
        }

        
        if (!m_isCaseInsensitive) {
            addSorted(m_matchesUnicode, ch);
            return;
        }

        
        UCS2CanonicalizationRange* info = rangeInfoFor(ch);
        if (info->type == CanonicalizeUnique)
            addSorted(m_matchesUnicode, ch);
        else
            putUnicodeIgnoreCase(ch, info);
    }

    void putUnicodeIgnoreCase(UChar ch, UCS2CanonicalizationRange* info)
    {
        ASSERT(m_isCaseInsensitive);
        ASSERT(ch > 0x7f);
        ASSERT(ch >= info->begin && ch <= info->end);
        ASSERT(info->type != CanonicalizeUnique);
        if (info->type == CanonicalizeSet) {
            for (uint16_t* set = characterSetInfo[info->value]; (ch = *set); ++set)
                addSorted(m_matchesUnicode, ch);
        } else {
            addSorted(m_matchesUnicode, ch);
            addSorted(m_matchesUnicode, getCanonicalPair(info, ch));
        }
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
        if (hi <= 0x7f)
            return;

        lo = std::max(lo, (UChar)0x80);
        addSortedRange(m_rangesUnicode, lo, hi);
        
        if (!m_isCaseInsensitive)
            return;

        UCS2CanonicalizationRange* info = rangeInfoFor(lo);
        while (true) {
            
            UChar end = std::min<UChar>(info->end, hi);

            switch (info->type) {
            case CanonicalizeUnique:
                
                break;
            case CanonicalizeSet: {
                UChar ch;
                for (uint16_t* set = characterSetInfo[info->value]; (ch = *set); ++set)
                    addSorted(m_matchesUnicode, ch);
                break;
            }
            case CanonicalizeRangeLo:
                addSortedRange(m_rangesUnicode, lo + info->value, end + info->value);
                break;
            case CanonicalizeRangeHi:
                addSortedRange(m_rangesUnicode, lo - info->value, end - info->value);
                break;
            case CanonicalizeAlternatingAligned:
                
                if (lo & 1)
                    addSortedRange(m_rangesUnicode, lo - 1, lo - 1);
                if (!(end & 1))
                    addSortedRange(m_rangesUnicode, end + 1, end + 1);
                break;
            case CanonicalizeAlternatingUnaligned:
                
                if (!(lo & 1))
                    addSortedRange(m_rangesUnicode, lo - 1, lo - 1);
                if (end & 1)
                    addSortedRange(m_rangesUnicode, end + 1, end + 1);
                break;
            }

            if (hi == end)
                return;

            ++info;
            lo = info->begin;
        };

    }

    CharacterClass* charClass()
    {
        CharacterClass* characterClass = js_new<CharacterClass>(PassRefPtr<CharacterClassTable>(0));

        characterClass->m_matches.swap(m_matches);
        characterClass->m_ranges.swap(m_ranges);
        characterClass->m_matchesUnicode.swap(m_matchesUnicode);
        characterClass->m_rangesUnicode.swap(m_rangesUnicode);

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

class YarrPatternConstructor {
public:
    YarrPatternConstructor(YarrPattern& pattern)
        : m_pattern(pattern)
        , m_characterClassConstructor(pattern.m_ignoreCase)
        , m_invertParentheticalAssertion(false)
    {
        m_pattern.m_body = js_new<PatternDisjunction>();
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

        m_pattern.m_body = js_new<PatternDisjunction>();
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
        
        
        if (!m_pattern.m_ignoreCase || isASCII(ch)) {
            m_alternative->m_terms.append(PatternTerm(ch));
            return;
        }

        UCS2CanonicalizationRange* info = rangeInfoFor(ch);
        if (info->type == CanonicalizeUnique) {
            m_alternative->m_terms.append(PatternTerm(ch));
            return;
        }

        m_characterClassConstructor.putUnicodeIgnoreCase(ch, info);
        CharacterClass* newCharacterClass = m_characterClassConstructor.charClass();
        m_pattern.m_userCharacterClasses.append(newCharacterClass);
        m_alternative->m_terms.append(PatternTerm(newCharacterClass, false));
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

        PatternDisjunction* parenthesesDisjunction = js_new<PatternDisjunction>(m_alternative);
        m_pattern.m_disjunctions.append(parenthesesDisjunction);
        m_alternative->m_terms.append(PatternTerm(PatternTerm::TypeParenthesesSubpattern, subpatternId, parenthesesDisjunction, capture, false));
        m_alternative = parenthesesDisjunction->addNewAlternative();
    }

    void atomParentheticalAssertionBegin(bool invert = false)
    {
        PatternDisjunction* parenthesesDisjunction = js_new<PatternDisjunction>(m_alternative);
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
                    newDisjunction = js_new<PatternDisjunction>();
                    newDisjunction->m_parent = disjunction->m_parent;
                }
                PatternAlternative* newAlternative = newDisjunction->addNewAlternative();
                newAlternative->m_terms.reserve(alternative->m_terms.size());
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

    ErrorCode setupAlternativeOffsets(PatternAlternative* alternative, unsigned currentCallFrameSize, unsigned initialInputPosition,
                                      unsigned *callFrameSizeOut)
    {
        alternative->m_hasFixedSize = true;
        Checked<unsigned> currentInputPosition = initialInputPosition;

        for (unsigned i = 0; i < alternative->m_terms.size(); ++i) {
            PatternTerm& term = alternative->m_terms[i];

            switch (term.type) {
            case PatternTerm::TypeAssertionBOL:
            case PatternTerm::TypeAssertionEOL:
            case PatternTerm::TypeAssertionWordBoundary:
                term.inputPosition = currentInputPosition.unsafeGet();
                break;

            case PatternTerm::TypeBackReference:
                term.inputPosition = currentInputPosition.unsafeGet();
                term.frameLocation = currentCallFrameSize;
                currentCallFrameSize += YarrStackSpaceForBackTrackInfoBackReference;
                alternative->m_hasFixedSize = false;
                break;

            case PatternTerm::TypeForwardReference:
                break;

            case PatternTerm::TypePatternCharacter:
                term.inputPosition = currentInputPosition.unsafeGet();
                if (term.quantityType != QuantifierFixedCount) {
                    term.frameLocation = currentCallFrameSize;
                    currentCallFrameSize += YarrStackSpaceForBackTrackInfoPatternCharacter;
                    alternative->m_hasFixedSize = false;
                } else
                    currentInputPosition += term.quantityCount;
                break;

            case PatternTerm::TypeCharacterClass:
                term.inputPosition = currentInputPosition.unsafeGet();
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
                    if (ErrorCode error = setupDisjunctionOffsets(term.parentheses.disjunction, currentCallFrameSize, currentInputPosition.unsafeGet(), &currentCallFrameSize))
                        return error;
                    
                    if (term.quantityType == QuantifierFixedCount)
                        currentInputPosition += term.parentheses.disjunction->m_minimumSize;
                    term.inputPosition = currentInputPosition.unsafeGet();
                } else if (term.parentheses.isTerminal) {
                    currentCallFrameSize += YarrStackSpaceForBackTrackInfoParenthesesTerminal;
                    if (ErrorCode error = setupDisjunctionOffsets(term.parentheses.disjunction, currentCallFrameSize, currentInputPosition.unsafeGet(), &currentCallFrameSize))
                        return error;
                    term.inputPosition = currentInputPosition.unsafeGet();
                } else {
                    term.inputPosition = currentInputPosition.unsafeGet();
                    unsigned dummy;
                    if (ErrorCode error = setupDisjunctionOffsets(term.parentheses.disjunction, BASE_FRAME_SIZE, currentInputPosition.unsafeGet(), &dummy))
                        return error;
                    currentCallFrameSize += YarrStackSpaceForBackTrackInfoParentheses;
                }
                
                alternative->m_hasFixedSize = false;
                break;

            case PatternTerm::TypeParentheticalAssertion:
                term.inputPosition = currentInputPosition.unsafeGet();
                term.frameLocation = currentCallFrameSize;
                if (ErrorCode error = setupDisjunctionOffsets(term.parentheses.disjunction, currentCallFrameSize + YarrStackSpaceForBackTrackInfoParentheticalAssertion, currentInputPosition.unsafeGet(), &currentCallFrameSize))
                    return error;
                break;

            case PatternTerm::TypeDotStarEnclosure:
                alternative->m_hasFixedSize = false;
                term.inputPosition = initialInputPosition;
                break;
            }
        }

        alternative->m_minimumSize = (currentInputPosition - initialInputPosition).unsafeGet();
        *callFrameSizeOut = currentCallFrameSize;
        return NoError;
    }

    ErrorCode setupDisjunctionOffsets(PatternDisjunction* disjunction, unsigned initialCallFrameSize, unsigned initialInputPosition, unsigned *maximumCallFrameSizeOut)
    {
        if ((disjunction != m_pattern.m_body) && (disjunction->m_alternatives.size() > 1))
            initialCallFrameSize += YarrStackSpaceForBackTrackInfoAlternative;

        unsigned minimumInputSize = UINT_MAX;
        unsigned maximumCallFrameSize = 0;
        bool hasFixedSize = true;

        for (unsigned alt = 0; alt < disjunction->m_alternatives.size(); ++alt) {
            PatternAlternative* alternative = disjunction->m_alternatives[alt];
            unsigned currentAlternativeCallFrameSize;
            if (ErrorCode error = setupAlternativeOffsets(alternative, initialCallFrameSize, initialInputPosition, &currentAlternativeCallFrameSize))
                return error;
            minimumInputSize = std::min(minimumInputSize, alternative->m_minimumSize);
            maximumCallFrameSize = std::max(maximumCallFrameSize, currentAlternativeCallFrameSize);
            hasFixedSize &= alternative->m_hasFixedSize;
        }

        ASSERT(minimumInputSize != UINT_MAX);
        if (minimumInputSize == UINT_MAX)
            return PatternTooLarge;
        
        ASSERT(minimumInputSize != UINT_MAX);
        ASSERT(maximumCallFrameSize >= initialCallFrameSize);

        disjunction->m_hasFixedSize = hasFixedSize;
        disjunction->m_minimumSize = minimumInputSize;
        disjunction->m_callFrameSize = maximumCallFrameSize;
        *maximumCallFrameSizeOut = maximumCallFrameSize;
        return NoError;
    }

    ErrorCode setupOffsets()
    {
        unsigned dummy;
        return setupDisjunctionOffsets(m_pattern.m_body, BASE_FRAME_SIZE, 0, &dummy);
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

    bool containsCapturingTerms(PatternAlternative* alternative, size_t firstTermIndex, size_t lastTermIndex)
    {
        Vector<PatternTerm>& terms = alternative->m_terms;

        for (size_t termIndex = firstTermIndex; termIndex <= lastTermIndex; ++termIndex) {
            PatternTerm& term = terms[termIndex];

            if (term.m_capture)
                return true;

            if (term.type == PatternTerm::TypeParenthesesSubpattern) {
                PatternDisjunction* nestedDisjunction = term.parentheses.disjunction;
                for (unsigned alt = 0; alt < nestedDisjunction->m_alternatives.size(); ++alt) {
                    if (containsCapturingTerms(nestedDisjunction->m_alternatives[alt], 0, nestedDisjunction->m_alternatives[alt]->m_terms.size() - 1))
                        return true;
                }
            }
        }

        return false;
    }

    
    
    
    
    
    void optimizeDotStarWrappedExpressions()
    {
        Vector<PatternAlternative*>& alternatives = m_pattern.m_body->m_alternatives;
        if (alternatives.size() != 1)
            return;

        PatternAlternative* alternative = alternatives[0];
        Vector<PatternTerm>& terms = alternative->m_terms;
        if (terms.size() >= 3) {
            bool startsWithBOL = false;
            bool endsWithEOL = false;
            size_t termIndex, firstExpressionTerm, lastExpressionTerm;

            termIndex = 0;
            if (terms[termIndex].type == PatternTerm::TypeAssertionBOL) {
                startsWithBOL = true;
                ++termIndex;
            }
            
            PatternTerm& firstNonAnchorTerm = terms[termIndex];
            if ((firstNonAnchorTerm.type != PatternTerm::TypeCharacterClass) || (firstNonAnchorTerm.characterClass != m_pattern.newlineCharacterClass()) || !((firstNonAnchorTerm.quantityType == QuantifierGreedy) || (firstNonAnchorTerm.quantityType == QuantifierNonGreedy)))
                return;
            
            firstExpressionTerm = termIndex + 1;
            
            termIndex = terms.size() - 1;
            if (terms[termIndex].type == PatternTerm::TypeAssertionEOL) {
                endsWithEOL = true;
                --termIndex;
            }
            
            PatternTerm& lastNonAnchorTerm = terms[termIndex];
            if ((lastNonAnchorTerm.type != PatternTerm::TypeCharacterClass) || (lastNonAnchorTerm.characterClass != m_pattern.newlineCharacterClass()) || (lastNonAnchorTerm.quantityType != QuantifierGreedy))
                return;
            
            lastExpressionTerm = termIndex - 1;

            if (firstExpressionTerm > lastExpressionTerm)
                return;

            if (!containsCapturingTerms(alternative, firstExpressionTerm, lastExpressionTerm)) {
                for (termIndex = terms.size() - 1; termIndex > lastExpressionTerm; --termIndex)
                    terms.remove(termIndex);

                for (termIndex = firstExpressionTerm; termIndex > 0; --termIndex)
                    terms.remove(termIndex - 1);

                terms.append(PatternTerm(startsWithBOL, endsWithEOL));
                
                m_pattern.m_containsBOL = false;
            }
        }
    }

private:
    YarrPattern& m_pattern;
    PatternAlternative* m_alternative;
    CharacterClassConstructor m_characterClassConstructor;
    bool m_invertCharacterClass;
    bool m_invertParentheticalAssertion;
};

ErrorCode YarrPattern::compile(const String& patternString)
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
    constructor.optimizeDotStarWrappedExpressions();
    constructor.optimizeBOL();
        
    if (ErrorCode error = constructor.setupOffsets())
        return error;

    return NoError;
}

YarrPattern::YarrPattern(const String& pattern, bool ignoreCase, bool multiline, ErrorCode* error)
    : m_ignoreCase(ignoreCase)
    , m_multiline(multiline)
    , m_containsBackreferences(false)
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
