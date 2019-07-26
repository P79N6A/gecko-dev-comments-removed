








#ifndef js_PropertyKey_h___
#define js_PropertyKey_h___

#include "mozilla/Attributes.h"

#include "js/Value.h"

struct JSContext;

namespace JS {

class PropertyKey;

namespace detail {

extern JS_PUBLIC_API(bool)
ToPropertyKeySlow(JSContext *cx, HandleValue v, PropertyKey *key);

} 









class PropertyKey
{
    Value v;
    friend JS_PUBLIC_API(bool) detail::ToPropertyKeySlow(JSContext *cx, HandleValue v, PropertyKey *key);

  public:
    explicit PropertyKey(uint32_t index) : v(PrivateUint32Value(index)) {}

    




    bool isIndex(uint32_t *index) {
        
        
        
        if (!v.isInt32())
            return false;
        *index = v.toPrivateUint32();
        return true;
    }

    




    bool isName(JSString **str) {
        uint32_t dummy;
        if (isIndex(&dummy))
            return false;
        *str = v.toString();
        return true;
    }

    






    bool isSymbol() {
        return false;
    }
};

inline bool
ToPropertyKey(JSContext *cx, HandleValue v, PropertyKey *key)
{
    if (v.isInt32() && v.toInt32() >= 0) {
        *key = PropertyKey(uint32_t(v.toInt32()));
        return true;
    }

    return detail::ToPropertyKeySlow(cx, v, key);
}

} 

#endif 
