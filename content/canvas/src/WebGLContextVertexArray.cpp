




#include "WebGLContext.h"
#include "WebGLBuffer.h"
#include "WebGLVertexAttribData.h"
#include "WebGLVertexArray.h"
#include "GLContext.h"

using namespace mozilla;

void
WebGLContext::BindVertexArray(WebGLVertexArray *array)
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

    if (array) {
        gl->fBindVertexArray(array->GLName());
        array->SetHasEverBeenBound(true);
        mBoundVertexArray = array;
    }
    else {
        gl->fBindVertexArray(0);
        mBoundVertexArray = mDefaultVertexArray;
    }
}

already_AddRefed<WebGLVertexArray>
WebGLContext::CreateVertexArray()
{
    if (IsContextLost())
        return nullptr;

    nsRefPtr<WebGLVertexArray> globj = new WebGLVertexArray(this);

    MakeContextCurrent();
    gl->fGenVertexArrays(1, &globj->mGLName);

    mVertexArrays.insertBack(globj);

    return globj.forget();
}

void
WebGLContext::DeleteVertexArray(WebGLVertexArray *array)
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
WebGLContext::IsVertexArray(WebGLVertexArray *array)
{
    if (IsContextLost())
        return false;

    if (!array)
        return false;

    return ValidateObjectAllowDeleted("isVertexArray", array) &&
           !array->IsDeleted() &&
           array->HasEverBeenBound();
}


