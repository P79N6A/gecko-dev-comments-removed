








































#include <stdlib.h>
#include "nsHttpResponseHead.h"
#include "nsPrintfCString.h"
#include "prprf.h"
#include "prtime.h"
#include "nsCRT.h"





nsresult
nsHttpResponseHead::SetHeader(nsHttpAtom hdr,
                              const nsACString &val,
                              PRBool merge)
{
    nsresult rv = mHeaders.SetHeader(hdr, val, merge);
    if (NS_FAILED(rv)) return rv;

    
    
    if (hdr == nsHttp::Cache_Control)
        ParseCacheControl(mHeaders.PeekHeader(hdr));
    else if (hdr == nsHttp::Pragma)
        ParsePragma(mHeaders.PeekHeader(hdr));

    return NS_OK;
}

void
nsHttpResponseHead::SetContentLength(PRInt64 len)
{
    mContentLength = len;
    if (!LL_GE_ZERO(len)) 
        mHeaders.ClearHeader(nsHttp::Content_Length);
    else
        mHeaders.SetHeader(nsHttp::Content_Length, nsPrintfCString(20, "%lld", len));
}

void
nsHttpResponseHead::Flatten(nsACString &buf, PRBool pruneTransients)
{
    if (mVersion == NS_HTTP_VERSION_0_9)
        return;

    buf.AppendLiteral("HTTP/");
    if (mVersion == NS_HTTP_VERSION_1_1)
        buf.AppendLiteral("1.1 ");
    else
        buf.AppendLiteral("1.0 ");

    buf.Append(nsPrintfCString("%u", PRUintn(mStatus)) +
               NS_LITERAL_CSTRING(" ") +
               mStatusText +
               NS_LITERAL_CSTRING("\r\n"));

    if (!pruneTransients) {
        mHeaders.Flatten(buf, PR_FALSE);
        return;
    }

    
    
    PRUint32 i, count = mHeaders.Count();
    for (i=0; i<count; ++i) {
        nsHttpAtom header;
        const char *value = mHeaders.PeekHeaderAt(i, header);

        if (!value || header == nsHttp::Connection
                   || header == nsHttp::Proxy_Connection
                   || header == nsHttp::Keep_Alive
                   || header == nsHttp::WWW_Authenticate
                   || header == nsHttp::Proxy_Authenticate
                   || header == nsHttp::Trailer
                   || header == nsHttp::Transfer_Encoding
                   || header == nsHttp::Upgrade
                   
                   
                   || header == nsHttp::Set_Cookie)
            continue;

        
        buf.Append(nsDependentCString(header.get()) +
                   NS_LITERAL_CSTRING(": ") +
                   nsDependentCString(value) +
                   NS_LITERAL_CSTRING("\r\n"));
    }
}

nsresult
nsHttpResponseHead::Parse(char *block)
{

    LOG(("nsHttpResponseHead::Parse [this=%x]\n", this));

    
    

    char *p = PL_strstr(block, "\r\n");
    if (!p)
        return NS_ERROR_UNEXPECTED;

    *p = 0;
    ParseStatusLine(block);

    do {
        block = p + 2;

		if (*block == 0)
			break;

        p = PL_strstr(block, "\r\n");
        if (!p)
            return NS_ERROR_UNEXPECTED;

        *p = 0;
        ParseHeaderLine(block);

    } while (1);

    return NS_OK;
}

void
nsHttpResponseHead::ParseStatusLine(const char *line)
{
    
    
    
 
    
    ParseVersion(line);
    
    if ((mVersion == NS_HTTP_VERSION_0_9) || !(line = PL_strchr(line, ' '))) {
        mStatus = 200;
        mStatusText.AssignLiteral("OK");
    }
    else {
        
        mStatus = (PRUint16) atoi(++line);
        if (mStatus == 0) {
            LOG(("mal-formed response status; assuming status = 200\n"));
            mStatus = 200;
        }

        
        if (!(line = PL_strchr(line, ' '))) {
            LOG(("mal-formed response status line; assuming statusText = 'OK'\n"));
            mStatusText.AssignLiteral("OK");
        }
        else
            mStatusText = ++line;
    }

    LOG(("Have status line [version=%u status=%u statusText=%s]\n",
        PRUintn(mVersion), PRUintn(mStatus), mStatusText.get()));
}

