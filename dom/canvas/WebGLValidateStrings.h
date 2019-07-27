

























#ifndef WEBGLVALIDATESTRINGS_H_
#define WEBGLVALIDATESTRINGS_H_

#include "WebGLContext.h"

namespace mozilla {






    bool WebGLContext::ValidateGLSLCharacter(char16_t c)
    {
        
        if (c >= 32 && c <= 126 &&
            c != '"' && c != '$' && c != '`' && c != '@' && c != '\\' && c != '\'')
        {
             return true;
        }

        
        if (c >= 9 && c <= 13) {
             return true;
        }

        return false;
    }

    
    
    
    class StripComments {
    public:
        explicit StripComments(const nsAString& aStr)
            : m_parseState(BeginningOfLine)
            , m_end(aStr.EndReading())
            , m_current(aStr.BeginReading())
            , m_position(0)
        {
            m_result.SetLength(aStr.Length());
            parse();
        }

        const nsTArray<char16_t>& result()
        {
            return m_result;
        }

        size_t length()
        {
            return m_position;
        }

    private:
        bool hasMoreCharacters()
        {
            return (m_current < m_end);
        }

        void parse()
        {
            while (hasMoreCharacters()) {
                process(current());
                
                if (hasMoreCharacters())
                    advance();
            }
        }

        void process(char16_t);

        bool peek(char16_t& character)
        {
            if (m_current + 1 >= m_end)
                return false;
            character = *(m_current + 1);
            return true;
        }

        char16_t current()
        {
            
            return *m_current;
        }

        void advance()
        {
            ++m_current;
        }

        bool isNewline(char16_t character)
        {
            
            return (character == '\n' || character == '\r');
        }

        void emit(char16_t character)
        {
            m_result[m_position++] = character;
        }

        enum ParseState {
            
            
            
            BeginningOfLine,

            
            
            MiddleOfLine,

            
            
            
            InPreprocessorDirective,

            
            
            InSingleLineComment,

            
            
            InMultiLineComment
        };

        ParseState m_parseState;
        const char16_t* m_end;
        const char16_t* m_current;
        size_t m_position;
        nsTArray<char16_t> m_result;
    };

    void StripComments::process(char16_t c)
    {
        if (isNewline(c)) {
            
            
            emit(c);

            if (m_parseState != InMultiLineComment)
                m_parseState = BeginningOfLine;

            return;
        }

        char16_t temp = 0;
        switch (m_parseState) {
        case BeginningOfLine:
            
            if (c <= ' ' && (c == ' ' || (c <= 0xD && c >= 0x9))) {
                emit(c);
                break;
            }

            if (c == '#') {
                m_parseState = InPreprocessorDirective;
                emit(c);
                break;
            }

            
            m_parseState = MiddleOfLine;
            process(c);
            break;

        case MiddleOfLine:
            if (c == '/' && peek(temp)) {
                if (temp == '/') {
                    m_parseState = InSingleLineComment;
                    emit(' ');
                    advance();
                    break;
                }

                if (temp == '*') {
                    m_parseState = InMultiLineComment;
                    
                    
                    
                    emit('/');
                    emit('*');
                    advance();
                    break;
                }
            }

            emit(c);
            break;

        case InPreprocessorDirective:
            
            
            
            
            emit(c);
            break;

        case InSingleLineComment:
            
            
            
            break;

        case InMultiLineComment:
            if (c == '*' && peek(temp) && temp == '/') {
                emit('*');
                emit('/');
                m_parseState = MiddleOfLine;
                advance();
                break;
            }

            
            
            
            break;
        }
    }



} 

#endif 
