




























#ifndef YarrParser_h
#define YarrParser_h

#include "Yarr.h"

namespace JSC { namespace Yarr {

#define REGEXP_ERROR_PREFIX "Invalid regular expression: "

enum BuiltInCharacterClassID {
    DigitClassID,
    SpaceClassID,
    WordClassID,
    NewlineClassID
};


template<class Delegate>
class Parser {
private:
    template<class FriendDelegate>
    friend ErrorCode parse(FriendDelegate& delegate, const UString& pattern, unsigned backReferenceLimit);

    








    class CharacterClassParserDelegate {
    public:
        CharacterClassParserDelegate(Delegate& delegate, ErrorCode& err)
            : m_delegate(delegate)
            , m_err(err)
            , m_state(Empty)
            , m_character(0)
        {
        }

        




        void begin(bool invert)
        {
            m_delegate.atomCharacterClassBegin(invert);
        }

        








        void atomPatternCharacter(UChar ch, bool hyphenIsRange = false)
        {
            switch (m_state) {
            case AfterCharacterClass:
                
                
                
                
                
                
                
                if (hyphenIsRange && ch == '-') {
                    m_delegate.atomCharacterClassAtom('-');
                    m_state = AfterCharacterClassHyphen;
                    return;
                }
                

            case Empty:
                m_character = ch;
                m_state = CachedCharacter;
                return;

            case CachedCharacter:
                if (hyphenIsRange && ch == '-')
                    m_state = CachedCharacterHyphen;
                else {
                    m_delegate.atomCharacterClassAtom(m_character);
                    m_character = ch;
                }
                return;

            case CachedCharacterHyphen:
                if (ch < m_character) {
                    m_err = CharacterClassOutOfOrder;
                    return;
                }
                m_delegate.atomCharacterClassRange(m_character, ch);
                m_state = Empty;
                return;

            case AfterCharacterClassHyphen:
                m_delegate.atomCharacterClassAtom(ch);
                m_state = Empty;
                return;
            }
        }

        




        void atomBuiltInCharacterClass(BuiltInCharacterClassID classID, bool invert)
        {
            switch (m_state) {
            case CachedCharacter:
                
                m_delegate.atomCharacterClassAtom(m_character);

            case Empty:
            case AfterCharacterClass:
                m_state = AfterCharacterClass;
                m_delegate.atomCharacterClassBuiltIn(classID, invert);
                return;

            case CachedCharacterHyphen:
                
                
                m_err = CharacterClassInvalidRange;
                return;

            case AfterCharacterClassHyphen:
                m_delegate.atomCharacterClassBuiltIn(classID, invert);
                m_state = Empty;
                return;
            }
        }

        




        void end()
        {
            if (m_state == CachedCharacter)
                m_delegate.atomCharacterClassAtom(m_character);
            else if (m_state == CachedCharacterHyphen) {
                m_delegate.atomCharacterClassAtom(m_character);
                m_delegate.atomCharacterClassAtom('-');
            }
            m_delegate.atomCharacterClassEnd();
        }

        
        
        void assertionWordBoundary(bool) { ASSERT_NOT_REACHED(); }
        void atomBackReference(unsigned) { ASSERT_NOT_REACHED(); }

    private:
        Delegate& m_delegate;
        ErrorCode& m_err;
        enum CharacterClassConstructionState {
            Empty,
            CachedCharacter,
            CachedCharacterHyphen,
            AfterCharacterClass,
            AfterCharacterClassHyphen
        } m_state;
        UChar m_character;
    };

    Parser(Delegate& delegate, const UString& pattern, unsigned backReferenceLimit)
        : m_delegate(delegate)
        , m_backReferenceLimit(backReferenceLimit)
        , m_err(NoError)
        , m_data(pattern.chars())
        , m_size(pattern.length())
        , m_index(0)
        , m_parenthesesNestingDepth(0)
    {
    }
    
    



















