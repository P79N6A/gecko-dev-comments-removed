






#include "HttpLog.h"

#include "nsHttpResponseHead.h"
#include "nsPrintfCString.h"
#include "prtime.h"
#include "nsURLHelper.h"
#include <algorithm>

namespace mozilla {
namespace net {





nsresult
nsHttpResponseHead::SetHeader(nsHttpAtom hdr,
                              const nsACString &val,
                              bool merge)
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
nsHttpResponseHead::SetContentLength(int64_t len)
{
    mContentLength = len;
    if (len < 0)
        mHeaders.ClearHeader(nsHttp::Content_Length);
    else
        mHeaders.SetHeader(nsHttp::Content_Length, nsPrintfCString("%lld", len));
}

void
nsHttpResponseHead::Flatten(nsACString &buf, bool pruneTransients)
{
    if (mVersion == NS_HTTP_VERSION_0_9)
        return;

    buf.AppendLiteral("HTTP/");
    if (mVersion == NS_HTTP_VERSION_2_0)
        buf.AppendLiteral("2.0 ");
    else if (mVersion == NS_HTTP_VERSION_1_1)
        buf.AppendLiteral("1.1 ");
    else
        buf.AppendLiteral("1.0 ");

    buf.Append(nsPrintfCString("%u", unsigned(mStatus)) +
               NS_LITERAL_CSTRING(" ") +
               mStatusText +
               NS_LITERAL_CSTRING("\r\n"));

    if (!pruneTransients) {
        mHeaders.Flatten(buf, false);
        return;
    }

    
    
    uint32_t i, count = mHeaders.Count();
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

    LOG(("nsHttpResponseHead::Parse [this=%p]\n", this));

    
    

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
nsHttpResponseHead::AssignDefaultStatusText()
{
    LOG(("response status line needs default reason phrase\n"));

    
    
    
    
    
    

    switch (mStatus) {
        
    case 200:
        mStatusText.AssignLiteral("OK");
        break;
    case 404:
        mStatusText.AssignLiteral("Not Found");
        break;
    case 301:
        mStatusText.AssignLiteral("Moved Permanently");
        break;
    case 304:
        mStatusText.AssignLiteral("Not Modified");
        break;
    case 307:
        mStatusText.AssignLiteral("Temporary Redirect");
        break;
    case 500:
        mStatusText.AssignLiteral("Internal Server Error");
        break;

        
    case 100:
        mStatusText.AssignLiteral("Continue");
        break;
    case 101:
        mStatusText.AssignLiteral("Switching Protocols");
        break;
    case 201:
        mStatusText.AssignLiteral("Created");
        break;
    case 202:
        mStatusText.AssignLiteral("Accepted");
        break;
    case 203:
        mStatusText.AssignLiteral("Non Authoritative");
        break;
    case 204:
        mStatusText.AssignLiteral("No Content");
        break;
    case 205:
        mStatusText.AssignLiteral("Reset Content");
        break;
    case 206:
        mStatusText.AssignLiteral("Partial Content");
        break;
    case 207:
        mStatusText.AssignLiteral("Multi-Status");
        break;
    case 208:
        mStatusText.AssignLiteral("Already Reported");
        break;
    case 300:
        mStatusText.AssignLiteral("Multiple Choices");
        break;
    case 302:
        mStatusText.AssignLiteral("Found");
        break;
    case 303:
        mStatusText.AssignLiteral("See Other");
        break;
    case 305:
        mStatusText.AssignLiteral("Use Proxy");
        break;
    case 308:
        mStatusText.AssignLiteral("Permanent Redirect");
        break;
    case 400:
        mStatusText.AssignLiteral("Bad Request");
        break;
    case 401:
        mStatusText.AssignLiteral("Unauthorized");
        break;
    case 402:
        mStatusText.AssignLiteral("Payment Required");
        break;
    case 403:
        mStatusText.AssignLiteral("Forbidden");
        break;
    case 405:
        mStatusText.AssignLiteral("Method Not Allowed");
        break;
    case 406:
        mStatusText.AssignLiteral("Not Acceptable");
        break;
    case 407:
        mStatusText.AssignLiteral("Proxy Authentication Required");
        break;
    case 408:
        mStatusText.AssignLiteral("Request Timeout");
        break;
    case 409:
        mStatusText.AssignLiteral("Conflict");
        break;
    case 410:
        mStatusText.AssignLiteral("Gone");
        break;
    case 411:
        mStatusText.AssignLiteral("Length Required");
        break;
    case 412:
        mStatusText.AssignLiteral("Precondition Failed");
        break;
    case 413:
        mStatusText.AssignLiteral("Request Entity Too Large");
        break;
    case 414:
        mStatusText.AssignLiteral("Request URI Too Long");
        break;
    case 415:
        mStatusText.AssignLiteral("Unsupported Media Type");
        break;
    case 416:
        mStatusText.AssignLiteral("Requested Range Not Satisfiable");
        break;
    case 417:
        mStatusText.AssignLiteral("Expectation Failed");
        break;
    case 421:
        mStatusText.AssignLiteral("Misdirected Request");
        break;
    case 501:
        mStatusText.AssignLiteral("Not Implemented");
        break;
    case 502:
        mStatusText.AssignLiteral("Bad Gateway");
        break;
    case 503:
        mStatusText.AssignLiteral("Service Unavailable");
        break;
    case 504:
        mStatusText.AssignLiteral("Gateway Timeout");
        break;
    case 505:
        mStatusText.AssignLiteral("HTTP Version Unsupported");
        break;
    default:
        mStatusText.AssignLiteral("No Reason Phrase");
        break;
    }
}

void
nsHttpResponseHead::ParseStatusLine(const char *line)
{
    
    
    

    
    ParseVersion(line);

    if ((mVersion == NS_HTTP_VERSION_0_9) || !(line = PL_strchr(line, ' '))) {
        mStatus = 200;
        AssignDefaultStatusText();
    }
    else {
        
        mStatus = (uint16_t) atoi(++line);
        if (mStatus == 0) {
            LOG(("mal-formed response status; assuming status = 200\n"));
            mStatus = 200;
        }

        
        if (!(line = PL_strchr(line, ' '))) {
            AssignDefaultStatusText();
        }
        else
            mStatusText = nsDependentCString(++line);
    }

    LOG(("Have status line [version=%u status=%u statusText=%s]\n",
        unsigned(mVersion), unsigned(mStatus), mStatusText.get()));
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
        int64_t len;
        const char *ignored;
        
        if (nsHttp::ParseInt64(val, &ignored, &len)) {
            mContentLength = len;
        }
        else {
            
            LOG(("invalid content-length! %s\n", val));
        }
    }
    else if (hdr == nsHttp::Content_Type) {
        LOG(("ParseContentType [type=%s]\n", val));
        bool dummy;
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
nsHttpResponseHead::ComputeCurrentAge(uint32_t now,
                                      uint32_t requestTime,
                                      uint32_t *result) const
{
    uint32_t dateValue;
    uint32_t ageValue;

    *result = 0;

    if (requestTime > now) {
        
        requestTime = now;
    }

    if (NS_FAILED(GetDateValue(&dateValue))) {
        LOG(("nsHttpResponseHead::ComputeCurrentAge [this=%p] "
             "Date response header not set!\n", this));
        
        
        dateValue = now;
    }

    
    if (now > dateValue)
        *result = now - dateValue;

    
    if (NS_SUCCEEDED(GetAgeValue(&ageValue)))
        *result = std::max(*result, ageValue);

    
    *result += (now - requestTime);
    return NS_OK;
}












nsresult
nsHttpResponseHead::ComputeFreshnessLifetime(uint32_t *result) const
{
    *result = 0;

    
    if (NS_SUCCEEDED(GetMaxAgeValue(result)))
        return NS_OK;

    *result = 0;

    uint32_t date = 0, date2 = 0;
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

    
    if ((mStatus == 300) || nsHttp::IsPermanentRedirect(mStatus)) {
        *result = uint32_t(-1);
        return NS_OK;
    }

    LOG(("nsHttpResponseHead::ComputeFreshnessLifetime [this = %x] "
         "Insufficient information to compute a non-zero freshness "
         "lifetime!\n", this));

    return NS_OK;
}

bool
nsHttpResponseHead::MustValidate() const
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
    case 308:
        break;
        
    case 303:
    case 305:
        
    case 401:
    case 407:
    case 412:
    case 416:
    default:  
        LOG(("Must validate since response is an uncacheable error page\n"));
        return true;
    }

    
    
    if (NoCache()) {
        LOG(("Must validate since response contains 'no-cache' header\n"));
        return true;
    }

    
    
    
    
    if (NoStore()) {
        LOG(("Must validate since response contains 'no-store' header\n"));
        return true;
    }

    
    
    
    if (ExpiresInPast()) {
        LOG(("Must validate since Expires < Date\n"));
        return true;
    }

    LOG(("no mandatory validation requirement\n"));
    return false;
}

bool
nsHttpResponseHead::MustValidateIfExpired() const
{
    
    
    
    
    
    
    return HasHeaderValue(nsHttp::Cache_Control, "must-revalidate");
}

bool
nsHttpResponseHead::IsResumable() const
{
    
    
    
    
    
    return mStatus == 200 &&
           mVersion >= NS_HTTP_VERSION_1_1 &&
           PeekHeader(nsHttp::Content_Length) &&
          (PeekHeader(nsHttp::ETag) || PeekHeader(nsHttp::Last_Modified)) &&
           HasHeaderValue(nsHttp::Accept_Ranges, "bytes");
}

bool
nsHttpResponseHead::ExpiresInPast() const
{
    uint32_t maxAgeVal, expiresVal, dateVal;

    
    if (NS_SUCCEEDED(GetMaxAgeValue(&maxAgeVal))) {
        return false;
    }

    return NS_SUCCEEDED(GetExpiresValue(&expiresVal)) &&
           NS_SUCCEEDED(GetDateValue(&dateVal)) &&
           expiresVal < dateVal;
}

nsresult
nsHttpResponseHead::UpdateHeaders(const nsHttpHeaderArray &headers)
{
    LOG(("nsHttpResponseHead::UpdateHeaders [this=%p]\n", this));

    uint32_t i, count = headers.Count();
    for (i=0; i<count; ++i) {
        nsHttpAtom header;
        const char *val = headers.PeekHeaderAt(i, header);

        if (!val) {
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
    mContentLength = -1;
    mCacheControlPrivate = false;
    mCacheControlNoStore = false;
    mCacheControlNoCache = false;
    mPragmaNoCache = false;
    mStatusText.Truncate();
    mContentType.Truncate();
    mContentCharset.Truncate();
}

nsresult
nsHttpResponseHead::ParseDateHeader(nsHttpAtom header, uint32_t *result) const
{
    const char *val = PeekHeader(header);
    if (!val)
        return NS_ERROR_NOT_AVAILABLE;

    PRTime time;
    PRStatus st = PR_ParseTimeString(val, true, &time);
    if (st != PR_SUCCESS)
        return NS_ERROR_NOT_AVAILABLE;

    *result = PRTimeToSeconds(time);
    return NS_OK;
}

nsresult
nsHttpResponseHead::GetAgeValue(uint32_t *result) const
{
    const char *val = PeekHeader(nsHttp::Age);
    if (!val)
        return NS_ERROR_NOT_AVAILABLE;

    *result = (uint32_t) atoi(val);
    return NS_OK;
}



nsresult
nsHttpResponseHead::GetMaxAgeValue(uint32_t *result) const
{
    const char *val = PeekHeader(nsHttp::Cache_Control);
    if (!val)
        return NS_ERROR_NOT_AVAILABLE;

    const char *p = nsHttp::FindToken(val, "max-age", HTTP_HEADER_VALUE_SEPS "=");
    if (!p)
        return NS_ERROR_NOT_AVAILABLE;
    p += 7;
    while (*p == ' ' || *p == '\t')
        ++p;
    if (*p != '=')
        return NS_ERROR_NOT_AVAILABLE;
    ++p;
    while (*p == ' ' || *p == '\t')
        ++p;

    int maxAgeValue = atoi(p);
    if (maxAgeValue < 0)
        maxAgeValue = 0;
    *result = static_cast<uint32_t>(maxAgeValue);
    return NS_OK;
}

nsresult
nsHttpResponseHead::GetExpiresValue(uint32_t *result) const
{
    const char *val = PeekHeader(nsHttp::Expires);
    if (!val)
        return NS_ERROR_NOT_AVAILABLE;

    PRTime time;
    PRStatus st = PR_ParseTimeString(val, true, &time);
    if (st != PR_SUCCESS) {
        
        
        *result = 0;
        return NS_OK;
    }

    if (time < 0)
        *result = 0;
    else
        *result = PRTimeToSeconds(time);
    return NS_OK;
}

int64_t
nsHttpResponseHead::TotalEntitySize() const
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

    int64_t size;
    if (!nsHttp::ParseInt64(slash, &size))
        size = UINT64_MAX;
    return size;
}





void
nsHttpResponseHead::ParseVersion(const char *str)
{
    

    LOG(("nsHttpResponseHead::ParseVersion [version=%s]\n", str));

    
    if (PL_strncasecmp(str, "HTTP", 4) != 0) {
        if (PL_strncasecmp(str, "ICY ", 4) == 0) {
            
            LOG(("Treating ICY as HTTP 1.0\n"));
            mVersion = NS_HTTP_VERSION_1_0;
            return;
        }
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
    if (p == nullptr) {
        LOG(("mal-formed server version; assuming HTTP/1.0\n"));
        mVersion = NS_HTTP_VERSION_1_0;
        return;
    }

    ++p; 

    int major = atoi(str + 1);
    int minor = atoi(p);

    if ((major > 2) || ((major == 2) && (minor >= 0)))
        mVersion = NS_HTTP_VERSION_2_0;
    else if ((major == 1) && (minor >= 1))
        
        mVersion = NS_HTTP_VERSION_1_1;
    else
        
        mVersion = NS_HTTP_VERSION_1_0;
}

void
nsHttpResponseHead::ParseCacheControl(const char *val)
{
    if (!(val && *val)) {
        
        mCacheControlPrivate = false;
        mCacheControlNoCache = false;
        mCacheControlNoStore = false;
        return;
    }

    
    if (nsHttp::FindToken(val, "private", HTTP_HEADER_VALUE_SEPS))
        mCacheControlPrivate = true;

    
    
    if (nsHttp::FindToken(val, "no-cache", HTTP_HEADER_VALUE_SEPS))
        mCacheControlNoCache = true;

    
    if (nsHttp::FindToken(val, "no-store", HTTP_HEADER_VALUE_SEPS))
        mCacheControlNoStore = true;
}

void
nsHttpResponseHead::ParsePragma(const char *val)
{
    LOG(("nsHttpResponseHead::ParsePragma [val=%s]\n", val));

    if (!(val && *val)) {
        
        mPragmaNoCache = false;
        return;
    }

    
    
    
    if (nsHttp::FindToken(val, "no-cache", HTTP_HEADER_VALUE_SEPS))
        mPragmaNoCache = true;
}

} 
} 
