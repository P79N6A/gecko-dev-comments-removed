




#ifndef nsHttpResponseHead_h__
#define nsHttpResponseHead_h__

#include "nsHttpHeaderArray.h"
#include "nsHttp.h"
#include "nsString.h"



namespace IPC {
    template <typename> struct ParamTraits;
}

namespace mozilla { namespace net {






class nsHttpResponseHead
{
public:
    nsHttpResponseHead() : mVersion(NS_HTTP_VERSION_1_1)
                         , mStatus(200)
                         , mContentLength(-1)
                         , mCacheControlPrivate(false)
                         , mCacheControlNoStore(false)
                         , mCacheControlNoCache(false)
                         , mPragmaNoCache(false) {}

    const nsHttpHeaderArray & Headers()   const { return mHeaders; }
    nsHttpHeaderArray    &Headers()             { return mHeaders; }
    nsHttpVersion         Version()       const { return mVersion; }

#undef Status
    uint16_t              Status()        const { return mStatus; }
    const nsAFlatCString &StatusText()    const { return mStatusText; }
    int64_t               ContentLength() const { return mContentLength; }
    const nsAFlatCString &ContentType()   const { return mContentType; }
    const nsAFlatCString &ContentCharset() const { return mContentCharset; }
    bool                  Private() const { return mCacheControlPrivate; }
    bool                  NoStore() const { return mCacheControlNoStore; }
    bool                  NoCache() const { return (mCacheControlNoCache || mPragmaNoCache); }
    




    int64_t               TotalEntitySize() const;

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
    void     SetContentLength(int64_t);

    
    
    
    void     Flatten(nsACString &, bool pruneTransients);

    
    
    nsresult Parse(char *block);

    
    void     ParseStatusLine(const char *line);

    
    nsresult ParseHeaderLine(const char *line);

    
    nsresult ComputeFreshnessLifetime(uint32_t *) const;
    nsresult ComputeCurrentAge(uint32_t now, uint32_t requestTime, uint32_t *result) const;
    bool     MustValidate() const;
    bool     MustValidateIfExpired() const;

    
    bool     IsResumable() const;

    
    
    bool     ExpiresInPast() const;

    
    nsresult UpdateHeaders(const nsHttpHeaderArray &headers);

    
    void     Reset();

    
    nsresult ParseDateHeader(nsHttpAtom header, uint32_t *result) const;
    nsresult GetAgeValue(uint32_t *result) const;
    nsresult GetMaxAgeValue(uint32_t *result) const;
    nsresult GetDateValue(uint32_t *result) const
    {
        return ParseDateHeader(nsHttp::Date, result);
    }
    nsresult GetExpiresValue(uint32_t *result) const ;
    nsresult GetLastModifiedValue(uint32_t *result) const
    {
        return ParseDateHeader(nsHttp::Last_Modified, result);
    }

    bool operator==(const nsHttpResponseHead& aOther) const
    {
        return mHeaders == aOther.mHeaders &&
               mVersion == aOther.mVersion &&
               mStatus == aOther.mStatus &&
               mStatusText == aOther.mStatusText &&
               mContentLength == aOther.mContentLength &&
               mContentType == aOther.mContentType &&
               mContentCharset == aOther.mContentCharset &&
               mCacheControlPrivate == aOther.mCacheControlPrivate &&
               mCacheControlNoCache == aOther.mCacheControlNoCache &&
               mCacheControlNoStore == aOther.mCacheControlNoStore &&
               mPragmaNoCache == aOther.mPragmaNoCache;
    }

private:
    void     AssignDefaultStatusText();
    void     ParseVersion(const char *);
    void     ParseCacheControl(const char *);
    void     ParsePragma(const char *);

private:
    
    nsHttpHeaderArray mHeaders;
    nsHttpVersion     mVersion;
    uint16_t          mStatus;
    nsCString         mStatusText;
    int64_t           mContentLength;
    nsCString         mContentType;
    nsCString         mContentCharset;
    bool              mCacheControlPrivate;
    bool              mCacheControlNoStore;
    bool              mCacheControlNoCache;
    bool              mPragmaNoCache;

    friend struct IPC::ParamTraits<nsHttpResponseHead>;
};
}} 

#endif 