    template<bool inCharacterClass, class EscapeDelegate>
    bool parseEscape(EscapeDelegate& delegate)
    {
        ASSERT(!m_err);
        ASSERT(peek() == '\\');
        consume();

        if (atEndOfPattern()) {
            m_err = EscapeUnterminated;
            return false;
        }

        switch (peek()) {
        
        case 'b':
            consume();
            if (inCharacterClass)
                delegate.atomPatternCharacter('\b');
            else {
                delegate.assertionWordBoundary(false);
                return false;
            }
            break;
        case 'B':
            consume();
            if (inCharacterClass)
                delegate.atomPatternCharacter('B');
            else {
                delegate.assertionWordBoundary(true);
                return false;
            }
            break;

        
        case 'd':
            consume();
            delegate.atomBuiltInCharacterClass(DigitClassID, false);
            break;
        case 's':
            consume();
            delegate.atomBuiltInCharacterClass(SpaceClassID, false);
            break;
        case 'w':
            consume();
            delegate.atomBuiltInCharacterClass(WordClassID, false);
            break;
        case 'D':
            consume();
            delegate.atomBuiltInCharacterClass(DigitClassID, true);
            break;
        case 'S':
            consume();
            delegate.atomBuiltInCharacterClass(SpaceClassID, true);
            break;
        case 'W':
            consume();
            delegate.atomBuiltInCharacterClass(WordClassID, true);
            break;

        
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9': {
            
            
            if (!inCharacterClass) {
                ParseState state = saveState();

                unsigned backReference;
                if (!consumeNumber(backReference))
                    break;
                if (backReference <= m_backReferenceLimit) {
                    delegate.atomBackReference(backReference);
                    break;
                }

                restoreState(state);
            }
            
            
            if (peek() >= '8') {
                delegate.atomPatternCharacter('\\');
                break;
            }

            
        }

        
        case '0':
            delegate.atomPatternCharacter(consumeOctal());
            break;

        
        case 'f':
            consume();
            delegate.atomPatternCharacter('\f');
            break;
        case 'n':
            consume();
            delegate.atomPatternCharacter('\n');
            break;
        case 'r':
            consume();
            delegate.atomPatternCharacter('\r');
            break;
        case 't':
            consume();
            delegate.atomPatternCharacter('\t');
            break;
        case 'v':
            consume();
            delegate.atomPatternCharacter('\v');
            break;

        
        case 'c': {
            ParseState state = saveState();
            consume();
            if (!atEndOfPattern()) {
                int control = consume();

                
                if (inCharacterClass ? WTF::isASCIIAlphanumeric(control) || (control == '_') : WTF::isASCIIAlpha(control)) {
                    delegate.atomPatternCharacter(control & 0x1f);
                    break;
                }
            }
            restoreState(state);
            delegate.atomPatternCharacter('\\');
            break;
        }

        
        case 'x': {
            consume();
            int x = tryConsumeHex(2);
            if (x == -1)
                delegate.atomPatternCharacter('x');
            else
                delegate.atomPatternCharacter(x);
            break;
        }

        
        case 'u': {
            consume();
            int u = tryConsumeHex(4);
            if (u == -1)
                delegate.atomPatternCharacter('u');
            else
                delegate.atomPatternCharacter(u);
            break;
        }

        
        default:
            delegate.atomPatternCharacter(consume());
        }
        
        return true;
    }

    




    bool parseAtomEscape()
    {
        return parseEscape<false>(m_delegate);
    }
    void parseCharacterClassEscape(CharacterClassParserDelegate& delegate)
    {
        parseEscape<true>(delegate);
    }

    






    void parseCharacterClass()
    {
        ASSERT(!m_err);
        ASSERT(peek() == '[');
        consume();

        CharacterClassParserDelegate characterClassConstructor(m_delegate, m_err);

        characterClassConstructor.begin(tryConsume('^'));

        while (!atEndOfPattern()) {
            switch (peek()) {
            case ']':
                consume();
                characterClassConstructor.end();
                return;

            case '\\':
                parseCharacterClassEscape(characterClassConstructor);
                break;

            default:
                characterClassConstructor.atomPatternCharacter(consume(), true);
            }

            if (m_err)
                return;
        }

        m_err = CharacterClassUnmatched;
    }

    




    void parseParenthesesBegin()
    {
        ASSERT(!m_err);
        ASSERT(peek() == '(');
        consume();

        if (tryConsume('?')) {
            if (atEndOfPattern()) {
                m_err = ParenthesesTypeInvalid;
                return;
            }

            switch (consume()) {
            case ':':
                m_delegate.atomParenthesesSubpatternBegin(false);
                break;
            
            case '=':
                m_delegate.atomParentheticalAssertionBegin();
                break;

            case '!':
                m_delegate.atomParentheticalAssertionBegin(true);
                break;
            
            default:
                m_err = ParenthesesTypeInvalid;
            }
        } else
            m_delegate.atomParenthesesSubpatternBegin();

        ++m_parenthesesNestingDepth;
    }

    




    void parseParenthesesEnd()
    {
        ASSERT(!m_err);
        ASSERT(peek() == ')');
        consume();

        if (m_parenthesesNestingDepth > 0)
            m_delegate.atomParenthesesEnd();
        else
            m_err = ParenthesesUnmatched;

        --m_parenthesesNestingDepth;
    }

    




    void parseQuantifier(bool lastTokenWasAnAtom, unsigned min, unsigned max)
    {
        ASSERT(!m_err);
        ASSERT(min <= max);

        if (lastTokenWasAnAtom)
            m_delegate.quantifyAtom(min, max, !tryConsume('?'));
        else
            m_err = QuantifierWithoutAtom;
    }

    








