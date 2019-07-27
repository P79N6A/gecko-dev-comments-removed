




#include "WebGLContext.h"
#include "WebGLQuery.h"
#include "GLContext.h"

using namespace mozilla;











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

already_AddRefed<WebGLQuery>
WebGLContext::CreateQuery()
{
    if (IsContextLost())
        return nullptr;

    if (mActiveOcclusionQuery && !gl->IsGLES()) {
        




        GenerateWarning("createQuery: the WebGL 2 prototype might generate INVALID_OPERATION"
                        "when creating a query object while one other is active.");
        



    }

    nsRefPtr<WebGLQuery> globj = new WebGLQuery(this);

    return globj.forget();
}

void
WebGLContext::DeleteQuery(WebGLQuery *query)
{
    if (IsContextLost())
        return;

    if (!query)
        return;

    if (query->IsDeleted())
        return;

    if (query->IsActive()) {
        EndQuery(query->mType);
    }

    if (mActiveOcclusionQuery && !gl->IsGLES()) {
        




        GenerateWarning("deleteQuery: the WebGL 2 prototype might generate INVALID_OPERATION"
                        "when deleting a query object while one other is active.");
    }

    query->RequestDelete();
}

void
WebGLContext::BeginQuery(GLenum target, WebGLQuery *query)
{
    if (IsContextLost())
        return;

    WebGLRefPtr<WebGLQuery>* targetSlot = GetQueryTargetSlot(target, "beginQuery");
    if (!targetSlot) {
        return;
    }

    if (!query) {
        










        ErrorInvalidOperation("beginQuery: query should not be null");
        return;
    }

    if (query->IsDeleted()) {
        




        ErrorInvalidOperation("beginQuery: query has been deleted");
        return;
    }

    if (query->HasEverBeenActive() &&
        query->mType != target)
    {
        


        ErrorInvalidOperation("beginQuery: target doesn't match with the query type");
        return;
    }

    if (*targetSlot) {
        


        ErrorInvalidOperation("beginQuery: an other query already active");
        return;
    }

    if (!query->HasEverBeenActive()) {
        query->mType = target;
    }

    MakeContextCurrent();

    if (target == LOCAL_GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN) {
        gl->fBeginQuery(LOCAL_GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, query->mGLName);
    } else {
        gl->fBeginQuery(SimulateOcclusionQueryTarget(gl, target), query->mGLName);
    }

    *targetSlot = query;
}

void
WebGLContext::EndQuery(GLenum target)
{
    if (IsContextLost())
        return;

    WebGLRefPtr<WebGLQuery>* targetSlot = GetQueryTargetSlot(target, "endQuery");
    if (!targetSlot) {
        return;
    }

    if (!*targetSlot ||
        target != (*targetSlot)->mType)
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

    *targetSlot = nullptr;
}

bool
WebGLContext::IsQuery(WebGLQuery *query)
{
    if (IsContextLost())
        return false;

    if (!query)
        return false;

    return ValidateObjectAllowDeleted("isQuery", query) &&
           !query->IsDeleted() &&
           query->HasEverBeenActive();
}

already_AddRefed<WebGLQuery>
WebGLContext::GetQuery(GLenum target, GLenum pname)
{
    if (IsContextLost())
        return nullptr;

    WebGLRefPtr<WebGLQuery>* targetSlot = GetQueryTargetSlot(target, "getQuery");
    if (!targetSlot) {
        return nullptr;
    }

    if (pname != LOCAL_GL_CURRENT_QUERY) {
        


        ErrorInvalidEnum("getQuery: pname must be CURRENT_QUERY");
        return nullptr;
    }

    nsRefPtr<WebGLQuery> tmp = targetSlot->get();
    return tmp.forget();
}

JS::Value
WebGLContext::GetQueryObject(JSContext* cx, WebGLQuery *query, GLenum pname)
{
    if (IsContextLost())
        return JS::NullValue();

    if (!query) {
        




        ErrorInvalidOperation("getQueryObject: query should not be null");
        return JS::NullValue();
    }

    if (query->IsDeleted()) {
        
        ErrorInvalidOperation("getQueryObject: query has been deleted");
        return JS::NullValue();
    }

    if (query->IsActive()) {
        
        ErrorInvalidOperation("getQueryObject: query is active");
        return JS::NullValue();
    }

    if (!query->HasEverBeenActive()) {
        



        ErrorInvalidOperation("getQueryObject: query has never been active");
        return JS::NullValue();
    }

    switch (pname)
    {
        case LOCAL_GL_QUERY_RESULT_AVAILABLE:
        {
            GLuint returned = 0;

            MakeContextCurrent();
            gl->fGetQueryObjectuiv(query->mGLName, LOCAL_GL_QUERY_RESULT_AVAILABLE, &returned);

            return JS::BooleanValue(returned != 0);
        }

        case LOCAL_GL_QUERY_RESULT:
        {
            GLuint returned = 0;

            MakeContextCurrent();
            gl->fGetQueryObjectuiv(query->mGLName, LOCAL_GL_QUERY_RESULT, &returned);

            if (query->mType == LOCAL_GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN) {
                return JS::NumberValue(uint32_t(returned));
            }

            




            return JS::BooleanValue(returned != 0);
        }

        default:
            break;
    }

    ErrorInvalidEnum("getQueryObject: pname must be QUERY_RESULT{_AVAILABLE}");
    return JS::NullValue();
}

WebGLRefPtr<WebGLQuery>*
WebGLContext::GetQueryTargetSlot(GLenum target, const char* infos)
{
    switch (target) {
        case LOCAL_GL_ANY_SAMPLES_PASSED:
        case LOCAL_GL_ANY_SAMPLES_PASSED_CONSERVATIVE:
            return &mActiveOcclusionQuery;
        case LOCAL_GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN:
            return &mActiveTransformFeedbackQuery;
    }

    ErrorInvalidEnum("%s: unknown query target", infos);
    return nullptr;
}


