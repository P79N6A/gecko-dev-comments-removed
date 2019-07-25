
























#ifndef RegexParser_h
#define RegexParser_h

#include <limits.h>
#include <wtf/ASCIICType.h>
#include "yarr/jswtfbridge.h"
#include "yarr/yarr/RegexCommon.h"

namespace JSC { namespace Yarr {

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
    friend int parse(FriendDelegate& delegate, const UString& pattern, unsigned backReferenceLimit);

    








    class CharacterClassParserDelegate {
    public:
        CharacterClassParserDelegate(Delegate& delegate, ErrorCode& err)
            : m_delegate(delegate)
            , m_err(err)
            , m_state(empty)
#if __GNUC__ >= 4 && __GNUC_MINOR__ >= 5 
            , m_character(0xFFFF)
#endif
        {
        }

        




        void begin(bool invert)
        {
            m_delegate.atomCharacterClassBegin(invert);
        }

        








        void atomPatternCharacterUnescaped(UChar ch)
        {
            switch (m_state) {
            case empty:
                m_character = ch;
                m_state = cachedCharacter;
                break;

            case cachedCharacter:
                if (ch == '-')
                    m_state = cachedCharacterHyphen;
                else {
                    m_delegate.atomCharacterClassAtom(m_character);
                    m_character = ch;
                }
                break;

            case cachedCharacterHyphen:
                if (ch >= m_character)
                    m_delegate.atomCharacterClassRange(m_character, ch);
                else
                    m_err = CharacterClassOutOfOrder;
                m_state = empty;
            }
        }

        





        void atomPatternCharacter(UChar ch)
        {
            
            
            if((ch == '-') && (m_state == cachedCharacter))
                flush();

            atomPatternCharacterUnescaped(ch);
        }

        




        void atomBuiltInCharacterClass(BuiltInCharacterClassID classID, bool invert)
        {
            if (m_state == cachedCharacterHyphen) {
                
                
                
                
                m_err = CharacterClassRangeSingleChar;
                m_state = empty;
                return;
            }
            flush();
            m_delegate.atomCharacterClassBuiltIn(classID, invert);
        }

        




        void end()
        {
            flush();
            m_delegate.atomCharacterClassEnd();
        }

        
        
        void assertionWordBoundary(bool) { JS_NOT_REACHED("parseEscape() should never call this"); }
        void atomBackReference(unsigned) { JS_NOT_REACHED("parseEscape() should never call this"); }

    private:
        void flush()
        {
            if (m_state != empty) 
                m_delegate.atomCharacterClassAtom(m_character);
            if (m_state == cachedCharacterHyphen)
                m_delegate.atomCharacterClassAtom('-');
            m_state = empty;
        }
    
        Delegate& m_delegate;
        ErrorCode& m_err;
        enum CharacterClassConstructionState {
            empty,
            cachedCharacter,
            cachedCharacterHyphen
        } m_state;
        UChar m_character;
    };

    Parser(Delegate& delegate, const UString& pattern, unsigned backReferenceLimit)
        : m_delegate(delegate)
        , m_backReferenceLimit(backReferenceLimit)
        , m_err(NoError)
        , m_data(const_cast<UString &>(pattern).chars())
        , m_size(pattern.length())
        , m_index(0)
        , m_parenthesesNestingDepth(0)
    {
    }
    
    



















    template<bool inCharacterClass, class EscapeDelegate>
    bool parseEscape(EscapeDelegate& delegate)
    {
        JS_ASSERT(!m_err);
        JS_ASSERT(peek() == '\\');
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
                    return false;
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
        JS_ASSERT(!m_err);
        JS_ASSERT(peek() == '[');
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
                characterClassConstructor.atomPatternCharacterUnescaped(consume());
            }

            if (m_err)
                return;
        }

        m_err = CharacterClassUnmatched;
    }

    




    void parseParenthesesBegin()
    {
        JS_ASSERT(!m_err);
        JS_ASSERT(peek() == '(');
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
        JS_ASSERT(!m_err);
        JS_ASSERT(peek() == ')');
        consume();

        if (m_parenthesesNestingDepth > 0)
            m_delegate.atomParenthesesEnd();
        else
            m_err = ParenthesesUnmatched;

        --m_parenthesesNestingDepth;
    }

    




    void parseQuantifier(bool lastTokenWasAnAtom, unsigned min, unsigned max)
    {
        JS_ASSERT(!m_err);
        JS_ASSERT(min <= max);

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
                parseQuantifier(lastTokenWasAnAtom, 0, UINT_MAX);
                lastTokenWasAnAtom = false;
                break;

            case '+':
                consume();
                parseQuantifier(lastTokenWasAnAtom, 1, UINT_MAX);
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
                            max = UINT_MAX;
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

    






    int parse()
    {
        m_delegate.regexBegin();

        if (m_size > MAX_PATTERN_SIZE)
            m_err = PatternTooLarge;
        else
            parseTokens();
        JS_ASSERT(atEndOfPattern() || m_err);

        if (m_err)
            m_delegate.regexError();
        else
            m_delegate.regexEnd();

        return static_cast<int>(m_err);
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
        JS_ASSERT(m_index <= m_size);
        return m_index == m_size;
    }

    int peek()
    {
        JS_ASSERT(m_index < m_size);
        return m_data[m_index];
    }

    bool peekIsDigit()
    {
        return !atEndOfPattern() && WTF::isASCIIDigit(peek());
    }

    unsigned peekDigit()
    {
        JS_ASSERT(peekIsDigit());
        return peek() - '0';
    }

    int consume()
    {
        JS_ASSERT(m_index < m_size);
        return m_data[m_index++];
    }

    unsigned consumeDigit()
    {
        JS_ASSERT(peekIsDigit());
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
        JS_ASSERT(WTF::isASCIIOctalDigit(peek()));

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
int parse(Delegate& delegate, const UString& pattern, unsigned backReferenceLimit = UINT_MAX)
{
    return Parser<Delegate>(delegate, pattern, backReferenceLimit).parse();
}

} } 

#endif 
