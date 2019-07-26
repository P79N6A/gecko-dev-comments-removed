





#ifndef json_h___
#define json_h___

#include "js/CharacterEncoding.h"
#include "js/RootingAPI.h"
#include "js/Value.h"
#include "js/Vector.h"
#include "vm/StringBuffer.h"

extern JSObject *
js_InitJSONClass(JSContext *cx, js::HandleObject obj);

extern JSBool
js_Stringify(JSContext *cx, js::MutableHandleValue vp, JSObject *replacer,
             js::Value space, js::StringBuffer &sb);

namespace js {

extern bool
ParseJSONWithReviver(JSContext *cx, JS::StableCharPtr chars, size_t length, HandleValue reviver,
                     MutableHandleValue vp);

} 

#endif 
