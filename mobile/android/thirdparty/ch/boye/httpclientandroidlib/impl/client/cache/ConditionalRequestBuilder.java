

























package ch.boye.httpclientandroidlib.impl.client.cache;

import java.util.Map;

import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.HeaderElement;
import ch.boye.httpclientandroidlib.ProtocolException;
import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.client.cache.HeaderConstants;
import ch.boye.httpclientandroidlib.client.cache.HttpCacheEntry;
import ch.boye.httpclientandroidlib.client.methods.HttpRequestWrapper;




@Immutable
class ConditionalRequestBuilder {

    










    public HttpRequestWrapper buildConditionalRequest(final HttpRequestWrapper request, final HttpCacheEntry cacheEntry)
            throws ProtocolException {
        final HttpRequestWrapper newRequest = HttpRequestWrapper.wrap(request.getOriginal());
        newRequest.setHeaders(request.getAllHeaders());
        final Header eTag = cacheEntry.getFirstHeader(HeaderConstants.ETAG);
        if (eTag != null) {
            newRequest.setHeader(HeaderConstants.IF_NONE_MATCH, eTag.getValue());
        }
        final Header lastModified = cacheEntry.getFirstHeader(HeaderConstants.LAST_MODIFIED);
        if (lastModified != null) {
            newRequest.setHeader(HeaderConstants.IF_MODIFIED_SINCE, lastModified.getValue());
        }
        boolean mustRevalidate = false;
        for(final Header h : cacheEntry.getHeaders(HeaderConstants.CACHE_CONTROL)) {
            for(final HeaderElement elt : h.getElements()) {
                if (HeaderConstants.CACHE_CONTROL_MUST_REVALIDATE.equalsIgnoreCase(elt.getName())
                    || HeaderConstants.CACHE_CONTROL_PROXY_REVALIDATE.equalsIgnoreCase(elt.getName())) {
                    mustRevalidate = true;
                    break;
                }
            }
        }
        if (mustRevalidate) {
            newRequest.addHeader(HeaderConstants.CACHE_CONTROL, HeaderConstants.CACHE_CONTROL_MAX_AGE + "=0");
        }
        return newRequest;

    }

    










    public HttpRequestWrapper buildConditionalRequestFromVariants(final HttpRequestWrapper request,
            final Map<String, Variant> variants) {
        final HttpRequestWrapper newRequest = HttpRequestWrapper.wrap(request.getOriginal());
        newRequest.setHeaders(request.getAllHeaders());

        
        final StringBuilder etags = new StringBuilder();
        boolean first = true;
        for(final String etag : variants.keySet()) {
            if (!first) {
                etags.append(",");
            }
            first = false;
            etags.append(etag);
        }

        newRequest.setHeader(HeaderConstants.IF_NONE_MATCH, etags.toString());
        return newRequest;
    }

    










    public HttpRequestWrapper buildUnconditionalRequest(final HttpRequestWrapper request, final HttpCacheEntry entry) {
        final HttpRequestWrapper newRequest = HttpRequestWrapper.wrap(request.getOriginal());
        newRequest.setHeaders(request.getAllHeaders());
        newRequest.addHeader(HeaderConstants.CACHE_CONTROL,HeaderConstants.CACHE_CONTROL_NO_CACHE);
        newRequest.addHeader(HeaderConstants.PRAGMA,HeaderConstants.CACHE_CONTROL_NO_CACHE);
        newRequest.removeHeaders(HeaderConstants.IF_RANGE);
        newRequest.removeHeaders(HeaderConstants.IF_MATCH);
        newRequest.removeHeaders(HeaderConstants.IF_NONE_MATCH);
        newRequest.removeHeaders(HeaderConstants.IF_UNMODIFIED_SINCE);
        newRequest.removeHeaders(HeaderConstants.IF_MODIFIED_SINCE);
        return newRequest;
    }

}