    void parseTokens()
    {
        bool lastTokenWasAnAtom = false;

        while (!atEndOfPattern()) {
            switch (peek()) {
            case '|':
                consume();
                m_delegate.disjunction();
                lastTokenWasAnAtom = false;
                break;

            case '(':
                parseParenthesesBegin();
                lastTokenWasAnAtom = false;
                break;

            case ')':
                parseParenthesesEnd();
                lastTokenWasAnAtom = true;
                break;

            case '^':
                consume();
                m_delegate.assertionBOL();
                lastTokenWasAnAtom = false;
                break;

            case '$':
                consume();
                m_delegate.assertionEOL();
                lastTokenWasAnAtom = false;
                break;

            case '.':
                consume();
                m_delegate.atomBuiltInCharacterClass(NewlineClassID, true);
                lastTokenWasAnAtom = true;
                break;

            case '[':
                parseCharacterClass();
                lastTokenWasAnAtom = true;
                break;

            case '\\':
                lastTokenWasAnAtom = parseAtomEscape();
                break;

            case '*':
                consume();
                parseQuantifier(lastTokenWasAnAtom, 0, quantifyInfinite);
                lastTokenWasAnAtom = false;
                break;

            case '+':
                consume();
                parseQuantifier(lastTokenWasAnAtom, 1, quantifyInfinite);
                lastTokenWasAnAtom = false;
                break;

            case '?':
                consume();
                parseQuantifier(lastTokenWasAnAtom, 0, 1);
                lastTokenWasAnAtom = false;
                break;

            case '{': {
                ParseState state = saveState();

                consume();
                if (peekIsDigit()) {
                    unsigned min;
                    if (!consumeNumber(min))
                        break;
                    unsigned max = min;
                    
                    if (tryConsume(',')) {
                        if (peekIsDigit()) {
                            if (!consumeNumber(max))
                                break;
                        } else {
                            max = quantifyInfinite;
                        }
                    }

                    if (tryConsume('}')) {
                        if (min <= max)
                            parseQuantifier(lastTokenWasAnAtom, min, max);
                        else
                            m_err = QuantifierOutOfOrder;
                        lastTokenWasAnAtom = false;
                        break;
                    }
                }

                restoreState(state);
            } 

            default:
                m_delegate.atomPatternCharacter(consume());
                lastTokenWasAnAtom = true;
            }

            if (m_err)
                return;
        }

        if (m_parenthesesNestingDepth > 0)
            m_err = MissingParentheses;
    }

    





    ErrorCode parse()
    {
        if (m_size > MAX_PATTERN_SIZE)
            m_err = PatternTooLarge;
        else
            parseTokens();
        ASSERT(atEndOfPattern() || m_err);

        return m_err;
    }


    

    typedef unsigned ParseState;
    
    ParseState saveState()
    {
        return m_index;
    }

    void restoreState(ParseState state)
    {
        m_index = state;
    }

    bool atEndOfPattern()
    {
        ASSERT(m_index <= m_size);
        return m_index == m_size;
    }

    int peek()
    {
        ASSERT(m_index < m_size);
        return m_data[m_index];
    }

    bool peekIsDigit()
    {
        return !atEndOfPattern() && WTF::isASCIIDigit(peek());
    }

    unsigned peekDigit()
    {
        ASSERT(peekIsDigit());
        return peek() - '0';
    }

    int consume()
    {
        ASSERT(m_index < m_size);
        return m_data[m_index++];
    }

    unsigned consumeDigit()
    {
        ASSERT(peekIsDigit());
        return consume() - '0';
    }

    bool consumeNumber(unsigned &accum)
    {
        accum = consumeDigit();
        while (peekIsDigit()) {
            unsigned newValue = accum * 10 + peekDigit();
            if (newValue < accum) { 
                m_err = QuantifierTooLarge;
                return false;
            }
            accum = newValue;
            consume();
        }
        return true;
    }

    unsigned consumeOctal()
    {
        ASSERT(WTF::isASCIIOctalDigit(peek()));

        unsigned n = consumeDigit();
        while (n < 32 && !atEndOfPattern() && WTF::isASCIIOctalDigit(peek()))
            n = n * 8 + consumeDigit();
        return n;
    }

    bool tryConsume(UChar ch)
    {
        if (atEndOfPattern() || (m_data[m_index] != ch))
            return false;
        ++m_index;
        return true;
    }

    int tryConsumeHex(int count)
    {
        ParseState state = saveState();

        int n = 0;
        while (count--) {
            if (atEndOfPattern() || !WTF::isASCIIHexDigit(peek())) {
                restoreState(state);
                return -1;
            }
            n = (n << 4) | WTF::toASCIIHexValue(consume());
        }
        return n;
    }

    Delegate& m_delegate;
    unsigned m_backReferenceLimit;
    ErrorCode m_err;
    const UChar* m_data;
    unsigned m_size;
    unsigned m_index;
    unsigned m_parenthesesNestingDepth;

    
    static const unsigned MAX_PATTERN_SIZE = 1024 * 1024;
};




























































template<class Delegate>
ErrorCode parse(Delegate& delegate, const UString& pattern, unsigned backReferenceLimit = quantifyInfinite)
{
    return Parser<Delegate>(delegate, pattern, backReferenceLimit).parse();
}

} } 

#endif 
