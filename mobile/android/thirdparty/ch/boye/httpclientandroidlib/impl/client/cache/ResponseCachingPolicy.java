

























package ch.boye.httpclientandroidlib.impl.client.cache;

import java.util.Arrays;
import java.util.Date;
import java.util.HashSet;
import java.util.Set;

import ch.boye.httpclientandroidlib.androidextra.HttpClientAndroidLog;

import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.HeaderElement;
import ch.boye.httpclientandroidlib.HttpMessage;
import ch.boye.httpclientandroidlib.HttpRequest;
import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.HttpStatus;
import ch.boye.httpclientandroidlib.HttpVersion;
import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.client.cache.HeaderConstants;
import ch.boye.httpclientandroidlib.client.utils.DateUtils;
import ch.boye.httpclientandroidlib.protocol.HTTP;






@Immutable
class ResponseCachingPolicy {

    private static final String[] AUTH_CACHEABLE_PARAMS = {
            "s-maxage", HeaderConstants.CACHE_CONTROL_MUST_REVALIDATE, HeaderConstants.PUBLIC
    };
    private final long maxObjectSizeBytes;
    private final boolean sharedCache;
    private final boolean neverCache1_0ResponsesWithQueryString;
    public HttpClientAndroidLog log = new HttpClientAndroidLog(getClass());
    private static final Set<Integer> cacheableStatuses =
        new HashSet<Integer>(Arrays.asList(HttpStatus.SC_OK,
                HttpStatus.SC_NON_AUTHORITATIVE_INFORMATION,
                HttpStatus.SC_MULTIPLE_CHOICES,
                HttpStatus.SC_MOVED_PERMANENTLY,
                HttpStatus.SC_GONE));
    private final Set<Integer> uncacheableStatuses;
    










    public ResponseCachingPolicy(final long maxObjectSizeBytes,
            final boolean sharedCache,
            final boolean neverCache1_0ResponsesWithQueryString,
            final boolean allow303Caching) {
        this.maxObjectSizeBytes = maxObjectSizeBytes;
        this.sharedCache = sharedCache;
        this.neverCache1_0ResponsesWithQueryString = neverCache1_0ResponsesWithQueryString;
        if (allow303Caching) {
            uncacheableStatuses = new HashSet<Integer>(
                    Arrays.asList(HttpStatus.SC_PARTIAL_CONTENT));
        } else {
            uncacheableStatuses = new HashSet<Integer>(Arrays.asList(
                    HttpStatus.SC_PARTIAL_CONTENT, HttpStatus.SC_SEE_OTHER));
        }
    }

    






    public boolean isResponseCacheable(final String httpMethod, final HttpResponse response) {
        boolean cacheable = false;

        if (!HeaderConstants.GET_METHOD.equals(httpMethod)) {
            log.debug("Response was not cacheable.");
            return false;
        }

        final int status = response.getStatusLine().getStatusCode();
        if (cacheableStatuses.contains(status)) {
            
            cacheable = true;
        } else if (uncacheableStatuses.contains(status)) {
            return false;
        } else if (unknownStatusCode(status)) {
            
            
            return false;
        }

        final Header contentLength = response.getFirstHeader(HTTP.CONTENT_LEN);
        if (contentLength != null) {
            final int contentLengthValue = Integer.parseInt(contentLength.getValue());
            if (contentLengthValue > this.maxObjectSizeBytes) {
                return false;
            }
        }

        final Header[] ageHeaders = response.getHeaders(HeaderConstants.AGE);

        if (ageHeaders.length > 1) {
            return false;
        }

        final Header[] expiresHeaders = response.getHeaders(HeaderConstants.EXPIRES);

        if (expiresHeaders.length > 1) {
            return false;
        }

        final Header[] dateHeaders = response.getHeaders(HTTP.DATE_HEADER);

        if (dateHeaders.length != 1) {
            return false;
        }

        final Date date = DateUtils.parseDate(dateHeaders[0].getValue());
        if (date == null) {
            return false;
        }

        for (final Header varyHdr : response.getHeaders(HeaderConstants.VARY)) {
            for (final HeaderElement elem : varyHdr.getElements()) {
                if ("*".equals(elem.getName())) {
                    return false;
                }
            }
        }

        if (isExplicitlyNonCacheable(response)) {
            return false;
        }

        return (cacheable || isExplicitlyCacheable(response));
    }

    private boolean unknownStatusCode(final int status) {
        if (status >= 100 && status <= 101) {
            return false;
        }
        if (status >= 200 && status <= 206) {
            return false;
        }
        if (status >= 300 && status <= 307) {
            return false;
        }
        if (status >= 400 && status <= 417) {
            return false;
        }
        if (status >= 500 && status <= 505) {
            return false;
        }
        return true;
    }

