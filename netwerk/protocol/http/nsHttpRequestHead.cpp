





































#include "nsHttpRequestHead.h"





void
nsHttpRequestHead::Flatten(nsACString &buf, PRBool pruneProxyHeaders)
{
    
 
    buf.Append(mMethod.get());
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
