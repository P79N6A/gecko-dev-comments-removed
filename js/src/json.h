





#ifndef json_h
#define json_h

#include "mozilla/Range.h"

#include "NamespaceImports.h"

#include "js/RootingAPI.h"

namespace js {
class StringBuffer;

extern JSObject *
InitJSONClass(JSContext *cx, HandleObject obj);

extern bool
Stringify(JSContext *cx, js::MutableHandleValue vp, JSObject *replacer,
          Value space, StringBuffer &sb);

template <typename CharT>
extern bool
ParseJSONWithReviver(JSContext *cx, const mozilla::Range<const CharT> chars,
                     HandleValue reviver, MutableHandleValue vp);

} 

#endif 
