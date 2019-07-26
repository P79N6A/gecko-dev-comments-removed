






#include "jscntxt.h"

#include "js/CharacterEncoding.h"

using namespace JS;

Latin1CharsZ
JS::LossyTwoByteCharsToNewLatin1CharsZ(JSContext *cx, TwoByteChars tbchars)
{
    AutoAssertNoGC nogc;
    JS_ASSERT(cx);
    size_t len = tbchars.length();
    unsigned char *latin1 = cx->pod_malloc<unsigned char>(len + 1);
    if (!latin1)
        return Latin1CharsZ();
    for (size_t i = 0; i < len; ++i)
        latin1[i] = static_cast<unsigned char>(tbchars[i]);
    latin1[len] = '\0';
    return Latin1CharsZ(latin1, len);
}