nsresult
nsHttpResponseHead::ParseHeaderLine(const char *line)
{
    nsHttpAtom hdr = {0};
    char *val;
    nsresult rv;
    
    rv = mHeaders.ParseHeaderLine(line, &hdr, &val);
    if (NS_FAILED(rv))
        return rv;
    
    

    
    if (hdr == nsHttp::Content_Length) {
        PRInt64 len;
        
        if (nsHttp::ParseInt64(val, &len)) {
            mContentLength = len;
        }
        else {
            LOG(("invalid content-length!\n"));
            return NS_ERROR_CORRUPTED_CONTENT;
        }
    }
    else if (hdr == nsHttp::Content_Type) {
        LOG(("ParseContentType [type=%s]\n", val));
        PRBool dummy;
        net_ParseContentType(nsDependentCString(val),
                             mContentType, mContentCharset, &dummy);
    }
    else if (hdr == nsHttp::Cache_Control)
        ParseCacheControl(val);
    else if (hdr == nsHttp::Pragma)
        ParsePragma(val);
    return NS_OK;
}











nsresult
nsHttpResponseHead::ComputeCurrentAge(PRUint32 now,
                                      PRUint32 requestTime,
                                      PRUint32 *result)
{
    PRUint32 dateValue;
    PRUint32 ageValue;

    *result = 0;

    if (NS_FAILED(GetDateValue(&dateValue))) {
        LOG(("nsHttpResponseHead::ComputeCurrentAge [this=%x] "
             "Date response header not set!\n", this));
        
        
        dateValue = now;
    }

    
    if (now > dateValue)
        *result = now - dateValue;

    
    if (NS_SUCCEEDED(GetAgeValue(&ageValue)))
        *result = PR_MAX(*result, ageValue);

    NS_ASSERTION(now >= requestTime, "bogus request time");

    
    *result += (now - requestTime);
    return NS_OK;
}












nsresult
nsHttpResponseHead::ComputeFreshnessLifetime(PRUint32 *result)
{
    *result = 0;

    
    if (NS_SUCCEEDED(GetMaxAgeValue(result)))
        return NS_OK;

    *result = 0;

    PRUint32 date = 0, date2 = 0;
    if (NS_FAILED(GetDateValue(&date)))
        date = NowInSeconds(); 

    
    if (NS_SUCCEEDED(GetExpiresValue(&date2))) {
        if (date2 > date)
            *result = date2 - date;
        
        return NS_OK;
    }
    
    
    if (NS_SUCCEEDED(GetLastModifiedValue(&date2))) {
        LOG(("using last-modified to determine freshness-lifetime\n"));
        LOG(("last-modified = %u, date = %u\n", date2, date));
        if (date2 <= date) {
            
            *result = (date - date2) / 10;
            return NS_OK;
        }
    }

    
    if ((mStatus == 300) || (mStatus == 301)) {
        *result = PRUint32(-1);
        return NS_OK;
    }

    LOG(("nsHttpResponseHead::ComputeFreshnessLifetime [this = %x] "
         "Insufficient information to compute a non-zero freshness "
         "lifetime!\n", this));

    return NS_OK;
}

PRBool
nsHttpResponseHead::MustValidate()
{
    LOG(("nsHttpResponseHead::MustValidate ??\n"));

    
    
    switch (mStatus) {
        
    case 200:
    case 203:
    case 206:
        
    case 300:
    case 301:
    case 302:
    case 304:
    case 307:
        break;
        
    case 303:
    case 305:
        
    case 401:
    case 407:
    case 412:
    case 416:
    default:  
        LOG(("Must validate since response is an uncacheable error page\n"));
        return PR_TRUE;
    }
    
    
    
    if (NoCache()) {
        LOG(("Must validate since response contains 'no-cache' header\n"));
        return PR_TRUE;
    }

    
    
    
    
    if (NoStore()) {
        LOG(("Must validate since response contains 'no-store' header\n"));
        return PR_TRUE;
    }

    
    
    
    if (ExpiresInPast()) {
        LOG(("Must validate since Expires < Date\n"));
        return PR_TRUE;
    }

    LOG(("no mandatory validation requirement\n"));
    return PR_FALSE;
}

