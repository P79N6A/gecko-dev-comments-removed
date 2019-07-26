




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
                         , mCacheControlNoStore(false)
                         , mCacheControlNoCache(false)
                         , mPragmaNoCache(false) {}
    
    const nsHttpHeaderArray & Headers()   const { return mHeaders; }
    nsHttpHeaderArray    &Headers()             { return mHeaders; }
    nsHttpVersion         Version()       const { return mVersion; }
    PRUint16              Status()        const { return mStatus; }
    const nsAFlatCString &StatusText()    const { return mStatusText; }
    PRInt64               ContentLength() const { return mContentLength; }
    const nsAFlatCString &ContentType()   const { return mContentType; }
    const nsAFlatCString &ContentCharset() const { return mContentCharset; }
    bool                  NoStore() const { return mCacheControlNoStore; }
    bool                  NoCache() const { return (mCacheControlNoCache || mPragmaNoCache); }
    




    PRInt64               TotalEntitySize() const;

    const char *PeekHeader(nsHttpAtom h) const      { return mHeaders.PeekHeader(h); }
    nsresult SetHeader(nsHttpAtom h, const nsACString &v, bool m=false);
    nsresult GetHeader(nsHttpAtom h, nsACString &v) const { return mHeaders.GetHeader(h, v); }
    void     ClearHeader(nsHttpAtom h)              { mHeaders.ClearHeader(h); }
    void     ClearHeaders()                         { mHeaders.Clear(); }

    const char *FindHeaderValue(nsHttpAtom h, const char *v) const
    {
      return mHeaders.FindHeaderValue(h, v);
    }
    bool        HasHeaderValue(nsHttpAtom h, const char *v) const
    {
      return mHeaders.HasHeaderValue(h, v);
    }

    void     SetContentType(const nsACString &s)    { mContentType = s; }
    void     SetContentCharset(const nsACString &s) { mContentCharset = s; }
    void     SetContentLength(PRInt64);

    
    
    
    void     Flatten(nsACString &, bool pruneTransients);

    
    
    nsresult Parse(char *block);

    
    void     ParseStatusLine(const char *line);

    
    nsresult ParseHeaderLine(const char *line);

    
    nsresult ComputeFreshnessLifetime(PRUint32 *) const;
    nsresult ComputeCurrentAge(PRUint32 now, PRUint32 requestTime, PRUint32 *result) const;
    bool     MustValidate() const;
    bool     MustValidateIfExpired() const;

    
    bool     IsResumable() const;

    
    
    bool     ExpiresInPast() const;

    
    nsresult UpdateHeaders(const nsHttpHeaderArray &headers); 

    
    void     Reset();

    
    nsresult ParseDateHeader(nsHttpAtom header, PRUint32 *result) const;
    nsresult GetAgeValue(PRUint32 *result) const;
    nsresult GetMaxAgeValue(PRUint32 *result) const;
    nsresult GetDateValue(PRUint32 *result) const
    {
        return ParseDateHeader(nsHttp::Date, result);
    }
    nsresult GetExpiresValue(PRUint32 *result) const ;
    nsresult GetLastModifiedValue(PRUint32 *result) const
    {
        return ParseDateHeader(nsHttp::Last_Modified, result);
    }

private:
    void     ParseVersion(const char *);
    void     ParseCacheControl(const char *);
    void     ParsePragma(const char *);

private:
    
    nsHttpHeaderArray mHeaders;
    nsHttpVersion     mVersion;
    PRUint16          mStatus;
    mozilla::net::InfallableCopyCString mStatusText;
    PRInt64           mContentLength;
    mozilla::net::InfallableCopyCString mContentType;
    mozilla::net::InfallableCopyCString mContentCharset;
    bool              mCacheControlNoStore;
    bool              mCacheControlNoCache;
    bool              mPragmaNoCache;

    friend struct IPC::ParamTraits<nsHttpResponseHead>;
};

#endif 
