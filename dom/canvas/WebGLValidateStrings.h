

























#ifndef WEBGL_VALIDATE_STRINGS_H_
#define WEBGL_VALIDATE_STRINGS_H_

#include "nsString.h"
#include "nsTArray.h"

namespace mozilla {

class WebGLContext;









class StripComments {
public:
    explicit StripComments(const nsAString& str)
        : m_parseState(BeginningOfLine)
        , m_end(str.EndReading())
        , m_current(str.BeginReading())
        , m_position(0)
    {
        m_result.SetLength(str.Length());
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



bool ValidateGLSLString(const nsAString& string, WebGLContext* webgl,
                        const char* funcName);

bool ValidateGLSLVariableName(const nsAString& name, WebGLContext* webgl,
                              const char* funcName);

} 

#endif 
