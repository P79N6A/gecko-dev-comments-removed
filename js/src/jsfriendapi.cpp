






































#include "jscntxt.h"
#include "jsfriendapi.h"

JS_FRIEND_API(JSString *)
JS_GetAnonymousString(JSRuntime *rt)
{
    JS_ASSERT(rt->state == JSRTS_UP);
    return ATOM_TO_STRING(rt->atomState.anonymousAtom);
}
