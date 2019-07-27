




#include "WebGLValidateStrings.h"

#include "nsString.h"
#include "WebGLContext.h"

namespace mozilla {





bool IsValidGLSLCharacter(char16_t c)
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



bool
ValidateGLSLString(const nsAString& string, WebGLContext* webgl, const char* funcName)
{
    for (size_t i = 0; i < string.Length(); ++i) {
        if (!IsValidGLSLCharacter(string.CharAt(i))) {
           webgl->ErrorInvalidValue("%s: String contains the illegal character '%d'",
                                    funcName, string.CharAt(i));
           return false;
        }
    }

    return true;
}

bool
ValidateGLSLVariableName(const nsAString& name, WebGLContext* webgl, const char* funcName)
{
    if (name.IsEmpty())
        return false;

    const uint32_t maxSize = 256;
    if (name.Length() > maxSize) {
        webgl->ErrorInvalidValue("%s: Identifier is %d characters long, exceeds the"
                                 " maximum allowed length of %d characters.",
                                 funcName, name.Length(), maxSize);
        return false;
    }

    if (!ValidateGLSLString(name, webgl, funcName))
        return false;

    nsString prefix1 = NS_LITERAL_STRING("webgl_");
    nsString prefix2 = NS_LITERAL_STRING("_webgl_");

    if (Substring(name, 0, prefix1.Length()).Equals(prefix1) ||
        Substring(name, 0, prefix2.Length()).Equals(prefix2))
    {
        webgl->ErrorInvalidOperation("%s: String contains a reserved GLSL prefix.",
                                     funcName);
        return false;
    }

    return true;
}

} 
