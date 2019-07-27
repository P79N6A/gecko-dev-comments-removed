




#include "WebGLContext.h"

#include "GLContext.h"
#include "WebGLBuffer.h"
#include "WebGLVertexArray.h"
#include "WebGLVertexAttribData.h"

namespace mozilla {

void
WebGLContext::BindVertexArray(WebGLVertexArray* array)
{
    if (IsContextLost())
        return;

    if (!ValidateObjectAllowDeletedOrNull("bindVertexArrayObject", array))
        return;

    if (array && array->IsDeleted()) {
        





        ErrorInvalidOperation("bindVertexArray: can't bind a deleted array!");
        return;
    }

    InvalidateBufferFetching();

    MakeContextCurrent();

    if (array == nullptr) {
        array = mDefaultVertexArray;
    }

    array->BindVertexArray();

    MOZ_ASSERT(mBoundVertexArray == array);
}

already_AddRefed<WebGLVertexArray>
WebGLContext::CreateVertexArray()
{
    if (IsContextLost())
        return nullptr;

    nsRefPtr<WebGLVertexArray> globj = CreateVertexArrayImpl();

    MakeContextCurrent();
    globj->GenVertexArray();

    return globj.forget();
}

WebGLVertexArray*
WebGLContext::CreateVertexArrayImpl()
{
    return WebGLVertexArray::Create(this);
}

void
WebGLContext::DeleteVertexArray(WebGLVertexArray* array)
{
    if (IsContextLost())
        return;

    if (array == nullptr)
        return;

    if (array->IsDeleted())
        return;

    if (mBoundVertexArray == array)
        BindVertexArray(static_cast<WebGLVertexArray*>(nullptr));

    array->RequestDelete();
}

bool
WebGLContext::IsVertexArray(WebGLVertexArray* array)
{
    if (IsContextLost())
        return false;

    if (!array)
        return false;

    if (!ValidateObjectAllowDeleted("isVertexArray", array))
        return false;

    if (array->IsDeleted())
        return false;

    MakeContextCurrent();
    return array->IsVertexArray();
}

} 