    protected boolean isExplicitlyNonCacheable(final HttpResponse response) {
        final Header[] cacheControlHeaders = response.getHeaders(HeaderConstants.CACHE_CONTROL);
        for (final Header header : cacheControlHeaders) {
            for (final HeaderElement elem : header.getElements()) {
                if (HeaderConstants.CACHE_CONTROL_NO_STORE.equals(elem.getName())
                        || HeaderConstants.CACHE_CONTROL_NO_CACHE.equals(elem.getName())
                        || (sharedCache && HeaderConstants.PRIVATE.equals(elem.getName()))) {
                    return true;
                }
            }
        }
        return false;
    }

    protected boolean hasCacheControlParameterFrom(final HttpMessage msg, final String[] params) {
        final Header[] cacheControlHeaders = msg.getHeaders(HeaderConstants.CACHE_CONTROL);
        for (final Header header : cacheControlHeaders) {
            for (final HeaderElement elem : header.getElements()) {
                for (final String param : params) {
                    if (param.equalsIgnoreCase(elem.getName())) {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    protected boolean isExplicitlyCacheable(final HttpResponse response) {
        if (response.getFirstHeader(HeaderConstants.EXPIRES) != null) {
            return true;
        }
        final String[] cacheableParams = { HeaderConstants.CACHE_CONTROL_MAX_AGE, "s-maxage",
                HeaderConstants.CACHE_CONTROL_MUST_REVALIDATE,
                HeaderConstants.CACHE_CONTROL_PROXY_REVALIDATE,
                HeaderConstants.PUBLIC
        };
        return hasCacheControlParameterFrom(response, cacheableParams);
    }

    







    public boolean isResponseCacheable(final HttpRequest request, final HttpResponse response) {
        if (requestProtocolGreaterThanAccepted(request)) {
            log.debug("Response was not cacheable.");
            return false;
        }

        final String[] uncacheableRequestDirectives = { HeaderConstants.CACHE_CONTROL_NO_STORE };
        if (hasCacheControlParameterFrom(request,uncacheableRequestDirectives)) {
            return false;
        }

        if (request.getRequestLine().getUri().contains("?")) {
            if (neverCache1_0ResponsesWithQueryString && from1_0Origin(response)) {
                log.debug("Response was not cacheable as it had a query string.");
                return false;
            } else if (!isExplicitlyCacheable(response)) {
                log.debug("Response was not cacheable as it is missing explicit caching headers.");
                return false;
            }
        }

        if (expiresHeaderLessOrEqualToDateHeaderAndNoCacheControl(response)) {
            return false;
        }

        if (sharedCache) {
            final Header[] authNHeaders = request.getHeaders(HeaderConstants.AUTHORIZATION);
            if (authNHeaders != null && authNHeaders.length > 0
                    && !hasCacheControlParameterFrom(response, AUTH_CACHEABLE_PARAMS)) {
                return false;
            }
        }

        final String method = request.getRequestLine().getMethod();
        return isResponseCacheable(method, response);
    }

    private boolean expiresHeaderLessOrEqualToDateHeaderAndNoCacheControl(
            final HttpResponse response) {
        if (response.getFirstHeader(HeaderConstants.CACHE_CONTROL) != null) {
            return false;
        }
        final Header expiresHdr = response.getFirstHeader(HeaderConstants.EXPIRES);
        final Header dateHdr = response.getFirstHeader(HTTP.DATE_HEADER);
        if (expiresHdr == null || dateHdr == null) {
            return false;
        }
        final Date expires = DateUtils.parseDate(expiresHdr.getValue());
        final Date date = DateUtils.parseDate(dateHdr.getValue());
        if (expires == null || date == null) {
            return false;
        }
        return expires.equals(date) || expires.before(date);
    }

    private boolean from1_0Origin(final HttpResponse response) {
        final Header via = response.getFirstHeader(HeaderConstants.VIA);
        if (via != null) {
            for(final HeaderElement elt : via.getElements()) {
                final String proto = elt.toString().split("\\s")[0];
                if (proto.contains("/")) {
                    return proto.equals("HTTP/1.0");
                } else {
                    return proto.equals("1.0");
                }
            }
        }
        return HttpVersion.HTTP_1_0.equals(response.getProtocolVersion());
    }

    private boolean requestProtocolGreaterThanAccepted(final HttpRequest req) {
        return req.getProtocolVersion().compareToVersion(HttpVersion.HTTP_1_1) > 0;
    }

}
