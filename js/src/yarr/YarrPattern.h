





























#ifndef YarrPattern_h
#define YarrPattern_h

#include "wtfbridge.h"
#include "ASCIICType.h"

namespace JSC { namespace Yarr {

enum ErrorCode {
    NoError,
    PatternTooLarge,
    QuantifierOutOfOrder,
    QuantifierWithoutAtom,
    MissingParentheses,
    ParenthesesUnmatched,
    ParenthesesTypeInvalid,
    CharacterClassUnmatched,
    CharacterClassInvalidRange,
    CharacterClassOutOfOrder,
    EscapeUnterminated,
    QuantifierTooLarge,
    NumberOfErrorCodes
};

struct PatternDisjunction;

struct CharacterRange {
    UChar begin;
    UChar end;

    CharacterRange(UChar begin, UChar end)
        : begin(begin)
        , end(end)
    {
    }
};

struct CharacterClassTable : RefCounted<CharacterClassTable> {
    friend class js::OffTheBooks;
    const char* m_table;
    bool m_inverted;
    static PassRefPtr<CharacterClassTable> create(const char* table, bool inverted)
    {
        return adoptRef(js::OffTheBooks::new_<CharacterClassTable>(table, inverted));
    }

private:
    CharacterClassTable(const char* table, bool inverted)
        : m_table(table)
        , m_inverted(inverted)
    {
    }
};

struct CharacterClass {
    WTF_MAKE_FAST_ALLOCATED
public:
    
    
    
    CharacterClass(PassRefPtr<CharacterClassTable> table)
        : m_table(table)
    {
    }
    ~CharacterClass()
    {
        js::Foreground::delete_(m_table.get());
    }
    Vector<UChar> m_matches;
    Vector<CharacterRange> m_ranges;
    Vector<UChar> m_matchesUnicode;
    Vector<CharacterRange> m_rangesUnicode;
    RefPtr<CharacterClassTable> m_table;
};

enum QuantifierType {
    QuantifierFixedCount,
    QuantifierGreedy,
    QuantifierNonGreedy
};

struct PatternTerm {
    enum Type {
        TypeAssertionBOL,
        TypeAssertionEOL,
        TypeAssertionWordBoundary,
        TypePatternCharacter,
        TypeCharacterClass,
        TypeBackReference,
        TypeForwardReference,
        TypeParenthesesSubpattern,
        TypeParentheticalAssertion
    } type;
    bool m_capture :1;
    bool m_invert :1;
    union {
        UChar patternCharacter;
        CharacterClass* characterClass;
        unsigned backReferenceSubpatternId;
        struct {
            PatternDisjunction* disjunction;
            unsigned subpatternId;
            unsigned lastSubpatternId;
            bool isCopy;
            bool isTerminal;
        } parentheses;
    };
    QuantifierType quantityType;
    unsigned quantityCount;
    int inputPosition;
    unsigned frameLocation;

    
    PatternTerm()
        : type(PatternTerm::TypePatternCharacter)
        , m_capture(false)
        , m_invert(false)
    {
        patternCharacter = 0;
        quantityType = QuantifierFixedCount;
        quantityCount = 1;
    }

    PatternTerm(UChar ch)
        : type(PatternTerm::TypePatternCharacter)
        , m_capture(false)
        , m_invert(false)
    {
        patternCharacter = ch;
        quantityType = QuantifierFixedCount;
        quantityCount = 1;
    }

    PatternTerm(CharacterClass* charClass, bool invert)
        : type(PatternTerm::TypeCharacterClass)
        , m_capture(false)
        , m_invert(invert)
    {
        characterClass = charClass;
        quantityType = QuantifierFixedCount;
        quantityCount = 1;
    }

    PatternTerm(Type type, unsigned subpatternId, PatternDisjunction* disjunction, bool capture = false, bool invert = false)
        : type(type)
        , m_capture(capture)
        , m_invert(invert)
    {
        parentheses.disjunction = disjunction;
        parentheses.subpatternId = subpatternId;
        parentheses.isCopy = false;
        parentheses.isTerminal = false;
        quantityType = QuantifierFixedCount;
        quantityCount = 1;
    }
    
    PatternTerm(Type type, bool invert = false)
        : type(type)
        , m_capture(false)
        , m_invert(invert)
    {
        quantityType = QuantifierFixedCount;
        quantityCount = 1;
    }

    PatternTerm(unsigned spatternId)
        : type(TypeBackReference)
        , m_capture(false)
        , m_invert(false)
    {
        backReferenceSubpatternId = spatternId;
        quantityType = QuantifierFixedCount;
        quantityCount = 1;
    }

    static PatternTerm ForwardReference()
    {
        return PatternTerm(TypeForwardReference);
    }

    static PatternTerm BOL()
    {
        return PatternTerm(TypeAssertionBOL);
    }

    static PatternTerm EOL()
    {
        return PatternTerm(TypeAssertionEOL);
    }

    static PatternTerm WordBoundary(bool invert)
    {
        return PatternTerm(TypeAssertionWordBoundary, invert);
    }
    
    bool invert()
    {
        return m_invert;
    }

    bool capture()
    {
        return m_capture;
    }
    
    void quantify(unsigned count, QuantifierType type)
    {
        quantityCount = count;
        quantityType = type;
    }
};

struct PatternAlternative {
    WTF_MAKE_FAST_ALLOCATED
public:
    PatternAlternative(PatternDisjunction* disjunction)
        : m_parent(disjunction)
        , m_onceThrough(false)
        , m_hasFixedSize(false)
        , m_startsWithBOL(false)
        , m_containsBOL(false)
    {
    }

