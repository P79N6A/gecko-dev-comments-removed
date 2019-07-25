






#include "mozilla/Assertions.h"
#include "mozilla/Attributes.h"

#include "jsscope.h"

#include "ObjectImpl.h"

#include "ObjectImpl-inl.h"

using namespace js;

static ObjectElements emptyElementsHeader(0, 0);


HeapValue *js::emptyObjectElements =
    reinterpret_cast<HeapValue *>(uintptr_t(&emptyElementsHeader) + sizeof(ObjectElements));

#if defined(_MSC_VER) && _MSC_VER >= 1500





MOZ_NEVER_INLINE
#endif
const Shape *
js::ObjectImpl::nativeLookup(JSContext *cx, jsid id)
{
    MOZ_ASSERT(isNative());
    Shape **spp;
    return Shape::search(cx, lastProperty(), id, &spp);
}
