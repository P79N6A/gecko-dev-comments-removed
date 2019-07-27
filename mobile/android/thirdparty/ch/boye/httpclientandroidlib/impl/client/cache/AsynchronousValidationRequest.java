

























package ch.boye.httpclientandroidlib.impl.client.cache;

import java.io.IOException;

import ch.boye.httpclientandroidlib.androidextra.HttpClientAndroidLog;

import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.HttpException;
import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.client.cache.HeaderConstants;
import ch.boye.httpclientandroidlib.client.cache.HttpCacheEntry;
import ch.boye.httpclientandroidlib.client.methods.HttpExecutionAware;
import ch.boye.httpclientandroidlib.client.methods.HttpRequestWrapper;
import ch.boye.httpclientandroidlib.client.methods.CloseableHttpResponse;
import ch.boye.httpclientandroidlib.client.protocol.HttpClientContext;
import ch.boye.httpclientandroidlib.conn.routing.HttpRoute;





class AsynchronousValidationRequest implements Runnable {
    private final AsynchronousValidator parent;
    private final CachingExec cachingExec;
    private final HttpRoute route;
    private final HttpRequestWrapper request;
    private final HttpClientContext context;
    private final HttpExecutionAware execAware;
    private final HttpCacheEntry cacheEntry;
    private final String identifier;
    private final int consecutiveFailedAttempts;

    public HttpClientAndroidLog log = new HttpClientAndroidLog(getClass());

    








    AsynchronousValidationRequest(
            final AsynchronousValidator parent,
            final CachingExec cachingExec,
            final HttpRoute route,
            final HttpRequestWrapper request,
            final HttpClientContext context,
            final HttpExecutionAware execAware,
            final HttpCacheEntry cacheEntry,
            final String identifier,
            final int consecutiveFailedAttempts) {
        this.parent = parent;
        this.cachingExec = cachingExec;
        this.route = route;
        this.request = request;
        this.context = context;
        this.execAware = execAware;
        this.cacheEntry = cacheEntry;
        this.identifier = identifier;
        this.consecutiveFailedAttempts = consecutiveFailedAttempts;
    }

    public void run() {
        try {
            if (revalidateCacheEntry()) {
                parent.jobSuccessful(identifier);
            } else {
                parent.jobFailed(identifier);
            }
        } finally {
            parent.markComplete(identifier);
        }
    }

    






    protected boolean revalidateCacheEntry() {
        try {
            final CloseableHttpResponse httpResponse = cachingExec.revalidateCacheEntry(route, request, context, execAware, cacheEntry);
            try {
                final int statusCode = httpResponse.getStatusLine().getStatusCode();
                return isNotServerError(statusCode) && isNotStale(httpResponse);
            } finally {
                httpResponse.close();
            }
        } catch (final IOException ioe) {
            log.debug("Asynchronous revalidation failed due to I/O error", ioe);
            return false;
        } catch (final HttpException pe) {
            log.error("HTTP protocol exception during asynchronous revalidation", pe);
            return false;
        } catch (final RuntimeException re) {
            log.error("RuntimeException thrown during asynchronous revalidation: " + re);
            return false;
        }
    }

    




    private boolean isNotServerError(final int statusCode) {
        return statusCode < 500;
    }

    




    private boolean isNotStale(final HttpResponse httpResponse) {
        final Header[] warnings = httpResponse.getHeaders(HeaderConstants.WARNING);
        if (warnings != null)
        {
            for (final Header warning : warnings)
            {
                




                final String warningValue = warning.getValue();
                if (warningValue.startsWith("110") || warningValue.startsWith("111"))
                {
                    return false;
                }
            }
        }
        return true;
    }

    String getIdentifier() {
        return identifier;
    }

    



    public int getConsecutiveFailedAttempts() {
        return consecutiveFailedAttempts;
    }

}