    PatternTerm& lastTerm()
    {
        ASSERT(m_terms.size());
        return m_terms[m_terms.size() - 1];
    }
    
    void removeLastTerm()
    {
        ASSERT(m_terms.size());
        m_terms.shrink(m_terms.size() - 1);
    }
    
    void setOnceThrough()
    {
        m_onceThrough = true;
    }
    
    bool onceThrough()
    {
        return m_onceThrough;
    }

    Vector<PatternTerm> m_terms;
    PatternDisjunction* m_parent;
    unsigned m_minimumSize;
    bool m_onceThrough : 1;
    bool m_hasFixedSize : 1;
    bool m_startsWithBOL : 1;
    bool m_containsBOL : 1;
};

struct PatternDisjunction {
    WTF_MAKE_FAST_ALLOCATED
public:
    PatternDisjunction(PatternAlternative* parent = 0)
        : m_parent(parent)
        , m_hasFixedSize(false)
    {
    }
    
    ~PatternDisjunction()
    {
        deleteAllValues(m_alternatives);
    }

    PatternAlternative* addNewAlternative()
    {
        PatternAlternative* alternative = js::OffTheBooks::new_<PatternAlternative>(this);
        m_alternatives.append(alternative);
        return alternative;
    }

    Vector<PatternAlternative*> m_alternatives;
    PatternAlternative* m_parent;
    unsigned m_minimumSize;
    unsigned m_callFrameSize;
    bool m_hasFixedSize;
};





CharacterClass* newlineCreate();
CharacterClass* digitsCreate();
CharacterClass* spacesCreate();
CharacterClass* wordcharCreate();
CharacterClass* nondigitsCreate();
CharacterClass* nonspacesCreate();
CharacterClass* nonwordcharCreate();

struct TermChain {
    TermChain(PatternTerm term)
        : term(term)
    {}

    PatternTerm term;
    Vector<TermChain> hotTerms;
};

struct BeginChar {
    BeginChar()
        : value(0)
        , mask(0)
    {}

    BeginChar(unsigned value, unsigned mask)
        : value(value)
        , mask(mask)
    {}

    unsigned value;
    unsigned mask;
};

struct YarrPattern {
    YarrPattern(const UString& pattern, bool ignoreCase, bool multiline, ErrorCode* error);

    ~YarrPattern()
    {
        deleteAllValues(m_disjunctions);
        deleteAllValues(m_userCharacterClasses);
    }

    void reset()
    {
        m_numSubpatterns = 0;
        m_maxBackReference = 0;

        m_containsBackreferences = false;
        m_containsBeginChars = false;
        m_containsBOL = false;

        newlineCached = 0;
        digitsCached = 0;
        spacesCached = 0;
        wordcharCached = 0;
        nondigitsCached = 0;
        nonspacesCached = 0;
        nonwordcharCached = 0;

        deleteAllValues(m_disjunctions);
        m_disjunctions.clear();
        deleteAllValues(m_userCharacterClasses);
        m_userCharacterClasses.clear();
        m_beginChars.clear();
    }

    bool containsIllegalBackReference()
    {
        return m_maxBackReference > m_numSubpatterns;
    }

    CharacterClass* newlineCharacterClass()
    {
        if (!newlineCached)
            m_userCharacterClasses.append(newlineCached = newlineCreate());
        return newlineCached;
    }
    CharacterClass* digitsCharacterClass()
    {
        if (!digitsCached)
            m_userCharacterClasses.append(digitsCached = digitsCreate());
        return digitsCached;
    }
    CharacterClass* spacesCharacterClass()
    {
        if (!spacesCached)
            m_userCharacterClasses.append(spacesCached = spacesCreate());
        return spacesCached;
    }
    CharacterClass* wordcharCharacterClass()
    {
        if (!wordcharCached)
            m_userCharacterClasses.append(wordcharCached = wordcharCreate());
        return wordcharCached;
    }
    CharacterClass* nondigitsCharacterClass()
    {
        if (!nondigitsCached)
            m_userCharacterClasses.append(nondigitsCached = nondigitsCreate());
        return nondigitsCached;
    }
    CharacterClass* nonspacesCharacterClass()
    {
        if (!nonspacesCached)
            m_userCharacterClasses.append(nonspacesCached = nonspacesCreate());
        return nonspacesCached;
    }
    CharacterClass* nonwordcharCharacterClass()
    {
        if (!nonwordcharCached)
            m_userCharacterClasses.append(nonwordcharCached = nonwordcharCreate());
        return nonwordcharCached;
    }

    bool m_ignoreCase : 1;
    bool m_multiline : 1;
    bool m_containsBackreferences : 1;
    bool m_containsBeginChars : 1;
    bool m_containsBOL : 1;
    unsigned m_numSubpatterns;
    unsigned m_maxBackReference;
    PatternDisjunction* m_body;
    Vector<PatternDisjunction*, 4> m_disjunctions;
    Vector<CharacterClass*> m_userCharacterClasses;
    Vector<BeginChar> m_beginChars;

private:
    ErrorCode compile(const UString& patternString);

    CharacterClass* newlineCached;
    CharacterClass* digitsCached;
    CharacterClass* spacesCached;
    CharacterClass* wordcharCached;
    CharacterClass* nondigitsCached;
    CharacterClass* nonspacesCached;
    CharacterClass* nonwordcharCached;
};

} } 

#endif 
