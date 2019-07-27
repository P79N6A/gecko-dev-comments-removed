




#include "WebGL2Context.h"
#include "GLContext.h"
#include "WebGLQuery.h"

namespace mozilla {












static const char*
GetQueryTargetEnumString(GLenum target)
{
    switch (target)
    {
    case LOCAL_GL_ANY_SAMPLES_PASSED:
        return "ANY_SAMPLES_PASSED";
    case LOCAL_GL_ANY_SAMPLES_PASSED_CONSERVATIVE:
        return "ANY_SAMPLES_PASSED_CONSERVATIVE";
    case LOCAL_GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN:
        return "TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN";
    default:
        break;
    }

    MOZ_ASSERT(false, "Unknown query `target`.");
    return "UNKNOWN_QUERY_TARGET";
}

static inline GLenum
SimulateOcclusionQueryTarget(const gl::GLContext* gl, GLenum target)
{
    MOZ_ASSERT(target == LOCAL_GL_ANY_SAMPLES_PASSED ||
               target == LOCAL_GL_ANY_SAMPLES_PASSED_CONSERVATIVE,
               "unknown occlusion query target");

    if (gl->IsSupported(gl::GLFeature::occlusion_query_boolean)) {
        return target;
    } else if (gl->IsSupported(gl::GLFeature::occlusion_query2)) {
        return LOCAL_GL_ANY_SAMPLES_PASSED;
    }

    return LOCAL_GL_SAMPLES_PASSED;
}

WebGLRefPtr<WebGLQuery>&
WebGLContext::GetQuerySlotByTarget(GLenum target)
{
    


    switch (target) {
    case LOCAL_GL_ANY_SAMPLES_PASSED:
    case LOCAL_GL_ANY_SAMPLES_PASSED_CONSERVATIVE:
        return mActiveOcclusionQuery;

    case LOCAL_GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN:
        return mActiveTransformFeedbackQuery;

    default:
        MOZ_CRASH("Should not get here.");
    }
}





already_AddRefed<WebGLQuery>
WebGL2Context::CreateQuery()
{
    if (IsContextLost())
        return nullptr;

    if (mActiveOcclusionQuery && !gl->IsGLES()) {
        





        GenerateWarning("createQuery: The WebGL 2 prototype might generate"
                        " INVALID_OPERATION when creating a query object while"
                        " one other is active.");
        



    }

    nsRefPtr<WebGLQuery> globj = new WebGLQuery(this);

    return globj.forget();
}

void
WebGL2Context::DeleteQuery(WebGLQuery* query)
{
    if (IsContextLost())
        return;

    if (!query)
        return;

    if (query->IsDeleted())
        return;

    if (query->IsActive())
        EndQuery(query->mType);

    if (mActiveOcclusionQuery && !gl->IsGLES()) {
        





        GenerateWarning("deleteQuery: The WebGL 2 prototype might generate"
                        " INVALID_OPERATION when deleting a query object while"
                        " one other is active.");
    }

    query->RequestDelete();
}

bool
WebGL2Context::IsQuery(WebGLQuery* query)
{
    if (IsContextLost())
        return false;

    if (!query)
        return false;

    return (ValidateObjectAllowDeleted("isQuery", query) &&
            !query->IsDeleted() &&
            query->HasEverBeenActive());
}

void
WebGL2Context::BeginQuery(GLenum target, WebGLQuery* query)
{
    if (IsContextLost())
        return;

    if (!ValidateQueryTarget(target, "beginQuery"))
        return;

    if (!query) {
        










        ErrorInvalidOperation("beginQuery: Query should not be null.");
        return;
    }

    if (query->IsDeleted()) {
        





        ErrorInvalidOperation("beginQuery: Query has been deleted.");
        return;
    }

    if (query->HasEverBeenActive() &&
        query->mType != target)
    {
        ErrorInvalidOperation("beginQuery: Target doesn't match with the query"
                              " type.");
        return;
    }

    WebGLRefPtr<WebGLQuery>& querySlot = GetQuerySlotByTarget(target);
    WebGLQuery* activeQuery = querySlot.get();
    if (activeQuery)
        return ErrorInvalidOperation("beginQuery: An other query already active.");

    if (!query->HasEverBeenActive())
        query->mType = target;

    MakeContextCurrent();

    if (target == LOCAL_GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN) {
        gl->fBeginQuery(LOCAL_GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN,
                        query->mGLName);
    } else {
        gl->fBeginQuery(SimulateOcclusionQueryTarget(gl, target),
                        query->mGLName);
    }

    UpdateBoundQuery(target, query);
}

void
WebGL2Context::EndQuery(GLenum target)
{
    if (IsContextLost())
        return;

    if (!ValidateQueryTarget(target, "endQuery"))
        return;

    WebGLRefPtr<WebGLQuery>& querySlot = GetQuerySlotByTarget(target);
    WebGLQuery* activeQuery = querySlot.get();

    if (!activeQuery || target != activeQuery->mType)
    {
        












        ErrorInvalidOperation("endQuery: There is no active query of type %s.",
                              GetQueryTargetEnumString(target));
        return;
    }

    MakeContextCurrent();

    if (target == LOCAL_GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN) {
        gl->fEndQuery(LOCAL_GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);
    } else {
        gl->fEndQuery(SimulateOcclusionQueryTarget(gl, target));
    }

    UpdateBoundQuery(target, nullptr);
}

already_AddRefed<WebGLQuery>
WebGL2Context::GetQuery(GLenum target, GLenum pname)
{
    if (IsContextLost())
        return nullptr;

    if (!ValidateQueryTarget(target, "getQuery"))
        return nullptr;

    if (pname != LOCAL_GL_CURRENT_QUERY) {
        


        ErrorInvalidEnum("getQuery: `pname` must be CURRENT_QUERY.");
        return nullptr;
    }

    WebGLRefPtr<WebGLQuery>& targetSlot = GetQuerySlotByTarget(target);

    nsRefPtr<WebGLQuery> tmp = targetSlot.get();
    return tmp.forget();
}

void
WebGL2Context::GetQueryParameter(JSContext*, WebGLQuery* query, GLenum pname,
                                 JS::MutableHandleValue retval)
{
    retval.set(JS::NullValue());

    if (IsContextLost())
        return;

    if (!query) {
        





        ErrorInvalidOperation("getQueryObject: `query` should not be null.");
        return;
    }

    if (query->IsDeleted()) {
        
        ErrorInvalidOperation("getQueryObject: `query` has been deleted.");
        return;
    }

    if (query->IsActive()) {
        
        ErrorInvalidOperation("getQueryObject: `query` is active.");
        return;
    }

    if (!query->HasEverBeenActive()) {
        



        ErrorInvalidOperation("getQueryObject: `query` has never been active.");
        return;
    }

    MakeContextCurrent();
    GLuint returned = 0;
    switch (pname) {
    case LOCAL_GL_QUERY_RESULT_AVAILABLE:
        gl->fGetQueryObjectuiv(query->mGLName, LOCAL_GL_QUERY_RESULT_AVAILABLE, &returned);
        retval.set(JS::BooleanValue(returned != 0));
        return;

    case LOCAL_GL_QUERY_RESULT:
        gl->fGetQueryObjectuiv(query->mGLName, LOCAL_GL_QUERY_RESULT, &returned);

        if (query->mType == LOCAL_GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN) {
            retval.set(JS::NumberValue(returned));
            return;
        }

        




        retval.set(JS::BooleanValue(returned != 0));
        return;

    default:
        break;
    }

    ErrorInvalidEnum("getQueryObject: `pname` must be QUERY_RESULT{_AVAILABLE}.");
}

void
WebGL2Context::UpdateBoundQuery(GLenum target, WebGLQuery* query)
{
    WebGLRefPtr<WebGLQuery>& querySlot = GetQuerySlotByTarget(target);
    querySlot = query;
}

bool
WebGL2Context::ValidateQueryTarget(GLenum target, const char* info)
{
    switch (target) {
    case LOCAL_GL_ANY_SAMPLES_PASSED:
    case LOCAL_GL_ANY_SAMPLES_PASSED_CONSERVATIVE:
    case LOCAL_GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN:
        return true;

    default:
        ErrorInvalidEnumInfo(info, target);
        return false;
    }
}

} 
