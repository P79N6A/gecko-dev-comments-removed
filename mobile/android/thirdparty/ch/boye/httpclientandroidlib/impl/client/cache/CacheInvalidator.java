

























package ch.boye.httpclientandroidlib.impl.client.cache;

import java.io.IOException;
import java.net.MalformedURLException;
import java.net.URL;
import java.util.Date;

import ch.boye.httpclientandroidlib.androidextra.HttpClientAndroidLog;

import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.HttpHost;
import ch.boye.httpclientandroidlib.HttpRequest;
import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.client.cache.HeaderConstants;
import ch.boye.httpclientandroidlib.client.cache.HttpCacheEntry;
import ch.boye.httpclientandroidlib.client.cache.HttpCacheInvalidator;
import ch.boye.httpclientandroidlib.client.cache.HttpCacheStorage;
import ch.boye.httpclientandroidlib.client.utils.DateUtils;
import ch.boye.httpclientandroidlib.protocol.HTTP;







@Immutable
class CacheInvalidator implements HttpCacheInvalidator {

    private final HttpCacheStorage storage;
    private final CacheKeyGenerator cacheKeyGenerator;

    public HttpClientAndroidLog log = new HttpClientAndroidLog(getClass());

    






    public CacheInvalidator(
            final CacheKeyGenerator uriExtractor,
            final HttpCacheStorage storage) {
        this.cacheKeyGenerator = uriExtractor;
        this.storage = storage;
    }

    






    public void flushInvalidatedCacheEntries(final HttpHost host, final HttpRequest req)  {
        if (requestShouldNotBeCached(req)) {
            log.debug("Request should not be cached");

            final String theUri = cacheKeyGenerator.getURI(host, req);

            final HttpCacheEntry parent = getEntry(theUri);

            log.debug("parent entry: " + parent);

            if (parent != null) {
                for (final String variantURI : parent.getVariantMap().values()) {
                    flushEntry(variantURI);
                }
                flushEntry(theUri);
            }
            final URL reqURL = getAbsoluteURL(theUri);
            if (reqURL == null) {
                log.error("Couldn't transform request into valid URL");
                return;
            }
            final Header clHdr = req.getFirstHeader("Content-Location");
            if (clHdr != null) {
                final String contentLocation = clHdr.getValue();
                if (!flushAbsoluteUriFromSameHost(reqURL, contentLocation)) {
                    flushRelativeUriFromSameHost(reqURL, contentLocation);
                }
            }
            final Header lHdr = req.getFirstHeader("Location");
            if (lHdr != null) {
                flushAbsoluteUriFromSameHost(reqURL, lHdr.getValue());
            }
        }
    }

    private void flushEntry(final String uri) {
        try {
            storage.removeEntry(uri);
        } catch (final IOException ioe) {
            log.warn("unable to flush cache entry", ioe);
        }
    }

    private HttpCacheEntry getEntry(final String theUri) {
        try {
            return storage.getEntry(theUri);
        } catch (final IOException ioe) {
            log.warn("could not retrieve entry from storage", ioe);
        }
        return null;
    }

    protected void flushUriIfSameHost(final URL requestURL, final URL targetURL) {
        final URL canonicalTarget = getAbsoluteURL(cacheKeyGenerator.canonicalizeUri(targetURL.toString()));
        if (canonicalTarget == null) {
            return;
        }
        if (canonicalTarget.getAuthority().equalsIgnoreCase(requestURL.getAuthority())) {
            flushEntry(canonicalTarget.toString());
        }
    }

    protected void flushRelativeUriFromSameHost(final URL reqURL, final String relUri) {
        final URL relURL = getRelativeURL(reqURL, relUri);
        if (relURL == null) {
            return;
        }
        flushUriIfSameHost(reqURL, relURL);
    }


    protected boolean flushAbsoluteUriFromSameHost(final URL reqURL, final String uri) {
        final URL absURL = getAbsoluteURL(uri);
        if (absURL == null) {
            return false;
        }
        flushUriIfSameHost(reqURL,absURL);
        return true;
    }

