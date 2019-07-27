

























package ch.boye.httpclientandroidlib.impl.client.cache;

import ch.boye.httpclientandroidlib.androidextra.HttpClientAndroidLog;

import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.HeaderElement;
import ch.boye.httpclientandroidlib.HttpRequest;
import ch.boye.httpclientandroidlib.HttpVersion;
import ch.boye.httpclientandroidlib.ProtocolVersion;
import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.client.cache.HeaderConstants;






@Immutable
class CacheableRequestPolicy {

    public HttpClientAndroidLog log = new HttpClientAndroidLog(getClass());

    






    public boolean isServableFromCache(final HttpRequest request) {
        final String method = request.getRequestLine().getMethod();

        final ProtocolVersion pv = request.getRequestLine().getProtocolVersion();
        if (HttpVersion.HTTP_1_1.compareToVersion(pv) != 0) {
            log.trace("non-HTTP/1.1 request was not serveable from cache");
            return false;
        }

        if (!method.equals(HeaderConstants.GET_METHOD)) {
            log.trace("non-GET request was not serveable from cache");
            return false;
        }

        if (request.getHeaders(HeaderConstants.PRAGMA).length > 0) {
            log.trace("request with Pragma header was not serveable from cache");
            return false;
        }

        final Header[] cacheControlHeaders = request.getHeaders(HeaderConstants.CACHE_CONTROL);
        for (final Header cacheControl : cacheControlHeaders) {
            for (final HeaderElement cacheControlElement : cacheControl.getElements()) {
                if (HeaderConstants.CACHE_CONTROL_NO_STORE.equalsIgnoreCase(cacheControlElement
                        .getName())) {
                    log.trace("Request with no-store was not serveable from cache");
                    return false;
                }

                if (HeaderConstants.CACHE_CONTROL_NO_CACHE.equalsIgnoreCase(cacheControlElement
                        .getName())) {
                    log.trace("Request with no-cache was not serveable from cache");
                    return false;
                }
            }
        }

        log.trace("Request was serveable from cache");
        return true;
    }

}
