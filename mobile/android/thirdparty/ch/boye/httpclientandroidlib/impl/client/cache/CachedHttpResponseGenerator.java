

























package ch.boye.httpclientandroidlib.impl.client.cache;

import java.util.Date;

import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.HttpEntity;
import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.HttpStatus;
import ch.boye.httpclientandroidlib.HttpVersion;
import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.client.cache.HeaderConstants;
import ch.boye.httpclientandroidlib.client.cache.HttpCacheEntry;
import ch.boye.httpclientandroidlib.client.methods.CloseableHttpResponse;
import ch.boye.httpclientandroidlib.client.utils.DateUtils;
import ch.boye.httpclientandroidlib.message.BasicHeader;
import ch.boye.httpclientandroidlib.message.BasicHttpResponse;
import ch.boye.httpclientandroidlib.protocol.HTTP;






@Immutable
class CachedHttpResponseGenerator {

    private final CacheValidityPolicy validityStrategy;

    CachedHttpResponseGenerator(final CacheValidityPolicy validityStrategy) {
        super();
        this.validityStrategy = validityStrategy;
    }

    CachedHttpResponseGenerator() {
        this(new CacheValidityPolicy());
    }

    






    CloseableHttpResponse generateResponse(final HttpCacheEntry entry) {

        final Date now = new Date();
        final HttpResponse response = new BasicHttpResponse(HttpVersion.HTTP_1_1, entry
                .getStatusCode(), entry.getReasonPhrase());

        response.setHeaders(entry.getAllHeaders());

        if (entry.getResource() != null) {
            final HttpEntity entity = new CacheEntity(entry);
            addMissingContentLengthHeader(response, entity);
            response.setEntity(entity);
        }

        final long age = this.validityStrategy.getCurrentAgeSecs(entry, now);
        if (age > 0) {
            if (age >= Integer.MAX_VALUE) {
                response.setHeader(HeaderConstants.AGE, "2147483648");
            } else {
                response.setHeader(HeaderConstants.AGE, "" + ((int) age));
            }
        }

        return Proxies.enhanceResponse(response);
    }

    



    CloseableHttpResponse generateNotModifiedResponse(final HttpCacheEntry entry) {

        final HttpResponse response = new BasicHttpResponse(HttpVersion.HTTP_1_1,
                HttpStatus.SC_NOT_MODIFIED, "Not Modified");

        
        

        
        Header dateHeader = entry.getFirstHeader(HTTP.DATE_HEADER);
        if (dateHeader == null) {
             dateHeader = new BasicHeader(HTTP.DATE_HEADER, DateUtils.formatDate(new Date()));
        }
        response.addHeader(dateHeader);

        
        
        final Header etagHeader = entry.getFirstHeader(HeaderConstants.ETAG);
        if (etagHeader != null) {
            response.addHeader(etagHeader);
        }

        final Header contentLocationHeader = entry.getFirstHeader("Content-Location");
        if (contentLocationHeader != null) {
            response.addHeader(contentLocationHeader);
        }

        
        
        
        final Header expiresHeader = entry.getFirstHeader(HeaderConstants.EXPIRES);
        if (expiresHeader != null) {
            response.addHeader(expiresHeader);
        }

        final Header cacheControlHeader = entry.getFirstHeader(HeaderConstants.CACHE_CONTROL);
        if (cacheControlHeader != null) {
            response.addHeader(cacheControlHeader);
        }

        final Header varyHeader = entry.getFirstHeader(HeaderConstants.VARY);
        if (varyHeader != null) {
            response.addHeader(varyHeader);
        }

        return Proxies.enhanceResponse(response);
    }

    private void addMissingContentLengthHeader(final HttpResponse response, final HttpEntity entity) {
        if (transferEncodingIsPresent(response)) {
            return;
        }

        Header contentLength = response.getFirstHeader(HTTP.CONTENT_LEN);
        if (contentLength == null) {
            contentLength = new BasicHeader(HTTP.CONTENT_LEN, Long.toString(entity
                    .getContentLength()));
            response.setHeader(contentLength);
        }
    }

    private boolean transferEncodingIsPresent(final HttpResponse response) {
        final Header hdr = response.getFirstHeader(HTTP.TRANSFER_ENCODING);
        return hdr != null;
    }
}