    private URL getAbsoluteURL(final String uri) {
        URL absURL = null;
        try {
            absURL = new URL(uri);
        } catch (final MalformedURLException mue) {
            
        }
        return absURL;
    }

    private URL getRelativeURL(final URL reqURL, final String relUri) {
        URL relURL = null;
        try {
            relURL = new URL(reqURL,relUri);
        } catch (final MalformedURLException e) {
            
        }
        return relURL;
    }

    protected boolean requestShouldNotBeCached(final HttpRequest req) {
        final String method = req.getRequestLine().getMethod();
        return notGetOrHeadRequest(method);
    }

    private boolean notGetOrHeadRequest(final String method) {
        return !(HeaderConstants.GET_METHOD.equals(method) || HeaderConstants.HEAD_METHOD
                .equals(method));
    }

    


    public void flushInvalidatedCacheEntries(final HttpHost host,
            final HttpRequest request, final HttpResponse response) {
        final int status = response.getStatusLine().getStatusCode();
        if (status < 200 || status > 299) {
            return;
        }
        final URL reqURL = getAbsoluteURL(cacheKeyGenerator.getURI(host, request));
        if (reqURL == null) {
            return;
        }
        final URL contentLocation = getContentLocationURL(reqURL, response);
        if (contentLocation != null) {
            flushLocationCacheEntry(reqURL, response, contentLocation);
        }
        final URL location = getLocationURL(reqURL, response);
        if (location != null) {
            flushLocationCacheEntry(reqURL, response, location);
        }
    }

    private void flushLocationCacheEntry(final URL reqURL,
            final HttpResponse response, final URL location) {
        final String cacheKey = cacheKeyGenerator.canonicalizeUri(location.toString());
        final HttpCacheEntry entry = getEntry(cacheKey);
        if (entry == null) {
            return;
        }

        
        

        if (responseDateOlderThanEntryDate(response, entry)) {
            return;
        }
        if (!responseAndEntryEtagsDiffer(response, entry)) {
            return;
        }

        flushUriIfSameHost(reqURL, location);
    }

    private URL getContentLocationURL(final URL reqURL, final HttpResponse response) {
        final Header clHeader = response.getFirstHeader("Content-Location");
        if (clHeader == null) {
            return null;
        }
        final String contentLocation = clHeader.getValue();
        final URL canonURL = getAbsoluteURL(contentLocation);
        if (canonURL != null) {
            return canonURL;
        }
        return getRelativeURL(reqURL, contentLocation);
    }

    private URL getLocationURL(final URL reqURL, final HttpResponse response) {
        final Header clHeader = response.getFirstHeader("Location");
        if (clHeader == null) {
            return null;
        }
        final String location = clHeader.getValue();
        final URL canonURL = getAbsoluteURL(location);
        if (canonURL != null) {
            return canonURL;
        }
        return getRelativeURL(reqURL, location);
    }

    private boolean responseAndEntryEtagsDiffer(final HttpResponse response,
            final HttpCacheEntry entry) {
        final Header entryEtag = entry.getFirstHeader(HeaderConstants.ETAG);
        final Header responseEtag = response.getFirstHeader(HeaderConstants.ETAG);
        if (entryEtag == null || responseEtag == null) {
            return false;
        }
        return (!entryEtag.getValue().equals(responseEtag.getValue()));
    }

    private boolean responseDateOlderThanEntryDate(final HttpResponse response,
            final HttpCacheEntry entry) {
        final Header entryDateHeader = entry.getFirstHeader(HTTP.DATE_HEADER);
        final Header responseDateHeader = response.getFirstHeader(HTTP.DATE_HEADER);
        if (entryDateHeader == null || responseDateHeader == null) {
            
            return false;
        }
        final Date entryDate = DateUtils.parseDate(entryDateHeader.getValue());
        final Date responseDate = DateUtils.parseDate(responseDateHeader.getValue());
        if (entryDate == null || responseDate == null) {
            return false;
        }
        return responseDate.before(entryDate);
    }
}
