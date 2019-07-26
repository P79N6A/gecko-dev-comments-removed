




#ifndef nsHttpRequestHead_h__
#define nsHttpRequestHead_h__

#include "nsHttp.h"
#include "nsHttpHeaderArray.h"
#include "nsString.h"






class nsHttpRequestHead
{
public:
    nsHttpRequestHead() : mMethod(nsHttp::Get), mVersion(NS_HTTP_VERSION_1_1) {}

    void SetMethod(nsHttpAtom method) { mMethod = method; }
    void SetVersion(nsHttpVersion version) { mVersion = version; }
    void SetRequestURI(const nsCSubstring &s) { mRequestURI = s; }

    const nsHttpHeaderArray &Headers() const { return mHeaders; }
    nsHttpHeaderArray & Headers()          { return mHeaders; }
    nsHttpAtom          Method()     const { return mMethod; }
    nsHttpVersion       Version()    const { return mVersion; }
    const nsCSubstring &RequestURI() const { return mRequestURI; }

    const char *PeekHeader(nsHttpAtom h) const
    {
        return mHeaders.PeekHeader(h);
    }
    nsresult SetHeader(nsHttpAtom h, const nsACString &v, bool m=false) { return mHeaders.SetHeader(h, v, m); }
    nsresult GetHeader(nsHttpAtom h, nsACString &v) const
    {
        return mHeaders.GetHeader(h, v);
    }
    void ClearHeader(nsHttpAtom h)                                           { mHeaders.ClearHeader(h); }
    void ClearHeaders()                                                      { mHeaders.Clear(); }

    const char *FindHeaderValue(nsHttpAtom h, const char *v) const
    {
        return mHeaders.FindHeaderValue(h, v);
    }
    bool HasHeaderValue(nsHttpAtom h, const char *v) const
    {
      return mHeaders.HasHeaderValue(h, v);
    }

    void Flatten(nsACString &, bool pruneProxyHeaders = false);

    
    nsresult SetHeaderOnce(nsHttpAtom h, const char *v, bool merge = false)
    {
        if (!merge || !HasHeaderValue(h, v))
            return mHeaders.SetHeader(h, nsDependentCString(v), merge);
        return NS_OK;
    }

private:
    
    nsHttpHeaderArray mHeaders;
    nsHttpAtom        mMethod;
    nsHttpVersion     mVersion;
    nsCString         mRequestURI;
};

#endif 
