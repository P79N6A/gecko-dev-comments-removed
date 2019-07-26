





#include "HttpLog.h"

#include "nsHttpRequestHead.h"





namespace mozilla {
namespace net {

nsHttpRequestHead::nsHttpRequestHead()
    : mMethod(NS_LITERAL_CSTRING("GET"))
    , mVersion(NS_HTTP_VERSION_1_1)
    , mParsedMethod(kMethod_Get)
{
}

void
nsHttpRequestHead::SetMethod(const nsACString &method)
{
    mParsedMethod = kMethod_Custom;
    mMethod = method;
    if (!strcmp(mMethod.get(), "GET")) {
        mParsedMethod = kMethod_Get;
    } else if (!strcmp(mMethod.get(), "POST")) {
        mParsedMethod = kMethod_Post;
    } else if (!strcmp(mMethod.get(), "OPTIONS")) {
        mParsedMethod = kMethod_Options;
    } else if (!strcmp(mMethod.get(), "CONNECT")) {
        mParsedMethod = kMethod_Connect;
    } else if (!strcmp(mMethod.get(), "HEAD")) {
        mParsedMethod = kMethod_Head;
    } else if (!strcmp(mMethod.get(), "PUT")) {
        mParsedMethod = kMethod_Put;
    } else if (!strcmp(mMethod.get(), "TRACE")) {
        mParsedMethod = kMethod_Trace;
    }
}

bool
nsHttpRequestHead::IsSafeMethod() const
{
  
  
    if (IsGet() || IsHead() || IsOptions() || IsTrace()) {
        return true;
    }

    if (mParsedMethod != kMethod_Custom) {
        return false;
    }

    return (!strcmp(mMethod.get(), "PROPFIND") ||
            !strcmp(mMethod.get(), "REPORT") ||
            !strcmp(mMethod.get(), "SEARCH"));
}

void
nsHttpRequestHead::Flatten(nsACString &buf, bool pruneProxyHeaders)
{
    

    buf.Append(mMethod);
    buf.Append(' ');
    buf.Append(mRequestURI);
    buf.AppendLiteral(" HTTP/");

    switch (mVersion) {
    case NS_HTTP_VERSION_1_1:
        buf.AppendLiteral("1.1");
        break;
    case NS_HTTP_VERSION_0_9:
        buf.AppendLiteral("0.9");
        break;
    default:
        buf.AppendLiteral("1.0");
    }

    buf.AppendLiteral("\r\n");

    mHeaders.Flatten(buf, pruneProxyHeaders);
}

} 
} 
