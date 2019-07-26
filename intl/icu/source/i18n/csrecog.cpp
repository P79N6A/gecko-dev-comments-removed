






#include "unicode/utypes.h"

#if !UCONFIG_NO_CONVERSION

#include "csrecog.h"

U_NAMESPACE_BEGIN

CharsetRecognizer::~CharsetRecognizer()
{
    
}

const char *CharsetRecognizer::getLanguage() const
{
    return "";
}

U_NAMESPACE_END    

#endif
