





































#ifndef nsHttpResponseHead_h__
#define nsHttpResponseHead_h__

#include "nsHttpHeaderArray.h"
#include "nsHttp.h"
#include "nsString.h"






class nsHttpResponseHead
{
public:
    nsHttpResponseHead() : mVersion(NS_HTTP_VERSION_1_1)
                         , mStatus(200)
                         , mContentLength(LL_MAXUINT)
                         , mCacheControlNoStore(PR_FALSE)
                         , mCacheControlNoCache(PR_FALSE)
                         , mPragmaNoCache(PR_FALSE) {}
    ~nsHttpResponseHead() 
    {
        Reset();
    }
    
    nsHttpHeaderArray    &Headers()        { return mHeaders; }
    nsHttpVersion         Version()        { return mVersion; }
    PRUint16              Status()         { return mStatus; }
    const nsAFlatCString &StatusText()     { return mStatusText; }
    PRInt64               ContentLength()  { return mContentLength; }
    const nsAFlatCString &ContentType()    { return mContentType; }
    const nsAFlatCString &ContentCharset() { return mContentCharset; }
    bool                  NoStore()        { return mCacheControlNoStore; }
    bool                  NoCache()        { return (mCacheControlNoCache || mPragmaNoCache); }
    




    PRInt64               TotalEntitySize();

    const char *PeekHeader(nsHttpAtom h)            { return mHeaders.PeekHeader(h); }
    nsresult SetHeader(nsHttpAtom h, const nsACString &v, bool m=false);
    nsresult GetHeader(nsHttpAtom h, nsACString &v) { return mHeaders.GetHeader(h, v); }
    void     ClearHeader(nsHttpAtom h)              { mHeaders.ClearHeader(h); }
    void     ClearHeaders()                         { mHeaders.Clear(); }

    const char *FindHeaderValue(nsHttpAtom h, const char *v) { return mHeaders.FindHeaderValue(h, v); }
    bool        HasHeaderValue(nsHttpAtom h, const char *v) { return mHeaders.HasHeaderValue(h, v); }

    void     SetContentType(const nsACString &s)    { mContentType = s; }
    void     SetContentCharset(const nsACString &s) { mContentCharset = s; }
    void     SetContentLength(PRInt64);

    
    
    
    void     Flatten(nsACString &, bool pruneTransients);

    
    
    nsresult Parse(char *block);

    
    void     ParseStatusLine(const char *line);

    
    nsresult ParseHeaderLine(const char *line);

    
    nsresult ComputeFreshnessLifetime(PRUint32 *);
    nsresult ComputeCurrentAge(PRUint32 now, PRUint32 requestTime, PRUint32 *result);
    bool     MustValidate();
    bool     MustValidateIfExpired();

    
    bool     IsResumable();

    
    
    bool     ExpiresInPast();

    
    nsresult UpdateHeaders(nsHttpHeaderArray &headers); 

    
    void     Reset();

    
    nsresult ParseDateHeader(nsHttpAtom header, PRUint32 *result);
    nsresult GetAgeValue(PRUint32 *result);
    nsresult GetMaxAgeValue(PRUint32 *result);
    nsresult GetDateValue(PRUint32 *result)         { return ParseDateHeader(nsHttp::Date, result); }
    nsresult GetExpiresValue(PRUint32 *result);
    nsresult GetLastModifiedValue(PRUint32 *result) { return ParseDateHeader(nsHttp::Last_Modified, result); }

private:
    void     ParseVersion(const char *);
    void     ParseCacheControl(const char *);
    void     ParsePragma(const char *);

private:
    nsHttpHeaderArray mHeaders;
    nsHttpVersion     mVersion;
    PRUint16          mStatus;
    nsCString         mStatusText;
    PRInt64           mContentLength;
    nsCString         mContentType;
    nsCString         mContentCharset;
    bool              mCacheControlNoStore;
    bool              mCacheControlNoCache;
    bool              mPragmaNoCache;

    friend struct IPC::ParamTraits<nsHttpResponseHead>;
};

#endif 
