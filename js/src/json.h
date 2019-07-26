





#ifndef json_h
#define json_h

#include "mozilla/Range.h"

#include "NamespaceImports.h"

#include "js/RootingAPI.h"

namespace js {
class StringBuffer;
}

extern JSObject *
js_InitJSONClass(JSContext *cx, js::HandleObject obj);

extern bool
js_Stringify(JSContext *cx, js::MutableHandleValue vp, JSObject *replacer,
             js::Value space, js::StringBuffer &sb);

namespace js {

template <typename CharT>
extern bool
ParseJSONWithReviver(JSContext *cx, mozilla::Range<const CharT> chars,
                     HandleValue reviver, MutableHandleValue vp);

} 

#endif 