PRBool
nsHttpResponseHead::MustValidateIfExpired()
{
    
    
    
    
    
    
    return HasHeaderValue(nsHttp::Cache_Control, "must-revalidate");
}

PRBool
nsHttpResponseHead::IsResumable()
{
    
    
    return mVersion >= NS_HTTP_VERSION_1_1 &&
           PeekHeader(nsHttp::Content_Length) && 
          (PeekHeader(nsHttp::ETag) || PeekHeader(nsHttp::Last_Modified)) &&
           HasHeaderValue(nsHttp::Accept_Ranges, "bytes");
}

PRBool
nsHttpResponseHead::ExpiresInPast()
{
    PRUint32 maxAgeVal, expiresVal, dateVal;
    
    
    if (NS_SUCCEEDED(GetMaxAgeValue(&maxAgeVal))) {
        return PR_FALSE;
    }
    
    return NS_SUCCEEDED(GetExpiresValue(&expiresVal)) &&
           NS_SUCCEEDED(GetDateValue(&dateVal)) &&
           expiresVal < dateVal;
}

nsresult
nsHttpResponseHead::UpdateHeaders(nsHttpHeaderArray &headers)
{
    LOG(("nsHttpResponseHead::UpdateHeaders [this=%x]\n", this));

    PRUint32 i, count = headers.Count();
    for (i=0; i<count; ++i) {
        nsHttpAtom header;
        const char *val = headers.PeekHeaderAt(i, header);

        if (!val) {
            NS_NOTREACHED("null header value");
            continue;
        }

        
        if (header == nsHttp::Connection          ||
            header == nsHttp::Proxy_Connection    ||
            header == nsHttp::Keep_Alive          ||
            header == nsHttp::Proxy_Authenticate  ||
            header == nsHttp::Proxy_Authorization || 
            header == nsHttp::TE                  ||
            header == nsHttp::Trailer             ||
            header == nsHttp::Transfer_Encoding   ||
            header == nsHttp::Upgrade             ||
        
            header == nsHttp::Content_Location    ||
            header == nsHttp::Content_MD5         ||
            header == nsHttp::ETag                ||
        
            header == nsHttp::Content_Encoding    ||
            header == nsHttp::Content_Range       ||
            header == nsHttp::Content_Type        ||
        
            
            
            header == nsHttp::Content_Length) {
            LOG(("ignoring response header [%s: %s]\n", header.get(), val));
        }
        else {
            LOG(("new response header [%s: %s]\n", header.get(), val));

            
            SetHeader(header, nsDependentCString(val));
        }
    }

    return NS_OK;
}

void
nsHttpResponseHead::Reset()
{
    LOG(("nsHttpResponseHead::Reset\n"));

    ClearHeaders();

    mVersion = NS_HTTP_VERSION_1_1;
    mStatus = 200;
    mContentLength = LL_MAXUINT;
    mCacheControlNoStore = PR_FALSE;
    mCacheControlNoCache = PR_FALSE;
    mPragmaNoCache = PR_FALSE;
    mStatusText.Truncate();
    mContentType.Truncate();
    mContentCharset.Truncate();
}

nsresult
nsHttpResponseHead::ParseDateHeader(nsHttpAtom header, PRUint32 *result)
{
    const char *val = PeekHeader(header);
    if (!val)
        return NS_ERROR_NOT_AVAILABLE;

    PRTime time;
    PRStatus st = PR_ParseTimeString(val, PR_TRUE, &time);
    if (st != PR_SUCCESS)
        return NS_ERROR_NOT_AVAILABLE;

    *result = PRTimeToSeconds(time); 
    return NS_OK;
}

