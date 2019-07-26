







#include "mozilla/Assertions.h"

#include "js/PropertyKey.h"
#include "js/RootingAPI.h"
#include "js/Value.h"
#include "vm/String.h"

#include "jsinferinlines.h"
#include "jsatominlines.h"

using namespace js;

bool
JS::detail::ToPropertyKeySlow(JSContext *cx, HandleValue v, PropertyKey *key)
{
    MOZ_ASSERT_IF(v.isInt32(), v.toInt32() < 0);

    RootedAtom atom(cx);
    if (!ToAtom<CanGC>(cx, v))
        return false;

    uint32_t index;
    if (atom->isIndex(&index)) {
        *key = PropertyKey(index);
        return true;
    }

    key->v.setString(atom->asPropertyName());
    return true;
}
