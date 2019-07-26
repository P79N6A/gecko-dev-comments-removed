




#include "WebGLContext.h"
#include "WebGLQuery.h"

using namespace mozilla;











static const char*
GetQueryTargetEnumString(WebGLenum target)
{
    switch (target)
    {
        case LOCAL_GL_ANY_SAMPLES_PASSED:
            return "ANY_SAMPLES_PASSED";
        case LOCAL_GL_ANY_SAMPLES_PASSED_CONSERVATIVE:
            return "ANY_SAMPLES_PASSED_CONSERVATIVE";
        default:
            break;
    }
    
    MOZ_ASSERT(false, "Unknown query `target`.");
    return "UNKNOWN_QUERY_TARGET";
}

already_AddRefed<WebGLQuery>
WebGLContext::CreateQuery()
{
    if (!IsContextStable())
        return nullptr;

    if (mActiveOcclusionQuery && !gl->IsGLES2()) {
        




        GenerateWarning("createQuery: the WebGL 2 prototype might generate INVALID_OPERATION"
                        "when creating a query object while one other is active.");
        



    }

    nsRefPtr<WebGLQuery> globj = new WebGLQuery(this);

    return globj.forget();
}

void
WebGLContext::DeleteQuery(WebGLQuery *query)
{
    if (!IsContextStable())
        return;

    if (!query)
        return;

    if (query->IsDeleted())
        return;

    if (query->IsActive()) {
        EndQuery(query->mType);
    }

    if (mActiveOcclusionQuery && !gl->IsGLES2()) {
        




        GenerateWarning("deleteQuery: the WebGL 2 prototype might generate INVALID_OPERATION"
                        "when deleting a query object while one other is active.");
    }

    query->RequestDelete();
}

void
WebGLContext::BeginQuery(WebGLenum target, WebGLQuery *query)
{
    if (!IsContextStable())
        return;

    if (!ValidateTargetParameter(target, "beginQuery")) {
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

    if (GetActiveQueryByTarget(target)) {
        


        ErrorInvalidOperation("beginQuery: an other query already active");
        return;
    }

    if (!query->HasEverBeenActive()) {
        query->mType = target;
    }

    MakeContextCurrent();

    if (!gl->IsGLES2()) {
        gl->fBeginQuery(LOCAL_GL_SAMPLES_PASSED, query->mGLName);
    } else {
        gl->fBeginQuery(target, query->mGLName);
    }

    GetActiveQueryByTarget(target) = query;
}

void
WebGLContext::EndQuery(WebGLenum target)
{
    if (!IsContextStable())
        return;

    if (!ValidateTargetParameter(target, "endQuery")) {
        return;
    }

    if (!GetActiveQueryByTarget(target) ||
        target != GetActiveQueryByTarget(target)->mType)
    {
        











        ErrorInvalidOperation("endQuery: There is no active query of type %s.",
                              GetQueryTargetEnumString(target));
        return;
    }

    MakeContextCurrent();

    if (!gl->IsGLES2()) {
        gl->fEndQuery(LOCAL_GL_SAMPLES_PASSED);
    } else {
        gl->fEndQuery(target);
    }

    GetActiveQueryByTarget(target) = nullptr;
}

bool
WebGLContext::IsQuery(WebGLQuery *query)
{
    if (!IsContextStable())
        return false;

    if (!query)
        return false;

    return ValidateObjectAllowDeleted("isQuery", query) &&
           !query->IsDeleted() &&
           query->HasEverBeenActive();
}

already_AddRefed<WebGLQuery>
WebGLContext::GetQuery(WebGLenum target, WebGLenum pname)
{
    if (!IsContextStable())
        return nullptr;

    if (!ValidateTargetParameter(target, "getQuery")) {
        return nullptr;
    }

    if (pname != LOCAL_GL_CURRENT_QUERY) {
        


        ErrorInvalidEnum("getQuery: pname must be CURRENT_QUERY");
        return nullptr;
    }

    nsRefPtr<WebGLQuery> tmp = GetActiveQueryByTarget(target).get();
    return tmp.forget();
}

JS::Value
WebGLContext::GetQueryObject(JSContext* cx, WebGLQuery *query, WebGLenum pname)
{
    if (!IsContextStable())
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

            




            return JS::BooleanValue(returned != 0);
        }

        default:
            break;
    }

    ErrorInvalidEnum("getQueryObject: pname must be QUERY_RESULT{_AVAILABLE}");
    return JS::NullValue();
}

bool
WebGLContext::ValidateTargetParameter(WebGLenum target, const char* infos)
{
    if (target != LOCAL_GL_ANY_SAMPLES_PASSED &&
        target != LOCAL_GL_ANY_SAMPLES_PASSED_CONSERVATIVE)
    {
        ErrorInvalidEnum("%s: target must be ANY_SAMPLES_PASSED{_CONSERVATIVE}", infos);
        return false;
    }

    return true;
}

WebGLRefPtr<WebGLQuery>&
WebGLContext::GetActiveQueryByTarget(WebGLenum target)
{
    MOZ_ASSERT(ValidateTargetParameter(target, "private WebGLContext::GetActiveQueryByTarget"));

    return mActiveOcclusionQuery;
}