nsresult
nsHttpResponseHead::GetAgeValue(PRUint32 *result)
{
    const char *val = PeekHeader(nsHttp::Age);
    if (!val)
        return NS_ERROR_NOT_AVAILABLE;

    *result = (PRUint32) atoi(val);
    return NS_OK;
}



nsresult
nsHttpResponseHead::GetMaxAgeValue(PRUint32 *result)
{
    const char *val = PeekHeader(nsHttp::Cache_Control);
    if (!val)
        return NS_ERROR_NOT_AVAILABLE;

    const char *p = PL_strcasestr(val, "max-age=");
    if (!p)
        return NS_ERROR_NOT_AVAILABLE;

    int maxAgeValue = atoi(p + 8);
    if (maxAgeValue < 0)
        maxAgeValue = 0;
    *result = PRUint32(maxAgeValue);
    return NS_OK;
}

nsresult
nsHttpResponseHead::GetExpiresValue(PRUint32 *result)
{
    const char *val = PeekHeader(nsHttp::Expires);
    if (!val)
        return NS_ERROR_NOT_AVAILABLE;

    PRTime time;
    PRStatus st = PR_ParseTimeString(val, PR_TRUE, &time);
    if (st != PR_SUCCESS) {
        
        
        *result = 0;
        return NS_OK;
    }

    if (LL_CMP(time, <, LL_Zero()))
        *result = 0;
    else
        *result = PRTimeToSeconds(time); 
    return NS_OK;
}

PRInt64
nsHttpResponseHead::TotalEntitySize()
{
    const char* contentRange = PeekHeader(nsHttp::Content_Range);
    if (!contentRange)
        return ContentLength();

    
    const char* slash = strrchr(contentRange, '/');
    if (!slash)
        return -1; 

    slash++;
    if (*slash == '*') 
        return -1;

    PRInt64 size;
    if (!nsHttp::ParseInt64(slash, &size))
        size = LL_MAXUINT;
    return size;
}





void
nsHttpResponseHead::ParseVersion(const char *str)
{
    

    LOG(("nsHttpResponseHead::ParseVersion [version=%s]\n", str));

    
    if (PL_strncasecmp(str, "HTTP", 4) != 0) {
        LOG(("looks like a HTTP/0.9 response\n"));
        mVersion = NS_HTTP_VERSION_0_9;
        return;
    }
    str += 4;

    if (*str != '/') {
        LOG(("server did not send a version number; assuming HTTP/1.0\n"));
        
        
        mVersion = NS_HTTP_VERSION_1_0;
        return;
    }

    char *p = PL_strchr(str, '.');
    if (p == nsnull) {
        LOG(("mal-formed server version; assuming HTTP/1.0\n"));
        mVersion = NS_HTTP_VERSION_1_0;
        return;
    }

    ++p; 

    int major = atoi(str + 1);
    int minor = atoi(p);

    if ((major > 1) || ((major == 1) && (minor >= 1)))
        
        mVersion = NS_HTTP_VERSION_1_1;
    else
        
        mVersion = NS_HTTP_VERSION_1_0;
}

void
nsHttpResponseHead::ParseCacheControl(const char *val)
{
    if (!(val && *val)) {
        
        mCacheControlNoCache = PR_FALSE;
        mCacheControlNoStore = PR_FALSE;
        return;
    }

    
    
    if (nsHttp::FindToken(val, "no-cache", HTTP_HEADER_VALUE_SEPS))
        mCacheControlNoCache = PR_TRUE;

    
    if (nsHttp::FindToken(val, "no-store", HTTP_HEADER_VALUE_SEPS))
        mCacheControlNoStore = PR_TRUE;
}

void
nsHttpResponseHead::ParsePragma(const char *val)
{
    LOG(("nsHttpResponseHead::ParsePragma [val=%s]\n", val));

    if (!(val && *val)) {
        
        mPragmaNoCache = PR_FALSE;
        return;
    }

    
    
    
    if (nsHttp::FindToken(val, "no-cache", HTTP_HEADER_VALUE_SEPS))
        mPragmaNoCache = PR_TRUE;
}
