






































#ifndef jswtfbridge_h__
#define jswtfbridge_h__





#include "assembler/wtf/Platform.h"
#include "jsstr.h"
#include "jsprvtd.h"
#include "jstl.h"

typedef jschar UChar;
typedef JSString UString;

template <typename T>
class ValueDeleter
{
  public:
    void operator()(T &t) { delete t; }
};

template<typename T, size_t N, class AP>
static inline void
deleteAllValues(js::Vector<T,N,AP> &vector)
{
    js::ForEach(vector.begin(), vector.end(), ValueDeleter<T>());
}

class Unicode {
  public:
    static UChar toUpper(UChar c) { return JS_TOUPPER(c); }
    static UChar toLower(UChar c) { return JS_TOLOWER(c); }
};

#endif
