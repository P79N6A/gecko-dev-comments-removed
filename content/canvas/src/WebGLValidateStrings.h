

























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


} 

#endif 
