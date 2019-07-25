

























#ifndef WEBGLVALIDATESTRINGS_H_
#define WEBGLVALIDATESTRINGS_H_

#include "WebGLContext.h"

namespace mozilla {






    bool WebGLContext::ValidateGLSLCharacter(PRUnichar c)
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
        StripComments(const nsAString& str)
            : m_parseState(BeginningOfLine)
            , m_end(str.EndReading())
            , m_current(str.BeginReading())
            , m_position(0)
        {
            m_result.SetLength(str.Length());
            parse();
        }

        const nsTArray<PRUnichar>& result()
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

        void process(PRUnichar);

        bool peek(PRUnichar& character)
        {
            if (m_current + 1 >= m_end)
                return false;
            character = *(m_current + 1);
            return true;
        }

        PRUnichar current()
        {
            
            return *m_current;
        }

        void advance()
        {
            ++m_current;
        }

        bool isNewline(PRUnichar character)
        {
            
            return (character == '\n' || character == '\r');
        }

        void emit(PRUnichar character)
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
        const PRUnichar* m_end;
        const PRUnichar* m_current;
        size_t m_position;
        nsTArray<PRUnichar> m_result;
    };

    void StripComments::process(PRUnichar c)
    {
        if (isNewline(c)) {
            
            
            emit(c);

            if (m_parseState != InMultiLineComment)
                m_parseState = BeginningOfLine;

            return;
        }

        PRUnichar temp = 0;
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
